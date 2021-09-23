/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2017 AlliedModders LLC.  All rights reserved.
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

#include <sourcemod_version.h>
#include "extension.h"
#include <compat_wrappers.h>
#include "vcallbuilder.h"
#include "vnatives.h"
#include "vhelpers.h"
#include "vglobals.h"
#include "tempents.h"
#include "vsound.h"
#include "variant-t.h"
#include "output.h"
#include "hooks.h"
#include "gamerulesnatives.h"
#include <ISDKTools.h>
#include "clientnatives.h"
#include "teamnatives.h"
#include "filesystem.h"
#include "am-string.h"

/**
 * @file extension.cpp
 * @brief Implements SDK Tools extension code.
 */

SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, false, bool, const char *, const char *, const char *, const char *, bool, bool);
#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_CSGO
SH_DECL_HOOK1_void_vafmt(IVEngineServer, ClientCommand, SH_NOATTRIB, 0, edict_t *);
#endif
#if defined CLIENTVOICE_HOOK_SUPPORT
SH_DECL_HOOK1_void(IServerGameClients, ClientVoice, SH_NOATTRIB, 0, edict_t *);
#endif

SDKTools g_SdkTools;		/**< Global singleton for extension's main interface */
IServerGameEnts *gameents = NULL;
IEngineTrace *enginetrace = NULL;
ISpatialPartition *partition = NULL;
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
IServer *iserver = NULL;
IBaseFileSystem *basefilesystem = NULL;
CGlobalVars *gpGlobals;
ISoundEmitterSystemBase *soundemitterbase = NULL;
ITimer *g_hTimerSpeaking[SM_MAXPLAYERS+1];
IForward *m_OnClientSpeaking;
IForward *m_OnClientSpeakingEnd;

#if SOURCE_ENGINE >= SE_ORANGEBOX
IServerTools *servertools = NULL;
#endif

SourceHook::CallClass<IVEngineServer> *enginePatch = NULL;
SourceHook::CallClass<IEngineSound> *enginesoundPatch = NULL;
HandleType_t g_CallHandle = 0;
HandleType_t g_TraceHandle = 0;
ISDKTools *g_pSDKTools;

SMEXT_LINK(&g_SdkTools);

extern sp_nativeinfo_t g_CallNatives[];
extern sp_nativeinfo_t g_TENatives[];
extern sp_nativeinfo_t g_TRNatives[];
extern sp_nativeinfo_t g_StringTableNatives[];
extern sp_nativeinfo_t g_VoiceNatives[];
extern sp_nativeinfo_t g_EntInputNatives[];
extern sp_nativeinfo_t g_TeamNatives[];
extern sp_nativeinfo_t g_GameRulesNatives[];
extern sp_nativeinfo_t g_ClientNatives[];

static void InitSDKToolsAPI();

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
CDetour *g_WriteBaselinesDetour = NULL;

DETOUR_DECL_MEMBER3(CNetworkStringTableContainer__WriteBaselines, void, char const *, mapName, void *, buffer, int, currentTick)
{
	// Replace nAtTick with INT_MAX to work around CS:GO engine bug.
	// Due to a timing issue in the engine, stringtable entries added in OnConfigsExecuted can be considered
	// to have been added in the future for the first client that connects, which causes them to be ignored
	// when iterating for networking, which triggers a Host_Error encoding the CreateStringTable netmsg.
	return DETOUR_MEMBER_CALL(CNetworkStringTableContainer__WriteBaselines)(mapName, buffer, INT_MAX);
}
#endif

bool SDKTools::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	HandleError err;

	if (!gameconfs->LoadGameConfigFile("sdktools.games", &g_pGameConf, error, maxlength))
	{
		return false;
	}

	sharesys->AddDependency(myself, "bintools.ext", true, true);
	sharesys->AddNatives(myself, g_CallNatives);
	sharesys->AddNatives(myself, g_Natives);
	sharesys->AddNatives(myself, g_TENatives);
	sharesys->AddNatives(myself, g_SoundNatives);
	sharesys->AddNatives(myself, g_TRNatives);
	sharesys->AddNatives(myself, g_StringTableNatives);
	sharesys->AddNatives(myself, g_VoiceNatives);
	sharesys->AddNatives(myself, g_VariantTNatives);
	sharesys->AddNatives(myself, g_EntInputNatives);
	sharesys->AddNatives(myself, g_TeamNatives);
	sharesys->AddNatives(myself, g_EntOutputNatives);
	sharesys->AddNatives(myself, g_GameRulesNatives);
	sharesys->AddNatives(myself, g_ClientNatives);

	SM_GET_IFACE(GAMEHELPERS, g_pGameHelpers);

	playerhelpers->AddClientListener(&g_SdkTools);
	g_CallHandle = handlesys->CreateType("ValveCall", this, 0, NULL, NULL, myself->GetIdentity(), &err);
	if (g_CallHandle == 0)
	{
		ke::SafeSprintf(error, maxlength, "Could not create call handle type (err: %d)", err);	
		return false;
	}

	TypeAccess TraceAccess;
	handlesys->InitAccessDefaults(&TraceAccess, NULL);
	TraceAccess.ident = myself->GetIdentity();
	TraceAccess.access[HTypeAccess_Create] = true;
	TraceAccess.access[HTypeAccess_Inherit] = true;
	g_TraceHandle = handlesys->CreateType("TraceRay", this, 0, &TraceAccess, NULL, myself->GetIdentity(), &err);
	if (g_TraceHandle == 0)
	{
		handlesys->RemoveType(g_CallHandle, myself->GetIdentity());
		g_CallHandle = 0;
		ke::SafeSprintf(error, maxlength, "Could not create traceray handle type (err: %d)", err);
		return false;
	}

#if SOURCE_ENGINE >= SE_ORANGEBOX
	g_pCVar = icvar;
#endif
	CONVAR_REGISTER(this);

	SH_ADD_HOOK(IServerGameDLL, LevelInit, gamedll, SH_MEMBER(this, &SDKTools::LevelInit), true);

	playerhelpers->RegisterCommandTargetProcessor(this);

	MathLib_Init(2.2f, 2.2f, 0.0f, 2);

	spengine = g_pSM->GetScriptingEngine();

	plsys->AddPluginsListener(&g_OutputManager);

	CDetourManager::Init(g_pSM->GetScriptingEngine(), g_pGameConf);
	g_OutputManager.Init();

	VoiceInit();

	GetIServer();

	GameRulesNativesInit();

	InitSDKToolsAPI();

#if SOURCE_ENGINE == SE_CSGO
	m_bFollowCSGOServerGuidelines = true;
	const char *pszValue = g_pSM->GetCoreConfigValue("FollowCSGOServerGuidelines");
	if (pszValue && strcasecmp(pszValue, "no") == 0)
	{
		m_bFollowCSGOServerGuidelines = false;
	}

	m_CSGOBadList.init();
	m_CSGOBadList.add("m_bIsValveDS");
	m_CSGOBadList.add("m_bIsQuestEligible");
#endif

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	g_WriteBaselinesDetour = DETOUR_CREATE_MEMBER(CNetworkStringTableContainer__WriteBaselines, "WriteBaselines");
	if (g_WriteBaselinesDetour) {
		g_WriteBaselinesDetour->EnableDetour();
	} else {
		g_pSM->LogError(myself, "Failed to find WriteBaselines signature -- stringtable error workaround disabled.");
	}
#endif

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

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	if (g_WriteBaselinesDetour) {
		g_WriteBaselinesDetour->DisableDetour();
		g_WriteBaselinesDetour = NULL;
	}
#endif

	if (g_pAcceptInput)
	{
		g_pAcceptInput->Destroy();
		g_pAcceptInput = NULL;
	}

	g_TEManager.Shutdown();
	s_TempEntHooks.Shutdown();
	s_SoundHooks.Shutdown();
	g_Hooks.Shutdown();
	g_OutputManager.Shutdown();
	VoiceShutdown();

	forwards->ReleaseForward(m_OnClientSpeaking);
	forwards->ReleaseForward(m_OnClientSpeakingEnd);

	gameconfs->CloseGameConfigFile(g_pGameConf);
	playerhelpers->RemoveClientListener(&g_SdkTools);
	playerhelpers->UnregisterCommandTargetProcessor(this);
	plsys->RemovePluginsListener(&g_OutputManager);

	SH_REMOVE_HOOK(IServerGameDLL, LevelInit, gamedll, SH_MEMBER(this, &SDKTools::LevelInit), true);

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

	bool err;
	if (g_CallHandle != 0)
	{
		if ((err = handlesys->RemoveType(g_CallHandle, myself->GetIdentity())) != true)
		{
			g_pSM->LogError(myself, "Could not remove call handle (type=%x, err=%d)", g_CallHandle, err);
		}
	}

	if (g_TraceHandle != 0)
	{
		if ((err = handlesys->RemoveType(g_TraceHandle, myself->GetIdentity())) != true)
		{
			g_pSM->LogError(myself, "Could not remove trace handle (type=%x, err=%d)", g_TraceHandle, err);
		}
	}
}

bool SDKTools::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_ANY(GetServerFactory, gameents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);
	GET_V_IFACE_ANY(GetEngineFactory, engsound, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, enginetrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER);
	GET_V_IFACE_ANY(GetEngineFactory, partition, ISpatialPartition, INTERFACEVERSION_SPATIALPARTITION);
	GET_V_IFACE_ANY(GetEngineFactory, netstringtables, INetworkStringTableContainer, INTERFACENAME_NETWORKSTRINGTABLESERVER);
	GET_V_IFACE_ANY(GetEngineFactory, pluginhelpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);
	GET_V_IFACE_ANY(GetServerFactory, serverClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(GetEngineFactory, voiceserver, IVoiceServer, INTERFACEVERSION_VOICESERVER);
	GET_V_IFACE_ANY(GetServerFactory, playerinfomngr, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, basefilesystem, IBaseFileSystem, BASEFILESYSTEM_INTERFACE_VERSION);

#if SOURCE_ENGINE >= SE_ORANGEBOX
	GET_V_IFACE_ANY(GetServerFactory, servertools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION);
#endif
	GET_V_IFACE_ANY(GetEngineFactory, soundemitterbase, ISoundEmitterSystemBase, SOUNDEMITTERSYSTEM_INTERFACE_VERSION);

#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_CSGO
	SH_ADD_HOOK(IVEngineServer, ClientCommand, engine, SH_MEMBER(this, &SDKTools::OnSendClientCommand), false);
#endif
#if defined CLIENTVOICE_HOOK_SUPPORT
	SH_ADD_HOOK(IServerGameClients, ClientVoice, serverClients, SH_MEMBER(this, &SDKTools::OnClientVoice), true);
#endif
	gpGlobals = ismm->GetCGlobals();
	enginePatch = SH_GET_CALLCLASS(engine);
	enginesoundPatch = SH_GET_CALLCLASS(engsound);

	return true;
}

bool SDKTools::SDK_OnMetamodUnload(char *error, size_t maxlen)
{
#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_CSGO
	SH_REMOVE_HOOK(IVEngineServer, ClientCommand, engine, SH_MEMBER(this, &SDKTools::OnSendClientCommand), false);
#endif
#if defined CLIENTVOICE_HOOK_SUPPORT
	SH_REMOVE_HOOK(IServerGameClients, ClientVoice, serverClients, SH_MEMBER(this, &SDKTools::OnClientVoice), true);
#endif
	return true;
}

void SDKTools::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);

	if (!g_pBinTools)
	{
		return;
	}

	g_TEManager.Initialize();
	s_TempEntHooks.Initialize();
	s_SoundHooks.Initialize();
	g_Hooks.Initialize();
	InitializeValveGlobals();
	
	m_OnClientSpeaking = forwards->CreateForward("OnClientSpeaking", ET_Ignore, 1, NULL, Param_Cell);
	m_OnClientSpeakingEnd = forwards->CreateForward("OnClientSpeakingEnd", ET_Ignore, 1, NULL, Param_Cell);
}

void SDKTools::OnCoreMapStart(edict_t *pEdictList, int edictCount, int clientMax)
{
	InitTeamNatives();
	GetResourceEntity();
	g_Hooks.OnMapStart();
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
	return g_SMAPI->RegisterConCommandBase(g_PLAPI, pVar);
}

bool SDKTools::LevelInit(char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background)
{
	m_bAnyLevelInited = true;

	UpdateValveGlobals();

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
		ke::SafeSprintf(key, sizeof(key), "SlapSound%d", n);
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
	IGamePlayer *pAdmin = info->admin ? playerhelpers->GetGamePlayer(info->admin) : NULL;

	if (strcmp(info->pattern, "@aim") == 0)
	{
		/* The server can't aim, of course. */
		if (pAdmin == NULL)
		{
			return false;
		}

		int player_index;
		if ((player_index = GetClientAimTarget(pAdmin->GetEdict(), true)) < 1)
		{
			info->reason = COMMAND_TARGET_NONE;
			info->num_targets = 0;
			return true;
		}

		IGamePlayer *pTarget = playerhelpers->GetGamePlayer(player_index);

		if (pTarget == NULL)
		{
			info->reason = COMMAND_TARGET_NONE;
			info->num_targets = 0;
			return true;
		}

		info->reason = playerhelpers->FilterCommandTarget(pAdmin, pTarget, info->flags);
		if (info->reason != COMMAND_TARGET_VALID)
		{
			info->num_targets = 0;
			return true;
		}

		info->targets[0] = player_index;
		info->num_targets = 1;
		info->reason = COMMAND_TARGET_VALID;
		info->target_name_style = COMMAND_TARGETNAME_RAW;
		ke::SafeStrcpy(info->target_name, info->target_name_maxlength, pTarget->GetName());
		return true;
	}
	else if (strcmp(info->pattern, "@spec") == 0)
	{
		const char *teamname = tools_GetTeamName(1);
		if (strcasecmp(teamname, "spectator") != 0)
			return false;
		info->num_targets = 0;
		for (int i = 1; i <= playerhelpers->GetMaxClients(); i++)
		{
			IGamePlayer *player = playerhelpers->GetGamePlayer(i);
			if (player == NULL || !player->IsInGame() || player->IsSourceTV() || player->IsReplay())
				continue;
			IPlayerInfo *plinfo = player->GetPlayerInfo();
			if (plinfo == NULL)
				continue;
			if (plinfo->GetTeamIndex() == 1 &&
			    playerhelpers->FilterCommandTarget(pAdmin, player, info->flags) ==
				COMMAND_TARGET_VALID)
			{
				info->targets[info->num_targets++] = i;
			}
		}
		info->reason = info->num_targets > 0 ? COMMAND_TARGET_VALID : COMMAND_TARGET_EMPTY_FILTER;
		info->target_name_style = COMMAND_TARGETNAME_ML;
		ke::SafeStrcpy(info->target_name, info->target_name_maxlength, "all spectators");
		return true;
	}

	return false;
}

const char *SDKTools::GetExtensionVerString()
{
	return SOURCEMOD_VERSION;
}

const char *SDKTools::GetExtensionDateString()
{
	return SOURCEMOD_BUILD_TIME;
}

bool SDKTools::InterceptClientConnect(int client, char *error, size_t maxlength)
{
	g_Hooks.OnClientConnect(client);
	return true;
}

#if !defined CLIENTVOICE_HOOK_SUPPORT
void SDKTools::OnClientConnected(int client)
{
	g_Hooks.OnClientConnected(client);
}
#endif

#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_CSGO
void SDKTools::OnSendClientCommand(edict_t *pPlayer, const char *szFormat)
{
	// Due to legacy code, CS:S and CS:GO still sends "name \"newname\"" to the
	// client after aname change. The engine has a change hook on name causing
	// it to reset to the player's Steam name. This quashes that to make
	// SetClientName work properly.
	if (!strncmp(szFormat, "name ", 5))
	{
		RETURN_META(MRES_SUPERCEDE);
	}

	RETURN_META(MRES_IGNORED);
}
#endif

SourceMod::ResultType SDKTools::OnTimer(ITimer *pTimer, void *pData)
{
	int client = (int)(intptr_t)pData;

	m_OnClientSpeakingEnd->PushCell(client);
	m_OnClientSpeakingEnd->Execute();

	return Pl_Stop;
}

void SDKTools::OnTimerEnd(ITimer *pTimer, void *pData)
{
	g_hTimerSpeaking[(int)(intptr_t)pData] = nullptr;
}

#if defined CLIENTVOICE_HOOK_SUPPORT
void SDKTools::OnClientVoice(edict_t *pPlayer)
{
	if (!pPlayer)
	{
		return;
	}

	int client = IndexOfEdict(pPlayer);

	if (g_hTimerSpeaking[client])
	{
		timersys->KillTimer(g_hTimerSpeaking[client]);
	}

	g_hTimerSpeaking[client] = timersys->CreateTimer(this, 0.3f, (void *)(intptr_t)client, 0);

	m_OnClientSpeaking->PushCell(client);
	m_OnClientSpeaking->Execute();
}
#endif

void SDKTools::OnClientPutInServer(int client)
{
	g_Hooks.OnClientPutInServer(client);
}

class SDKTools_API : public ISDKTools
{
public:
	virtual const char *GetInterfaceName()
	{
		return SMINTERFACE_SDKTOOLS_NAME;
	}

	virtual unsigned int GetInterfaceVersion()
	{
		return SMINTERFACE_SDKTOOLS_VERSION;
	}

	virtual IServer *GetIServer()
	{
		return iserver;
	}
	
	virtual void *GetGameRules()
	{
		return GameRules();
	}
} g_SDKTools_API;

static void InitSDKToolsAPI()
{
	g_pSDKTools = &g_SDKTools_API;
	sharesys->AddInterface(myself, g_pSDKTools);
}
