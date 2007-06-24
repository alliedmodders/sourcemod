/**
 * vim: set ts=4 :
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
#include "HandleSys.h"
#include "PlayerManager.h"
#include "HalfLife2.h"

IServerPluginCallbacks *g_VSP = NULL;

class HalfLifeNatives : public SMGlobalClass
{
public: //SMGlobalClass
	void OnSourceModVSPReceived(IServerPluginCallbacks *iface)
	{
		g_VSP = iface;
	}
};

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
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	if (!pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is not a fake client", params[1]);
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

static cell_t GetGameFolderName(IPluginContext *pContext, const cell_t *params)
{
	const char *name = g_SourceMod.GetGameFolderName();
	size_t numBytes;

	pContext->StringToLocalUTF8(params[1], params[2], name, &numBytes);

	return numBytes;
}

static cell_t GetCurrentMap(IPluginContext *pContext, const cell_t *params)
{
	size_t bytes;
	pContext->StringToLocalUTF8(params[1], params[2], STRING(gpGlobals->mapname), &bytes);
	return bytes;
}

static cell_t PrecacheModel(IPluginContext *pContext, const cell_t *params)
{
	char *model;
	pContext->LocalToString(params[1], &model);

	return engine->PrecacheModel(model, params[2] ? true : false);
}

static cell_t PrecacheSentenceFile(IPluginContext *pContext, const cell_t *params)
{
	char *sentencefile;
	pContext->LocalToString(params[1], &sentencefile);

	return engine->PrecacheSentenceFile(sentencefile, params[2] ? true : false);
}

static cell_t PrecacheDecal(IPluginContext *pContext, const cell_t *params)
{
	char *decal;
	pContext->LocalToString(params[1], &decal);

	return engine->PrecacheDecal(decal, params[2] ? true : false);
}

static cell_t PrecacheGeneric(IPluginContext *pContext, const cell_t *params)
{
	char *generic;
	pContext->LocalToString(params[1], &generic);

	return engine->PrecacheGeneric(generic, params[2] ? true : false);
}

static cell_t IsModelPrecached(IPluginContext *pContext, const cell_t *params)
{
	char *model;
	pContext->LocalToString(params[1], &model);

	return engine->IsModelPrecached(model) ? 1 : 0;
}

static cell_t IsDecalPrecached(IPluginContext *pContext, const cell_t *params)
{
	char *decal;
	pContext->LocalToString(params[1], &decal);

	return engine->IsDecalPrecached(decal) ? 1 : 0;
}

static cell_t IsGenericPrecached(IPluginContext *pContext, const cell_t *params)
{
	char *generic;
	pContext->LocalToString(params[1], &generic);

	return engine->IsGenericPrecached(generic) ? 1 : 0;
}

static cell_t PrecacheSound(IPluginContext *pContext, const cell_t *params)
{
	char *sample;
	pContext->LocalToString(params[1], &sample);

	return enginesound->PrecacheSound(sample, params[2] ? true : false) ? 1 : 0;
}

static cell_t IsSoundPrecached(IPluginContext *pContext, const cell_t *params)
{
	char *sample;
	pContext->LocalToString(params[1], &sample);

	return enginesound->IsSoundPrecached(sample) ? 1 : 0;
}

static cell_t smn_CreateDialog(IPluginContext *pContext, const cell_t *params)
{
	KeyValues *pKV;
	HandleError herr;
	Handle_t hndl = static_cast<Handle_t>(params[2]);
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(params[1]);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", params[1]);
	}

	pKV = g_SourceMod.ReadKeyValuesHandle(hndl, &herr, true);
	if (herr != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	serverpluginhelpers->CreateMessage(pPlayer->GetEdict(), static_cast<DIALOG_TYPE>(params[3]), pKV, g_VSP);

	return 1;
}

static cell_t PrintToChat(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}

	if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	g_SourceMod.SetGlobalTarget(client);

	char buffer[192];
	g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, 2);

	g_HL2.TextMsg(client, HUD_PRINTTALK, buffer);

	return 1;
}

static cell_t PrintCenterText(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}

	if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	g_SourceMod.SetGlobalTarget(client);

	char buffer[192];
	g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, 2);

	g_HL2.TextMsg(client, HUD_PRINTCENTER, buffer);

	return 1;
}

static HalfLifeNatives s_HalfLifeNatives;

REGISTER_NATIVES(halflifeNatives)
{
	{"CreateFakeClient",		CreateFakeClient},
	{"GetCurrentMap",			GetCurrentMap},
	{"GetEngineTime",			GetEngineTime},
	{"GetGameDescription",		GetGameDescription},
	{"GetGameFolderName",		GetGameFolderName},
	{"GetGameTime",				GetGameTime},
	{"GetRandomFloat",			GetRandomFloat},
	{"GetRandomInt",			GetRandomInt},
	{"IsDedicatedServer",		IsDedicatedServer},
	{"IsMapValid",				IsMapValid},
	{"SetFakeClientConVar",		SetFakeClientConVar},
	{"SetRandomSeed",			SetRandomSeed},
	{"PrecacheModel",			PrecacheModel},
	{"PrecacheSentenceFile",	PrecacheSentenceFile},
	{"PrecacheDecal",			PrecacheDecal},
	{"PrecacheGeneric",			PrecacheGeneric},
	{"IsModelPrecached",		IsModelPrecached},
	{"IsDecalPrecached",		IsDecalPrecached},
	{"IsGenericPrecached",		IsGenericPrecached},
	{"PrecacheSound",			PrecacheSound},
	{"IsSoundPrecached",		IsSoundPrecached},
	{"CreateDialog",			smn_CreateDialog},
	{"PrintToChat",				PrintToChat},
	{"PrintCenterText",			PrintCenterText},
	{NULL,						NULL},
};
