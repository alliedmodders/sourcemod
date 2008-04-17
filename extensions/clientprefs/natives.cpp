/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Client Preferences Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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
#include "cookie.h"

cell_t RegClientPrefCookie(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	pContext->LocalToString(params[1], &name);

	if (name[0] == '\0')
	{
		return pContext->ThrowNativeError("Cannot create preference cookie with no name");
	}

	char *desc;
	pContext->LocalToString(params[2], &desc);

	Cookie *pCookie = g_CookieManager.CreateCookie(name, desc);

	if (!pCookie)
	{
		return BAD_HANDLE;
	}

	return handlesys->CreateHandle(g_CookieType, 
		pCookie, 
		pContext->GetIdentity(), 
		myself->GetIdentity(), 
		NULL);
}

cell_t FindClientPrefCookie(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	pContext->LocalToString(params[1], &name);

	Cookie *pCookie = g_CookieManager.FindCookie(name);

	if (!pCookie)
	{
		return BAD_HANDLE;
	}

	return handlesys->CreateHandle(g_CookieType, 
		pCookie, 
		pContext->GetIdentity(), 
		myself->GetIdentity(), 
		NULL);
}

cell_t SetClientPrefCookie(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	if ((client < 1) || (client > playerhelpers->GetMaxClients()))
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}

	Handle_t hndl = static_cast<Handle_t>(params[2]);
	HandleError err;
	HandleSecurity sec;
 
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	Cookie *pCookie;

	if ((err = handlesys->ReadHandle(hndl, g_CookieType, &sec, (void **)&pCookie))
	     != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Cookie handle %x (error %d)", hndl, err);
	}

	char *value;
	pContext->LocalToString(params[3], &value);

	return g_CookieManager.SetCookieValue(pCookie, client, value);
}

cell_t GetClientPrefCookie(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	if ((client < 1) || (client > playerhelpers->GetMaxClients()))
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}

	Handle_t hndl = static_cast<Handle_t>(params[2]);
	HandleError err;
	HandleSecurity sec;
 
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	Cookie *pCookie;

	if ((err = handlesys->ReadHandle(hndl, g_CookieType, &sec, (void **)&pCookie))
	     != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Cookie handle %x (error %d)", hndl, err);
	}
	
	char *value = NULL;

	g_CookieManager.GetCookieValue(pCookie, client, &value);

	pContext->StringToLocal(params[3], params[4], value);

	return 1;
}

cell_t AreClientCookiesCached(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	if ((client < 1) || (client > playerhelpers->GetMaxClients()))
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}

	return g_CookieManager.AreClientCookiesCached(client);
}

sp_nativeinfo_t g_ClientPrefNatives[] = 
{
	{"RegClientPrefCookie",			RegClientPrefCookie},
	{"FindClientPrefCookie",		FindClientPrefCookie},
	{"SetClientPrefCookie",			SetClientPrefCookie},
	{"GetClientPrefCookie",			GetClientPrefCookie},
	{"AreClientCookiesCached",		AreClientCookiesCached},
	{NULL,							NULL}
};
