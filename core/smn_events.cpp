/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
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

#include "sm_globals.h"
#include "sourcemm_api.h"
#include "EventManager.h"
#include "PlayerManager.h"
#include "logic_bridge.h"

static cell_t sm_HookEvent(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	IPluginFunction *pFunction;

	pContext->LocalToString(params[1], &name);
	pFunction = pContext->GetFunctionById(params[2]);

	if (!pFunction)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[2]);
	}

	if (g_EventManager.HookEvent(name, pFunction, static_cast<EventHookMode>(params[3])) == EventHookErr_InvalidEvent)
	{
		return pContext->ThrowNativeError("Game event \"%s\" does not exist", name);
	}

	return 1;
}

static cell_t sm_HookEventEx(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	IPluginFunction *pFunction;

	pContext->LocalToString(params[1], &name);
	pFunction = pContext->GetFunctionById(params[2]);

	if (!pFunction)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[2]);
	}

	if (g_EventManager.HookEvent(name, pFunction, static_cast<EventHookMode>(params[3])) == EventHookErr_InvalidEvent)
	{
		return 0;
	}

	return 1;
}

static cell_t sm_UnhookEvent(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	IPluginFunction *pFunction;

	pContext->LocalToString(params[1], &name);
	pFunction = pContext->GetFunctionById(params[2]);

	if (!pFunction)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[2]);
	}

	EventHookError err = g_EventManager.UnhookEvent(name, pFunction, static_cast<EventHookMode>(params[3]));

	/* Possible errors that UnhookGameEvent can return */
	if (err == EventHookErr_NotActive)
	{
		return pContext->ThrowNativeError("Game event \"%s\" has no active hook", name);
	} else if (err == EventHookErr_InvalidCallback) {
		return pContext->ThrowNativeError("Invalid hook callback specified for game event \"%s\"", name);
	}

	return 1;
}

static cell_t sm_CreateEvent(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	EventInfo *pInfo;

	pContext->LocalToString(params[1], &name);

	pInfo = g_EventManager.CreateEvent(pContext, name, params[2] ? true : false);

	if (pInfo)
	{
		return handlesys->CreateHandle(g_EventManager.GetHandleType(), pInfo, pContext->GetIdentity(), g_pCoreIdent, NULL);
	}

	return BAD_HANDLE;
}

static cell_t sm_FireEvent(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	EventInfo *pInfo;
        HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(hndl, g_EventManager.GetHandleType(), &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid game event handle %x (error %d)", hndl, err);
	}

	/* If identities do not match, don't fire event */
	if (pContext->GetIdentity() != pInfo->pOwner)
	{
		return pContext->ThrowNativeError("Game event \"%s\" could not be fired because it was not created by this plugin", pInfo->pEvent->GetName());
	}

	g_EventManager.FireEvent(pInfo, params[2] ? true : false);

	/* Free handle on game event */
	handlesys->FreeHandle(hndl, &sec);

	return 1;
}

static cell_t sm_FireEventToClient(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	EventInfo *pInfo;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(hndl, g_EventManager.GetHandleType(), &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid game event handle %x (error %d)", hndl, err);
	}

	int client = params[2];
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}

	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	IClient *pClient = pPlayer->GetIClient();
	if (!pClient)
	{
		return pContext->ThrowNativeError("Sending events to fakeclients is not supported on this game (client %d)", client);
	}

	g_EventManager.FireEventToClient(pInfo, pClient);

	return 1;
}

static cell_t sm_CancelCreatedEvent(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	EventInfo *pInfo;
        HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(hndl, g_EventManager.GetHandleType(), &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid game event handle %x (error %d)", hndl, err);
	}

	/* If identities do not match, don't cancel event */
	if (pContext->GetIdentity() != pInfo->pOwner)
	{
		return pContext->ThrowNativeError("Game event \"%s\" could not be canceled because it was not created by this plugin", pInfo->pEvent->GetName());
	}

	g_EventManager.CancelCreatedEvent(pInfo);

	/* Free handle on game event */
	handlesys->FreeHandle(hndl, &sec);

	return 1;
}

static cell_t sm_GetEventName(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	EventInfo *pInfo;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(hndl, g_EventManager.GetHandleType(), &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid game event handle %x (error %d)", hndl, err);
	}

	pContext->StringToLocalUTF8(params[2], params[3], pInfo->pEvent->GetName(), NULL);

	return 1;
}

static cell_t sm_GetEventBool(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	EventInfo *pInfo;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(hndl, g_EventManager.GetHandleType(), &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid game event handle %x (error %d)", hndl, err);
	}

	char *key;
	pContext->LocalToString(params[2], &key);

	bool defValue = false;
	if (params[0] > 2)
	{
		defValue = !!params[3];
	}

	return pInfo->pEvent->GetBool(key, defValue);
}

static cell_t sm_GetEventInt(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	EventInfo *pInfo;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(hndl, g_EventManager.GetHandleType(), &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid game event handle %x (error %d)", hndl, err);
	}

	char *key;
	pContext->LocalToString(params[2], &key);

	int defValue = 0;
	if (params[0] > 2)
	{
		defValue = params[3];
	}

	return pInfo->pEvent->GetInt(key, defValue);
}

static cell_t sm_GetEventFloat(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	EventInfo *pInfo;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(hndl, g_EventManager.GetHandleType(), &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid game event handle %x (error %d)", hndl, err);
	}

	char *key;
	pContext->LocalToString(params[2], &key);

	float defValue = 0.0f;
	if (params[0] > 2)
	{
		defValue = sp_ctof(params[3]);
	}

	float value = pInfo->pEvent->GetFloat(key, defValue);

	return sp_ftoc(value);
}

static cell_t sm_GetEventString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	EventInfo *pInfo;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(hndl, g_EventManager.GetHandleType(), &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid game event handle %x (error %d)", hndl, err);
	}

	char *key;
	pContext->LocalToString(params[2], &key);

	char *defValue = NULL;
	if (params[0] > 4)
	{
		pContext->LocalToString(params[5], &defValue);
	}

	const char *value = pInfo->pEvent->GetString(key, defValue ? defValue : "");
	pContext->StringToLocalUTF8(params[3], params[4], value, NULL);

	return 1;
}

static cell_t sm_SetEventBool(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	EventInfo *pInfo;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(hndl, g_EventManager.GetHandleType(), &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid game event handle %x (error %d)", hndl, err);
	}

	char *key;
	pContext->LocalToString(params[2], &key);

	pInfo->pEvent->SetBool(key, params[3] ? true : false);

	return 1;
}

static cell_t sm_SetEventInt(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	EventInfo *pInfo;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(hndl, g_EventManager.GetHandleType(), &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid game event handle %x (error %d)", hndl, err);
	}

	char *key;
	pContext->LocalToString(params[2], &key);

	pInfo->pEvent->SetInt(key, params[3]);

	return 1;
}

static cell_t sm_SetEventFloat(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	EventInfo *pInfo;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(hndl, g_EventManager.GetHandleType(), &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid game event handle %x (error %d)", hndl, err);
	}

	char *key;
	pContext->LocalToString(params[2], &key);

	float value = sp_ctof(params[3]);
	pInfo->pEvent->SetFloat(key, value);

	return 1;
}

static cell_t sm_SetEventString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	EventInfo *pInfo;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(hndl, g_EventManager.GetHandleType(), &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid game event handle %x (error %d)", hndl, err);
	}

	char *key, *value;
	pContext->LocalToString(params[2], &key);
	pContext->LocalToString(params[3], &value);

	pInfo->pEvent->SetString(key, value);

	return 1;
}

static cell_t sm_SetEventBroadcast(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	EventInfo *pInfo;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(hndl, g_EventManager.GetHandleType(), &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid game event handle %x (error %d)", hndl, err);
	}

	pInfo->bDontBroadcast = params[2] ? true : false;

	return 1;
}

static cell_t sm_GetEventBroadcast(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	EventInfo *pInfo;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(hndl, g_EventManager.GetHandleType(), &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid game event handle %x (error %d)", hndl, err);
	}

	return pInfo->bDontBroadcast;
}

REGISTER_NATIVES(gameEventNatives)
{
	{"HookEvent",			sm_HookEvent},
	{"HookEventEx",			sm_HookEventEx},
	{"UnhookEvent",			sm_UnhookEvent},
	{"CreateEvent",			sm_CreateEvent},
	{"FireEvent",			sm_FireEvent},
	{"CancelCreatedEvent",	sm_CancelCreatedEvent},
	{"GetEventName",		sm_GetEventName},
	{"GetEventBool",		sm_GetEventBool},
	{"GetEventInt",			sm_GetEventInt},
	{"GetEventFloat",		sm_GetEventFloat},
	{"GetEventString",		sm_GetEventString},
	{"SetEventBool",		sm_SetEventBool},
	{"SetEventInt",			sm_SetEventInt},
	{"SetEventFloat",		sm_SetEventFloat},
	{"SetEventString",		sm_SetEventString},
	{"SetEventBroadcast",   sm_SetEventBroadcast},

	// Transitional syntax support.
	{"Event.Fire",			sm_FireEvent},
	{"Event.FireToClient",	sm_FireEventToClient},
	{"Event.Cancel",		sm_CancelCreatedEvent},
	{"Event.GetName",		sm_GetEventName},
	{"Event.GetBool",		sm_GetEventBool},
	{"Event.GetInt",		sm_GetEventInt},
	{"Event.GetFloat",		sm_GetEventFloat},
	{"Event.GetString",		sm_GetEventString},
	{"Event.SetBool",		sm_SetEventBool},
	{"Event.SetInt",		sm_SetEventInt},
	{"Event.SetFloat",		sm_SetEventFloat},
	{"Event.SetString",		sm_SetEventString},
	{"Event.BroadcastDisabled.set", sm_SetEventBroadcast},
	{"Event.BroadcastDisabled.get", sm_GetEventBroadcast},

	{NULL,					NULL}
};

