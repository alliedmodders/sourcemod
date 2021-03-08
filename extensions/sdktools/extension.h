/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
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

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

/**
 * @file extension.h
 * @brief SDK Tools extension code header.
 */

#include "smsdk_ext.h"
#include <IBinTools.h>
#include <IPlayerHelpers.h>
#include <IGameHelpers.h>
#include <IEngineTrace.h>
#include <ispatialpartition.h>
#include <IEngineSound.h>
#include <ivoiceserver.h>
#include <iplayerinfo.h>
#include <server_class.h>
#include <datamap.h>
#include <convar.h>
#include <iserver.h>
#include <cdll_int.h>
#if SOURCE_ENGINE == SE_CSGO
#include <am-hashset.h>
#include <sm_stringhashmap.h>
#endif
#include "SoundEmitterSystem/isoundemittersystembase.h"

#if SOURCE_ENGINE >= SE_ORANGEBOX
#include <itoolentity.h>
#endif

#if SOURCE_ENGINE == SE_ALIENSWARM || SOURCE_ENGINE == SE_PORTAL2 || SOURCE_ENGINE == SE_INSURGENCY || SOURCE_ENGINE == SE_DOI || SOURCE_ENGINE == SE_BLADE || SOURCE_ENGINE == SE_CSGO
#define CLIENTVOICE_HOOK_SUPPORT
#else
#include <inetmsghandler.h>
#endif

/**
 * @brief Implementation of the SDK Tools extension.
 * Note: Uncomment one of the pre-defined virtual functions in order to use it.
 */
class SDKTools : 
	public SDKExtension, 
	public IHandleTypeDispatch,
	public IConCommandBaseAccessor,
	public IClientListener,
	public ITimedEvent,
	public ICommandTargetProcessor
{
public: //public IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object);
public: //public SDKExtension
	virtual bool SDK_OnLoad(char *error, size_t maxlength, bool late);
	virtual void SDK_OnUnload();
	virtual void SDK_OnAllLoaded();
	//virtual void SDK_OnPauseChange(bool paused);
	virtual bool QueryRunning(char *error, size_t maxlength);
	virtual bool QueryInterfaceDrop(SMInterface *pInterface);
	virtual void NotifyInterfaceDrop(SMInterface *pInterface);
	virtual void OnCoreMapStart(edict_t *pEdictList, int edictCount, int clientMax);
	const char *GetExtensionVerString();
	const char *GetExtensionDateString();
public:
#if defined SMEXT_CONF_METAMOD
	virtual bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	virtual bool SDK_OnMetamodUnload(char *error, size_t maxlen);
	//virtual bool SDK_OnMetamodPauseChange(bool paused, char *error, size_t maxlen);
#endif
public: //IConCommandBaseAccessor
	bool RegisterConCommandBase(ConCommandBase *pVar);
public: //IClientListner
	bool InterceptClientConnect(int client, char *error, size_t maxlength);
#if !defined CLIENTVOICE_HOOK_SUPPORT
	void OnClientConnected(int client);
#endif
	void OnClientPutInServer(int client);
	void OnClientDisconnecting(int client);
public:
#if defined CLIENTVOICE_HOOK_SUPPORT
	void OnClientVoice(edict_t *pPlayer);
#endif
public: // IVoiceServer
	bool OnSetClientListening(int iReceiver, int iSender, bool bListen);
	void VoiceInit();
	void VoiceShutdown();
#if SOURCE_ENGINE >= SE_ORANGEBOX
	void OnClientCommand(edict_t *pEntity, const CCommand &args);
#else
	void OnClientCommand(edict_t *pEntity);
#endif
#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_CSGO
	void OnSendClientCommand(edict_t *pPlayer, const char *szFormat);
#endif
public: //ITimedEvent
	ResultType OnTimer(ITimer *pTimer, void *pData);
	void OnTimerEnd(ITimer *pTimer, void *pData);

public: //ICommandTargetProcessor
	bool ProcessCommandTarget(cmd_target_info_t *info);
public:
	bool LevelInit(char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background);
	void OnServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
public:
	bool HasAnyLevelInited() { return m_bAnyLevelInited; }
#if SOURCE_ENGINE == SE_CSGO
public:
	bool ShouldFollowCSGOServerGuidelines() const { return m_bFollowCSGOServerGuidelines; }
	bool CanSetCSGOEntProp(const char *pszPropName) { return !ShouldFollowCSGOServerGuidelines() || !m_CSGOBadList.has(pszPropName); }
private:
	ke::HashSet<std::string, detail::StringHashMapPolicy> m_CSGOBadList;
	bool m_bFollowCSGOServerGuidelines = false;
#endif
private:
	bool m_bAnyLevelInited = false;
};

extern SDKTools g_SdkTools;
/* Interfaces from engine or gamedll */
extern IServerGameEnts *gameents;
extern IEngineTrace *enginetrace;
extern ISpatialPartition *partition;
extern IEngineSound *engsound;
extern INetworkStringTableContainer *netstringtables;
extern IServerPluginHelpers *pluginhelpers;
extern IServerGameClients *serverClients;
extern IVoiceServer *voiceserver;
extern IPlayerInfoManager *playerinfomngr;
extern ICvar *icvar;
extern IServer *iserver;
extern IBaseFileSystem *basefilesystem;
extern CGlobalVars *gpGlobals;
#if SOURCE_ENGINE >= SE_ORANGEBOX
extern IServerTools *servertools;
#endif
extern ISoundEmitterSystemBase *soundemitterbase;
/* Interfaces from SourceMod */
extern IBinTools *g_pBinTools;
extern IGameConfig *g_pGameConf;
extern IGameHelpers *g_pGameHelpers;
/* Handle types */
extern HandleType_t g_CallHandle;
extern HandleType_t g_TraceHandle;
/* Call Wrappers */
extern ICallWrapper *g_pAcceptInput;
/* Timers */
extern ITimer *g_hTimerSpeaking[SM_MAXPLAYERS+1];
/* Forwards */
extern IForward *m_OnClientSpeaking;
extern IForward *m_OnClientSpeakingEnd;
/* Call classes */
extern SourceHook::CallClass<IVEngineServer> *enginePatch;
extern SourceHook::CallClass<IEngineSound> *enginesoundPatch;

#include <compat_wrappers.h>

#define ENGINE_CALL(func)		SH_CALL(enginePatch, &IVEngineServer::func)

#endif //_INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
