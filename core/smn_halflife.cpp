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
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "HandleSys.h"
#include "PlayerManager.h"
#include "HalfLife2.h"

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

	return IndexOfEdict(pEdict);
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

/* Useless comment to bump the build */
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

	serverpluginhelpers->CreateMessage(pPlayer->GetEdict(), 
		static_cast<DIALOG_TYPE>(params[3]), 
		pKV, 
		vsp_interface);

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

	/* Check for an error before printing to the client */
	if (pContext->GetLastNativeError() != SP_ERROR_NONE)
	{
		return 0;
	}

	if (!g_HL2.TextMsg(client, HUD_PRINTTALK, buffer))
	{
		return pContext->ThrowNativeError("Could not send a usermessage");
	}

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

	/* Check for an error before printing to the client */
	if (pContext->GetLastNativeError() != SP_ERROR_NONE)
	{
		return 0;
	}

	if (!g_HL2.TextMsg(client, HUD_PRINTCENTER, buffer))
	{
		return pContext->ThrowNativeError("Could not send a usermessage");
	}

	return 1;
}

static cell_t PrintHintText(IPluginContext *pContext, const cell_t *params)
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

	/* Check for an error before printing to the client */
	if (pContext->GetLastNativeError() != SP_ERROR_NONE)
	{
		return 0;
	}

	if (!g_HL2.HintTextMsg(client, buffer))
	{
		return pContext->ThrowNativeError("Could not send a usermessage");
	}

	return 1;
}

static cell_t ShowVGUIPanel(IPluginContext *pContext, const cell_t *params)
{
	HandleError herr;
	char *name;
	KeyValues *pKV = NULL;
	int client = params[1];
	Handle_t hndl = static_cast<Handle_t>(params[3]);
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}

	if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	if (hndl != BAD_HANDLE)
	{
		pKV = g_SourceMod.ReadKeyValuesHandle(hndl, &herr, true);
		if (herr != HandleError_None)
		{
			return pContext->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
		}
	}

	pContext->LocalToString(params[2], &name);

	if (!g_HL2.ShowVGUIMenu(client, name, pKV, (params[4]) ? true : false))
	{
		return pContext->ThrowNativeError("Could not send a usermessage");
	}

	return 1;
}

static cell_t smn_IsPlayerAlive(IPluginContext *pContext, const cell_t *params)
{
	CPlayer *player = g_Players.GetPlayerByIndex(params[1]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Invalid client index %d", params[1]);
	}
	else if (!player->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", params[1]);
	}

	unsigned int state = player->GetLifeState();

	if (state == PLAYER_LIFE_UNKNOWN)
	{
		return pContext->ThrowNativeError("\"IsPlayerAlive\" not supported by this mod");
	}
	else if (state == PLAYER_LIFE_ALIVE)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static cell_t GuessSDKVersion(IPluginContext *pContext, const cell_t *params)
{
#if defined METAMOD_PLAPI_VERSION || PLAPI_VERSION >= 11
	int version = g_SMAPI->GetSourceEngineBuild();

	switch (version)
	{
	case SOURCE_ENGINE_ORIGINAL:
		return 10;
	case SOURCE_ENGINE_EPISODEONE:
		return 20;

# if defined METAMOD_PLAPI_VERISON
	/* Newer games. */
	case SOURCE_ENGINE_DARKMESSIAH:
		return 15;
	case SOURCE_ENGINE_ORANGEBOX:
		return 30;
	case SOURCE_ENGINE_LEFT4DEAD:
		return 40;
# endif
	}
#else
	if (g_HL2.IsOriginalEngine())
	{
		return 10;
	}
	else
	{
		return 20;
	}
#endif

	return 0;
}

static cell_t IndexToReference(IPluginContext *pContext, const cell_t *params)
{
	if (params[1] >= NUM_ENT_ENTRIES || params[1] < 0)
	{
		return pContext->ThrowNativeError("Invalid entity index %i", params[1]);
	}

	return g_HL2.IndexToReference(params[1]);
}

static cell_t ReferenceToIndex(IPluginContext *pContext, const cell_t *params)
{
	return g_HL2.ReferenceToIndex(params[1]);
}

static cell_t ReferenceToBCompatRef(IPluginContext *pContext, const cell_t *params)
{
	return g_HL2.ReferenceToBCompatRef(params[1]);
}

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
	{"PrintHintText",			PrintHintText},
	{"ShowVGUIPanel",			ShowVGUIPanel},
	{"IsPlayerAlive",			smn_IsPlayerAlive},
	{"GuessSDKVersion",			GuessSDKVersion},
	{"EntIndexToEntRef",		IndexToReference},
	{"EntRefToEntIndex",		ReferenceToIndex},
	{"MakeCompatEntRef",		ReferenceToBCompatRef},
	{NULL,						NULL},
};
