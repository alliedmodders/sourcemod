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

#ifndef _INCLUDE_SOURCEMOD_WEBCLIENT_EXTENSION_H_
#define _INCLUDE_SOURCEMOD_WEBCLIENT_EXTENSION_H_

/**
 * @file extension.h
 * @brief WebClient extension: async HTTP + WebSocket client natives.
 */

#include "smsdk_ext.h"
#include <IPluginSys.h>
#include <IHandleSys.h>
#include <stdint.h>
#include <new>

#include "ffi.h"

/* Heap buffer that yields NULL instead of throwing/crashing on OOM. The SDK
 * replaces global operator new with a non-throwing malloc on Linux, so a failed
 * std::string/vector alloc would NULL-deref rather than throw; callers that copy
 * plugin- or peer-sized data check .p and degrade gracefully instead. */
struct NoThrowBuf
{
	char *p;
	explicit NoThrowBuf(size_t n) : p(new (std::nothrow) char[n]) {}
	~NoThrowBuf() { delete[] p; }
	NoThrowBuf(const NoThrowBuf &) = delete;
	NoThrowBuf &operator=(const NoThrowBuf &) = delete;
};

/**
 * @brief WebClient extension main interface + plugin-unload listener.
 *
 * The C++ half is a thin shim: it owns the SourceMod-facing objects (Handles,
 * natives, the per-frame completion drain, and the table that maps a Rust
 * object id back to the Pawn callback it must invoke). All networking lives in
 * Rust (see rust/). The SourcePawn VM is touched only here, on the main thread.
 */
class WebClient : public SDKExtension, public IPluginsListener
{
public:
	bool SDK_OnLoad(char *error, size_t maxlength, bool late) override;
	void SDK_OnUnload() override;
	void SDK_OnAllLoaded() override;

public: // IPluginsListener
	// When a plugin unloads, abort all of its in-flight work in Rust so that a
	// stale IPluginFunction can never be invoked.
	void OnPluginUnloaded(IPlugin *plugin) override;
};

/* The per-request HTTP callback, stored on the C++ side keyed by request id. */
struct HttpCallback
{
	IPluginFunction *fn;
	cell_t data;
	IdentityToken_t *owner;
	uint32_t token;             /* cancellation token handed back to the plugin */
};

/* A WebSocket's persistent callbacks, stored keyed by websocket id. */
struct WsCallback
{
	Handle_t handle;            /* the WebSocket Handle the plugin holds       */
	IdentityToken_t *owner;
	cell_t data;                /* user value supplied at Connect()            */
	IPluginFunction *onConnect;
	IPluginFunction *onMessage;
	IPluginFunction *onDisconnect;
	IPluginFunction *onError;
};

namespace webclient {
	/* Callback-table management (main thread only). Returns a cancellation token
	 * (nonzero) the plugin can later pass to CancelHttp. */
	uint32_t RegisterHttp(uint64_t request_id, IPluginFunction *fn, cell_t data, IdentityToken_t *owner);

	/* Cancel the request behind a token if it belongs to owner and is still in
	 * flight; drops its callback so it never fires. True if it was cancelled. */
	bool CancelHttp(uint32_t token, IdentityToken_t *owner);

	void RegisterWs(uint64_t ws_id, Handle_t handle, IdentityToken_t *owner);
	void SetWsConnect(uint64_t ws_id, IPluginFunction *fn, cell_t data);
	void SetWsMessage(uint64_t ws_id, IPluginFunction *fn);
	void SetWsDisconnect(uint64_t ws_id, IPluginFunction *fn);
	void SetWsError(uint64_t ws_id, IPluginFunction *fn);
	WsCallback *FindWs(uint64_t ws_id);
	void ForgetWs(uint64_t ws_id);

	/* Per-frame drain of the Rust completion queue (registered game frame hook). */
	void OnGameFrame(bool simulating);
}

/* Handle types, defined in handles.cpp, created in SDK_OnLoad. */
extern HandleType_t g_HttpRequestType;
extern HandleType_t g_HttpResponseType;
extern HandleType_t g_WebSocketType;

bool CreateHandleTypes(char *error, size_t maxlength);
void RemoveHandleTypes();

/* Handles store a heap-boxed uint64_t id (a void* is only 32-bit on x86, but
 * the Rust ids are 64-bit). These helpers own that boxing on both sides. */
Handle_t CreateIdHandle(HandleType_t type, uint64_t id, IdentityToken_t *owner);
bool ReadIdHandle(Handle_t handle, HandleType_t type, uint64_t *outId);

/* Native table, defined in natives.cpp. */
extern const sp_nativeinfo_t g_WebClientNatives[];

#endif // _INCLUDE_SOURCEMOD_WEBCLIENT_EXTENSION_H_
