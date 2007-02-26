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
#include "CPlayerManager.h"

static cell_t SetRandomSeed(IPluginContext *pContext, const cell_t *params)
{
	engrandom->SetSeed(params[1]);

	return 1;
}

static cell_t GetRandomFloat(IPluginContext *pContext, const cell_t *params)
{
	float fMin = sp_ctof(params[1]);
	float fMax = sp_ctof(params[2]);

	float fRandom = engrandom->RandomFloat(fMin, fMax);

	return sp_ftoc(fRandom);
}

static cell_t GetRandomInt(IPluginContext *pContext, const cell_t *params)
{
	return engrandom->RandomInt(params[1], params[2]);
}

static cell_t IsMapValid(IPluginContext *pContext, const cell_t *params)
{
	char *map;
	pContext->LocalToString(params[1], &map);

	return engine->IsMapValid(map);
}

static cell_t IsDedicatedServer(IPluginContext *pContext, const cell_t *params)
{
	return engine->IsDedicatedServer();
}

static cell_t GetEntityCount(IPluginContext *pContext, const cell_t *params)
{
	return engine->GetEntityCount();
}

static cell_t IsValidEntity(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = engine->PEntityOfEntIndex(params[1]);

	if (!pEdict)
	{
		return 0;
	}

	/* Shouldn't be necessary... not sure though */
	return pEdict->IsFree() ? 0 : 1;
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

static cell_t GetEngineTime(IPluginContext *pContext, const cell_t *params)
{
	float fTime = engine->Time();
	return sp_ftoc(fTime);
}

static cell_t GetGameTime(IPluginContext *pContext, const cell_t *params)
{
	return sp_ftoc(gpGlobals->curtime);
}

static cell_t CreateFakeClient(IPluginContext *pContext, const cell_t *params)
{
	char *netname;

	pContext->LocalToString(params[1], &netname);

	edict_t *pEdict = engine->CreateFakeClient(netname);

	/* :TODO: does the engine fire forwards for us and whatnot? no idea... */

	if (!pEdict)
	{
		return 0;
	}

	return engine->IndexOfEdict(pEdict);
}

static cell_t SetFakeClientConVar(IPluginContext *pContext, const cell_t *params)
{
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(params[1]);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Player %d is not a valid player", params[1]);
	}

	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Player %d is not connected", params[1]);
	}

	if (!pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Player %d is not a fake client", params[1]);
	}

	char *cvar, *value;

	pContext->LocalToString(params[2], &cvar);
	pContext->LocalToString(params[3], &value);

	engine->SetFakeClientConVarValue(pPlayer->GetEdict(), cvar, value);

	return 1;
}

static cell_t GetGameDescription(IPluginContext *pContext, const cell_t *params)
{
	const char *description;

	if (params[3])
	{
		description = gamedll->GetGameDescription();
	} else {
		description = SERVER_CALL(GetGameDescription)();
	}

	size_t numBytes;
	
	pContext->StringToLocalUTF8(params[1], params[2], description, &numBytes);

	return numBytes;
}

static cell_t GetMaxEntities(IPluginContext *pContext, const cell_t *params)
{
	return gpGlobals->maxEntities;
}

static cell_t GetCurrentMap(IPluginContext *pContext, const cell_t *params)
{
	size_t bytes;
	pContext->StringToLocalUTF8(params[1], params[2], STRING(gpGlobals->mapname), &bytes);
	return bytes;
}

REGISTER_NATIVES(halflifeNatives)
{
	{"CreateEdict",				CreateEdict},
	{"CreateFakeClient",		CreateFakeClient},
	{"GetCurrentMap",			GetCurrentMap},
	{"GetEngineTime",			GetEngineTime},
	{"GetEntityCount",			GetEntityCount},
	{"GetGameDescription",		GetGameDescription},
	{"GetGameTime",				GetGameTime},
	{"GetMaxEntities",			GetMaxEntities},
	{"GetRandomFloat",			GetRandomFloat},
	{"GetRandomInt",			GetRandomInt},
	{"IsDedicatedServer",		IsDedicatedServer},
	{"IsMapValid",				IsMapValid},
	{"IsValidEntity",			IsValidEntity},
	{"RemoveEdict",				RemoveEdict},
	{"SetFakeClientConVar",		SetFakeClientConVar},
	{"SetRandomSeed",			SetRandomSeed},
	{NULL,						NULL},
};
