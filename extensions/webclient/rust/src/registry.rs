//! Shared state: the generational-id registry and the completion queue.
//!
//! Everything the worker tasks and the FFI accessors touch lives here behind a
//! single `parking_lot::Mutex`. Object ids handed to C++ are slot-map keys
//! (`KeyData::as_ffi`); a stale id resolves to "gone" instead of aliasing a
//! reused slot, so a use-after-free across the boundary is a clean miss.

use std::collections::VecDeque;
use std::time::Duration;

use once_cell::sync::Lazy;
use parking_lot::{Mutex, MutexGuard};
use slotmap::{DefaultKey, Key, KeyData, SlotMap};
use tokio::sync::mpsc::Sender;
use tokio::task::AbortHandle;

/// Completion kinds (mirror WC_KIND_* in ffi.h).
pub const KIND_HTTP_RESPONSE: i32 = 0;
pub const KIND_HTTP_ERROR: i32 = 1;
pub const KIND_WS_EVENT: i32 = 2;

/// WebSocket event sub-kinds (mirror WC_WS_* in ffi.h).
pub const WS_CONNECTED: i32 = 0;
pub const WS_TEXT: i32 = 1;
pub const WS_BINARY: i32 = 2;
pub const WS_DISCONNECTED: i32 = 3;
pub const WS_ERROR: i32 = 4;

/// WebSocket ready states (mirror WC_WS_STATE_* in ffi.h / WebSocketReadyState
/// in webclient.inc).
pub const WS_STATE_CONNECTING: u8 = 0;
pub const WS_STATE_OPEN: u8 = 1;
pub const WS_STATE_CLOSING: u8 = 2;
pub const WS_STATE_CLOSED: u8 = 3;

/// Cap on total buffered inbound WebSocket payload bytes awaiting dispatch. A
/// flooding peer past this limit has its data frames dropped (lifecycle events
/// are always kept) so the registry can't grow without bound.
pub const MAX_WS_EVENT_BYTES: usize = 32 * 1024 * 1024;

/// Per-event bookkeeping charged on top of the payload length, so a flood of
/// empty/tiny frames (which carry real struct/slot/queue overhead) still counts
/// against MAX_WS_EVENT_BYTES instead of growing the registry unbounded.
pub const WS_EVENT_OVERHEAD: usize = 256;

/// Pending HTTP request configuration (the Pawn-side builder).
pub struct HttpBuilder {
    pub url: String,
    pub headers: Vec<(String, Vec<u8>)>,
    pub body: Option<Vec<u8>>,
    /// URL query parameters, percent-encoded and appended at submit time.
    pub query: Vec<(String, String)>,
    /// application/x-www-form-urlencoded body parts (used when no explicit body).
    pub form: Vec<(String, String)>,
    /// Absolute path whose contents become the request body (UploadFile).
    pub body_file: Option<String>,
    /// Absolute path the response body is streamed to instead of being buffered
    /// in memory (DownloadFile); lifts the in-memory body cap.
    pub download_file: Option<String>,
    pub timeout: Option<Duration>,
    pub connect_timeout: Option<Duration>,
    /// Max redirects to follow. None => library default; Some(0) => don't follow.
    pub max_redirects: Option<u32>,
    /// Opt out of the SSRF guard to allow private/loopback/link-local targets.
    pub allow_private: bool,
    /// Skip TLS cert/hostname verification. Honored only together with
    /// allow_private, so verification can only be waived for local targets.
    pub allow_invalid_certs: bool,
}

/// An in-flight or completed HTTP request.
pub struct RequestState {
    pub owner: u64,
    pub abort: Option<AbortHandle>,
    pub result: Option<HttpResult>,
}

pub enum HttpResult {
    Response {
        status: i32,
        /// Final URL after any redirects.
        final_url: String,
        headers: Vec<(String, String)>,
        body: Vec<u8>,
    },
    Error(String),
}

/// Commands sent from the FFI to a running WebSocket task.
pub enum WsCommand {
    Text(Vec<u8>),
    Binary(Vec<u8>),
    Ping(Vec<u8>),
    Close { code: u16, reason: Vec<u8> },
}

/// A connecting/connected WebSocket.
pub struct SocketState {
    pub url: String,
    pub owner: u64,
    pub abort: Option<AbortHandle>,
    pub tx: Option<Sender<WsCommand>>,
    pub headers: Vec<(String, Vec<u8>)>,
    /// One of WS_STATE_*.
    pub state: u8,
    /// Opt out of the SSRF pre-check to allow private/loopback/link-local targets.
    pub allow_private: bool,
    /// Skip TLS cert/hostname verification. Honored only together with allow_private.
    pub allow_invalid_certs: bool,
}

/// A single buffered WebSocket event awaiting dispatch on the main thread.
pub struct WsEvent {
    pub kind: i32,
    pub data: Vec<u8>,
    pub close_code: u16,
}

/// One queued completion: the main thread drains these each game frame.
#[derive(Clone, Copy)]
pub struct Completion {
    pub id: u64,
    pub target: u64,
    pub kind: i32,
}

#[derive(Default)]
pub struct Registry {
    pub builders: SlotMap<DefaultKey, HttpBuilder>,
    pub requests: SlotMap<DefaultKey, RequestState>,
    pub sockets: SlotMap<DefaultKey, SocketState>,
    pub events: SlotMap<DefaultKey, WsEvent>,
    pub completions: VecDeque<Completion>,
    /// Running total of `events[*].data` lengths (bounded by MAX_WS_EVENT_BYTES).
    pub ws_event_bytes: usize,
}

static REG: Lazy<Mutex<Registry>> = Lazy::new(|| Mutex::new(Registry::default()));

#[inline]
pub fn reg() -> MutexGuard<'static, Registry> {
    REG.lock()
}

#[inline]
pub fn to_id(key: DefaultKey) -> u64 {
    key.data().as_ffi()
}

#[inline]
pub fn from_id(id: u64) -> DefaultKey {
    DefaultKey::from(KeyData::from_ffi(id))
}

/// Called from a worker task when an HTTP request finishes. No-op if the request
/// was cancelled/removed in the meantime (its result is simply dropped).
pub fn complete_http(key: DefaultKey, result: HttpResult) {
    let mut guard = reg();
    let r = &mut *guard;
    if let Some(st) = r.requests.get_mut(key) {
        st.abort = None;
        let kind = match &result {
            HttpResult::Response { .. } => KIND_HTTP_RESPONSE,
            HttpResult::Error(_) => KIND_HTTP_ERROR,
        };
        st.result = Some(result);
        let id = to_id(key);
        r.completions.push_back(Completion { id, target: id, kind });
    }
}

/// Called from a WebSocket task to enqueue an event. No-op if the socket is gone.
pub fn push_ws_event(socket: DefaultKey, kind: i32, data: Vec<u8>, close_code: u16) {
    let mut guard = reg();
    let r = &mut *guard;
    if !r.sockets.contains_key(socket) {
        return;
    }
    // Backpressure: drop data frames once the buffered total is over the cap, but
    // always deliver lifecycle events (connect/disconnect/error); they're small
    // and a plugin needs them to reason about the socket.
    let is_data = kind == WS_TEXT || kind == WS_BINARY;
    let cost = data.len().saturating_add(WS_EVENT_OVERHEAD);
    if is_data && r.ws_event_bytes.saturating_add(cost) > MAX_WS_EVENT_BYTES {
        return;
    }
    if kind == WS_CONNECTED {
        if let Some(s) = r.sockets.get_mut(socket) {
            s.state = WS_STATE_OPEN;
        }
    }
    r.ws_event_bytes = r.ws_event_bytes.saturating_add(cost);
    let event = r.events.insert(WsEvent {
        kind,
        data,
        close_code,
    });
    r.completions.push_back(Completion {
        id: to_id(event),
        target: to_id(socket),
        kind: KIND_WS_EVENT,
    });
}
