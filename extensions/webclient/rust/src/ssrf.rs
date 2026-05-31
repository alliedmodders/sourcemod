//! Outbound SSRF guard.
//!
//! A reqwest DNS resolver that drops private/loopback/link-local/unique-local
//! addresses (and the metadata IP 169.254.169.254). reqwest connects only to the
//! addresses returned and re-resolves each redirect hop, so this blocks SSRF
//! (incl. redirects and DNS rebinding) with no TOCTOU window. Opt out per request
//! via `allow_private` (builds a client without this resolver).

use std::net::{IpAddr, Ipv4Addr, SocketAddr};
use std::sync::Arc;

/// Generic refusal message. Deliberately does not reflect the resolved internal
/// IP back to the caller.
pub const BLOCKED_MSG: &str =
    "blocked address (private/loopback/link-local); enable AllowLocalNetwork to permit";

/// True if connecting to `ip` should be refused by default. Conservative: blocks
/// every non-globally-routable range we can identify on both IPv4 and IPv6,
/// including IPv6 transition ranges that embed a v4 target.
pub fn is_blocked_ip(ip: IpAddr) -> bool {
    match ip {
        IpAddr::V4(v4) => {
            let o = v4.octets();
            v4.is_loopback()                                // 127.0.0.0/8
                || v4.is_private()                          // 10/8, 172.16/12, 192.168/16
                || v4.is_link_local()                       // 169.254/16 (incl. metadata)
                || v4.is_broadcast()                        // 255.255.255.255
                || v4.is_documentation()                    // 192.0.2/24, 198.51.100/24, 203.0.113/24
                || v4.is_unspecified()                      // 0.0.0.0
                || v4.is_multicast()                        // 224/4
                || o[0] == 0                                // 0.0.0.0/8
                || (o[0] & 0xf0) == 0xf0                    // 240/4 reserved (class E)
                || (o[0] == 100 && (o[1] & 0xc0) == 64)     // 100.64/10 CGNAT
                || (o[0] == 192 && o[1] == 0 && o[2] == 0)  // 192.0.0.0/24 IETF
                || (o[0] == 198 && (o[1] & 0xfe) == 18)     // 198.18/15 benchmarking
        }
        IpAddr::V6(v6) => {
            // IPv4-mapped (::ffff:a.b.c.d) must be checked as the underlying v4.
            if let Some(m) = v6.to_ipv4_mapped() {
                return is_blocked_ip(IpAddr::V4(m));
            }
            let s = v6.segments();
            // Transition ranges that carry a v4 target: re-check the embedded v4
            // so e.g. 64:ff9b::169.254.169.254 or 2002:7f00:1:: can't slip through.
            if s[0] == 0x2002 {
                // 6to4 2002::/16 -> v4 in segments 1..2
                let v4 = Ipv4Addr::from(((s[1] as u32) << 16) | (s[2] as u32));
                return is_blocked_ip(IpAddr::V4(v4));
            }
            if s[0] == 0x0064 && s[1] == 0xff9b {
                // NAT64 64:ff9b::/96 and local-use 64:ff9b:1::/48: synthesised
                // addresses that can reach internal v4 via DNS64/NAT64. Block all.
                return true;
            }
            if s[..6].iter().all(|&x| x == 0) && !(s[6] == 0 && (s[7] == 0 || s[7] == 1)) {
                // Deprecated IPv4-compatible ::a.b.c.d (excluding :: and ::1).
                let v4 = Ipv4Addr::from(((s[6] as u32) << 16) | (s[7] as u32));
                return is_blocked_ip(IpAddr::V4(v4));
            }
            v6.is_loopback()                        // ::1
                || v6.is_unspecified()              // ::
                || v6.is_multicast()                // ff00::/8
                || (s[0] & 0xfe00) == 0xfc00        // fc00::/7 unique-local
                || (s[0] & 0xffc0) == 0xfe80        // fe80::/10 link-local
        }
    }
}

/// Extract a literal IP host from a URL (IPv4 or bracketed IPv6). Returns None
/// for hostname hosts (those go through the DNS resolver instead).
pub fn host_ip_literal(url: &reqwest::Url) -> Option<IpAddr> {
    let h = url.host_str()?;
    let h = h
        .strip_prefix('[')
        .and_then(|s| s.strip_suffix(']'))
        .unwrap_or(h);
    h.parse::<IpAddr>().ok()
}

/// Default redirect-chain cap (matches reqwest's own default).
pub const DEFAULT_MAX_REDIRECTS: usize = 10;

/// A redirect policy that refuses hops to blocked IP-literal hosts (hostname hops
/// are covered by the resolver, which hyper bypasses for literals) and otherwise
/// caps the redirect chain at the default.
pub fn redirect_policy() -> reqwest::redirect::Policy {
    redirect_policy_with_limit(DEFAULT_MAX_REDIRECTS)
}

/// Same SSRF-checking policy as `redirect_policy`, but capped at `max` hops.
pub fn redirect_policy_with_limit(max: usize) -> reqwest::redirect::Policy {
    reqwest::redirect::Policy::custom(move |attempt| {
        if let Some(ip) = host_ip_literal(attempt.url()) {
            if is_blocked_ip(ip) {
                return attempt.error(BLOCKED_MSG);
            }
        }
        // `previous()` includes the original URL, so it starts at 1; mirror
        // reqwest's own `limited` semantics (`> max`) and surface an error at
        // the cap instead of silently returning the last 3xx as success.
        if attempt.previous().len() > max {
            attempt.error(format!("exceeded redirect limit of {max}"))
        } else {
            attempt.follow()
        }
    })
}

pub struct SsrfResolver;

impl reqwest::dns::Resolve for SsrfResolver {
    fn resolve(&self, name: reqwest::dns::Name) -> reqwest::dns::Resolving {
        Box::pin(async move {
            let host = name.as_str().to_owned();
            let addrs = tokio::net::lookup_host((host.as_str(), 0)).await?;
            let allowed: Vec<SocketAddr> = addrs.filter(|a| !is_blocked_ip(a.ip())).collect();
            if allowed.is_empty() {
                return Err(Box::<dyn std::error::Error + Send + Sync>::from(BLOCKED_MSG));
            }
            Ok(Box::new(allowed.into_iter()) as reqwest::dns::Addrs)
        })
    }
}

/// A shared resolver instance for installing on a reqwest client.
pub fn resolver() -> Arc<SsrfResolver> {
    Arc::new(SsrfResolver)
}

/// SSRF pre-check for a WebSocket URL (tokio-tungstenite has no resolver hook).
/// Connect re-resolves, so a rebinding peer could differ; closes literal/internal hosts.
pub async fn ws_guard(url_str: &str) -> Result<(), String> {
    let url = reqwest::Url::parse(url_str).map_err(|e| format!("invalid URL: {e}"))?;
    if let Some(ip) = host_ip_literal(&url) {
        return if is_blocked_ip(ip) {
            Err(BLOCKED_MSG.to_string())
        } else {
            Ok(())
        };
    }
    let host = url.host_str().ok_or_else(|| "missing host".to_string())?;
    let port = url.port_or_known_default().unwrap_or(443);
    let addrs = tokio::net::lookup_host((host, port))
        .await
        .map_err(|_| "dns resolution failed".to_string())?;
    let mut any = false;
    for a in addrs {
        any = true;
        if is_blocked_ip(a.ip()) {
            return Err(BLOCKED_MSG.to_string());
        }
    }
    if any {
        Ok(())
    } else {
        Err("dns resolution failed".to_string())
    }
}
