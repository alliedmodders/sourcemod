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

/**
 * @file handles.cpp
 * @brief SourceMod Handle types for the WebClient objects.
 *
 * Each Handle's bound object is a heap-allocated uint64_t holding the Rust
 * object id. The OnHandleDestroy hooks are the single point where Rust-side
 * resources are released, so closing a Handle (by the plugin, by RemoveType on
 * unload, or by us after a callback) always frees the backing object exactly
 * once. wc_*_destroy are idempotent and safe for stale ids.
 */

HandleType_t g_HttpRequestType = 0;
HandleType_t g_HttpResponseType = 0;
HandleType_t g_WebSocketType = 0;

class HttpRequestHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void *object) override
	{
		uint64_t *box = reinterpret_cast<uint64_t *>(object);
		wc_http_builder_free(*box); /* no-op if already submitted */
		delete box;
	}
};

class HttpResponseHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void *object) override
	{
		uint64_t *box = reinterpret_cast<uint64_t *>(object);
		wc_http_destroy(*box);
		delete box;
	}
};

class WebSocketHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void *object) override
	{
		uint64_t *box = reinterpret_cast<uint64_t *>(object);
		wc_ws_destroy(*box);
		webclient::ForgetWs(*box);
		delete box;
	}
};

static HttpRequestHandler s_httpRequestHandler;
static HttpResponseHandler s_httpResponseHandler;
static WebSocketHandler s_webSocketHandler;

bool CreateHandleTypes(char *error, size_t maxlength)
{
	IdentityToken_t *ident = myself->GetIdentity();

	g_HttpRequestType = handlesys->CreateType("HTTPRequest", &s_httpRequestHandler, 0, NULL, NULL, ident, NULL);
	g_HttpResponseType = handlesys->CreateType("HTTPResponse", &s_httpResponseHandler, 0, NULL, NULL, ident, NULL);
	g_WebSocketType = handlesys->CreateType("WebSocket", &s_webSocketHandler, 0, NULL, NULL, ident, NULL);

	if (g_HttpRequestType == 0 || g_HttpResponseType == 0 || g_WebSocketType == 0)
	{
		smutils->Format(error, maxlength, "Could not create WebClient handle types");
		RemoveHandleTypes();
		return false;
	}
	return true;
}

void RemoveHandleTypes()
{
	IdentityToken_t *ident = myself->GetIdentity();
	if (g_HttpRequestType != 0)
	{
		handlesys->RemoveType(g_HttpRequestType, ident);
		g_HttpRequestType = 0;
	}
	if (g_HttpResponseType != 0)
	{
		handlesys->RemoveType(g_HttpResponseType, ident);
		g_HttpResponseType = 0;
	}
	if (g_WebSocketType != 0)
	{
		handlesys->RemoveType(g_WebSocketType, ident);
		g_WebSocketType = 0;
	}
}

Handle_t CreateIdHandle(HandleType_t type, uint64_t id, IdentityToken_t *owner)
{
	uint64_t *box = new uint64_t(id);
	HandleError err;
	Handle_t handle = handlesys->CreateHandle(type, box, owner, myself->GetIdentity(), &err);
	if (handle == 0)
		delete box;
	return handle;
}

bool ReadIdHandle(Handle_t handle, HandleType_t type, uint64_t *outId)
{
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	void *object = NULL;
	if (handlesys->ReadHandle(handle, type, &sec, &object) != HandleError_None || object == NULL)
		return false;

	*outId = *reinterpret_cast<uint64_t *>(object);
	return true;
}
