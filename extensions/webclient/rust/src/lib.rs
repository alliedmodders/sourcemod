//! C-ABI surface for the SourceMod WebClient extension.
//!
//! Every `extern "C"` entry point is wrapped in `catch_unwind`: a panic becomes
//! an error return rather than unwinding across the FFI boundary (UB) or
//! aborting the game server. All of these are invoked from SourceMod's main
//! thread; the worker tasks they spawn never call back into the VM. They only
//! push results onto the completion queue that `wc_poll_completion` drains.

mod http;
mod registry;
mod ssrf;
mod ws;

use std::os::raw::c_char;
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::sync::Arc;
use std::time::Duration;

use base64::Engine as _;
use once_cell::sync::Lazy;
use parking_lot::Mutex;
use percent_encoding::{percent_decode, percent_encode, AsciiSet, NON_ALPHANUMERIC};
use tokio::sync::Semaphore;

use registry::{
    from_id, reg, to_id, Completion, HttpBuilder, HttpResult, RequestState, SocketState, WsCommand,
    KIND_WS_EVENT, WS_EVENT_OVERHEAD, WS_STATE_CLOSING, WS_STATE_CONNECTING,
};

/// Percent-encode set for a URL component: keep only the RFC 3986 unreserved set
/// (ALPHA / DIGIT / `-` `_` `.` `~`); everything else becomes %XX.
const URL_COMPONENT: &AsciiSet = &NON_ALPHANUMERIC
    .remove(b'-')
    .remove(b'_')
    .remove(b'.')
    .remove(b'~');

/// Runtime + shared HTTP client + concurrency semaphores. Kept separate from the
/// registry so `wc_shutdown` can drop the runtime (joining worker threads)
/// without holding the registry lock; workers briefly take that lock to push
/// their final completions. The semaphores live here (not as process statics) so
/// a fresh `wc_init` after shutdown starts at full capacity.
struct RuntimeHolder {
    rt: tokio::runtime::Runtime,
    client: reqwest::Client,
    http_sem: Arc<Semaphore>,
    ws_sem: Arc<Semaphore>,
}

static RUNTIME: Lazy<Mutex<Option<RuntimeHolder>>> = Lazy::new(|| Mutex::new(None));

/// Bound on concurrent in-flight HTTP requests / live WebSockets, so a plugin (or
/// a burst of them) can't spawn unbounded tasks. Submits past the cap fail.
const MAX_HTTP_INFLIGHT: usize = 256;
const MAX_WS_LIVE: usize = 64;
/// Bound on queued outbound WebSocket messages per socket (`wc_ws_send` returns
/// failure when full instead of growing memory without limit).
const WS_SEND_CAP: usize = 1024;

/// Mirrors `wc_completion_t` in ffi.h.
#[repr(C)]
pub struct WcCompletion {
    pub id: u64,
    pub target: u64,
    pub kind: i32,
}

#[inline]
fn guard<T>(default: T, f: impl FnOnce() -> T) -> T {
    catch_unwind(AssertUnwindSafe(f)).unwrap_or(default)
}

fn bytes_to_vec(ptr: *const u8, len: usize) -> Vec<u8> {
    if ptr.is_null() || len == 0 {
        return Vec::new();
    }
    unsafe { std::slice::from_raw_parts(ptr, len).to_vec() }
}

fn bytes_to_string(ptr: *const u8, len: usize) -> String {
    String::from_utf8_lossy(&bytes_to_vec(ptr, len)).into_owned()
}

fn cstr_to_string(ptr: *const c_char) -> String {
    if ptr.is_null() {
        return String::new();
    }
    unsafe { std::ffi::CStr::from_ptr(ptr).to_string_lossy().into_owned() }
}

/// Copy a string into a C buffer (NUL-terminated, truncated). Returns bytes
/// written excluding the NUL.
fn copy_str(buf: *mut c_char, buf_len: usize, s: &str) -> i32 {
    if buf.is_null() || buf_len == 0 {
        return 0;
    }
    let bytes = s.as_bytes();
    let mut n = bytes.len().min(buf_len - 1);
    // Back off so we never split a multi-byte UTF-8 codepoint at the truncation.
    while n > 0 && !s.is_char_boundary(n) {
        n -= 1;
    }
    unsafe {
        std::ptr::copy_nonoverlapping(bytes.as_ptr(), buf as *mut u8, n);
        *buf.add(n) = 0;
    }
    n as i32
}

/* ----------------------------------------------------------------------- */
/* URL encoding helpers (pure; no runtime needed)                          */
/* ----------------------------------------------------------------------- */

/// Percent-encode `data` (URL-component set) into buf. Returns bytes written.
#[unsafe(no_mangle)]
pub extern "C" fn wc_url_encode(data: *const u8, len: usize, buf: *mut c_char, buf_len: usize) -> i32 {
    guard(0, || {
        let input = bytes_to_vec(data, len);
        let encoded = percent_encode(&input, URL_COMPONENT).to_string();
        copy_str(buf, buf_len, &encoded)
    })
}

/// Percent-decode `data` into buf (invalid UTF-8 becomes replacement chars).
/// Returns bytes written.
#[unsafe(no_mangle)]
pub extern "C" fn wc_url_decode(data: *const u8, len: usize, buf: *mut c_char, buf_len: usize) -> i32 {
    guard(0, || {
        let input = bytes_to_vec(data, len);
        let decoded = percent_decode(&input).decode_utf8_lossy();
        copy_str(buf, buf_len, &decoded)
    })
}

/* ----------------------------------------------------------------------- */
/* Lifecycle                                                               */
/* ----------------------------------------------------------------------- */

#[unsafe(no_mangle)]
pub extern "C" fn wc_init(err: *mut c_char, err_len: usize) -> i32 {
    guard(1, || {
        let mut slot = RUNTIME.lock();
        if slot.is_some() {
            return 0;
        }
        // Start a fresh session with an uninflated download budget.
        http::reset_download_bytes();

        // Install a process-default rustls crypto provider so the WebSocket
        // connector (which builds a ClientConfig without an explicit provider)
        // has one. Ignore the error if another provider is already installed.
        let _ = rustls::crypto::ring::default_provider().install_default();

        let rt = match tokio::runtime::Builder::new_multi_thread()
            .worker_threads(2)
            .enable_all()
            .thread_name("webclient")
            .build()
        {
            Ok(rt) => rt,
            Err(e) => {
                copy_str(err, err_len, &format!("runtime: {e}"));
                return 1;
            }
        };

        let client = match reqwest::Client::builder()
            .use_rustls_tls()
            .connect_timeout(Duration::from_secs(15))
            // SSRF guard: refuse private/loopback/link-local targets by default.
            // The resolver covers hostname hops; the redirect policy covers
            // IP-literal hops (which hyper connects to without the resolver). A
            // per-request opt-out builds its own client without either.
            .dns_resolver(ssrf::resolver())
            .redirect(ssrf::redirect_policy())
            .build()
        {
            Ok(c) => c,
            Err(e) => {
                copy_str(err, err_len, &format!("http client: {e}"));
                return 1;
            }
        };

        *slot = Some(RuntimeHolder {
            rt,
            client,
            http_sem: Arc::new(Semaphore::new(MAX_HTTP_INFLIGHT)),
            ws_sem: Arc::new(Semaphore::new(MAX_WS_LIVE)),
        });
        0
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_shutdown() {
    guard((), || {
        // Drop the runtime outside any registry lock. Bounded timeout: reqwest's
        // DNS uses non-cancellable spawn_blocking, so a bare drop could stall the
        // main thread on an in-flight getaddrinfo; after the timeout it's detached.
        let holder = RUNTIME.lock().take();
        if let Some(h) = holder {
            let RuntimeHolder { rt, client, .. } = h;
            drop(client);
            rt.shutdown_timeout(Duration::from_millis(500));
        }
        *reg() = registry::Registry::default();
        // Clear the global download budget so a fresh wc_init starts uninflated
        // even if a detached fs task outlived the runtime.
        http::reset_download_bytes();
    });
}

/* ----------------------------------------------------------------------- */
/* HTTP build / submit                                                     */
/* ----------------------------------------------------------------------- */

#[unsafe(no_mangle)]
pub extern "C" fn wc_http_new(url: *const u8, url_len: usize) -> u64 {
    guard(0, || {
        let url = bytes_to_string(url, url_len);
        if url.is_empty() {
            return 0;
        }
        let key = reg().builders.insert(HttpBuilder {
            url,
            headers: Vec::new(),
            body: None,
            query: Vec::new(),
            form: Vec::new(),
            body_file: None,
            download_file: None,
            timeout: None,
            connect_timeout: None,
            max_redirects: None,
            allow_private: false,
            allow_invalid_certs: false,
        });
        to_id(key)
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_http_set_header(builder: u64, name: *const c_char, value: *const u8, value_len: usize) {
    guard((), || {
        let name = cstr_to_string(name);
        let value = bytes_to_vec(value, value_len);
        if let Some(b) = reg().builders.get_mut(from_id(builder)) {
            b.headers.push((name, value));
        }
    });
}

/// Set an `Authorization: Basic <base64(user:pass)>` header on the builder.
#[unsafe(no_mangle)]
pub extern "C" fn wc_http_set_basic_auth(
    builder: u64,
    user: *const u8,
    user_len: usize,
    pass: *const u8,
    pass_len: usize,
) {
    guard((), || {
        let user = bytes_to_string(user, user_len);
        let pass = bytes_to_string(pass, pass_len);
        let token = base64::engine::general_purpose::STANDARD.encode(format!("{user}:{pass}"));
        let value = format!("Basic {token}").into_bytes();
        if let Some(b) = reg().builders.get_mut(from_id(builder)) {
            b.headers.push(("authorization".to_string(), value));
        }
    });
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_http_set_body(builder: u64, data: *const u8, len: usize) {
    guard((), || {
        let body = bytes_to_vec(data, len);
        if let Some(b) = reg().builders.get_mut(from_id(builder)) {
            b.body = Some(body);
        }
    });
}

/// Append a URL query parameter (percent-encoded at submit time).
#[unsafe(no_mangle)]
pub extern "C" fn wc_http_add_query(
    builder: u64,
    name: *const u8,
    name_len: usize,
    value: *const u8,
    value_len: usize,
) {
    guard((), || {
        let name = bytes_to_string(name, name_len);
        let value = bytes_to_string(value, value_len);
        if let Some(b) = reg().builders.get_mut(from_id(builder)) {
            b.query.push((name, value));
        }
    });
}

/// Append an application/x-www-form-urlencoded body parameter (sent when the
/// request has no explicit body).
#[unsafe(no_mangle)]
pub extern "C" fn wc_http_add_form(
    builder: u64,
    name: *const u8,
    name_len: usize,
    value: *const u8,
    value_len: usize,
) {
    guard((), || {
        let name = bytes_to_string(name, name_len);
        let value = bytes_to_string(value, value_len);
        if let Some(b) = reg().builders.get_mut(from_id(builder)) {
            b.form.push((name, value));
        }
    });
}

/// Use the file at `path` (an absolute path) as the request body. Read off the
/// main thread when the request runs.
#[unsafe(no_mangle)]
pub extern "C" fn wc_http_set_body_file(builder: u64, path: *const u8, path_len: usize) {
    guard((), || {
        let path = bytes_to_string(path, path_len);
        if let Some(b) = reg().builders.get_mut(from_id(builder)) {
            b.body_file = Some(path);
        }
    });
}

/// Stream the response body to the file at `path` (absolute) instead of buffering
/// it in memory.
#[unsafe(no_mangle)]
pub extern "C" fn wc_http_set_download_file(builder: u64, path: *const u8, path_len: usize) {
    guard((), || {
        let path = bytes_to_string(path, path_len);
        if let Some(b) = reg().builders.get_mut(from_id(builder)) {
            b.download_file = Some(path);
        }
    });
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_http_set_timeout_ms(builder: u64, ms: u32) {
    guard((), || {
        if let Some(b) = reg().builders.get_mut(from_id(builder)) {
            b.timeout = (ms > 0).then(|| Duration::from_millis(ms as u64));
        }
    });
}

/// Configured total timeout in ms, or 0 if unset (the default applies).
#[unsafe(no_mangle)]
pub extern "C" fn wc_http_get_timeout_ms(builder: u64) -> u32 {
    guard(0, || {
        reg()
            .builders
            .get(from_id(builder))
            .and_then(|b| b.timeout)
            .map(|d| d.as_millis().min(u32::MAX as u128) as u32)
            .unwrap_or(0)
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_http_set_connect_timeout_ms(builder: u64, ms: u32) {
    guard((), || {
        if let Some(b) = reg().builders.get_mut(from_id(builder)) {
            b.connect_timeout = (ms > 0).then(|| Duration::from_millis(ms as u64));
        }
    });
}

/// Configured connect timeout in ms, or 0 if unset.
#[unsafe(no_mangle)]
pub extern "C" fn wc_http_get_connect_timeout_ms(builder: u64) -> u32 {
    guard(0, || {
        reg()
            .builders
            .get(from_id(builder))
            .and_then(|b| b.connect_timeout)
            .map(|d| d.as_millis().min(u32::MAX as u128) as u32)
            .unwrap_or(0)
    })
}

/// Set the max redirects to follow (0 disables following).
#[unsafe(no_mangle)]
pub extern "C" fn wc_http_set_max_redirects(builder: u64, max: u32) {
    guard((), || {
        if let Some(b) = reg().builders.get_mut(from_id(builder)) {
            b.max_redirects = Some(max);
        }
    });
}

/// Configured max redirects, or the library default if the plugin didn't set one.
#[unsafe(no_mangle)]
pub extern "C" fn wc_http_get_max_redirects(builder: u64) -> i32 {
    guard(ssrf::DEFAULT_MAX_REDIRECTS as i32, || {
        reg()
            .builders
            .get(from_id(builder))
            .map(|b| b.max_redirects.map(|n| n as i32).unwrap_or(ssrf::DEFAULT_MAX_REDIRECTS as i32))
            .unwrap_or(ssrf::DEFAULT_MAX_REDIRECTS as i32)
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_http_set_allow_private(builder: u64, allow: i32) {
    guard((), || {
        if let Some(b) = reg().builders.get_mut(from_id(builder)) {
            b.allow_private = allow != 0;
        }
    });
}

/// Read back the allow-private (local-network) flag (0/1). 0 if the builder is gone.
#[unsafe(no_mangle)]
pub extern "C" fn wc_http_get_allow_private(builder: u64) -> i32 {
    guard(0, || {
        reg()
            .builders
            .get(from_id(builder))
            .map(|b| b.allow_private as i32)
            .unwrap_or(0)
    })
}

/// Skip TLS cert/hostname verification. Only honored together with allow_private.
#[unsafe(no_mangle)]
pub extern "C" fn wc_http_set_allow_invalid_certs(builder: u64, allow: i32) {
    guard((), || {
        if let Some(b) = reg().builders.get_mut(from_id(builder)) {
            b.allow_invalid_certs = allow != 0;
        }
    });
}

/// Read back the allow-invalid-certs flag (0/1). 0 if the builder is gone.
#[unsafe(no_mangle)]
pub extern "C" fn wc_http_get_allow_invalid_certs(builder: u64) -> i32 {
    guard(0, || {
        reg()
            .builders
            .get(from_id(builder))
            .map(|b| b.allow_invalid_certs as i32)
            .unwrap_or(0)
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_http_builder_free(builder: u64) {
    guard((), || {
        reg().builders.remove(from_id(builder));
    });
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_http_submit(builder: u64, method: i32, owner_token: u64) -> u64 {
    guard(0, || {
        let method = http::method_from(method);

        // Lock order: RUNTIME then REG (consistent everywhere). We hold REG across
        // the spawn so the task can't complete before we record its abort handle.
        let rt_guard = RUNTIME.lock();
        let (client, sem) = match rt_guard.as_ref() {
            Some(h) => (h.client.clone(), h.http_sem.clone()),
            None => return 0,
        };

        // Bound concurrent in-flight requests; the permit lives in the task and
        // frees its slot on completion/abort.
        let permit = match sem.try_acquire_owned() {
            Ok(p) => p,
            Err(_) => return 0,
        };

        let mut r = reg();
        let b = match r.builders.remove(from_id(builder)) {
            Some(b) => b,
            None => return 0,
        };
        let key = r.requests.insert(RequestState {
            owner: owner_token,
            abort: None,
            result: None,
        });
        let handle = rt_guard
            .as_ref()
            .unwrap()
            .rt
            .spawn(http::execute(client, b, method, key, permit));
        if let Some(st) = r.requests.get_mut(key) {
            st.abort = Some(handle.abort_handle());
        }
        to_id(key)
    })
}

/* ----------------------------------------------------------------------- */
/* Completion draining                                                     */
/* ----------------------------------------------------------------------- */

// `out` is a caller-provided buffer; the null check below makes the deref safe.
#[allow(clippy::not_unsafe_ptr_arg_deref)]
#[unsafe(no_mangle)]
pub extern "C" fn wc_poll_completion(out: *mut WcCompletion) -> i32 {
    guard(0, || {
        if out.is_null() {
            return 0;
        }
        let c: Option<Completion> = reg().completions.pop_front();
        match c {
            Some(c) => {
                unsafe {
                    out.write(WcCompletion {
                        id: c.id,
                        target: c.target,
                        kind: c.kind,
                    });
                }
                1
            }
            None => 0,
        }
    })
}

/* ----------------------------------------------------------------------- */
/* HTTP response accessors                                                 */
/* ----------------------------------------------------------------------- */

#[unsafe(no_mangle)]
pub extern "C" fn wc_resp_status(request_id: u64) -> i32 {
    guard(-1, || {
        match reg().requests.get(from_id(request_id)).and_then(|st| st.result.as_ref()) {
            Some(HttpResult::Response { status, .. }) => *status,
            _ => -1,
        }
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_resp_body_len(request_id: u64) -> usize {
    guard(0, || {
        match reg().requests.get(from_id(request_id)).and_then(|st| st.result.as_ref()) {
            Some(HttpResult::Response { body, .. }) => body.len(),
            _ => 0,
        }
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_resp_body_ptr(request_id: u64) -> *const u8 {
    guard(std::ptr::null(), || {
        // The returned pointer addresses the body Vec's heap buffer, which is
        // stable until the request slot is removed (on the main thread, via the
        // response Handle's destructor). The caller copies it out immediately.
        match reg().requests.get(from_id(request_id)).and_then(|st| st.result.as_ref()) {
            Some(HttpResult::Response { body, .. }) => body.as_ptr(),
            _ => std::ptr::null(),
        }
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_resp_header_count(request_id: u64) -> usize {
    guard(0, || {
        match reg().requests.get(from_id(request_id)).and_then(|st| st.result.as_ref()) {
            Some(HttpResult::Response { headers, .. }) => headers.len(),
            _ => 0,
        }
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_resp_header_name(request_id: u64, i: usize, buf: *mut c_char, buf_len: usize) -> i32 {
    guard(-1, || {
        match reg().requests.get(from_id(request_id)).and_then(|st| st.result.as_ref()) {
            Some(HttpResult::Response { headers, .. }) => match headers.get(i) {
                Some((name, _)) => copy_str(buf, buf_len, name),
                None => -1,
            },
            _ => -1,
        }
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_resp_header_value(request_id: u64, i: usize, buf: *mut c_char, buf_len: usize) -> i32 {
    guard(-1, || {
        match reg().requests.get(from_id(request_id)).and_then(|st| st.result.as_ref()) {
            Some(HttpResult::Response { headers, .. }) => match headers.get(i) {
                Some((_, value)) => copy_str(buf, buf_len, value),
                None => -1,
            },
            _ => -1,
        }
    })
}

/// Copy the final URL (after redirects) of a completed response into buf.
/// Returns bytes written, or -1 if the id has no response.
#[unsafe(no_mangle)]
pub extern "C" fn wc_resp_url(request_id: u64, buf: *mut c_char, buf_len: usize) -> i32 {
    guard(-1, || {
        match reg().requests.get(from_id(request_id)).and_then(|st| st.result.as_ref()) {
            Some(HttpResult::Response { final_url, .. }) => copy_str(buf, buf_len, final_url),
            _ => -1,
        }
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_resp_error(request_id: u64, buf: *mut c_char, buf_len: usize) -> i32 {
    guard(-1, || {
        match reg().requests.get(from_id(request_id)).and_then(|st| st.result.as_ref()) {
            Some(HttpResult::Error(msg)) => copy_str(buf, buf_len, msg),
            _ => -1,
        }
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_http_destroy(request_id: u64) {
    guard((), || {
        let mut r = reg();
        let key = from_id(request_id);
        if let Some(st) = r.requests.get(key) {
            if let Some(abort) = &st.abort {
                abort.abort();
            }
        }
        r.requests.remove(key);
    });
}

/* ----------------------------------------------------------------------- */
/* WebSocket                                                               */
/* ----------------------------------------------------------------------- */

#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_new(url: *const u8, url_len: usize) -> u64 {
    guard(0, || {
        let url = bytes_to_string(url, url_len);
        if url.is_empty() {
            return 0;
        }
        let key = reg().sockets.insert(SocketState {
            url,
            owner: 0,
            abort: None,
            tx: None,
            headers: Vec::new(),
            state: WS_STATE_CONNECTING,
            allow_private: false,
            allow_invalid_certs: false,
        });
        to_id(key)
    })
}

/// Opt this socket out of the SSRF pre-check (allow private/loopback/link-local
/// targets). Only effective before Connect().
#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_set_allow_private(ws: u64, allow: i32) {
    guard((), || {
        if let Some(s) = reg().sockets.get_mut(from_id(ws)) {
            if s.tx.is_none() {
                s.allow_private = allow != 0;
            }
        }
    });
}

/// Read back the socket's allow-private (local-network) flag (0/1). 0 if the socket is gone.
#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_get_allow_private(ws: u64) -> i32 {
    guard(0, || {
        reg()
            .sockets
            .get(from_id(ws))
            .map(|s| s.allow_private as i32)
            .unwrap_or(0)
    })
}

/// Skip TLS cert/hostname verification for this socket. Only honored together with
/// allow_private; effective only before connect (tx is set at connect).
#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_set_allow_invalid_certs(ws: u64, allow: i32) {
    guard((), || {
        if let Some(s) = reg().sockets.get_mut(from_id(ws)) {
            if s.tx.is_none() {
                s.allow_invalid_certs = allow != 0;
            }
        }
    });
}

/// Read back the socket's allow-invalid-certs flag (0/1). 0 if the socket is gone.
#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_get_allow_invalid_certs(ws: u64) -> i32 {
    guard(0, || {
        reg()
            .sockets
            .get(from_id(ws))
            .map(|s| s.allow_invalid_certs as i32)
            .unwrap_or(0)
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_set_header(ws: u64, name: *const c_char, value: *const u8, value_len: usize) {
    guard((), || {
        let name = cstr_to_string(name);
        let value = bytes_to_vec(value, value_len);
        if let Some(s) = reg().sockets.get_mut(from_id(ws)) {
            // Headers can only be added before Connect() (tx is set at connect).
            if s.tx.is_none() {
                s.headers.push((name, value));
            }
        }
    });
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_connect(ws: u64, owner_token: u64) -> i32 {
    guard(1, || {
        let rt_guard = RUNTIME.lock();
        let sem = match rt_guard.as_ref() {
            Some(h) => h.ws_sem.clone(),
            None => return 1,
        };

        // Bound concurrent live sockets; the permit lives in the task.
        let permit = match sem.try_acquire_owned() {
            Ok(p) => p,
            Err(_) => return 1,
        };

        let (tx, rx) = tokio::sync::mpsc::channel(WS_SEND_CAP);
        let mut r = reg();
        let s = match r.sockets.get_mut(from_id(ws)) {
            Some(s) => s,
            None => return 1,
        };
        if s.tx.is_some() {
            return 1; // already connecting/connected
        }
        s.owner = owner_token;
        s.tx = Some(tx);
        let url = s.url.clone();
        let headers = s.headers.clone();
        let allow_private = s.allow_private;
        let allow_invalid_certs = s.allow_invalid_certs;
        let handle = rt_guard.as_ref().unwrap().rt.spawn(ws::run(
            from_id(ws),
            url,
            headers,
            allow_private,
            allow_invalid_certs,
            rx,
            permit,
        ));
        s.abort = Some(handle.abort_handle());
        0
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_send(ws: u64, is_text: i32, data: *const u8, len: usize) -> i32 {
    guard(1, || {
        let payload = bytes_to_vec(data, len);
        let r = reg();
        let s = match r.sockets.get(from_id(ws)) {
            Some(s) => s,
            None => return 1,
        };
        let tx = match &s.tx {
            Some(tx) => tx,
            None => return 1,
        };
        let cmd = if is_text != 0 {
            WsCommand::Text(payload)
        } else {
            WsCommand::Binary(payload)
        };
        // Non-blocking: fail (1) if the queue is full or the socket is gone rather
        // than growing the send buffer without bound.
        match tx.try_send(cmd) {
            Ok(()) => 0,
            Err(_) => 1,
        }
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_close(ws: u64, code: u16, reason: *const u8, reason_len: usize) {
    guard((), || {
        let reason = bytes_to_vec(reason, reason_len);
        let mut r = reg();
        if let Some(s) = r.sockets.get_mut(from_id(ws)) {
            // Only advance to CLOSING if the close was actually queued; if the
            // send buffer is full the socket stays as-is rather than reporting a
            // close that will never be sent.
            if let Some(tx) = &s.tx {
                if tx.try_send(WsCommand::Close { code, reason }).is_ok() {
                    s.state = WS_STATE_CLOSING;
                }
            }
        }
    });
}

/// Ready state of a socket: one of WC_WS_STATE_*, or -1 if the id is unknown.
#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_state(ws: u64) -> i32 {
    guard(-1, || {
        reg()
            .sockets
            .get(from_id(ws))
            .map(|s| s.state as i32)
            .unwrap_or(-1)
    })
}

/// Queue a ping frame. 0 on success; non-zero if the socket is gone or the
/// bounded send queue is full.
#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_ping(ws: u64, data: *const u8, len: usize) -> i32 {
    guard(1, || {
        let payload = bytes_to_vec(data, len);
        let r = reg();
        let s = match r.sockets.get(from_id(ws)) {
            Some(s) => s,
            None => return 1,
        };
        match &s.tx {
            Some(tx) => match tx.try_send(WsCommand::Ping(payload)) {
                Ok(()) => 0,
                Err(_) => 1,
            },
            None => 1,
        }
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_destroy(ws: u64) {
    guard((), || {
        let mut r = reg();
        let key = from_id(ws);
        if let Some(s) = r.sockets.get(key) {
            if let Some(abort) = &s.abort {
                abort.abort();
            }
        }
        r.sockets.remove(key);
    });
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_event_kind(event_id: u64) -> i32 {
    guard(-1, || reg().events.get(from_id(event_id)).map(|e| e.kind).unwrap_or(-1))
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_event_data_len(event_id: u64) -> usize {
    guard(0, || reg().events.get(from_id(event_id)).map(|e| e.data.len()).unwrap_or(0))
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_event_data_ptr(event_id: u64) -> *const u8 {
    guard(std::ptr::null(), || {
        match reg().events.get(from_id(event_id)) {
            Some(e) if !e.data.is_empty() => e.data.as_ptr(),
            _ => std::ptr::null(),
        }
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_event_close_code(event_id: u64) -> u16 {
    guard(0, || reg().events.get(from_id(event_id)).map(|e| e.close_code).unwrap_or(0))
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_ws_event_release(event_id: u64) {
    guard((), || {
        let mut r = reg();
        if let Some(e) = r.events.remove(from_id(event_id)) {
            r.ws_event_bytes = r
                .ws_event_bytes
                .saturating_sub(e.data.len().saturating_add(WS_EVENT_OVERHEAD));
        }
    });
}

/* ----------------------------------------------------------------------- */
/* Cancellation                                                            */
/* ----------------------------------------------------------------------- */

/// Cancel one in-flight request, if it exists and is owned by owner_token: abort
/// the task, drop its slot, and discard any queued completion so its callback
/// never fires. Returns 1 if a request was cancelled, 0 otherwise.
#[unsafe(no_mangle)]
pub extern "C" fn wc_http_cancel(request_id: u64, owner_token: u64) -> i32 {
    guard(0, || {
        let mut r = reg();
        let key = from_id(request_id);
        let owned = matches!(r.requests.get(key), Some(st) if st.owner == owner_token);
        if !owned {
            return 0;
        }
        if let Some(st) = r.requests.get(key) {
            if let Some(abort) = &st.abort {
                abort.abort();
            }
        }
        r.requests.remove(key);
        // Drop a queued HTTP completion for this id (WS events live in a separate
        // keyspace, so guard on kind to avoid a chance numeric collision).
        r.completions
            .retain(|c| !(c.target == request_id && c.kind != KIND_WS_EVENT));
        1
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn wc_cancel_owner(owner_token: u64) {
    guard((), || {
        let mut r = reg();

        let req_keys: Vec<_> = r
            .requests
            .iter()
            .filter(|(_, st)| st.owner == owner_token)
            .map(|(k, _)| k)
            .collect();
        // Collect the ids first so we can also drop their queued completions.
        let req_ids: std::collections::HashSet<u64> = req_keys.iter().map(|&k| to_id(k)).collect();
        for k in req_keys {
            if let Some(st) = r.requests.get(k) {
                if let Some(abort) = &st.abort {
                    abort.abort();
                }
            }
            r.requests.remove(k);
        }

        let sock_keys: Vec<_> = r
            .sockets
            .iter()
            .filter(|(_, s)| s.owner == owner_token)
            .map(|(k, _)| k)
            .collect();
        let sock_ids: std::collections::HashSet<u64> = sock_keys.iter().map(|&k| to_id(k)).collect();
        for k in sock_keys {
            if let Some(s) = r.sockets.get(k) {
                if let Some(abort) = &s.abort {
                    abort.abort();
                }
            }
            r.sockets.remove(k);
        }

        // Drop queued completions for the removed owner (so a stale id is never
        // dispatched) and free the WS events they referenced. Guard on kind to
        // avoid request/socket id keyspace overlap.
        let mut orphan_event_ids: Vec<u64> = Vec::new();
        r.completions.retain(|c| {
            if c.kind == KIND_WS_EVENT {
                if sock_ids.contains(&c.target) {
                    orphan_event_ids.push(c.id);
                    return false;
                }
                true
            } else {
                !req_ids.contains(&c.target)
            }
        });
        for eid in orphan_event_ids {
            if let Some(e) = r.events.remove(from_id(eid)) {
                r.ws_event_bytes = r
                    .ws_event_bytes
                    .saturating_sub(e.data.len().saturating_add(WS_EVENT_OVERHEAD));
            }
        }
    });
}
