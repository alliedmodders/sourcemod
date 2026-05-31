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

#include "extension.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <new>

/**
 * @file natives.cpp
 * @brief Pawn-facing natives for the HTTPRequest/HTTPResponse/WebSocket API.
 *
 * Most strings cross from Pawn NUL-terminated (a Pawn limitation), so the text
 * natives treat bodies/payloads as text up to the first NUL. Binary-safe paths
 * carry an explicit length: HTTP request bodies (Post/Put with a length arg),
 * HTTPResponse.GetDataBinary, WebSocket.SendBinary, and inbound binary WS frames
 * (delivered as a counted binary buffer, not truncated at a NUL).
 */

static inline uint64_t OwnerToken(IPluginContext *pContext)
{
	return (uint64_t)(uintptr_t)pContext->GetIdentity();
}

/* Convert a Pawn seconds value to milliseconds without overflowing uint32: a
 * non-positive value means "no explicit timeout" (Rust applies its default),
 * and large values saturate instead of wrapping. */
static inline uint32_t SecondsToMs(cell_t seconds)
{
	if (seconds <= 0)
		return 0;
	uint64_t ms = (uint64_t)seconds * 1000ull;
	return ms > UINT32_MAX ? UINT32_MAX : (uint32_t)ms;
}

/* Clamp plugin-supplied copy-out buffer sizes so a bogus huge value can't throw
 * std::bad_alloc across the native boundary (the VM can't unwind it). NoThrowBuf
 * (extension.h) does the allocations and yields NULL instead of throwing. */
static const cell_t kMaxStrBuf = 16 * 1024 * 1024;

/* Reject paths that could escape confinement: absolute, drive/scheme (the ':'
 * test also kills file://), or any ".." component. */
static bool RelPathIsSafe(const char *p)
{
	if (p == NULL || p[0] == '\0')
		return false;
	if (p[0] == '/' || p[0] == '\\')
		return false;                       /* absolute */
	if (strchr(p, ':') != NULL)
		return false;                       /* scheme:// or drive letter */
	for (const char *s = p; *s != '\0'; )
	{
		if (s[0] == '.' && s[1] == '.' &&
			(s[2] == '\0' || s[2] == '/' || s[2] == '\\'))
			return false;                   /* ".." component */
		const char *sep = strpbrk(s, "/\\");
		if (sep == NULL)
			break;
		s = sep + 1;
	}
	return true;
}

/* Resolve a relative path inside SourceMod's data dir, addons/sourcemod/data/
 * webclient (created on demand) -- the conventional place for extension data.
 * False if the path is unsafe or the directory can't be created. */
static bool BuildConfinedPath(const char *rel, char *out, size_t outlen)
{
	if (!RelPathIsSafe(rel))
		return false;

	char dir[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_SM, dir, sizeof(dir), "data");
	libsys->CreateFolder(dir);              /* no-op if it already exists */
	smutils->BuildPath(Path_SM, dir, sizeof(dir), "data/webclient");
	libsys->CreateFolder(dir);
	if (!libsys->IsPathDirectory(dir))
		return false;

	smutils->BuildPath(Path_SM, out, outlen, "data/webclient/%s", rel);
	return true;
}

/* ------------------------------------------------------------------------- */
/* HTTPRequest                                                               */
/* ------------------------------------------------------------------------- */

static cell_t Native_HTTPRequest(IPluginContext *pContext, const cell_t *params)
{
	char *url;
	if (pContext->LocalToString(params[1], &url) != SP_ERROR_NONE)
		return pContext->ThrowNativeError("Invalid URL string");

	uint64_t builder = wc_http_new((const uint8_t *)url, strlen(url));
	if (builder == 0)
		return pContext->ThrowNativeError("Failed to create HTTP request");

	Handle_t handle = CreateIdHandle(g_HttpRequestType, builder, pContext->GetIdentity());
	if (handle == 0)
	{
		wc_http_builder_free(builder);
		return pContext->ThrowNativeError("Failed to create HTTPRequest handle");
	}
	return handle;
}

static cell_t Native_SetHeader(IPluginContext *pContext, const cell_t *params)
{
	uint64_t builder;
	if (!ReadIdHandle(params[1], g_HttpRequestType, &builder))
		return pContext->ThrowNativeError("Invalid HTTPRequest handle %x", params[1]);

	char *name;
	if (pContext->LocalToString(params[2], &name) != SP_ERROR_NONE)
		return pContext->ThrowNativeError("Invalid header name string");

	/* Value is a format string (params[3] + varargs). Callers passing a literal
	 * that may contain '%' must use "%s" (see SetBearerToken). */
	char value[4096];
	smutils->FormatString(value, sizeof(value), pContext, params, 3);
	wc_http_set_header(builder, name, (const uint8_t *)value, strlen(value));
	return 0;
}

/* Shared body for AppendQueryParam/AppendFormParam: name is plain (params[2]),
 * the value is a format string (params[3] + varargs). */
static cell_t AppendParam(IPluginContext *pContext, const cell_t *params,
	void (*add)(uint64_t, const uint8_t *, size_t, const uint8_t *, size_t))
{
	uint64_t builder;
	if (!ReadIdHandle(params[1], g_HttpRequestType, &builder))
		return pContext->ThrowNativeError("Invalid HTTPRequest handle %x", params[1]);

	char *name;
	if (pContext->LocalToString(params[2], &name) != SP_ERROR_NONE)
		return pContext->ThrowNativeError("Invalid parameter name string");

	char value[2048];
	smutils->FormatString(value, sizeof(value), pContext, params, 3);
	add(builder, (const uint8_t *)name, strlen(name), (const uint8_t *)value, strlen(value));
	return 0;
}

static cell_t Native_AppendQueryParam(IPluginContext *pContext, const cell_t *params)
{
	return AppendParam(pContext, params, wc_http_add_query);
}

static cell_t Native_AppendFormParam(IPluginContext *pContext, const cell_t *params)
{
	return AppendParam(pContext, params, wc_http_add_form);
}

/* Timeout / ConnectTimeout are exposed as int properties (seconds). The getter
 * returns the configured value, or 0 when unset (meaning "use the default"). */
static cell_t Native_Timeout_get(IPluginContext *pContext, const cell_t *params)
{
	uint64_t builder;
	if (!ReadIdHandle(params[1], g_HttpRequestType, &builder))
		return pContext->ThrowNativeError("Invalid HTTPRequest handle %x", params[1]);
	return (cell_t)(wc_http_get_timeout_ms(builder) / 1000);
}

static cell_t Native_Timeout_set(IPluginContext *pContext, const cell_t *params)
{
	uint64_t builder;
	if (!ReadIdHandle(params[1], g_HttpRequestType, &builder))
		return pContext->ThrowNativeError("Invalid HTTPRequest handle %x", params[1]);
	wc_http_set_timeout_ms(builder, SecondsToMs(params[2]));
	return 0;
}

static cell_t Native_ConnectTimeout_get(IPluginContext *pContext, const cell_t *params)
{
	uint64_t builder;
	if (!ReadIdHandle(params[1], g_HttpRequestType, &builder))
		return pContext->ThrowNativeError("Invalid HTTPRequest handle %x", params[1]);
	return (cell_t)(wc_http_get_connect_timeout_ms(builder) / 1000);
}

static cell_t Native_ConnectTimeout_set(IPluginContext *pContext, const cell_t *params)
{
	uint64_t builder;
	if (!ReadIdHandle(params[1], g_HttpRequestType, &builder))
		return pContext->ThrowNativeError("Invalid HTTPRequest handle %x", params[1]);
	wc_http_set_connect_timeout_ms(builder, SecondsToMs(params[2]));
	return 0;
}

/* MaxRedirects int property: 0 disables following; the getter returns the
 * library default when the plugin never set one. A negative set clamps to 0. */
static cell_t Native_MaxRedirects_get(IPluginContext *pContext, const cell_t *params)
{
	uint64_t builder;
	if (!ReadIdHandle(params[1], g_HttpRequestType, &builder))
		return pContext->ThrowNativeError("Invalid HTTPRequest handle %x", params[1]);
	return (cell_t)wc_http_get_max_redirects(builder);
}

static cell_t Native_MaxRedirects_set(IPluginContext *pContext, const cell_t *params)
{
	uint64_t builder;
	if (!ReadIdHandle(params[1], g_HttpRequestType, &builder))
		return pContext->ThrowNativeError("Invalid HTTPRequest handle %x", params[1]);
	wc_http_set_max_redirects(builder, params[2] < 0 ? 0 : (uint32_t)params[2]);
	return 0;
}

static cell_t Native_AllowLocalNetwork_get(IPluginContext *pContext, const cell_t *params)
{
	uint64_t builder;
	if (!ReadIdHandle(params[1], g_HttpRequestType, &builder))
		return pContext->ThrowNativeError("Invalid HTTPRequest handle %x", params[1]);
	return wc_http_get_allow_private(builder) ? 1 : 0;
}

static cell_t Native_AllowLocalNetwork_set(IPluginContext *pContext, const cell_t *params)
{
	uint64_t builder;
	if (!ReadIdHandle(params[1], g_HttpRequestType, &builder))
		return pContext->ThrowNativeError("Invalid HTTPRequest handle %x", params[1]);
	wc_http_set_allow_private(builder, params[2] ? 1 : 0);
	return 0;
}

static cell_t Native_AllowInvalidCerts_get(IPluginContext *pContext, const cell_t *params)
{
	uint64_t builder;
	if (!ReadIdHandle(params[1], g_HttpRequestType, &builder))
		return pContext->ThrowNativeError("Invalid HTTPRequest handle %x", params[1]);
	return wc_http_get_allow_invalid_certs(builder) ? 1 : 0;
}

static cell_t Native_AllowInvalidCerts_set(IPluginContext *pContext, const cell_t *params)
{
	uint64_t builder;
	if (!ReadIdHandle(params[1], g_HttpRequestType, &builder))
		return pContext->ThrowNativeError("Invalid HTTPRequest handle %x", params[1]);
	wc_http_set_allow_invalid_certs(builder, params[2] ? 1 : 0);
	return 0;
}

static cell_t Native_SetBasicAuth(IPluginContext *pContext, const cell_t *params)
{
	uint64_t builder;
	if (!ReadIdHandle(params[1], g_HttpRequestType, &builder))
		return pContext->ThrowNativeError("Invalid HTTPRequest handle %x", params[1]);

	char *user, *pass;
	if (pContext->LocalToString(params[2], &user) != SP_ERROR_NONE ||
		pContext->LocalToString(params[3], &pass) != SP_ERROR_NONE)
		return pContext->ThrowNativeError("Invalid credentials string");
	wc_http_set_basic_auth(builder, (const uint8_t *)user, strlen(user),
		(const uint8_t *)pass, strlen(pass));
	return 0;
}

/* Shared submit path for Get/Post/Put/Patch/Delete/Head/Options/PostForm/Upload/
 * Download. cbArg = param index of the callback; bodyArg = index of the body
 * string (0 if none); dataArg = the user value; lenArg = optional explicit body
 * length (0 if none). The HTTPRequest Handle is consumed (auto-closed) on submit;
 * returns a cancellation token. */
static cell_t SubmitHttp(IPluginContext *pContext, const cell_t *params, int method, int cbArg, int bodyArg, int dataArg, int lenArg)
{
	uint64_t builder;
	if (!ReadIdHandle(params[1], g_HttpRequestType, &builder))
		return pContext->ThrowNativeError("Invalid HTTPRequest handle %x", params[1]);

	IPluginFunction *fn = pContext->GetFunctionById(params[cbArg]);
	if (fn == NULL)
		return pContext->ThrowNativeError("Invalid callback function");

	if (bodyArg != 0)
	{
		char *body;
		if (pContext->LocalToString(params[bodyArg], &body) != SP_ERROR_NONE)
			return pContext->ThrowNativeError("Invalid body string");
		/* An explicit length (when supplied and >= 0) lets a body carry interior
		 * NUL bytes; otherwise use the NUL-terminated string length. */
		size_t blen;
		if (lenArg != 0 && params[0] >= lenArg && params[lenArg] >= 0)
			blen = (size_t)params[lenArg];
		else
			blen = strlen(body);
		wc_http_set_body(builder, (const uint8_t *)body, blen);
	}

	IdentityToken_t *owner = pContext->GetIdentity();
	uint64_t request = wc_http_submit(builder, method, OwnerToken(pContext));

	/* The builder Handle is spent; close it for the caller. */
	HandleSecurity sec(owner, myself->GetIdentity());
	handlesys->FreeHandle(params[1], &sec);

	if (request == 0)
		return pContext->ThrowNativeError("Failed to submit HTTP request (runtime unavailable or too many in-flight requests)");

	/* Token the plugin can pass to WebClient_CancelRequest before completion. */
	return (cell_t)webclient::RegisterHttp(request, fn, params[dataArg], owner);
}

static cell_t Native_Get(IPluginContext *pContext, const cell_t *params)
{
	return SubmitHttp(pContext, params, WC_METHOD_GET, 2, 0, 3, 0);
}
static cell_t Native_Post(IPluginContext *pContext, const cell_t *params)
{
	return SubmitHttp(pContext, params, WC_METHOD_POST, 2, 3, 4, 5);
}
static cell_t Native_Put(IPluginContext *pContext, const cell_t *params)
{
	return SubmitHttp(pContext, params, WC_METHOD_PUT, 2, 3, 4, 5);
}
static cell_t Native_Patch(IPluginContext *pContext, const cell_t *params)
{
	return SubmitHttp(pContext, params, WC_METHOD_PATCH, 2, 3, 4, 5);
}
static cell_t Native_Delete(IPluginContext *pContext, const cell_t *params)
{
	return SubmitHttp(pContext, params, WC_METHOD_DELETE, 2, 0, 3, 0);
}
static cell_t Native_Head(IPluginContext *pContext, const cell_t *params)
{
	return SubmitHttp(pContext, params, WC_METHOD_HEAD, 2, 0, 3, 0);
}
static cell_t Native_Options(IPluginContext *pContext, const cell_t *params)
{
	return SubmitHttp(pContext, params, WC_METHOD_OPTIONS, 2, 0, 3, 0);
}

/* POST the accumulated x-www-form-urlencoded parameters (no body string). */
static cell_t Native_PostForm(IPluginContext *pContext, const cell_t *params)
{
	return SubmitHttp(pContext, params, WC_METHOD_POST, 2, 0, 3, 0);
}

/* Resolve a plugin-supplied path under the game dir and stash it on the builder
 * via `set` (wc_http_set_body_file / wc_http_set_download_file), then submit. */
static cell_t SubmitWithFile(IPluginContext *pContext, const cell_t *params, int method,
	void (*set)(uint64_t, const uint8_t *, size_t))
{
	uint64_t builder;
	if (!ReadIdHandle(params[1], g_HttpRequestType, &builder))
		return pContext->ThrowNativeError("Invalid HTTPRequest handle %x", params[1]);

	char *path;
	if (pContext->LocalToString(params[2], &path) != SP_ERROR_NONE)
		return pContext->ThrowNativeError("Invalid path string");

	// Confine the plugin path to addons/sourcemod/data/webclient; the read/write runs off-thread.
	char full[PLATFORM_MAX_PATH];
	if (!BuildConfinedPath(path, full, sizeof(full)))
		return pContext->ThrowNativeError("Unsafe or invalid path '%s' (must be a relative path under data/webclient)", path);
	set(builder, (const uint8_t *)full, strlen(full));

	/* path=2, callback=3, value=4. */
	return SubmitHttp(pContext, params, method, 3, 0, 4, 0);
}

/* Upload a file as the request body (HTTP PUT). */
static cell_t Native_UploadFile(IPluginContext *pContext, const cell_t *params)
{
	return SubmitWithFile(pContext, params, WC_METHOD_PUT, wc_http_set_body_file);
}

/* Stream the response body to a file (HTTP GET). */
static cell_t Native_DownloadFile(IPluginContext *pContext, const cell_t *params)
{
	return SubmitWithFile(pContext, params, WC_METHOD_GET, wc_http_set_download_file);
}

/* ------------------------------------------------------------------------- */
/* HTTPResponse                                                              */
/* ------------------------------------------------------------------------- */

static cell_t Native_Resp_Status(IPluginContext *pContext, const cell_t *params)
{
	uint64_t id;
	if (!ReadIdHandle(params[1], g_HttpResponseType, &id))
		return pContext->ThrowNativeError("Invalid HTTPResponse handle %x", params[1]);
	return wc_resp_status(id);
}

static cell_t Native_Resp_Length(IPluginContext *pContext, const cell_t *params)
{
	uint64_t id;
	if (!ReadIdHandle(params[1], g_HttpResponseType, &id))
		return pContext->ThrowNativeError("Invalid HTTPResponse handle %x", params[1]);
	return (cell_t)wc_resp_body_len(id);
}

static cell_t Native_Resp_GetData(IPluginContext *pContext, const cell_t *params)
{
	uint64_t id;
	if (!ReadIdHandle(params[1], g_HttpResponseType, &id))
		return pContext->ThrowNativeError("Invalid HTTPResponse handle %x", params[1]);

	if (params[3] <= 0)
		return 0;

	size_t len = wc_resp_body_len(id);
	const uint8_t *body = wc_resp_body_ptr(id);

	/* Copy only what the caller's buffer can hold (then NUL-terminate); avoids
	 * duplicating a whole 64 MiB body just to truncate at the first NUL. */
	size_t cap = (size_t)params[3];
	size_t n = (body && len) ? (len < cap ? len : cap) : 0;
	NoThrowBuf tmp(n + 1);
	if (!tmp.p)
		return pContext->ThrowNativeError("Out of memory");
	if (n)
		memcpy(tmp.p, body, n);
	tmp.p[n] = '\0';

	size_t written = 0;
	pContext->StringToLocalUTF8(params[2], params[3], tmp.p, &written);
	return (cell_t)written;
}

static cell_t Native_Resp_GetHeader(IPluginContext *pContext, const cell_t *params)
{
	uint64_t id;
	if (!ReadIdHandle(params[1], g_HttpResponseType, &id))
		return pContext->ThrowNativeError("Invalid HTTPResponse handle %x", params[1]);

	char *name;
	if (pContext->LocalToString(params[2], &name) != SP_ERROR_NONE)
		return pContext->ThrowNativeError("Invalid header name string");

	cell_t maxlen = params[4];
	if (maxlen <= 0)
		return 0;
	if (maxlen > kMaxStrBuf)
		maxlen = kMaxStrBuf;

	size_t count = wc_resp_header_count(id);
	char hname[256];
	for (size_t i = 0; i < count; i++)
	{
		if (wc_resp_header_name(id, i, hname, sizeof(hname)) < 0)
			continue;
		if (strcasecmp(hname, name) != 0)
			continue;

		/* Size the temp to the caller's buffer so long values (Set-Cookie, CSP)
		 * aren't capped at a fixed size. */
		NoThrowBuf hvalue(maxlen);
		if (!hvalue.p)
			return pContext->ThrowNativeError("Out of memory");
		if (wc_resp_header_value(id, i, hvalue.p, (size_t)maxlen) < 0)
			return 0;
		pContext->StringToLocalUTF8(params[3], maxlen, hvalue.p, NULL);
		return 1;
	}
	return 0;
}

static cell_t Native_Resp_Headers(IPluginContext *pContext, const cell_t *params)
{
	uint64_t id;
	if (!ReadIdHandle(params[1], g_HttpResponseType, &id))
		return pContext->ThrowNativeError("Invalid HTTPResponse handle %x", params[1]);
	return (cell_t)wc_resp_header_count(id);
}

/* Copy the name/value of header[index]; false if index is out of range. The temp
 * is sized to the caller's buffer so long values (CSP, Set-Cookie) aren't capped. */
static cell_t Native_Resp_GetHeaderName(IPluginContext *pContext, const cell_t *params)
{
	uint64_t id;
	if (!ReadIdHandle(params[1], g_HttpResponseType, &id))
		return pContext->ThrowNativeError("Invalid HTTPResponse handle %x", params[1]);

	cell_t maxlen = params[4];
	if (params[2] < 0 || maxlen <= 0)
		return 0;
	if (maxlen > kMaxStrBuf)
		maxlen = kMaxStrBuf;

	NoThrowBuf buf(maxlen);
	if (!buf.p)
		return pContext->ThrowNativeError("Out of memory");
	if (wc_resp_header_name(id, (size_t)params[2], buf.p, (size_t)maxlen) < 0)
		return 0;
	pContext->StringToLocalUTF8(params[3], maxlen, buf.p, NULL);
	return 1;
}

static cell_t Native_Resp_GetHeaderValue(IPluginContext *pContext, const cell_t *params)
{
	uint64_t id;
	if (!ReadIdHandle(params[1], g_HttpResponseType, &id))
		return pContext->ThrowNativeError("Invalid HTTPResponse handle %x", params[1]);

	cell_t maxlen = params[4];
	if (params[2] < 0 || maxlen <= 0)
		return 0;
	if (maxlen > kMaxStrBuf)
		maxlen = kMaxStrBuf;

	NoThrowBuf buf(maxlen);
	if (!buf.p)
		return pContext->ThrowNativeError("Out of memory");
	if (wc_resp_header_value(id, (size_t)params[2], buf.p, (size_t)maxlen) < 0)
		return 0;
	pContext->StringToLocalUTF8(params[3], maxlen, buf.p, NULL);
	return 1;
}

static cell_t Native_Resp_GetURL(IPluginContext *pContext, const cell_t *params)
{
	uint64_t id;
	if (!ReadIdHandle(params[1], g_HttpResponseType, &id))
		return pContext->ThrowNativeError("Invalid HTTPResponse handle %x", params[1]);

	cell_t maxlen = params[3];
	if (maxlen <= 0)
		return 0;
	if (maxlen > kMaxStrBuf)
		maxlen = kMaxStrBuf;

	NoThrowBuf buf(maxlen);
	if (!buf.p)
		return pContext->ThrowNativeError("Out of memory");
	if (wc_resp_url(id, buf.p, (size_t)maxlen) < 0)
		return 0;
	size_t written = 0;
	pContext->StringToLocalUTF8(params[2], maxlen, buf.p, &written);
	return (cell_t)written;
}

/* Raw body copy that preserves interior NUL bytes (GetData stops at the first
 * NUL). The plugin relies on the returned length, not NUL scanning. */
static cell_t Native_Resp_GetDataBinary(IPluginContext *pContext, const cell_t *params)
{
	uint64_t id;
	if (!ReadIdHandle(params[1], g_HttpResponseType, &id))
		return pContext->ThrowNativeError("Invalid HTTPResponse handle %x", params[1]);

	cell_t maxlen = params[3];
	if (maxlen <= 0)
		return 0;

	size_t len = wc_resp_body_len(id);
	const uint8_t *body = wc_resp_body_ptr(id);

	char *out;
	if (pContext->LocalToString(params[2], &out) != SP_ERROR_NONE)
		return pContext->ThrowNativeError("Invalid buffer");

	size_t n = (body && len) ? len : 0;
	if (n > (size_t)(maxlen - 1))
		n = (size_t)(maxlen - 1);
	if (n)
		memcpy(out, body, n);
	out[n] = '\0';
	return (cell_t)n;
}

static cell_t Native_Resp_SaveToFile(IPluginContext *pContext, const cell_t *params)
{
	uint64_t id;
	if (!ReadIdHandle(params[1], g_HttpResponseType, &id))
		return pContext->ThrowNativeError("Invalid HTTPResponse handle %x", params[1]);

	char *path;
	if (pContext->LocalToString(params[2], &path) != SP_ERROR_NONE)
		return pContext->ThrowNativeError("Invalid path string");

	// Confine the plugin path to addons/sourcemod/data/webclient. Writes synchronously.
	char full[PLATFORM_MAX_PATH];
	if (!BuildConfinedPath(path, full, sizeof(full)))
		return pContext->ThrowNativeError("Unsafe or invalid path '%s' (must be a relative path under data/webclient)", path);

	size_t len = wc_resp_body_len(id);
	const uint8_t *body = wc_resp_body_ptr(id);

	FILE *fp = fopen(full, "wb");
	if (fp == NULL)
		return 0;
	size_t written = (body && len) ? fwrite(body, 1, len, fp) : 0;
	fclose(fp);
	if (written != len)
	{
		remove(full);   /* don't leave a truncated file behind */
		return 0;
	}
	return 1;
}

/* ------------------------------------------------------------------------- */
/* WebSocket                                                                 */
/* ------------------------------------------------------------------------- */

static cell_t Native_WebSocket(IPluginContext *pContext, const cell_t *params)
{
	char *url;
	if (pContext->LocalToString(params[1], &url) != SP_ERROR_NONE)
		return pContext->ThrowNativeError("Invalid URL string");

	uint64_t ws = wc_ws_new((const uint8_t *)url, strlen(url));
	if (ws == 0)
		return pContext->ThrowNativeError("Failed to create WebSocket");

	Handle_t handle = CreateIdHandle(g_WebSocketType, ws, pContext->GetIdentity());
	if (handle == 0)
	{
		wc_ws_destroy(ws);
		return pContext->ThrowNativeError("Failed to create WebSocket handle");
	}

	webclient::RegisterWs(ws, handle, pContext->GetIdentity());
	return handle;
}

static cell_t Native_Ws_SetHeader(IPluginContext *pContext, const cell_t *params)
{
	uint64_t ws;
	if (!ReadIdHandle(params[1], g_WebSocketType, &ws))
		return pContext->ThrowNativeError("Invalid WebSocket handle %x", params[1]);

	char *name;
	if (pContext->LocalToString(params[2], &name) != SP_ERROR_NONE)
		return pContext->ThrowNativeError("Invalid header name string");

	/* Value is a format string (params[3] + varargs), matching
	 * HTTPRequest.SetHeader; pass user data via "%s". */
	char value[4096];
	smutils->FormatString(value, sizeof(value), pContext, params, 3);
	wc_ws_set_header(ws, name, (const uint8_t *)value, strlen(value));
	return 0;
}

static cell_t Native_Ws_AllowLocalNetwork_get(IPluginContext *pContext, const cell_t *params)
{
	uint64_t ws;
	if (!ReadIdHandle(params[1], g_WebSocketType, &ws))
		return pContext->ThrowNativeError("Invalid WebSocket handle %x", params[1]);
	return wc_ws_get_allow_private(ws) ? 1 : 0;
}

static cell_t Native_Ws_AllowLocalNetwork_set(IPluginContext *pContext, const cell_t *params)
{
	uint64_t ws;
	if (!ReadIdHandle(params[1], g_WebSocketType, &ws))
		return pContext->ThrowNativeError("Invalid WebSocket handle %x", params[1]);
	wc_ws_set_allow_private(ws, params[2] ? 1 : 0);
	return 0;
}

static cell_t Native_Ws_AllowInvalidCerts_get(IPluginContext *pContext, const cell_t *params)
{
	uint64_t ws;
	if (!ReadIdHandle(params[1], g_WebSocketType, &ws))
		return pContext->ThrowNativeError("Invalid WebSocket handle %x", params[1]);
	return wc_ws_get_allow_invalid_certs(ws) ? 1 : 0;
}

static cell_t Native_Ws_AllowInvalidCerts_set(IPluginContext *pContext, const cell_t *params)
{
	uint64_t ws;
	if (!ReadIdHandle(params[1], g_WebSocketType, &ws))
		return pContext->ThrowNativeError("Invalid WebSocket handle %x", params[1]);
	wc_ws_set_allow_invalid_certs(ws, params[2] ? 1 : 0);
	return 0;
}

static cell_t Native_Ws_Connect(IPluginContext *pContext, const cell_t *params)
{
	uint64_t ws;
	if (!ReadIdHandle(params[1], g_WebSocketType, &ws))
		return pContext->ThrowNativeError("Invalid WebSocket handle %x", params[1]);

	IPluginFunction *fn = pContext->GetFunctionById(params[2]);
	if (fn == NULL)
		return pContext->ThrowNativeError("Invalid connect callback function");
	webclient::SetWsConnect(ws, fn, params[3]);
	if (wc_ws_connect(ws, OwnerToken(pContext)) != 0)
		return pContext->ThrowNativeError("Failed to start WebSocket connection");
	return 0;
}

static cell_t Native_Ws_SetMessageCallback(IPluginContext *pContext, const cell_t *params)
{
	uint64_t ws;
	if (!ReadIdHandle(params[1], g_WebSocketType, &ws))
		return pContext->ThrowNativeError("Invalid WebSocket handle %x", params[1]);
	IPluginFunction *fn = pContext->GetFunctionById(params[2]);
	if (fn == NULL)
		return pContext->ThrowNativeError("Invalid callback function");
	webclient::SetWsMessage(ws, fn);
	return 0;
}

static cell_t Native_Ws_SetDisconnectCallback(IPluginContext *pContext, const cell_t *params)
{
	uint64_t ws;
	if (!ReadIdHandle(params[1], g_WebSocketType, &ws))
		return pContext->ThrowNativeError("Invalid WebSocket handle %x", params[1]);
	IPluginFunction *fn = pContext->GetFunctionById(params[2]);
	if (fn == NULL)
		return pContext->ThrowNativeError("Invalid callback function");
	webclient::SetWsDisconnect(ws, fn);
	return 0;
}

static cell_t Native_Ws_SetErrorCallback(IPluginContext *pContext, const cell_t *params)
{
	uint64_t ws;
	if (!ReadIdHandle(params[1], g_WebSocketType, &ws))
		return pContext->ThrowNativeError("Invalid WebSocket handle %x", params[1]);
	IPluginFunction *fn = pContext->GetFunctionById(params[2]);
	if (fn == NULL)
		return pContext->ThrowNativeError("Invalid callback function");
	webclient::SetWsError(ws, fn);
	return 0;
}

static cell_t Native_Ws_Send(IPluginContext *pContext, const cell_t *params)
{
	uint64_t ws;
	if (!ReadIdHandle(params[1], g_WebSocketType, &ws))
		return pContext->ThrowNativeError("Invalid WebSocket handle %x", params[1]);

	char *data;
	if (pContext->LocalToString(params[2], &data) != SP_ERROR_NONE)
		return pContext->ThrowNativeError("Invalid data string");

	/* Text is NUL-terminated; never read past it. A negative length means "use
	 * the whole string"; a length longer than the string is clamped down. */
	size_t actual = strlen(data);
	size_t len = (params[3] < 0 || (size_t)params[3] > actual) ? actual : (size_t)params[3];
	return wc_ws_send(ws, 1, (const uint8_t *)data, len) == 0 ? 1 : 0;
}

static cell_t Native_Ws_SendBinary(IPluginContext *pContext, const cell_t *params)
{
	uint64_t ws;
	if (!ReadIdHandle(params[1], g_WebSocketType, &ws))
		return pContext->ThrowNativeError("Invalid WebSocket handle %x", params[1]);

	/* A negative length would become a huge size_t and read far out of bounds;
	 * reject it. The caller must pass a length within the buffer (as documented),
	 * matching SourceMod's other binary-write natives. */
	if (params[3] < 0)
		return pContext->ThrowNativeError("Invalid length %d", params[3]);

	char *data;
	if (pContext->LocalToString(params[2], &data) != SP_ERROR_NONE)
		return pContext->ThrowNativeError("Invalid data buffer");

	return wc_ws_send(ws, 0, (const uint8_t *)data, (size_t)params[3]) == 0 ? 1 : 0;
}

static cell_t Native_Ws_Close(IPluginContext *pContext, const cell_t *params)
{
	uint64_t ws;
	if (!ReadIdHandle(params[1], g_WebSocketType, &ws))
		return pContext->ThrowNativeError("Invalid WebSocket handle %x", params[1]);

	char *reason;
	if (pContext->LocalToString(params[3], &reason) != SP_ERROR_NONE)
		return pContext->ThrowNativeError("Invalid reason string");
	wc_ws_close(ws, (uint16_t)params[2], (const uint8_t *)reason, strlen(reason));
	return 0;
}

static cell_t Native_Ws_State(IPluginContext *pContext, const cell_t *params)
{
	uint64_t ws;
	if (!ReadIdHandle(params[1], g_WebSocketType, &ws))
		return pContext->ThrowNativeError("Invalid WebSocket handle %x", params[1]);
	return wc_ws_state(ws);
}

static cell_t Native_Ws_Ping(IPluginContext *pContext, const cell_t *params)
{
	uint64_t ws;
	if (!ReadIdHandle(params[1], g_WebSocketType, &ws))
		return pContext->ThrowNativeError("Invalid WebSocket handle %x", params[1]);

	char *data;
	if (pContext->LocalToString(params[2], &data) != SP_ERROR_NONE)
		return pContext->ThrowNativeError("Invalid data string");

	/* Like Send: NUL-terminated payload; a negative or oversized length clamps. */
	size_t actual = strlen(data);
	size_t len = (params[3] < 0 || (size_t)params[3] > actual) ? actual : (size_t)params[3];
	return wc_ws_ping(ws, (const uint8_t *)data, len) == 0 ? 1 : 0;
}

/* ------------------------------------------------------------------------- */
/* Global helpers                                                            */
/* ------------------------------------------------------------------------- */

static cell_t Native_Http_Cancel(IPluginContext *pContext, const cell_t *params)
{
	return webclient::CancelHttp((uint32_t)params[1], pContext->GetIdentity()) ? 1 : 0;
}

/* Shared percent-encode/decode body: fn is the Rust entry point to call. */
static cell_t UrlConvert(IPluginContext *pContext, const cell_t *params,
	int32_t (*fn)(const uint8_t *, size_t, char *, size_t))
{
	char *input;
	if (pContext->LocalToString(params[1], &input) != SP_ERROR_NONE)
		return pContext->ThrowNativeError("Invalid input string");

	cell_t maxlen = params[3];
	if (maxlen <= 0)
		return 0;
	if (maxlen > kMaxStrBuf)
		maxlen = kMaxStrBuf;

	NoThrowBuf buf(maxlen);
	if (!buf.p)
		return pContext->ThrowNativeError("Out of memory");
	if (fn((const uint8_t *)input, strlen(input), buf.p, (size_t)maxlen) < 0)
		return 0;
	size_t written = 0;
	pContext->StringToLocalUTF8(params[2], maxlen, buf.p, &written);
	return (cell_t)written;
}

static cell_t Native_UrlEncode(IPluginContext *pContext, const cell_t *params)
{
	return UrlConvert(pContext, params, wc_url_encode);
}

static cell_t Native_UrlDecode(IPluginContext *pContext, const cell_t *params)
{
	return UrlConvert(pContext, params, wc_url_decode);
}

/* ------------------------------------------------------------------------- */

const sp_nativeinfo_t g_WebClientNatives[] =
{
	{ "HTTPRequest.HTTPRequest",          Native_HTTPRequest },
	{ "HTTPRequest.SetHeader",            Native_SetHeader },
	{ "HTTPRequest.AppendQueryParam",     Native_AppendQueryParam },
	{ "HTTPRequest.AppendFormParam",      Native_AppendFormParam },
	{ "HTTPRequest.Timeout.get",          Native_Timeout_get },
	{ "HTTPRequest.Timeout.set",          Native_Timeout_set },
	{ "HTTPRequest.ConnectTimeout.get",   Native_ConnectTimeout_get },
	{ "HTTPRequest.ConnectTimeout.set",   Native_ConnectTimeout_set },
	{ "HTTPRequest.MaxRedirects.get",     Native_MaxRedirects_get },
	{ "HTTPRequest.MaxRedirects.set",     Native_MaxRedirects_set },
	{ "HTTPRequest.AllowLocalNetwork.get", Native_AllowLocalNetwork_get },
	{ "HTTPRequest.AllowLocalNetwork.set", Native_AllowLocalNetwork_set },
	{ "HTTPRequest.AllowInvalidCerts.get", Native_AllowInvalidCerts_get },
	{ "HTTPRequest.AllowInvalidCerts.set", Native_AllowInvalidCerts_set },
	{ "HTTPRequest.SetBasicAuth",         Native_SetBasicAuth },
	{ "HTTPRequest.Get",                  Native_Get },
	{ "HTTPRequest.Post",                 Native_Post },
	{ "HTTPRequest.Put",                  Native_Put },
	{ "HTTPRequest.Patch",                Native_Patch },
	{ "HTTPRequest.Delete",               Native_Delete },
	{ "HTTPRequest.Head",                 Native_Head },
	{ "HTTPRequest.Options",              Native_Options },
	{ "HTTPRequest.PostForm",             Native_PostForm },
	{ "HTTPRequest.UploadFile",           Native_UploadFile },
	{ "HTTPRequest.DownloadFile",         Native_DownloadFile },

	{ "HTTPResponse.Status.get",          Native_Resp_Status },
	{ "HTTPResponse.Length.get",          Native_Resp_Length },
	{ "HTTPResponse.Headers.get",         Native_Resp_Headers },
	{ "HTTPResponse.GetData",             Native_Resp_GetData },
	{ "HTTPResponse.GetDataBinary",       Native_Resp_GetDataBinary },
	{ "HTTPResponse.GetURL",              Native_Resp_GetURL },
	{ "HTTPResponse.SaveToFile",          Native_Resp_SaveToFile },
	{ "HTTPResponse.GetHeader",           Native_Resp_GetHeader },
	{ "HTTPResponse.GetHeaderName",       Native_Resp_GetHeaderName },
	{ "HTTPResponse.GetHeaderValue",      Native_Resp_GetHeaderValue },

	{ "WebSocket.WebSocket",              Native_WebSocket },
	{ "WebSocket.SetHeader",              Native_Ws_SetHeader },
	{ "WebSocket.AllowLocalNetwork.get",  Native_Ws_AllowLocalNetwork_get },
	{ "WebSocket.AllowLocalNetwork.set",  Native_Ws_AllowLocalNetwork_set },
	{ "WebSocket.AllowInvalidCerts.get",  Native_Ws_AllowInvalidCerts_get },
	{ "WebSocket.AllowInvalidCerts.set",  Native_Ws_AllowInvalidCerts_set },
	{ "WebSocket.Connect",                Native_Ws_Connect },
	{ "WebSocket.SetMessageCallback",     Native_Ws_SetMessageCallback },
	{ "WebSocket.SetDisconnectCallback",  Native_Ws_SetDisconnectCallback },
	{ "WebSocket.SetErrorCallback",       Native_Ws_SetErrorCallback },
	{ "WebSocket.Send",                   Native_Ws_Send },
	{ "WebSocket.SendBinary",             Native_Ws_SendBinary },
	{ "WebSocket.Ping",                   Native_Ws_Ping },
	{ "WebSocket.ReadyState.get",         Native_Ws_State },
	{ "WebSocket.Close",                  Native_Ws_Close },

	{ "WebClient_CancelRequest",          Native_Http_Cancel },
	{ "WebClient_URLEncode",              Native_UrlEncode },
	{ "WebClient_URLDecode",              Native_UrlDecode },

	{ NULL,                               NULL },
};
