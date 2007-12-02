/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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
#include "vcallbuilder.h"
#include "vnatives.h"
#include "vhelpers.h"
#include "vglobals.h"
#include "tempents.h"
#include "vsound.h"

#if defined ORANGEBOX_BUILD
	#define SDKTOOLS_GAME_FILE		"sdktools.games.ep2"
#else
	#define SDKTOOLS_GAME_FILE		"sdktools.games"
#endif

/**
 * @file extension.cpp
 * @brief Implements SDK Tools extension code.
 */

SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, false, bool, const char *, const char *, const char *, const char *, bool, bool);
SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);

SDKTools g_SdkTools;		/**< Global singleton for extension's main interface */
IServerGameEnts *gameents = NULL;
IEngineTrace *enginetrace = NULL;
IEngineSound *engsound = NULL;
INetworkStringTableContainer *netstringtables = NULL;
IServerPluginHelpers *pluginhelpers = NULL;
IBinTools *g_pBinTools = NULL;
IGameConfig *g_pGameConf = NULL;
IGameHelpers *g_pGameHelpers = NULL;
IServerGameClients *serverClients = NULL;
IVoiceServer *voiceserver = NULL;
IPlayerInfoManager *playerinfomngr = NULL;
ICvar *icvar = NULL;
SourceHook::CallClass<IVEngineServer> *enginePatch = NULL;
SourceHook::CallClass<IEngineSound> *enginesoundPatch = NULL;
HandleType_t g_CallHandle = 0;
HandleType_t g_TraceHandle = 0;

SMEXT_LINK(&g_SdkTools);

extern sp_nativeinfo_t g_CallNatives[];
extern sp_nativeinfo_t g_TENatives[];
extern sp_nativeinfo_t g_TRNatives[];
extern sp_nativeinfo_t g_StringTableNatives[];
extern sp_nativeinfo_t g_VoiceNatives[];
extern sp_nativeinfo_t g_EntInputNatives[];
extern sp_nativeinfo_t g_TeamNatives[];

bool SDKTools::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "bintools.ext", true, true);
	sharesys->AddNatives(myself, g_CallNatives);
	sharesys->AddNatives(myself, g_Natives);
	sharesys->AddNatives(myself, g_TENatives);
	sharesys->AddNatives(myself, g_SoundNatives);
	sharesys->AddNatives(myself, g_TRNatives);
	sharesys->AddNatives(myself, g_StringTableNatives);
	sharesys->AddNatives(myself, g_VoiceNatives);
	sharesys->AddNatives(myself, g_EntInputNatives);
	sharesys->AddNatives(myself, g_TeamNatives);

	SM_GET_IFACE(GAMEHELPERS, g_pGameHelpers);

	if (!gameconfs->LoadGameConfigFile(SDKTOOLS_GAME_FILE, &g_pGameConf, error, maxlength))
	{
		return false;
	}

	playerhelpers->AddClientListener(&g_SdkTools);
	g_CallHandle = handlesys->CreateType("ValveCall", this, 0, NULL, NULL, myself->GetIdentity(), NULL);
	g_TraceHandle = handlesys->CreateType("TraceRay", this, 0, NULL, NULL, myself->GetIdentity(), NULL);

#if defined ORANGEBOX_BUILD
	g_pCVar = icvar;
#endif
	CONVAR_REGISTER(this);

	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, gamedll, this, &SDKTools::LevelInit, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, gamedll, this, &SDKTools::OnServerActivate, false);

	playerhelpers->RegisterCommandTargetProcessor(this);

	MathLib_Init(2.2f, 2.2f, 0.0f, 2);

	return true;
}

void SDKTools::OnHandleDestroy(HandleType_t type, void *object)
{
	if (type == g_CallHandle)
	{
		ValveCall *v = (ValveCall *)object;
		delete v;
	}
	else if (type == g_TraceHandle)
	{
		trace_t *tr = (trace_t *)object;
		delete tr;
	}
}

void SDKTools::SDK_OnUnload()
{
	SourceHook::List<ValveCall *>::iterator iter;
	for (iter = g_RegCalls.begin();
		 iter != g_RegCalls.end();
		 iter++)
	{
		delete (*iter);
	}
	g_RegCalls.clear();
	ShutdownHelpers();

	if (g_pAcceptInput)
	{
		g_pAcceptInput->Destroy();
		g_pAcceptInput = NULL;
	}

	g_TEManager.Shutdown();
	s_TempEntHooks.Shutdown();
	s_SoundHooks.Shutdown();

	gameconfs->CloseGameConfigFile(g_pGameConf);
	playerhelpers->RemoveClientListener(&g_SdkTools);
	playerhelpers->UnregisterCommandTargetProcessor(this);

	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, gamedll, this, &SDKTools::LevelInit, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, gamedll, this, &SDKTools::OnServerActivate, false);

	if (enginePatch)
	{
		SH_RELEASE_CALLCLASS(enginePatch);
		enginePatch = NULL;
	}
	if (enginesoundPatch)
	{
		SH_RELEASE_CALLCLASS(enginesoundPatch);
		enginesoundPatch = NULL;
	}
}

bool SDKTools::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_ANY(GetServerFactory, gameents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);
	GET_V_IFACE_ANY(GetEngineFactory, engsound, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, enginetrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER);
	GET_V_IFACE_ANY(GetEngineFactory, netstringtables, INetworkStringTableContainer, INTERFACENAME_NETWORKSTRINGTABLESERVER);
	GET_V_IFACE_ANY(GetEngineFactory, pluginhelpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);
	GET_V_IFACE_ANY(GetServerFactory, serverClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(GetEngineFactory, voiceserver, IVoiceServer, INTERFACEVERSION_VOICESERVER);
	GET_V_IFACE_ANY(GetServerFactory, playerinfomngr, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);

	enginePatch = SH_GET_CALLCLASS(engine);
	enginesoundPatch = SH_GET_CALLCLASS(engsound);

	return true;
}

void SDKTools::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);

	g_TEManager.Initialize();
	s_TempEntHooks.Initialize();
	s_SoundHooks.Initialize();
	InitializeValveGlobals();
}

bool SDKTools::QueryRunning(char *error, size_t maxlength)
{
	SM_CHECK_IFACE(BINTOOLS, g_pBinTools);

	return true;
}

bool SDKTools::QueryInterfaceDrop(SMInterface *pInterface)
{
	if (pInterface == g_pBinTools)
	{
		return false;
	}

	return IExtensionInterface::QueryInterfaceDrop(pInterface);
}

void SDKTools::NotifyInterfaceDrop(SMInterface *pInterface)
{
	SourceHook::List<ValveCall *>::iterator iter;
	for (iter = g_RegCalls.begin();
		iter != g_RegCalls.end();
		iter++)
	{
		delete (*iter);
	}
	g_RegCalls.clear();
	ShutdownHelpers();

	g_TEManager.Shutdown();
	s_TempEntHooks.Shutdown();

	if (g_pAcceptInput)
	{
		g_pAcceptInput->Destroy();
		g_pAcceptInput = NULL;
	}
}

bool SDKTools::RegisterConCommandBase(ConCommandBase *pVar)
{
#if defined METAMOD_PLAPI_VERSION
	return g_SMAPI->RegisterConCommandBase(g_PLAPI, pVar);
#else
	return g_SMAPI->RegisterConCmdBase(g_PLAPI, pVar);
#endif
}

bool SDKTools::LevelInit(char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background)
{
	const char *name;
	char key[32];
	int count, n = 1;

	if (!(name=g_pGameConf->GetKeyValue("SlapSoundCount")))
	{
		RETURN_META_VALUE(MRES_IGNORED, true);
	}

	count = atoi(name);

	while (n <= count)
	{
		snprintf(key, sizeof(key), "SlapSound%d", n);
		if ((name=g_pGameConf->GetKeyValue(key)))
		{
			engsound->PrecacheSound(name, true);
		}
		n++;
	}

	RETURN_META_VALUE(MRES_IGNORED, true);
}

bool SDKTools::ProcessCommandTarget(cmd_target_info_t *info)
{
	if (strcmp(info->pattern, "@aim") != 0)
	{
		return false;
	}

	IGamePlayer *pAdmin = info->admin ? playerhelpers->GetGamePlayer(info->admin) : NULL;

	/* The server can't aim, of course. */
	if (pAdmin == NULL)
	{
		return false;
	}

	int player_index;
	if ((player_index = GetClientAimTarget(pAdmin->GetEdict(), true)) < 1)
	{
		return false;
	}

	IGamePlayer *pTarget = playerhelpers->GetGamePlayer(info->admin);

	info->targets[0] = player_index;
	info->num_targets = 1;
	info->reason = COMMAND_TARGET_VALID;
	info->target_name_style = COMMAND_TARGETNAME_RAW;
	snprintf(info->target_name, info->target_name_maxlength, "%s", pTarget->GetName());

	return true;
}
