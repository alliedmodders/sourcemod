/**
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#include "sm_globals.h"
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "server_class.h"
#include "CPlayerManager.h"

inline edict_t *GetEdict(cell_t num)
{
	edict_t *pEdict = engine->PEntityOfEntIndex(num);
	if (!pEdict || pEdict->IsFree())
	{
		return NULL;
	}
	if (num > 0 && num < g_Players.GetMaxClients())
	{
		CPlayer *pPlayer = g_Players.GetPlayerByIndex(num);
		if (!pPlayer || !pPlayer->IsConnected())
		{
			return NULL;
		}
	}
	return pEdict;
}

static cell_t GetMaxEntities(IPluginContext *pContext, const cell_t *params)
{
	return gpGlobals->maxEntities;
}

static cell_t CreateEdict(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = engine->CreateEdict();

	if (!pEdict)
	{
		return 0;
	}

	return engine->IndexOfEdict(pEdict);
}

static cell_t RemoveEdict(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = engine->PEntityOfEntIndex(params[1]);

	if (!pEdict)
	{
		return pContext->ThrowNativeError("Edict %d is not a valid edict", params[1]);
	}

	engine->RemoveEdict(pEdict);

	return 1;
}

static cell_t IsValidEdict(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return 0;
	}

	/* Shouldn't be necessary... not sure though */
	return pEdict->IsFree() ? 0 : 1;
}

static cell_t IsValidEntity(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return 0;
	}

	IServerUnknown *pUnknown = pEdict->GetUnknown();
	if (!pUnknown)
	{
		return 0;
	}

	CBaseEntity *pEntity = pUnknown->GetBaseEntity();
	return (pEntity != NULL) ? 1 : 0;
}

static cell_t IsEntNetworkable(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return 0;
	}

	return (pEdict->GetNetworkable() != NULL) ? 1 : 0;
}

static cell_t GetEntityCount(IPluginContext *pContext, const cell_t *params)
{
	return engine->GetEntityCount();
}

static cell_t GetEdictFlags(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return pContext->ThrowNativeError("Invalid edict (%d)", params[1]);
	}

	return pEdict->m_fStateFlags;
}

static cell_t SetEdictFlags(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return pContext->ThrowNativeError("Invalid edict (%d)", params[1]);
	}

	pEdict->m_fStateFlags = params[2];

	return 1;
}

static cell_t GetEdictClassname(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return pContext->ThrowNativeError("Invalid edict (%d)", params[1]);
	}

	const char *cls = pEdict->GetClassName();

	if (!cls || cls[0] == '\0')
	{
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], cls);

	return 1;
}

static cell_t GetEntityNetClass(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return pContext->ThrowNativeError("Invalid edict (%d)", params[1]);
	}

	IServerNetworkable *pNet = pEdict->GetNetworkable();
	if (!pNet)
	{
		return 0;
	}

	ServerClass *pClass = pNet->GetServerClass();

	pContext->StringToLocal(params[2], params[3], pClass->GetName());

	return 1;
}

REGISTER_NATIVES(entityNatives)
{
	{"CreateEdict",				CreateEdict},
	{"GetEdictClassname",		GetEdictClassname},
	{"GetEdictFlags",			GetEdictFlags},
	{"GetEntityNetClass",		GetEntityNetClass},
	{"GetMaxEntities",			GetMaxEntities},
	{"IsEntNetworkable",		IsEntNetworkable},
	{"IsValidEdict",			IsValidEdict},
	{"IsValidEntity",			IsValidEntity},
	{"RemoveEdict",				RemoveEdict},
	{"SetEdictFlags",			SetEdictFlags},
};
