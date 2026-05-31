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
#include <string.h>
#include <iterator>
#include <unordered_map>

/**
 * @file extension.cpp
 * @brief Extension lifecycle, completion drain, and Pawn callback dispatch.
 */

WebClient g_WebClient;
SMEXT_LINK(&g_WebClient);

/* Maximum completions dispatched per game frame. HTTP latency already rate
 * limits work; the cap simply bounds worst-case per-frame time when a burst of
 * responses (or a chatty WebSocket) lands at once. */
static const int kMaxCompletionsPerFrame = 64;

/* Callback tables. Touched only on the main thread (natives + the frame hook),
 * so no locking is needed here; Rust owns all cross-thread synchronisation. */
static std::unordered_map<uint64_t, HttpCallback> s_httpCallbacks;
static std::unordered_map<uint64_t, WsCallback> s_wsCallbacks;

/* Plugin-facing 32-bit cancellation tokens -> 64-bit request ids (the Rust ids
 * don't fit a Pawn cell). Entries live only while a request is in flight. */
static std::unordered_map<uint32_t, uint64_t> s_requestTokens;
static uint32_t s_nextToken = 0;

static uint32_t NextHttpToken()
{
	do { ++s_nextToken; } while (s_nextToken == 0); /* reserve 0 as "invalid" */
	return s_nextToken;
}

/* ------------------------------------------------------------------------- */
/* Extension lifecycle                                                       */
/* ------------------------------------------------------------------------- */

bool WebClient::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	char rustErr[256];
	rustErr[0] = '\0';
	if (wc_init(rustErr, sizeof(rustErr)) != 0)
	{
		smutils->Format(error, maxlength, "Failed to initialise networking runtime: %s",
			rustErr[0] ? rustErr : "unknown error");
		return false;
	}

	if (!CreateHandleTypes(error, maxlength))
	{
		wc_shutdown();
		return false;
	}

	plsys->AddPluginsListener(this);
	return true;
}

void WebClient::SDK_OnAllLoaded()
{
	sharesys->AddNatives(myself, g_WebClientNatives);
	sharesys->RegisterLibrary(myself, "webclient");
	smutils->AddGameFrameHook(&webclient::OnGameFrame);
}

void WebClient::SDK_OnUnload()
{
	/* Stop draining first so nothing fires mid-teardown. */
	smutils->RemoveGameFrameHook(&webclient::OnGameFrame);
	plsys->RemovePluginsListener(this);

	/* Force-free any outstanding Handles while the Rust registry is still alive
	 * (their destructors call wc_*_destroy). Then drop the runtime. */
	RemoveHandleTypes();
	s_httpCallbacks.clear();
	s_wsCallbacks.clear();
	wc_shutdown();
}

void WebClient::OnPluginUnloaded(IPlugin *plugin)
{
	IdentityToken_t *ident = plugin->GetIdentity();

	/* Abort the plugin's in-flight work in Rust and drop its queued completions
	 * so a stale IPluginFunction is never invoked. */
	wc_cancel_owner((uint64_t)(uintptr_t)ident);

	/* Drop any C++ callback slots we were holding for it. */
	for (auto it = s_httpCallbacks.begin(); it != s_httpCallbacks.end(); )
	{
		if (it->second.owner == ident)
		{
			s_requestTokens.erase(it->second.token);
			it = s_httpCallbacks.erase(it);
		}
		else
		{
			it = std::next(it);
		}
	}
	for (auto it = s_wsCallbacks.begin(); it != s_wsCallbacks.end(); )
		it = (it->second.owner == ident) ? s_wsCallbacks.erase(it) : std::next(it);
}

/* ------------------------------------------------------------------------- */
/* Callback-table management (declared in extension.h, namespace webclient)  */
/* ------------------------------------------------------------------------- */

uint32_t webclient::RegisterHttp(uint64_t request_id, IPluginFunction *fn, cell_t data, IdentityToken_t *owner)
{
	uint32_t token = NextHttpToken();
	HttpCallback cb = { fn, data, owner, token };
	s_httpCallbacks[request_id] = cb;
	s_requestTokens[token] = request_id;
	return token;
}

bool webclient::CancelHttp(uint32_t token, IdentityToken_t *owner)
{
	auto it = s_requestTokens.find(token);
	if (it == s_requestTokens.end())
		return false;
	uint64_t request_id = it->second;

	/* Only the plugin that started the request may cancel it. */
	auto cbIt = s_httpCallbacks.find(request_id);
	if (cbIt == s_httpCallbacks.end() || cbIt->second.owner != owner)
		return false;

	int32_t cancelled = wc_http_cancel(request_id, (uint64_t)(uintptr_t)owner);
	s_requestTokens.erase(it);
	s_httpCallbacks.erase(cbIt);
	return cancelled != 0;
}

void webclient::RegisterWs(uint64_t ws_id, Handle_t handle, IdentityToken_t *owner)
{
	WsCallback cb = { handle, owner, 0, nullptr, nullptr, nullptr, nullptr };
	s_wsCallbacks[ws_id] = cb;
}

WsCallback *webclient::FindWs(uint64_t ws_id)
{
	auto it = s_wsCallbacks.find(ws_id);
	return it == s_wsCallbacks.end() ? nullptr : &it->second;
}

void webclient::SetWsConnect(uint64_t ws_id, IPluginFunction *fn, cell_t data)
{
	if (WsCallback *cb = FindWs(ws_id)) { cb->onConnect = fn; cb->data = data; }
}
void webclient::SetWsMessage(uint64_t ws_id, IPluginFunction *fn)
{
	if (WsCallback *cb = FindWs(ws_id)) cb->onMessage = fn;
}
void webclient::SetWsDisconnect(uint64_t ws_id, IPluginFunction *fn)
{
	if (WsCallback *cb = FindWs(ws_id)) cb->onDisconnect = fn;
}
void webclient::SetWsError(uint64_t ws_id, IPluginFunction *fn)
{
	if (WsCallback *cb = FindWs(ws_id)) cb->onError = fn;
}
void webclient::ForgetWs(uint64_t ws_id)
{
	s_wsCallbacks.erase(ws_id);
}

/* ------------------------------------------------------------------------- */
/* Completion dispatch (main thread)                                         */
/* ------------------------------------------------------------------------- */

static void DispatchHttp(uint64_t request_id, bool isError)
{
	auto it = s_httpCallbacks.find(request_id);
	bool have = (it != s_httpCallbacks.end());

	/* Copy out and erase BEFORE firing: the callback may submit new requests and
	 * rehash the table, invalidating iterators/pointers. */
	IdentityToken_t *owner = have ? it->second.owner : myself->GetIdentity();
	cell_t data = have ? it->second.data : 0;
	IPluginFunction *fn = have ? it->second.fn : nullptr;
	if (have)
	{
		s_requestTokens.erase(it->second.token);
		s_httpCallbacks.erase(it);
	}

	/* A transport failure has no HTTP response, so the plugin gets INVALID_HANDLE
	 * (the documented contract); a real response always gets a handle. */
	Handle_t resp = isError ? 0 : CreateIdHandle(g_HttpResponseType, request_id, owner);

	if (fn && fn->IsRunnable())
	{
		char err[256];
		err[0] = '\0';
		if (isError)
			wc_resp_error(request_id, err, sizeof(err));

		fn->PushCell(resp);                 /* HTTPResponse, or INVALID_HANDLE on failure */
		fn->PushCell(data);
		fn->PushCell(isError ? 1 : 0);
		fn->PushString(err);
		fn->Execute(NULL);
	}

	/* Exactly one destroy path: freeing the Handle triggers wc_http_destroy via
	 * the type's OnHandleDestroy; if we never got a Handle, destroy directly. */
	if (resp != 0)
	{
		HandleSecurity sec(owner, myself->GetIdentity());
		handlesys->FreeHandle(resp, &sec);
	}
	else
	{
		wc_http_destroy(request_id);
	}
}

static void DispatchWs(uint64_t event_id, uint64_t ws_id)
{
	int32_t ev = wc_ws_event_kind(event_id);

	WsCallback *slot = webclient::FindWs(ws_id);
	if (slot)
	{
		/* Copy out before firing (a callback may create/close sockets). */
		Handle_t handle = slot->handle;
		cell_t data = slot->data;
		IPluginFunction *fn = nullptr;
		switch (ev)
		{
			case WC_WS_CONNECTED:    fn = slot->onConnect; break;
			case WC_WS_TEXT:
			case WC_WS_BINARY:       fn = slot->onMessage; break;
			case WC_WS_DISCONNECTED: fn = slot->onDisconnect; break;
			case WC_WS_ERROR:        fn = slot->onError; break;
		}

		if (fn && fn->IsRunnable())
		{
			/* Snapshot the borrowed payload into an owned, NUL-terminated buffer.
			 * NoThrowBuf yields NULL (not a throw/crash) on OOM - the SDK's global
			 * operator new is non-throwing on Linux - so a huge frame degrades to
			 * empty. The trailing NUL keeps text %s consumers in-bounds while
			 * interior NULs survive for callers that use `length`. */
			size_t len = wc_ws_event_data_len(event_id);
			const uint8_t *src = len ? wc_ws_event_data_ptr(event_id) : nullptr;
			if (!src)
				len = 0;
			NoThrowBuf payload(len + 1);
			if (!payload.p)
				len = 0;
			else
			{
				if (len)
					memcpy(payload.p, src, len);
				payload.p[len] = '\0';
			}
			const char *pl = payload.p ? payload.p : "";
			uint16_t closeCode = wc_ws_event_close_code(event_id);

			switch (ev)
			{
				case WC_WS_CONNECTED:
					fn->PushCell(handle);
					fn->PushCell(data);
					fn->Execute(NULL);
					break;
				case WC_WS_TEXT:
				case WC_WS_BINARY:
					fn->PushCell(handle);
					fn->PushCell(data);
					fn->PushStringEx(const_cast<char *>(pl), len + 1,
						SM_PARAM_STRING_BINARY | SM_PARAM_STRING_COPY, 0);
					fn->PushCell((cell_t)len);
					fn->PushCell(ev == WC_WS_TEXT ? 1 : 0);
					fn->Execute(NULL);
					break;
				case WC_WS_DISCONNECTED:
					fn->PushCell(handle);
					fn->PushCell(data);
					fn->PushCell((cell_t)closeCode);
					fn->PushString(pl);
					fn->Execute(NULL);
					break;
				case WC_WS_ERROR:
					fn->PushCell(handle);
					fn->PushCell(data);
					fn->PushString(pl);
					fn->Execute(NULL);
					break;
			}
		}
	}

	wc_ws_event_release(event_id);
}

void webclient::OnGameFrame(bool simulating)
{
	wc_completion_t c;
	int budget = kMaxCompletionsPerFrame;
	while (budget-- > 0 && wc_poll_completion(&c))
	{
		switch (c.kind)
		{
			case WC_KIND_HTTP_RESPONSE: DispatchHttp(c.target, false); break;
			case WC_KIND_HTTP_ERROR:    DispatchHttp(c.target, true);  break;
			case WC_KIND_WS_EVENT:      DispatchWs(c.id, c.target);    break;
		}
	}
}
