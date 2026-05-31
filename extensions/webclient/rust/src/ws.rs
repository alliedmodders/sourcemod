//! WebSocket task: connect, then pump outbound commands and inbound messages
//! until the peer closes, an error occurs, or the socket is destroyed.

use std::panic::AssertUnwindSafe;
use std::time::Duration;

use futures_util::{FutureExt, Sink, SinkExt, StreamExt};
use slotmap::DefaultKey;
use tokio::sync::mpsc::Receiver;
use tokio::sync::OwnedSemaphorePermit;
use tokio_tungstenite::tungstenite::client::IntoClientRequest;
use tokio_tungstenite::tungstenite::http::{HeaderName, HeaderValue};
use tokio_tungstenite::tungstenite::protocol::frame::coding::CloseCode;
use tokio_tungstenite::tungstenite::protocol::{CloseFrame, WebSocketConfig};
use tokio_tungstenite::tungstenite::Message;

use crate::registry::{
    push_ws_event, reg, WsCommand, WS_BINARY, WS_CONNECTED, WS_DISCONNECTED, WS_ERROR,
    WS_STATE_CLOSED, WS_TEXT,
};

/// Per-message / per-frame caps for inbound traffic, so a hostile peer can't
/// force tungstenite to buffer an oversized message. Kept at or below the
/// registry's MAX_WS_EVENT_BYTES.
const MAX_WS_MESSAGE: usize = 16 * 1024 * 1024;
const MAX_WS_FRAME: usize = 16 * 1024 * 1024;

/// Cap on the SSRF pre-check + connect + TLS handshake, mirroring the HTTP
/// connect timeout, so a black-holed target can't pin a live-socket permit.
const WS_CONNECT_TIMEOUT: Duration = Duration::from_secs(15);

/// Cap on a single outbound send. A peer that stops reading would otherwise let
/// write.send() pend forever, pinning a live-socket permit; on timeout we treat
/// the socket as dead.
const WS_SEND_TIMEOUT: Duration = Duration::from_secs(30);

/// Keepalive: once an open socket has gone this long without any inbound frame,
/// send a Ping. A live peer answers with a Pong (RFC 6455 requires it) or any
/// other frame, which resets the idle clock. tungstenite originates no Pings of
/// its own, so the heartbeat has to be driven here.
const WS_PING_INTERVAL: Duration = Duration::from_secs(30);

/// Liveness deadline: if nothing inbound arrives for this long -- i.e. the peer
/// went silent AND stopped answering our keepalive Pings -- treat the socket as
/// dead and fail it with a terminal error instead of pinning a live-socket
/// permit forever. Set to a multiple of WS_PING_INTERVAL so a single dropped
/// Pong/Pong-reply doesn't kill an otherwise-healthy connection.
const WS_IDLE_TIMEOUT: Duration = Duration::from_secs(90);

/// Mark a socket closed (if it still exists) on every task-exit path.
fn mark_closed(key: DefaultKey) {
    if let Some(s) = reg().sockets.get_mut(key) {
        s.state = WS_STATE_CLOSED;
    }
}

/// Send one frame with a cap; returns false on send error or timeout so the
/// caller tears the socket down instead of pumping a dead peer forever.
async fn send_with_timeout<S>(write: &mut S, msg: Message) -> bool
where
    S: Sink<Message> + Unpin,
{
    matches!(
        tokio::time::timeout(WS_SEND_TIMEOUT, write.send(msg)).await,
        Ok(Ok(()))
    )
}

/// Scrub a tungstenite error before it reaches the plugin: IO failures vary by
/// platform and aren't useful to scripts, so report them generically (mirrors
/// the HTTP describe_error policy); protocol/handshake detail is kept.
fn describe_ws_error(e: &tokio_tungstenite::tungstenite::Error) -> String {
    use tokio_tungstenite::tungstenite::Error as E;
    match e {
        E::Io(_) => "connection failed".to_string(),
        other => other.to_string(),
    }
}

/// A rustls verifier that accepts ANY server certificate. tungstenite has no
/// "accept invalid certs" switch (unlike reqwest), so a custom connector is the
/// only way to honor AllowInvalidCerts on a WebSocket. Used solely for sockets
/// that opted into both AllowLocalNetwork and AllowInvalidCerts.
#[derive(Debug)]
struct NoCertVerifier;

impl rustls::client::danger::ServerCertVerifier for NoCertVerifier {
    fn verify_server_cert(
        &self,
        _end_entity: &rustls::pki_types::CertificateDer<'_>,
        _intermediates: &[rustls::pki_types::CertificateDer<'_>],
        _server_name: &rustls::pki_types::ServerName<'_>,
        _ocsp_response: &[u8],
        _now: rustls::pki_types::UnixTime,
    ) -> Result<rustls::client::danger::ServerCertVerified, rustls::Error> {
        Ok(rustls::client::danger::ServerCertVerified::assertion())
    }

    fn verify_tls12_signature(
        &self,
        _message: &[u8],
        _cert: &rustls::pki_types::CertificateDer<'_>,
        _dss: &rustls::DigitallySignedStruct,
    ) -> Result<rustls::client::danger::HandshakeSignatureValid, rustls::Error> {
        Ok(rustls::client::danger::HandshakeSignatureValid::assertion())
    }

    fn verify_tls13_signature(
        &self,
        _message: &[u8],
        _cert: &rustls::pki_types::CertificateDer<'_>,
        _dss: &rustls::DigitallySignedStruct,
    ) -> Result<rustls::client::danger::HandshakeSignatureValid, rustls::Error> {
        Ok(rustls::client::danger::HandshakeSignatureValid::assertion())
    }

    fn supported_verify_schemes(&self) -> Vec<rustls::SignatureScheme> {
        rustls::crypto::ring::default_provider()
            .signature_verification_algorithms
            .supported_schemes()
    }
}

/// Build a rustls client config with verification disabled (uses the process
/// default crypto provider installed by wc_init).
fn danger_no_verify_config() -> rustls::ClientConfig {
    rustls::ClientConfig::builder()
        .dangerous()
        .with_custom_certificate_verifier(std::sync::Arc::new(NoCertVerifier))
        .with_no_client_auth()
}

pub async fn run(
    key: DefaultKey,
    url: String,
    headers: Vec<(String, Vec<u8>)>,
    allow_private: bool,
    allow_invalid_certs: bool,
    rx: Receiver<WsCommand>,
    permit: OwnedSemaphorePermit,
) {
    // A panic in the task must still resolve the socket (emit a terminal error,
    // mark it closed) rather than silently leaking the slot/permit.
    let inner = run_inner(
        key,
        url,
        headers,
        allow_private,
        allow_invalid_certs,
        rx,
        permit,
    );
    if AssertUnwindSafe(inner).catch_unwind().await.is_err() {
        push_ws_event(key, WS_ERROR, b"internal error".to_vec(), 0);
        mark_closed(key);
    }
}

async fn run_inner(
    key: DefaultKey,
    url: String,
    headers: Vec<(String, Vec<u8>)>,
    allow_private: bool,
    allow_invalid_certs: bool,
    mut rx: Receiver<WsCommand>,
    // Held for the socket's lifetime; releases a live-socket slot on drop.
    _permit: OwnedSemaphorePermit,
) {
    // One shared budget for the whole connect phase (SSRF pre-check + connect +
    // TLS handshake), so it can't add up to several multiples of the timeout.
    let deadline = tokio::time::Instant::now() + WS_CONNECT_TIMEOUT;

    // SSRF pre-check: tokio-tungstenite has no resolver hook, so vet the target
    // before connecting unless the socket opted out of the guard.
    if !allow_private {
        match tokio::time::timeout_at(deadline, crate::ssrf::ws_guard(&url)).await {
            Ok(Ok(())) => {}
            Ok(Err(msg)) => {
                push_ws_event(key, WS_ERROR, msg.into_bytes(), 0);
                mark_closed(key);
                return;
            }
            Err(_) => {
                push_ws_event(key, WS_ERROR, b"connect timed out".to_vec(), 0);
                mark_closed(key);
                return;
            }
        }
    }

    let request = match url.into_client_request() {
        Ok(mut req) => {
            for (name, value) in &headers {
                if let (Ok(name), Ok(value)) = (
                    HeaderName::from_bytes(name.as_bytes()),
                    HeaderValue::from_bytes(value),
                ) {
                    req.headers_mut().append(name, value);
                }
            }
            req
        }
        Err(e) => {
            push_ws_event(key, WS_ERROR, e.to_string().into_bytes(), 0);
            mark_closed(key);
            return;
        }
    };

    let config = WebSocketConfig {
        max_message_size: Some(MAX_WS_MESSAGE),
        max_frame_size: Some(MAX_WS_FRAME),
        ..Default::default()
    };
    // Waive TLS verification only when the socket opted into BOTH AllowLocalNetwork
    // and AllowInvalidCerts (a local/private target). Otherwise pass None, which
    // uses the default rustls + webpki-roots connector (full verification).
    let connector = if allow_invalid_certs && allow_private {
        Some(tokio_tungstenite::Connector::Rustls(std::sync::Arc::new(
            danger_no_verify_config(),
        )))
    } else {
        None
    };
    let connect =
        tokio_tungstenite::connect_async_tls_with_config(request, Some(config), false, connector);
    let stream = match tokio::time::timeout_at(deadline, connect).await {
        Ok(Ok((stream, _resp))) => stream,
        Ok(Err(e)) => {
            push_ws_event(key, WS_ERROR, describe_ws_error(&e).into_bytes(), 0);
            mark_closed(key);
            return;
        }
        Err(_) => {
            push_ws_event(key, WS_ERROR, b"connect timed out".to_vec(), 0);
            mark_closed(key);
            return;
        }
    };

    push_ws_event(key, WS_CONNECTED, Vec::new(), 0);

    let (mut write, mut read) = stream.split();
    // Tracks whether the loop already delivered a terminal event (error or
    // disconnect). The exit paths that don't (plugin Close, a failed send) still
    // owe the plugin exactly one disconnect so it can release the handle.
    let mut terminal_sent = false;
    // The close code the plugin asked for, reported back on its own disconnect.
    let mut local_close: Option<u16> = None;
    // Keepalive clock: reset by any inbound frame (data, Ping, or Pong). The
    // timer Pings a quiet-but-live peer and fails a fully-silent one.
    let mut last_activity = tokio::time::Instant::now();
    let mut keepalive =
        tokio::time::interval_at(last_activity + WS_PING_INTERVAL, WS_PING_INTERVAL);
    keepalive.set_missed_tick_behavior(tokio::time::MissedTickBehavior::Delay);
    loop {
        tokio::select! {
            cmd = rx.recv() => {
                match cmd {
                    Some(WsCommand::Text(d)) => {
                        let text = String::from_utf8_lossy(&d).into_owned();
                        if !send_with_timeout(&mut write, Message::Text(text)).await {
                            break;
                        }
                    }
                    Some(WsCommand::Binary(d)) => {
                        if !send_with_timeout(&mut write, Message::Binary(d)).await {
                            break;
                        }
                    }
                    Some(WsCommand::Ping(d)) => {
                        if !send_with_timeout(&mut write, Message::Ping(d)).await {
                            break;
                        }
                    }
                    Some(WsCommand::Close { code, reason }) => {
                        let frame = CloseFrame {
                            code: CloseCode::from(code),
                            reason: String::from_utf8_lossy(&reason).into_owned().into(),
                        };
                        let _ = send_with_timeout(&mut write, Message::Close(Some(frame))).await;
                        local_close = Some(code);
                        break;
                    }
                    None => break, // sender dropped (socket destroyed)
                }
            }
            msg = read.next() => {
                match msg {
                    Some(Ok(Message::Text(t))) => {
                        last_activity = tokio::time::Instant::now();
                        push_ws_event(key, WS_TEXT, t.as_bytes().to_vec(), 0);
                    }
                    Some(Ok(Message::Binary(b))) => {
                        last_activity = tokio::time::Instant::now();
                        push_ws_event(key, WS_BINARY, b.to_vec(), 0);
                    }
                    Some(Ok(Message::Close(frame))) => {
                        let (code, reason) = frame
                            .map(|f| (u16::from(f.code), f.reason.as_bytes().to_vec()))
                            .unwrap_or((1005, Vec::new()));
                        push_ws_event(key, WS_DISCONNECTED, reason, code);
                        terminal_sent = true;
                        break;
                    }
                    // Ping/Pong/raw frames are handled by tungstenite internally;
                    // they still count as peer liveness for the keepalive clock.
                    Some(Ok(_)) => {
                        last_activity = tokio::time::Instant::now();
                    }
                    Some(Err(e)) => {
                        push_ws_event(key, WS_ERROR, describe_ws_error(&e).into_bytes(), 0);
                        terminal_sent = true;
                        break;
                    }
                    None => {
                        push_ws_event(key, WS_DISCONNECTED, Vec::new(), 1006);
                        terminal_sent = true;
                        break;
                    }
                }
            }
            _ = keepalive.tick() => {
                let idle = last_activity.elapsed();
                if idle >= WS_IDLE_TIMEOUT {
                    // Peer went silent and stopped answering keepalive Pings: treat
                    // it as dead rather than holding the live-socket slot forever.
                    push_ws_event(key, WS_ERROR, b"connection timed out".to_vec(), 0);
                    terminal_sent = true;
                    break;
                }
                if idle >= WS_PING_INTERVAL
                    && !send_with_timeout(&mut write, Message::Ping(Vec::new())).await
                {
                    break;
                }
            }
        }
    }

    if !terminal_sent {
        // A local Close (or a failed/timed-out send) still owes the plugin one
        // disconnect: report the requested code, else 1006 (abnormal).
        push_ws_event(key, WS_DISCONNECTED, Vec::new(), local_close.unwrap_or(1006));
    }
    mark_closed(key);
}
