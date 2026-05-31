/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Web Client Extension
 * Copyright (C) 2024 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_WEBCLIENT_FFI_H_
#define _INCLUDE_WEBCLIENT_FFI_H_

/**
 * @file ffi.h
 * @brief C ABI contract between the C++ shim and the Rust networking core.
 *
 * This header is hand-written and MUST stay in sync with rust/src/lib.rs.
 *
 * Conventions:
 *  - Opaque object ids are uint64_t generational slot-map keys. The value 0 is
 *    never a valid id; it is the "null/invalid" sentinel returned on failure.
 *    A stale id resolves to "invalid" rather than aliasing a new object, so a
 *    use-after-free across the boundary becomes a clean error instead of UB.
 *  - Request bodies / message payloads cross as (ptr, len) byte ranges; they
 *    are NOT assumed NUL-terminated. Short strings (URLs, header names) cross
 *    as NUL-terminated `const char *`.
 *  - Pointers returned by accessor functions are borrowed and remain valid only
 *    until the owning object/event is released (see notes per function).
 *  - All functions are safe to call only as documented w.r.t. threading: every
 *    function here is invoked exclusively from SourceMod's main thread.
 */

#include <stddef.h>
#include <stdint.h>

#if defined __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Lifecycle                                                                 */
/* ------------------------------------------------------------------------- */

/**
 * Initialise the Rust runtime (tokio runtime, shared HTTP client, rustls crypto
 * provider). Call once from SDK_OnLoad. Returns 0 on success; on failure returns
 * non-zero and writes a NUL-terminated message into err (capped at err_len).
 */
int32_t wc_init(char *err, size_t err_len);

/**
 * Shut down the runtime: signal all in-flight tasks to abort, join the worker
 * threads, and drop the runtime. Queued completions are discarded WITHOUT firing
 * callbacks. Call once from SDK_OnUnload. Idempotent.
 */
void wc_shutdown(void);

/* Method codes for wc_http_submit (mirror the verb natives in natives.cpp). */
enum {
	WC_METHOD_GET = 0,
	WC_METHOD_POST = 1,
	WC_METHOD_PUT = 2,
	WC_METHOD_PATCH = 3,
	WC_METHOD_DELETE = 4,
	WC_METHOD_HEAD = 5,
	WC_METHOD_OPTIONS = 6,
};

/* Completion kinds. */
enum {
	WC_KIND_HTTP_RESPONSE = 0, /* HTTP request finished with a response   */
	WC_KIND_HTTP_ERROR = 1,    /* HTTP request failed at the transport    */
	WC_KIND_WS_EVENT = 2,      /* WebSocket lifecycle/message event       */
};

/* WebSocket event sub-kinds (returned by wc_ws_event_kind). */
enum {
	WC_WS_CONNECTED = 0,
	WC_WS_TEXT = 1,
	WC_WS_BINARY = 2,
	WC_WS_DISCONNECTED = 3,
	WC_WS_ERROR = 4,
};

/* WebSocket ready states (returned by wc_ws_state). */
enum {
	WC_WS_STATE_CONNECTING = 0,
	WC_WS_STATE_OPEN = 1,
	WC_WS_STATE_CLOSING = 2,
	WC_WS_STATE_CLOSED = 3,
};

/* ------------------------------------------------------------------------- */
/* URL encoding (pure helpers; no runtime needed)                            */
/* ------------------------------------------------------------------------- */

/* Percent-encode (URL-component set) / decode `data` into buf (NUL-terminated,
 * truncated to buf_len). Returns the number of bytes written (excluding NUL). */
int32_t wc_url_encode(const uint8_t *data, size_t len, char *buf, size_t buf_len);
int32_t wc_url_decode(const uint8_t *data, size_t len, char *buf, size_t buf_len);

/* ------------------------------------------------------------------------- */
/* HTTP: build / configure / submit                                          */
/* ------------------------------------------------------------------------- */

/**
 * Create a request builder for the given URL. Returns a builder id, or 0 on
 * failure (e.g. invalid URL bytes). The builder is consumed by wc_http_submit;
 * if never submitted it must be released with wc_http_builder_free.
 */
uint64_t wc_http_new(const uint8_t *url, size_t url_len);

void wc_http_set_header(uint64_t builder, const char *name, const uint8_t *value, size_t value_len);
/* Set an Authorization: Basic <base64(user:pass)> header. */
void wc_http_set_basic_auth(uint64_t builder, const uint8_t *user, size_t user_len, const uint8_t *pass, size_t pass_len);
void wc_http_set_body(uint64_t builder, const uint8_t *data, size_t len);       /* copies */
/* Append a URL query parameter (percent-encoded at submit time). */
void wc_http_add_query(uint64_t builder, const uint8_t *name, size_t name_len, const uint8_t *value, size_t value_len);
/* Append an x-www-form-urlencoded body part (used when no explicit body is set). */
void wc_http_add_form(uint64_t builder, const uint8_t *name, size_t name_len, const uint8_t *value, size_t value_len);
/* Use the file at `path` (absolute) as the request body; read off-thread at submit. */
void wc_http_set_body_file(uint64_t builder, const uint8_t *path, size_t path_len);
/* Stream the response body to the file at `path` (absolute) instead of buffering
 * it in memory; lifts the in-memory body cap (still bounded to avoid disk fill). */
void wc_http_set_download_file(uint64_t builder, const uint8_t *path, size_t path_len);
/* Total request timeout (0 => a built-in default is applied; never unbounded). */
void wc_http_set_timeout_ms(uint64_t builder, uint32_t ms);
uint32_t wc_http_get_timeout_ms(uint64_t builder);          /* 0 if unset */
void wc_http_set_connect_timeout_ms(uint64_t builder, uint32_t ms);
uint32_t wc_http_get_connect_timeout_ms(uint64_t builder);  /* 0 if unset */
/* Max redirects to follow (0 disables following). Getter returns the configured
 * value, or the library default if the plugin never set one. */
void wc_http_set_max_redirects(uint64_t builder, uint32_t max);
int32_t wc_http_get_max_redirects(uint64_t builder);
/* Opt out of the SSRF guard (allow private/loopback/link-local targets).
 * Off by default: such targets are refused, re-checked on every redirect hop. */
void wc_http_set_allow_private(uint64_t builder, int32_t allow);
int32_t wc_http_get_allow_private(uint64_t builder); /* 0/1 */
/* Skip TLS cert/hostname verification. Only honored together with
 * wc_http_set_allow_private, so verification is waived only for local targets. */
void wc_http_set_allow_invalid_certs(uint64_t builder, int32_t allow);
int32_t wc_http_get_allow_invalid_certs(uint64_t builder); /* 0/1 */

/** Free an unsubmitted builder. No-op if the builder was already consumed. */
void wc_http_builder_free(uint64_t builder);

/**
 * Submit the builder with the given method. Consumes the builder. Returns an
 * in-flight request id (used to key the eventual completion / response), or 0
 * on failure (runtime unavailable, or the global in-flight cap is reached).
 * owner_token ties the request to a plugin identity for bulk cancellation (see
 * wc_cancel_owner).
 */
uint64_t wc_http_submit(uint64_t builder, int32_t method, uint64_t owner_token);

/* ------------------------------------------------------------------------- */
/* Completion draining (MAIN THREAD ONLY)                                    */
/* ------------------------------------------------------------------------- */

typedef struct wc_completion_s {
	uint64_t id;     /* completion id: HTTP request id, or WS event id        */
	uint64_t target; /* owning object: HTTP request id (== id), or WS id      */
	int32_t kind;    /* one of WC_KIND_*                                      */
} wc_completion_t;

/**
 * Pop one ready completion. Returns 1 if *out was filled, 0 if the queue is
 * empty. Non-blocking. Must be called only on the main thread.
 */
int32_t wc_poll_completion(wc_completion_t *out);

/* ------------------------------------------------------------------------- */
/* HTTP response accessors (valid until wc_http_destroy(request_id))         */
/* ------------------------------------------------------------------------- */

int32_t wc_resp_status(uint64_t request_id);   /* HTTP status, or < 0 on transport error */
/* Copy the final URL (after redirects) into buf. Returns bytes written, or -1. */
int32_t wc_resp_url(uint64_t request_id, char *buf, size_t buf_len);
size_t wc_resp_body_len(uint64_t request_id);
const uint8_t *wc_resp_body_ptr(uint64_t request_id);   /* borrowed */
size_t wc_resp_header_count(uint64_t request_id);
/* Copy header i's name/value into buf (NUL-terminated, truncated to buf_len).
 * Returns the number of bytes written (excluding the NUL), or -1 if i is OOR. */
int32_t wc_resp_header_name(uint64_t request_id, size_t i, char *buf, size_t buf_len);
int32_t wc_resp_header_value(uint64_t request_id, size_t i, char *buf, size_t buf_len);
/* Copy the error string for a WC_KIND_HTTP_ERROR completion. */
int32_t wc_resp_error(uint64_t request_id, char *buf, size_t buf_len);

/**
 * Destroy an HTTP request/response: cancels it if still in flight, otherwise
 * frees the buffered response. Called from the response Handle's destructor and
 * is idempotent. Safe for any (even stale) id.
 */
void wc_http_destroy(uint64_t request_id);

/* ------------------------------------------------------------------------- */
/* WebSocket                                                                 */
/* ------------------------------------------------------------------------- */

/** Create a (not yet connected) WebSocket for ws:// or wss:// url. 0 on failure. */
uint64_t wc_ws_new(const uint8_t *url, size_t url_len);
void wc_ws_set_header(uint64_t ws, const char *name, const uint8_t *value, size_t value_len);
/* Opt out of the SSRF pre-check for this socket (allow private/loopback/
 * link-local). Off by default; only effective before wc_ws_connect. */
void wc_ws_set_allow_private(uint64_t ws, int32_t allow);
int32_t wc_ws_get_allow_private(uint64_t ws); /* 0/1 */
/* Skip TLS cert/hostname verification for this socket. Only honored together with
 * wc_ws_set_allow_private; effective only before wc_ws_connect. */
void wc_ws_set_allow_invalid_certs(uint64_t ws, int32_t allow);
int32_t wc_ws_get_allow_invalid_certs(uint64_t ws); /* 0/1 */
/** Begin connecting. Connection result + messages + close arrive as completions.
 * Returns non-zero on failure (runtime unavailable, or the live-socket cap). */
int32_t wc_ws_connect(uint64_t ws, uint64_t owner_token);
/** Queue an outbound message. is_text != 0 => text frame, else binary. 0 on
 * success; non-zero if the socket is gone or the bounded send queue is full. */
int32_t wc_ws_send(uint64_t ws, int32_t is_text, const uint8_t *data, size_t len);
/** Queue a ping frame. 0 on success; non-zero if gone or the send queue is full. */
int32_t wc_ws_ping(uint64_t ws, const uint8_t *data, size_t len);
/** Current ready state (one of WC_WS_STATE_*), or -1 if the id is unknown. */
int32_t wc_ws_state(uint64_t ws);
/** Request a graceful close with the given code/reason. */
void wc_ws_close(uint64_t ws, uint16_t code, const uint8_t *reason, size_t reason_len);
/** Destroy: force-close and free state. Idempotent; from the Handle destructor. */
void wc_ws_destroy(uint64_t ws);

/* WebSocket event accessors (valid until wc_ws_event_release(event_id)). */
int32_t wc_ws_event_kind(uint64_t event_id);        /* one of WC_WS_*        */
size_t wc_ws_event_data_len(uint64_t event_id);
const uint8_t *wc_ws_event_data_ptr(uint64_t event_id);   /* borrowed        */
uint16_t wc_ws_event_close_code(uint64_t event_id);       /* for DISCONNECTED */
/** Release a WebSocket event's buffered payload after dispatching it. */
void wc_ws_event_release(uint64_t event_id);

/* ------------------------------------------------------------------------- */
/* Cancellation                                                              */
/* ------------------------------------------------------------------------- */

/** Cancel one in-flight request owned by owner_token: abort it, drop its slot,
 * and discard any queued completion. Returns 1 if cancelled, 0 otherwise. */
int32_t wc_http_cancel(uint64_t request_id, uint64_t owner_token);

/** Abort every in-flight request/socket owned by owner_token and drop their
 * queued completions. Called when a plugin unloads. */
void wc_cancel_owner(uint64_t owner_token);

#if defined __cplusplus
}
#endif

#endif /* _INCLUDE_WEBCLIENT_FFI_H_ */
