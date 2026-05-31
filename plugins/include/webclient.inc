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

#if defined _webclient_included
 #endinput
#endif
#define _webclient_included

// HTTP status codes, for comparing against HTTPResponse.Status. Non-exhaustive:
// for a code without a named constant, compare numerically with view_as, e.g.
// `view_as<int>(response.Status) == 418`.
enum HTTPStatus
{
	// Placeholder for an unset/unknown status. A real transport failure delivers
	// INVALID_HANDLE (no response), so you normally won't see this value.
	HTTPStatus_Invalid = 0,

	// 1xx Informational
	HTTPStatus_Continue = 100,
	HTTPStatus_SwitchingProtocols = 101,

	// 2xx Success
	HTTPStatus_OK = 200,
	HTTPStatus_Created = 201,
	HTTPStatus_Accepted = 202,
	HTTPStatus_NonAuthoritativeInformation = 203,
	HTTPStatus_NoContent = 204,
	HTTPStatus_ResetContent = 205,
	HTTPStatus_PartialContent = 206,

	// 3xx Redirection
	HTTPStatus_MultipleChoices = 300,
	HTTPStatus_MovedPermanently = 301,
	HTTPStatus_Found = 302,
	HTTPStatus_SeeOther = 303,
	HTTPStatus_NotModified = 304,
	HTTPStatus_UseProxy = 305,
	HTTPStatus_TemporaryRedirect = 307,
	HTTPStatus_PermanentRedirect = 308,

	// 4xx Client Error
	HTTPStatus_BadRequest = 400,
	HTTPStatus_Unauthorized = 401,
	HTTPStatus_PaymentRequired = 402,
	HTTPStatus_Forbidden = 403,
	HTTPStatus_NotFound = 404,
	HTTPStatus_MethodNotAllowed = 405,
	HTTPStatus_NotAcceptable = 406,
	HTTPStatus_ProxyAuthenticationRequired = 407,
	HTTPStatus_RequestTimeout = 408,
	HTTPStatus_Conflict = 409,
	HTTPStatus_Gone = 410,
	HTTPStatus_LengthRequired = 411,
	HTTPStatus_PreconditionFailed = 412,
	HTTPStatus_RequestEntityTooLarge = 413,
	HTTPStatus_RequestURITooLong = 414,
	HTTPStatus_UnsupportedMediaType = 415,
	HTTPStatus_RequestedRangeNotSatisfiable = 416,
	HTTPStatus_ExpectationFailed = 417,
	HTTPStatus_MisdirectedRequest = 421,
	HTTPStatus_TooEarly = 425,
	HTTPStatus_UpgradeRequired = 426,
	HTTPStatus_PreconditionRequired = 428,
	HTTPStatus_TooManyRequests = 429,
	HTTPStatus_RequestHeaderFieldsTooLarge = 431,
	HTTPStatus_UnavailableForLegalReasons = 451,

	// 5xx Server Error
	HTTPStatus_InternalServerError = 500,
	HTTPStatus_NotImplemented = 501,
	HTTPStatus_BadGateway = 502,
	HTTPStatus_ServiceUnavailable = 503,
	HTTPStatus_GatewayTimeout = 504,
	HTTPStatus_HTTPVersionNotSupported = 505,
	HTTPStatus_VariantAlsoNegotiates = 506,
	HTTPStatus_NotExtended = 510,
	HTTPStatus_NetworkAuthenticationRequired = 511,
};

/**
 * Called once when an HTTP request completes, on the main thread.
 *
 * @param response  The response object. Valid ONLY for the duration of this
 *                  callback (do not store it, and do not delete/close it; the
 *                  extension frees it when the callback returns). On transport
 *                  failure this is INVALID_HANDLE and `failure` is true.
 * @param data      The user value passed to Get/Post/etc.
 * @param failure   True if the request failed before producing a response
 *                  (DNS/connect/TLS/timeout); inspect `error` in that case.
 * @param error     Human-readable error string when `failure` is true.
 */
typedef HTTPRequestCallback = function void (HTTPResponse response, any data, bool failure, const char[] error);

methodmap HTTPResponse < Handle
{
	// HTTP status code (e.g. HTTPStatus_OK). Only meaningful when the callback's
	// `failure` is false; on transport failure you get INVALID_HANDLE, not a
	// response. Tagged HTTPStatus: compare with named constants (== HTTPStatus_OK)
	// or wrap in view_as<int>(...) for numeric/range comparisons.
	property HTTPStatus Status {
		public native get();
	}

	// Length of the response body in bytes.
	property int Length {
		public native get();
	}

	// Number of response headers. Iterate 0..Headers-1 with GetHeaderName/
	// GetHeaderValue to see what's present (names can repeat, e.g. Set-Cookie).
	property int Headers {
		public native get();
	}

	// True for a 2xx status.
	public bool IsSuccess() {
		int status = view_as<int>(this.Status);
		return status >= 200 && status < 300;
	}

	// True for a 3xx status (only seen when redirects are disabled/capped out).
	public bool IsRedirect() {
		int status = view_as<int>(this.Status);
		return status >= 300 && status < 400;
	}

	// True for a 4xx status.
	public bool IsClientError() {
		int status = view_as<int>(this.Status);
		return status >= 400 && status < 500;
	}

	// True for a 5xx status.
	public bool IsServerError() {
		int status = view_as<int>(this.Status);
		return status >= 500 && status < 600;
	}

	// Copy the response body into buffer (as text, NUL-terminated). Returns the
	// number of bytes written.
	public native int GetData(char[] buffer, int maxlen);

	// Copy the body preserving interior NUL bytes (for binary payloads). Returns
	// the number of bytes written; the result is not meaningful as a C string.
	public native int GetDataBinary(char[] buffer, int maxlen);

	// Copy the final URL (after any redirects) into buffer. Returns bytes written.
	public native int GetURL(char[] buffer, int maxlen);

	// Write the response body to a file. `path` is a relative path confined to
	// addons/sourcemod/data/webclient (".." and absolute paths are rejected).
	// Returns true on success. Writes synchronously on the main thread.
	public native bool SaveToFile(const char[] path);

	// Look up a response header by name (case-insensitive). Returns true and
	// fills buffer if present.
	public native bool GetHeader(const char[] name, char[] buffer, int maxlen);

	// Name/value of the header at index (0-based). false if index is out of range.
	public native bool GetHeaderName(int index, char[] buffer, int maxlen);
	public native bool GetHeaderValue(int index, char[] buffer, int maxlen);
}

methodmap HTTPRequest < Handle
{
	// Create a request for the given URL (http:// or https://). The request is
	// consumed (and its Handle closed automatically) when you call one of the
	// method verbs below; do not close it yourself.
	// @error  Invalid URL string, or the request couldn't be created.
	public native HTTPRequest(const char[] url);

	// Add or set a request header. The value is a format string; pass user data
	// via "%s" so a literal '%' isn't interpreted.
	// @error  Invalid request handle or header name.
	public native void SetHeader(const char[] name, const char[] format, any ...);

	// Append a URL query parameter (name=value), percent-encoded. The value is a
	// format string. Applied to the URL when the request is sent.
	public native void AppendQueryParam(const char[] name, const char[] format, any ...);

	// Append an application/x-www-form-urlencoded body parameter. The value is a
	// format string. Send the accumulated parameters with PostForm().
	public native void AppendFormParam(const char[] name, const char[] format, any ...);

	// Total request timeout, in seconds. 0 (or unset) applies a built-in
	// default; the request is never left to run unbounded.
	property int Timeout {
		public native get();
		public native set(int seconds);
	}

	// Connection-phase timeout, in seconds. 0 (or unset) applies a built-in
	// default connect timeout.
	property int ConnectTimeout {
		public native get();
		public native set(int seconds);
	}

	// Maximum number of 3xx redirects to follow. 0 disables following. The getter
	// returns the library default when never set.
	property int MaxRedirects {
		public native get();
		public native set(int maxRedirects);
	}

	// Convenience over MaxRedirects: follow up to 10 redirects, or not at all.
	public void FollowRedirects(bool follow) {
		this.MaxRedirects = follow ? 10 : 0;
	}

	// Allow connecting to private/loopback/link-local addresses. Off by default:
	// such targets (incl. the cloud metadata IP) are refused to prevent SSRF,
	// re-checked on every redirect hop. Enable only for trusted intranet URLs.
	property bool AllowLocalNetwork {
		public native get();
		public native set(bool allow);
	}

	// Skip TLS certificate/hostname verification. Off by default, and ONLY honored
	// when AllowLocalNetwork is also enabled -- so verification can be waived for
	// a private/local target (e.g. a self-signed or friendly-named intranet
	// service), never for a public host. Leave off unless you trust the network.
	property bool AllowInvalidCerts {
		public native get();
		public native set(bool allow);
	}

	// Send an Authorization: Basic header built from username:password.
	public native void SetBasicAuth(const char[] username, const char[] password);

	// Convenience for Authorization: Bearer <token>.
	public void SetBearerToken(const char[] token) {
		this.SetHeader("Authorization", "Bearer %s", token);
	}

	// Send the request with the corresponding method. Returns a token usable with
	// WebClient_CancelRequest() to cancel before completion. The callback fires
	// later, on the main thread.
	// @error  Invalid callback, or submission failed (runtime down, or too many
	//         in-flight requests). UploadFile/DownloadFile also error on an unsafe
	//         path (see those methods).
	public native int Get(HTTPRequestCallback callback, any data = 0);
	// For Post/Put/Patch a negative length uses the NUL-terminated string length;
	// pass an explicit length to send a body that contains NUL bytes.
	public native int Post(HTTPRequestCallback callback, const char[] body, any data = 0, int length = -1);
	public native int Put(HTTPRequestCallback callback, const char[] body, any data = 0, int length = -1);
	public native int Patch(HTTPRequestCallback callback, const char[] body, any data = 0, int length = -1);
	public native int Delete(HTTPRequestCallback callback, any data = 0);

	// HEAD/OPTIONS: no request body; the response has status + headers only.
	public native int Head(HTTPRequestCallback callback, any data = 0);
	public native int Options(HTTPRequestCallback callback, any data = 0);

	// POST the parameters added with AppendFormParam as an
	// application/x-www-form-urlencoded body.
	public native int PostForm(HTTPRequestCallback callback, any data = 0);

	// Upload a file as the request body via HTTP PUT. `path` is a relative path
	// confined to addons/sourcemod/data/webclient (".." and absolute paths are
	// rejected); it is read off the main thread (subject to the body cap).
	// Returns a cancellation token (see WebClient_CancelRequest).
	public native int UploadFile(const char[] path, HTTPRequestCallback callback, any data = 0);

	// HTTP GET that streams the response body to a file instead of buffering it
	// in memory, so it isn't bound by the in-memory body cap. `path` is a relative
	// path confined to addons/sourcemod/data/webclient (".." and absolute paths
	// rejected). Only a 2xx body is written; in the callback the response body is empty
	// (check Status/IsSuccess). A failed or non-2xx download leaves no file.
	// Returns a cancellation token (see WebClient_CancelRequest).
	public native int DownloadFile(const char[] path, HTTPRequestCallback callback, any data = 0);
}

// WebSocket connection state, returned by WebSocket.ReadyState.
enum WebSocketReadyState
{
	WebSocket_Connecting = 0,
	WebSocket_Open       = 1,
	WebSocket_Closing    = 2,
	WebSocket_Closed     = 3,
};

/** Called when a WebSocket finishes connecting. */
typedef WebSocketConnectCallback = function void (WebSocket ws, any data);

/**
 * Called for each inbound WebSocket message.
 *
 * @param ws        The socket.
 * @param data      The user value passed to Connect().
 * @param payload   Message bytes. Text frames are NUL-terminated; binary frames
 *                  are delivered as exactly `length` bytes and may contain
 *                  interior NUL bytes, so use `length` rather than scanning.
 * @param length    Message length in bytes.
 * @param isText    True for a text frame, false for binary.
 */
typedef WebSocketMessageCallback = function void (WebSocket ws, any data, const char[] payload, int length, bool isText);

/**
 * Called once when the socket closes (peer close, transport drop, or a local
 * Close()). Exactly one terminal callback fires per socket - this one OR the
 * error callback, never both - so this is a safe place to delete the handle.
 */
typedef WebSocketDisconnectCallback = function void (WebSocket ws, any data, int code, const char[] reason);

/**
 * Called when a connection or protocol error occurs. Terminal: the disconnect
 * callback does NOT also fire, so delete the handle here too (see the example).
 */
typedef WebSocketErrorCallback = function void (WebSocket ws, any data, const char[] error);

methodmap WebSocket < Handle
{
	// Create a WebSocket for ws:// or wss:// (not yet connected). Close the
	// Handle (delete ws / CloseHandle) to disconnect and free it.
	// @error  Invalid URL string, or the socket couldn't be created.
	public native WebSocket(const char[] url);

	// Add a handshake header. Must be called before Connect(). The value is a
	// format string (matching HTTPRequest.SetHeader); pass user data via "%s".
	// @error  Invalid socket handle or header name.
	public native void SetHeader(const char[] name, const char[] format, any ...);

	// Allow connecting to private/loopback/link-local addresses. Off by default:
	// such targets are refused (SSRF guard, like HTTPRequest). Must be called
	// before Connect(). Enable only for trusted intranet URLs.
	property bool AllowLocalNetwork {
		public native get();
		public native set(bool allow);
	}

	// Skip TLS certificate/hostname verification. Off by default, and ONLY honored
	// when AllowLocalNetwork is also enabled (same gating as HTTPRequest). Must be
	// set before Connect(). For self-signed/local wss:// targets only.
	property bool AllowInvalidCerts {
		public native get();
		public native set(bool allow);
	}

	// Begin connecting. onConnect fires once the handshake completes.
	// @error  Invalid socket handle/callback, or the connection couldn't start.
	public native void Connect(WebSocketConnectCallback onConnect, any data = 0);

	// Register lifecycle callbacks (may be set any time before/after Connect).
	public native void SetMessageCallback(WebSocketMessageCallback callback);
	public native void SetDisconnectCallback(WebSocketDisconnectCallback callback);
	public native void SetErrorCallback(WebSocketErrorCallback callback);

	// Send a text frame; length < 0 uses the string length, and a smaller value
	// only truncates (text is NUL-terminated - use SendBinary for bytes with
	// interior NUL). Returns false if the socket is gone or the send queue is full.
	// @error  Invalid socket handle.
	public native bool Send(const char[] data, int length = -1);

	// Send a binary frame of exactly `length` bytes. Returns false if the socket
	// is gone or the send queue is full.
	// @error  Invalid socket handle, or negative length.
	public native bool SendBinary(const char[] data, int length);

	// Queue a ping frame (optional payload; negative length uses strlen). false if
	// the socket is gone or the send queue is full.
	// @error  Invalid socket handle.
	public native bool Ping(const char[] data = "", int length = -1);

	// Current connection state (Connecting/Open/Closing/Closed).
	property WebSocketReadyState ReadyState {
		public native get();
	}

	// True once the handshake has completed and the socket is open.
	public bool IsConnected() {
		return this.ReadyState == WebSocket_Open;
	}

	// Request a graceful close with the given status code/reason.
	public native void Close(int code = 1000, const char[] reason = "");
}

/**
 * Cancel an in-flight HTTP request by the token returned from Get/Post/etc.
 * Returns true if it was still in flight and got cancelled (its callback then
 * never fires); false if the token is unknown or the request already finished.
 */
native bool WebClient_CancelRequest(int token);

/**
 * Percent-encode / decode a string using URL-component rules (only A-Z a-z 0-9
 * - _ . ~ pass through unescaped). Returns the number of bytes written.
 */
native int WebClient_URLEncode(const char[] input, char[] output, int maxlen);
native int WebClient_URLDecode(const char[] input, char[] output, int maxlen);

/**
 * Do not edit below this line.
 */
public Extension __ext_webclient =
{
	name = "WebClient",
	file = "webclient.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public void __ext_webclient_SetNTVOptional()
{
	MarkNativeAsOptional("HTTPRequest.HTTPRequest");
	MarkNativeAsOptional("HTTPRequest.SetHeader");
	MarkNativeAsOptional("HTTPRequest.AppendQueryParam");
	MarkNativeAsOptional("HTTPRequest.AppendFormParam");
	MarkNativeAsOptional("HTTPRequest.Timeout.get");
	MarkNativeAsOptional("HTTPRequest.Timeout.set");
	MarkNativeAsOptional("HTTPRequest.ConnectTimeout.get");
	MarkNativeAsOptional("HTTPRequest.ConnectTimeout.set");
	MarkNativeAsOptional("HTTPRequest.MaxRedirects.get");
	MarkNativeAsOptional("HTTPRequest.MaxRedirects.set");
	MarkNativeAsOptional("HTTPRequest.AllowLocalNetwork.get");
	MarkNativeAsOptional("HTTPRequest.AllowLocalNetwork.set");
	MarkNativeAsOptional("HTTPRequest.AllowInvalidCerts.get");
	MarkNativeAsOptional("HTTPRequest.AllowInvalidCerts.set");
	MarkNativeAsOptional("HTTPRequest.SetBasicAuth");
	MarkNativeAsOptional("HTTPRequest.Get");
	MarkNativeAsOptional("HTTPRequest.Post");
	MarkNativeAsOptional("HTTPRequest.Put");
	MarkNativeAsOptional("HTTPRequest.Patch");
	MarkNativeAsOptional("HTTPRequest.Delete");
	MarkNativeAsOptional("HTTPRequest.Head");
	MarkNativeAsOptional("HTTPRequest.Options");
	MarkNativeAsOptional("HTTPRequest.PostForm");
	MarkNativeAsOptional("HTTPRequest.UploadFile");
	MarkNativeAsOptional("HTTPRequest.DownloadFile");
	MarkNativeAsOptional("HTTPResponse.Status.get");
	MarkNativeAsOptional("HTTPResponse.Length.get");
	MarkNativeAsOptional("HTTPResponse.Headers.get");
	MarkNativeAsOptional("HTTPResponse.GetData");
	MarkNativeAsOptional("HTTPResponse.GetDataBinary");
	MarkNativeAsOptional("HTTPResponse.GetURL");
	MarkNativeAsOptional("HTTPResponse.SaveToFile");
	MarkNativeAsOptional("HTTPResponse.GetHeader");
	MarkNativeAsOptional("HTTPResponse.GetHeaderName");
	MarkNativeAsOptional("HTTPResponse.GetHeaderValue");
	MarkNativeAsOptional("WebSocket.WebSocket");
	MarkNativeAsOptional("WebSocket.SetHeader");
	MarkNativeAsOptional("WebSocket.AllowLocalNetwork.get");
	MarkNativeAsOptional("WebSocket.AllowLocalNetwork.set");
	MarkNativeAsOptional("WebSocket.AllowInvalidCerts.get");
	MarkNativeAsOptional("WebSocket.AllowInvalidCerts.set");
	MarkNativeAsOptional("WebSocket.Connect");
	MarkNativeAsOptional("WebSocket.SetMessageCallback");
	MarkNativeAsOptional("WebSocket.SetDisconnectCallback");
	MarkNativeAsOptional("WebSocket.SetErrorCallback");
	MarkNativeAsOptional("WebSocket.Send");
	MarkNativeAsOptional("WebSocket.SendBinary");
	MarkNativeAsOptional("WebSocket.Ping");
	MarkNativeAsOptional("WebSocket.ReadyState.get");
	MarkNativeAsOptional("WebSocket.Close");
	MarkNativeAsOptional("WebClient_CancelRequest");
	MarkNativeAsOptional("WebClient_URLEncode");
	MarkNativeAsOptional("WebClient_URLDecode");
}
#endif
