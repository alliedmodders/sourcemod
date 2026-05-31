//! HTTP request execution on the tokio runtime.
//!
//! A shared `reqwest::Client` (connection pooling, default redirect policy, the
//! SSRF-guarding DNS resolver) is used for the common case; a one-off client is
//! built only when a request needs a non-default connect timeout, disables
//! redirects, or opts out of the SSRF guard (reqwest fixes those on the client,
//! not per-request). The total timeout *is* per-request.

use std::panic::AssertUnwindSafe;
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::Duration;

use slotmap::DefaultKey;
use tokio::sync::OwnedSemaphorePermit;

use crate::registry::{complete_http, HttpBuilder, HttpResult};

/// Applied when the plugin doesn't set an explicit timeout, so a stalled read
/// can't tie up a worker/buffer forever.
const DEFAULT_TIMEOUT: Duration = Duration::from_secs(60);

/// Connect timeout applied to one-off clients when the plugin doesn't set one
/// (mirrors the shared client so a custom client can't hang on connect).
const DEFAULT_CONNECT_TIMEOUT: Duration = Duration::from_secs(15);

/// Hard cap on a buffered response body (after transparent gzip/deflate), so a
/// large or compression-bombed response can't exhaust server memory.
const MAX_BODY: usize = 64 * 1024 * 1024;

/// Hard cap on a single streamed-to-disk download. Far larger than MAX_BODY (the
/// point of DownloadFile), but still bounded per file.
const MAX_DOWNLOAD: u64 = 2 * 1024 * 1024 * 1024;

/// Aggregate cap on all in-progress downloads, so many concurrent downloads
/// can't fill the disk even though each is individually under MAX_DOWNLOAD.
const MAX_TOTAL_DOWNLOAD: u64 = 4 * 1024 * 1024 * 1024;

/// Bytes currently being streamed to disk across all in-flight downloads.
static DOWNLOAD_BYTES: AtomicU64 = AtomicU64::new(0);

/// Decrements the global download counter on drop, covering every return path.
struct DownloadReservation(u64);
impl Drop for DownloadReservation {
    fn drop(&mut self) {
        if self.0 > 0 {
            // Saturating, so a stray decrement can't wrap the counter near u64::MAX
            // and wedge every later download at "aggregate limit reached".
            let _ = DOWNLOAD_BYTES.fetch_update(Ordering::Relaxed, Ordering::Relaxed, |cur| {
                Some(cur.saturating_sub(self.0))
            });
        }
    }
}

/// Zero the global download budget (called on shutdown).
pub(crate) fn reset_download_bytes() {
    DOWNLOAD_BYTES.store(0, Ordering::Relaxed);
}

/// Removes a partial download file on drop unless committed, so an aborted
/// (future-dropped) transfer doesn't leave a stray `.part` behind.
struct TempFileGuard(Option<String>);
impl TempFileGuard {
    fn commit(mut self) {
        self.0 = None;
    }
}
impl Drop for TempFileGuard {
    fn drop(&mut self) {
        if let Some(p) = self.0.take() {
            let _ = std::fs::remove_file(&p);
        }
    }
}

pub fn method_from(code: i32) -> reqwest::Method {
    match code {
        1 => reqwest::Method::POST,
        2 => reqwest::Method::PUT,
        3 => reqwest::Method::PATCH,
        4 => reqwest::Method::DELETE,
        5 => reqwest::Method::HEAD,
        6 => reqwest::Method::OPTIONS,
        _ => reqwest::Method::GET,
    }
}

pub async fn execute(
    shared: reqwest::Client,
    builder: HttpBuilder,
    method: reqwest::Method,
    key: DefaultKey,
    // Held for the lifetime of the request; releases an in-flight slot on drop.
    _permit: OwnedSemaphorePermit,
) {
    use futures_util::FutureExt;
    // Catch a panic in the request path so the slot still gets a completion (the
    // plugin callback always fires) instead of hanging forever.
    let result = AssertUnwindSafe(run(shared, builder, method))
        .catch_unwind()
        .await
        .unwrap_or_else(|_| HttpResult::Error("internal error".to_string()));
    complete_http(key, result);
}

/// Render a transport error without echoing the request URL (the outer reqwest
/// `Display` includes it), so a blocked/internal target isn't reflected back.
fn describe_error(e: &reqwest::Error) -> String {
    use std::error::Error;
    let mut msg = e.to_string();
    let mut src = e.source();
    while let Some(s) = src {
        msg = s.to_string();
        src = s.source();
    }
    // A connect-phase error can embed the resolved socket address; don't reflect
    // it back. Our own (addressless) SSRF refusal is preserved.
    if e.is_connect() && msg != crate::ssrf::BLOCKED_MSG {
        return "connection failed".to_string();
    }
    msg
}

/// Read an upload file off-thread, bounded so a huge/streaming/blocking path
/// can't OOM or hang: reject non-regular files and over-cap sizes, cap the read,
/// and time-box it.
async fn read_upload(path: &str) -> Result<Vec<u8>, String> {
    use tokio::io::AsyncReadExt;
    let fut = async {
        // std::io::Error's Display never includes the path, so these are safe.
        let meta = tokio::fs::metadata(path)
            .await
            .map_err(|e| format!("cannot read upload file: {e}"))?;
        if !meta.is_file() {
            return Err("upload path is not a regular file".to_string());
        }
        if meta.len() > MAX_BODY as u64 {
            return Err(format!("upload file exceeds {MAX_BODY}-byte limit"));
        }
        let file = tokio::fs::File::open(path)
            .await
            .map_err(|e| format!("cannot read upload file: {e}"))?;
        let mut buf = Vec::with_capacity(meta.len().min(MAX_BODY as u64) as usize);
        file.take(MAX_BODY as u64 + 1)
            .read_to_end(&mut buf)
            .await
            .map_err(|e| format!("cannot read upload file: {e}"))?;
        if buf.len() > MAX_BODY {
            return Err(format!("upload file exceeds {MAX_BODY}-byte limit"));
        }
        Ok(buf)
    };
    match tokio::time::timeout(DEFAULT_TIMEOUT, fut).await {
        Ok(r) => r,
        Err(_) => Err("upload file read timed out".to_string()),
    }
}

/// Stream a response body to `path` via a temp file (renamed on success, removed
/// on failure), enforcing the per-file and global download caps.
async fn stream_to_file(resp: &mut reqwest::Response, path: &str) -> Result<(), String> {
    use tokio::io::AsyncWriteExt;
    let tmp = format!("{path}.part");
    let mut file = tokio::fs::File::create(&tmp)
        .await
        .map_err(|e| format!("cannot create download file: {e}"))?;
    // Removes `tmp` on any early return or task abort until we commit on success.
    let guard = TempFileGuard(Some(tmp.clone()));

    // Releases its share of the global budget on every exit path.
    let mut reservation = DownloadReservation(0);
    let mut written: u64 = 0;
    let result: Result<(), String> = loop {
        match resp.chunk().await {
            Ok(Some(chunk)) => {
                let len = chunk.len() as u64;
                written = written.saturating_add(len);
                if written > MAX_DOWNLOAD {
                    break Err(format!("download exceeds {MAX_DOWNLOAD}-byte limit"));
                }
                let total = DOWNLOAD_BYTES.fetch_add(len, Ordering::Relaxed).saturating_add(len);
                reservation.0 = reservation.0.saturating_add(len);
                if total > MAX_TOTAL_DOWNLOAD {
                    break Err("aggregate download limit reached".to_string());
                }
                if let Err(e) = file.write_all(&chunk).await {
                    break Err(format!("write download file: {e}"));
                }
            }
            Ok(None) => break Ok(()),
            Err(e) => break Err(describe_error(&e)),
        }
    };
    drop(reservation);

    result?;
    file.flush()
        .await
        .map_err(|e| format!("flush download file: {e}"))?;
    drop(file);
    tokio::fs::rename(&tmp, path)
        .await
        .map_err(|e| format!("finalize download file: {e}"))?;
    guard.commit();
    Ok(())
}

async fn run(shared: reqwest::Client, b: HttpBuilder, method: reqwest::Method) -> HttpResult {
    let url = match reqwest::Url::parse(&b.url) {
        Ok(u) => u,
        Err(e) => return HttpResult::Error(format!("invalid URL: {e}")),
    };

    // IP-literal SSRF check: hyper connects to literal IPs without the resolver,
    // so check here. (Redirect-hop literals: redirect policy; hostnames: resolver.)
    if !b.allow_private {
        if let Some(ip) = crate::ssrf::host_ip_literal(&url) {
            if crate::ssrf::is_blocked_ip(ip) {
                return HttpResult::Error(crate::ssrf::BLOCKED_MSG.to_string());
            }
        }
    }

    // The shared client already applies the default redirect cap + SSRF guard, so
    // only a non-default redirect limit needs a one-off client.
    let custom_redirects = b
        .max_redirects
        .is_some_and(|n| n as usize != crate::ssrf::DEFAULT_MAX_REDIRECTS);
    let needs_custom = b.connect_timeout.is_some() || custom_redirects || b.allow_private;
    let client = if needs_custom {
        let mut cb = reqwest::Client::builder()
            .use_rustls_tls()
            .connect_timeout(b.connect_timeout.unwrap_or(DEFAULT_CONNECT_TIMEOUT));
        // Redirect policy: an explicit cap overrides the default. Keep the SSRF
        // check on every hop unless the request opted out of the guard.
        cb = match b.max_redirects {
            Some(0) => cb.redirect(reqwest::redirect::Policy::none()),
            Some(n) if b.allow_private => {
                cb.redirect(reqwest::redirect::Policy::limited(n as usize))
            }
            Some(n) => cb.redirect(crate::ssrf::redirect_policy_with_limit(n as usize)),
            None if !b.allow_private => cb.redirect(crate::ssrf::redirect_policy()),
            None => cb,
        };
        // Keep the SSRF guard unless the request explicitly opted out.
        if !b.allow_private {
            cb = cb.dns_resolver(crate::ssrf::resolver());
        }
        // Waive TLS verification only when the request opted into BOTH the SSRF
        // opt-out and invalid certs, i.e. a local/private target. reqwest's rustls
        // backend drops the whole verifier (cert chain + hostname) for this.
        if b.allow_invalid_certs && b.allow_private {
            cb = cb.danger_accept_invalid_certs(true);
        }
        match cb.build() {
            Ok(c) => c,
            Err(e) => return HttpResult::Error(format!("client build: {e}")),
        }
    } else {
        shared
    };

    let mut rb = client.request(method, url);
    for (name, value) in &b.headers {
        rb = rb.header(name.as_str(), value.as_slice());
    }
    if !b.query.is_empty() {
        rb = rb.query(&b.query);
    }
    // Body precedence: explicit body, then an upload file, then form parts.
    if let Some(body) = b.body {
        rb = rb.body(body);
    } else if let Some(path) = &b.body_file {
        match read_upload(path).await {
            Ok(bytes) => rb = rb.body(bytes),
            Err(e) => return HttpResult::Error(e),
        }
    } else if !b.form.is_empty() {
        rb = rb.form(&b.form);
    }
    rb = rb.timeout(b.timeout.unwrap_or(DEFAULT_TIMEOUT));

    match rb.send().await {
        Ok(mut resp) => {
            let status = resp.status().as_u16() as i32;
            // Final URL after redirects; captured before the body borrows resp.
            let final_url = resp.url().to_string();
            let headers = resp
                .headers()
                .iter()
                .map(|(k, v)| {
                    (
                        k.as_str().to_owned(),
                        String::from_utf8_lossy(v.as_bytes()).into_owned(),
                    )
                })
                .collect();
            // DownloadFile: stream straight to disk (body not buffered).
            if let Some(path) = b.download_file {
                // Only a 2xx body is persisted; an error page never lands on disk.
                // The caller still gets status/headers (with an empty body).
                if !(200..300).contains(&status) {
                    return HttpResult::Response {
                        status,
                        final_url,
                        headers,
                        body: Vec::new(),
                    };
                }
                if let Err(e) = stream_to_file(&mut resp, &path).await {
                    return HttpResult::Error(e);
                }
                // Body is on disk; report status/headers with an empty in-memory body.
                return HttpResult::Response {
                    status,
                    final_url,
                    headers,
                    body: Vec::new(),
                };
            }

            // Stream the body so we can abort once MAX_BODY is exceeded rather
            // than buffering an unbounded (possibly decompression-bombed) reply.
            let mut body = Vec::new();
            loop {
                match resp.chunk().await {
                    Ok(Some(chunk)) => {
                        if body.len().saturating_add(chunk.len()) > MAX_BODY {
                            return HttpResult::Error(format!(
                                "response body exceeds {MAX_BODY}-byte limit"
                            ));
                        }
                        body.extend_from_slice(&chunk);
                    }
                    Ok(None) => break,
                    Err(e) => return HttpResult::Error(describe_error(&e)),
                }
            }
            HttpResult::Response {
                status,
                final_url,
                headers,
                body,
            }
        }
        Err(e) => HttpResult::Error(describe_error(&e)),
    }
}
