/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2015 AlliedModders LLC.  All rights reserved.
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

#include "PlayerManager.h"
#include "sourcemod.h"
#include "IAdminSystem.h"
#include "ConCmdManager.h"
#include "MenuStyle_Valve.h"
#include "MenuStyle_Radio.h"
#include "sm_stringutil.h"
#include "CoreConfig.h"
#include "TimerSys.h"
#include "Logger.h"
#include "ChatTriggers.h"
#include "HalfLife2.h"
#include <inetchannel.h>
#include <iclient.h>
#include <iserver.h>
#include <IGameConfigs.h>
#include "ConsoleDetours.h"
#include "logic_bridge.h"
#include <sourcemod_version.h>
#include "smn_keyvalues.h"
#include "command_args.h"
#include <bridge/include/IExtensionBridge.h>
#include <bridge/include/IScriptManager.h>
#include <bridge/include/ILogger.h>

PlayerManager g_Players;
bool g_OnMapStarted = false;
IForward *PreAdminCheck = NULL;
IForward *PostAdminCheck = NULL;
IForward *PostAdminFilter = NULL;

const unsigned int *g_NumPlayersToAuth = NULL;
int lifestate_offset = -1;
List<ICommandTargetProcessor *> target_processors;

ConVar sm_debug_connect("sm_debug_connect", "1", 0, "Log Debug information about potential connection issues.");

SH_DECL_HOOK5(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, edict_t *, const char *, const char *, char *, int);
SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, const char *);
SH_DECL_HOOK1_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, edict_t *);
#if SOURCE_ENGINE >= SE_ORANGEBOX
SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *, const CCommand &);
#else
SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);
#endif
SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, edict_t *);

#if SOURCE_ENGINE >= SE_EYE
SH_DECL_HOOK2_void(IServerGameClients, ClientCommandKeyValues, SH_NOATTRIB, 0, edict_t *, KeyValues *);
#endif

SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);

#if SOURCE_ENGINE >= SE_LEFT4DEAD
SH_DECL_HOOK1_void(IServerGameDLL, ServerHibernationUpdate, SH_NOATTRIB, 0, bool);
#elif SOURCE_ENGINE > SE_EYE // 2013/orangebox, but not original orangebox.
SH_DECL_HOOK1_void(IServerGameDLL, SetServerHibernation, SH_NOATTRIB, 0, bool);
#endif

#if SOURCE_ENGINE >= SE_ORANGEBOX
SH_DECL_EXTERN1_void(ConCommand, Dispatch, SH_NOATTRIB, false, const CCommand &);
#else
SH_DECL_EXTERN0_void(ConCommand, Dispatch, SH_NOATTRIB, false);
#endif
SH_DECL_HOOK2_void(IVEngineServer, ClientPrintf, SH_NOATTRIB, 0, edict_t *, const char *);

static void PrintfBuffer_FrameAction(void *data)
{
	g_Players.OnPrintfFrameAction(static_cast<unsigned int>(reinterpret_cast<uintptr_t>(data)));
}

ConCommand *maxplayersCmd = NULL;

unsigned int g_PlayerSerialCount = 0;

class KickPlayerTimer : public ITimedEvent
{
public:
	ResultType OnTimer(ITimer *pTimer, void *pData)
	{
		int userid = (int)(intptr_t)pData;
		int client = g_Players.GetClientOfUserId(userid);
		if (client)
		{
			CPlayer *player = g_Players.GetPlayerByIndex(client);
			player->Kick("Your name is reserved by SourceMod; set your password to use it.");
		}
		return Pl_Stop;
	}
	void OnTimerEnd(ITimer *pTimer, void *pData)
	{
	}
} s_KickPlayerTimer;

PlayerManager::PlayerManager()
{
	m_AuthQueue = NULL;
	m_bServerActivated = false;
	m_maxClients = 0;

	m_SourceTVUserId = -1;
	m_ReplayUserId = -1;

	m_bInCCKVHook = false;
	m_bAuthstringValidation = true; // use steam auth by default

	m_UserIdLookUp = new int[USHRT_MAX+1];
	memset(m_UserIdLookUp, 0, sizeof(int) * (USHRT_MAX+1));
}

PlayerManager::~PlayerManager()
{
	g_NumPlayersToAuth = NULL;

	delete [] m_AuthQueue;
	delete [] m_UserIdLookUp;
}

void PlayerManager::OnSourceModStartup(bool late)
{
	/* Initialize all players */

	m_PlayerCount = 0;
	m_Players = new CPlayer[SM_MAXPLAYERS + 1];
	m_AuthQueue = new unsigned int[SM_MAXPLAYERS + 1];

	memset(m_AuthQueue, 0, sizeof(unsigned int) * (SM_MAXPLAYERS + 1));

	g_NumPlayersToAuth = &m_AuthQueue[0];
}

void PlayerManager::OnSourceModAllInitialized()
{
	SH_ADD_HOOK(IServerGameClients, ClientConnect, serverClients, SH_MEMBER(this, &PlayerManager::OnClientConnect), false);
	SH_ADD_HOOK(IServerGameClients, ClientConnect, serverClients, SH_MEMBER(this, &PlayerManager::OnClientConnect_Post), true);
	SH_ADD_HOOK(IServerGameClients, ClientPutInServer, serverClients, SH_MEMBER(this, &PlayerManager::OnClientPutInServer), true);
	SH_ADD_HOOK(IServerGameClients, ClientDisconnect, serverClients, SH_MEMBER(this, &PlayerManager::OnClientDisconnect), false);
	SH_ADD_HOOK(IServerGameClients, ClientDisconnect, serverClients, SH_MEMBER(this, &PlayerManager::OnClientDisconnect_Post), true);
	SH_ADD_HOOK(IServerGameClients, ClientCommand, serverClients, SH_MEMBER(this, &PlayerManager::OnClientCommand), false);
#if SOURCE_ENGINE >= SE_EYE
	SH_ADD_HOOK(IServerGameClients, ClientCommandKeyValues, serverClients, SH_MEMBER(this, &PlayerManager::OnClientCommandKeyValues), false);
	SH_ADD_HOOK(IServerGameClients, ClientCommandKeyValues, serverClients, SH_MEMBER(this, &PlayerManager::OnClientCommandKeyValues_Post), true);
#endif
	SH_ADD_HOOK(IServerGameClients, ClientSettingsChanged, serverClients, SH_MEMBER(this, &PlayerManager::OnClientSettingsChanged), true);
	SH_ADD_HOOK(IServerGameDLL, ServerActivate, gamedll, SH_MEMBER(this, &PlayerManager::OnServerActivate), true);
#if SOURCE_ENGINE >= SE_LEFT4DEAD
	SH_ADD_HOOK(IServerGameDLL, ServerHibernationUpdate, gamedll, SH_MEMBER(this, &PlayerManager::OnServerHibernationUpdate), true);
#elif SOURCE_ENGINE > SE_EYE // 2013/orangebox, but not original orangebox.
	SH_ADD_HOOK(IServerGameDLL, SetServerHibernation, gamedll, SH_MEMBER(this, &PlayerManager::OnServerHibernationUpdate), true);
#endif
	SH_ADD_HOOK(IVEngineServer, ClientPrintf, engine, SH_MEMBER(this, &PlayerManager::OnClientPrintf), false);

	sharesys->AddInterface(NULL, this);

	ParamType p1[] = {Param_Cell, Param_String, Param_Cell};
	ParamType p2[] = {Param_Cell};

	m_clconnect = forwardsys->CreateForward("OnClientConnect", ET_LowEvent, 3, p1);
	m_clconnect_post = forwardsys->CreateForward("OnClientConnected", ET_Ignore, 1, p2);
	m_clputinserver = forwardsys->CreateForward("OnClientPutInServer", ET_Ignore, 1, p2);
	m_cldisconnect = forwardsys->CreateForward("OnClientDisconnect", ET_Ignore, 1, p2);
	m_cldisconnect_post = forwardsys->CreateForward("OnClientDisconnect_Post", ET_Ignore, 1, p2);
	m_clcommand = forwardsys->CreateForward("OnClientCommand", ET_Hook, 2, NULL, Param_Cell, Param_Cell);
	m_clcommandkv = forwardsys->CreateForward("OnClientCommandKeyValues", ET_Hook, 2, NULL, Param_Cell, Param_Cell);
	m_clcommandkv_post = forwardsys->CreateForward("OnClientCommandKeyValues_Post", ET_Ignore, 2, NULL, Param_Cell, Param_Cell);
	m_clinfochanged = forwardsys->CreateForward("OnClientSettingsChanged", ET_Ignore, 1, p2);
	m_clauth = forwardsys->CreateForward("OnClientAuthorized", ET_Ignore, 2, NULL, Param_Cell, Param_String);
	m_cllang = forwardsys->CreateForward("OnClientLanguageChanged", ET_Ignore, 2, NULL, Param_Cell, Param_Cell);
	m_onActivate = forwardsys->CreateForward("OnServerLoad", ET_Ignore, 0, NULL);
	m_onActivate2 = forwardsys->CreateForward("OnMapStart", ET_Ignore, 0, NULL);

	PreAdminCheck = forwardsys->CreateForward("OnClientPreAdminCheck", ET_Event, 1, p1);
	PostAdminCheck = forwardsys->CreateForward("OnClientPostAdminCheck", ET_Ignore, 1, p1);
	PostAdminFilter = forwardsys->CreateForward("OnClientPostAdminFilter", ET_Ignore, 1, p1);

	m_bIsListenServer = !engine->IsDedicatedServer();
	m_ListenClient = 0;

	ConCommand *pCmd = FindCommand("maxplayers");
	if (pCmd != NULL)
	{
		SH_ADD_HOOK(ConCommand, Dispatch, pCmd, SH_STATIC(CmdMaxplayersCallback), true);
		maxplayersCmd = pCmd;
	}
}

void PlayerManager::OnSourceModShutdown()
{
	SH_REMOVE_HOOK(IServerGameClients, ClientConnect, serverClients, SH_MEMBER(this, &PlayerManager::OnClientConnect), false);
	SH_REMOVE_HOOK(IServerGameClients, ClientConnect, serverClients, SH_MEMBER(this, &PlayerManager::OnClientConnect_Post), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientPutInServer, serverClients, SH_MEMBER(this, &PlayerManager::OnClientPutInServer), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientDisconnect, serverClients, SH_MEMBER(this, &PlayerManager::OnClientDisconnect), false);
	SH_REMOVE_HOOK(IServerGameClients, ClientDisconnect, serverClients, SH_MEMBER(this, &PlayerManager::OnClientDisconnect_Post), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientCommand, serverClients, SH_MEMBER(this, &PlayerManager::OnClientCommand), false);
#if SOURCE_ENGINE >= SE_EYE
	SH_REMOVE_HOOK(IServerGameClients, ClientCommandKeyValues, serverClients, SH_MEMBER(this, &PlayerManager::OnClientCommandKeyValues), false);
	SH_REMOVE_HOOK(IServerGameClients, ClientCommandKeyValues, serverClients, SH_MEMBER(this, &PlayerManager::OnClientCommandKeyValues_Post), true);
#endif
	SH_REMOVE_HOOK(IServerGameClients, ClientSettingsChanged, serverClients, SH_MEMBER(this, &PlayerManager::OnClientSettingsChanged), true);
	SH_REMOVE_HOOK(IServerGameDLL, ServerActivate, gamedll, SH_MEMBER(this, &PlayerManager::OnServerActivate), true);
#if SOURCE_ENGINE >= SE_LEFT4DEAD
	SH_REMOVE_HOOK(IServerGameDLL, ServerHibernationUpdate, gamedll, SH_MEMBER(this, &PlayerManager::OnServerHibernationUpdate), true);
#elif SOURCE_ENGINE > SE_EYE // 2013/orangebox, but not original orangebox.
	SH_REMOVE_HOOK(IServerGameDLL, SetServerHibernation, gamedll, SH_MEMBER(this, &PlayerManager::OnServerHibernationUpdate), true);
#endif
	SH_REMOVE_HOOK(IVEngineServer, ClientPrintf, engine, SH_MEMBER(this, &PlayerManager::OnClientPrintf), false);

	/* Release forwards */
	forwardsys->ReleaseForward(m_clconnect);
	forwardsys->ReleaseForward(m_clconnect_post);
	forwardsys->ReleaseForward(m_clputinserver);
	forwardsys->ReleaseForward(m_cldisconnect);
	forwardsys->ReleaseForward(m_cldisconnect_post);
	forwardsys->ReleaseForward(m_clcommand);
	forwardsys->ReleaseForward(m_clcommandkv);
	forwardsys->ReleaseForward(m_clcommandkv_post);
	forwardsys->ReleaseForward(m_clinfochanged);
	forwardsys->ReleaseForward(m_clauth);
	forwardsys->ReleaseForward(m_cllang);
	forwardsys->ReleaseForward(m_onActivate);
	forwardsys->ReleaseForward(m_onActivate2);

	forwardsys->ReleaseForward(PreAdminCheck);
	forwardsys->ReleaseForward(PostAdminCheck);
	forwardsys->ReleaseForward(PostAdminFilter);

	delete [] m_Players;

	if (maxplayersCmd != NULL)
	{
		SH_REMOVE_HOOK(ConCommand, Dispatch, maxplayersCmd, SH_STATIC(CmdMaxplayersCallback), true);
	}
}

ConfigResult PlayerManager::OnSourceModConfigChanged(const char *key, 
												 const char *value, 
												 ConfigSource source, 
												 char *error, 
												 size_t maxlength)
{
	if (strcmp(key, "PassInfoVar") == 0)
	{
		if (strcmp(value, "_password") != 0)
		{
			m_PassInfoVar.assign(value);
		}
		return ConfigResult_Accept;
	} else if (strcmp(key, "AllowClLanguageVar") == 0) {
		if (strcasecmp(value, "on") == 0)
		{
			m_QueryLang = true;
		} else if (strcasecmp(value, "off") == 0) {
			m_QueryLang = false;
		} else {
			ke::SafeStrcpy(error, maxlength, "Invalid value: must be \"on\" or \"off\"");
			return ConfigResult_Reject;
		}
		return ConfigResult_Accept;
	} else if (strcmp( key, "SteamAuthstringValidation" ) == 0) {
		if (strcasecmp(value, "yes") == 0)
		{
			m_bAuthstringValidation = true;
		} else if ( strcasecmp(value, "no") == 0) {
			m_bAuthstringValidation = false;
		} else {
			ke::SafeStrcpy(error, maxlength, "Invalid value: must be \"yes\" or \"no\"");
			return ConfigResult_Reject;
		}
		return ConfigResult_Accept;
	}
	return ConfigResult_Ignore;
}

void PlayerManager::OnServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	static ConVar *tv_enable = icvar->FindVar("tv_enable");
#if SOURCE_ENGINE == SE_TF2
	static ConVar *replay_enable = icvar->FindVar("replay_enable");
#endif

	ICommandLine *commandLine = g_HL2.GetValveCommandLine();
	m_bIsSourceTVActive = (tv_enable && tv_enable->GetBool() && (!commandLine || commandLine->FindParm("-nohltv") == 0));
	m_bIsReplayActive = false;
#if SOURCE_ENGINE == SE_TF2
	m_bIsReplayActive = (replay_enable && replay_enable->GetBool());
#endif
	m_PlayersSinceActive = 0;
	
	g_OnMapStarted = true;
	m_bServerActivated = true;

	extsys->CallOnCoreMapStart(pEdictList, edictCount, m_maxClients);

	m_onActivate->Execute(NULL);
	m_onActivate2->Execute(NULL);

	List<IClientListener *>::iterator iter;
	for (iter = m_hooks.begin(); iter != m_hooks.end(); iter++)
	{
		if ((*iter)->GetClientListenerVersion() >= 5)
		{
			(*iter)->OnServerActivated(m_maxClients);
		}
	}

	SMGlobalClass *cls = SMGlobalClass::head;
	while (cls)
	{
		cls->OnSourceModLevelActivated();
		cls = cls->m_pGlobalClassNext;
	}

	SM_ExecuteAllConfigs();
}

bool PlayerManager::IsServerActivated()
{
	return m_bServerActivated;
}

bool PlayerManager::CheckSetAdmin(int index, CPlayer *pPlayer, AdminId id)
{
	const char *password = adminsys->GetAdminPassword(id);
	if (password != NULL)
	{
		if (m_PassInfoVar.size() < 1)
		{
			return false;
		}

		/* Whoa... the user needs a password! */
		const char *given = engine->GetClientConVarValue(index, m_PassInfoVar.c_str());
		if (!given || strcmp(given, password) != 0)
		{
			return false;
		}
	}

	pPlayer->SetAdminId(id, false);

	return true;
}

bool PlayerManager::CheckSetAdminName(int index, CPlayer *pPlayer, AdminId id)
{
	const char *password = adminsys->GetAdminPassword(id);
	if (password == NULL)
	{
		return false;
	}

	if (m_PassInfoVar.size() < 1)
	{
		return false;
	}

	/* Whoa... the user needs a password! */
	const char *given = engine->GetClientConVarValue(index, m_PassInfoVar.c_str());
	if (!given || strcmp(given, password) != 0)
	{
		return false;
	}

	pPlayer->SetAdminId(id, false);

	return true;
}

void PlayerManager::RunAuthChecks()
{
	CPlayer *pPlayer;
	const char *authstr;
	unsigned int removed = 0;
	for (unsigned int i=1; i<=m_AuthQueue[0]; i++)
	{
		pPlayer = &m_Players[m_AuthQueue[i]];
		pPlayer->UpdateAuthIds();
		
		authstr = pPlayer->m_AuthID.c_str();

		if (!pPlayer->IsAuthStringValidated())
		{
			continue; // we're using steam auth, and steam doesn't know about this player yet so we can't do anything about them for now
		}

		if (authstr && authstr[0] != '\0'
			&& (strcmp(authstr, "STEAM_ID_PENDING") != 0))
		{
			/* Set authorization */
			pPlayer->Authorize();

			/* Mark as removed from queue */
			unsigned int client = m_AuthQueue[i];
			m_AuthQueue[i] = 0;
			removed++;
			
			const char *steamId = pPlayer->GetSteam2Id();

			/* Send to extensions */
			List<IClientListener *>::iterator iter;
			IClientListener *pListener;
			for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
			{
				pListener = (*iter);
				pListener->OnClientAuthorized(client, steamId ? steamId : authstr);
				if (!pPlayer->IsConnected())
				{
					break;
				}
			}

			/* Send to plugins if player is still connected */
			if (pPlayer->IsConnected() && m_clauth->GetFunctionCount())
			{
				/* :TODO: handle the case of a player disconnecting in the middle */
				m_clauth->PushCell(client);
				/* For legacy reasons, people are expecting the Steam2 id here if using Steam auth */
				m_clauth->PushString(steamId ? steamId : authstr);
				m_clauth->Execute(NULL);
			}

			if (pPlayer->IsConnected())
			{
				pPlayer->Authorize_Post();
			}
		}
	}

	/* Clean up the queue */
	if (removed)
	{
		/* We don't have to compact the list if it's empty */
		if (removed != m_AuthQueue[0])
		{
			unsigned int diff = 0;
			for (unsigned int i=1; i<=m_AuthQueue[0]; i++)
			{
				/* If this member is removed... */
				if (m_AuthQueue[i] == 0)
				{
					/* Increase the differential */
					diff++;
				} else {
					/* diff cannot increase faster than i+1 */
					assert(i > diff);
					assert(i - diff >= 1);
					/* move this index down */
					m_AuthQueue[i - diff] = m_AuthQueue[i];
				}
			}
			m_AuthQueue[0] -= removed;
		} else {
			m_AuthQueue[0] = 0;
		}
	}
}

bool PlayerManager::OnClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	int client = IndexOfEdict(pEntity);
	CPlayer *pPlayer = &m_Players[client];
	++m_PlayersSinceActive;

	if (pPlayer->IsConnected())
	{
		if (sm_debug_connect.GetBool())
		{
			const char *pAuth = pPlayer->GetAuthString(false);
			if (pAuth == NULL)
			{
				pAuth = "";
			}

			logger->LogMessage("\"%s<%d><%s><>\" was already connected to the server.", pPlayer->GetName(), pPlayer->GetUserId(), pAuth);
		}

		OnClientDisconnect(pPlayer->GetEdict());
		OnClientDisconnect_Post(pPlayer->GetEdict());
	}

	pPlayer->Initialize(pszName, pszAddress, pEntity);
	
	/* Get the client's language */
	if (m_QueryLang)
	{
#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
		pPlayer->m_LangId = translator->GetServerLanguage();
#else
		const char *name;
		if (!pPlayer->IsFakeClient() && (name=engine->GetClientConVarValue(client, "cl_language")))
		{
			unsigned int langid;
			pPlayer->m_LangId = (translator->GetLanguageByName(name, &langid)) ? langid : translator->GetServerLanguage();

			OnClientLanguageChanged(client, pPlayer->m_LangId);
		} else {
			pPlayer->m_LangId = translator->GetServerLanguage();
		}
#endif
	}
	
	List<IClientListener *>::iterator iter;
	IClientListener *pListener = NULL;
	for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
	{
		pListener = (*iter);
		if (!pListener->InterceptClientConnect(client, reject, maxrejectlen))
		{
			RETURN_META_VALUE(MRES_SUPERCEDE, false);
		}
	}

	cell_t res = 1;

	m_clconnect->PushCell(client);
	m_clconnect->PushStringEx(reject, maxrejectlen, SM_PARAM_STRING_UTF8 | SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
	m_clconnect->PushCell(maxrejectlen);
	m_clconnect->Execute(&res);

	if (res)
	{
		if (!pPlayer->IsAuthorized() && !pPlayer->IsFakeClient())
		{
			m_AuthQueue[++m_AuthQueue[0]] = client;
		}

		m_UserIdLookUp[engine->GetPlayerUserId(pEntity)] = client;
	}
	else
	{
		if (!pPlayer->IsFakeClient())
		{
			RETURN_META_VALUE(MRES_SUPERCEDE, false);
		}
	}

	return true;
}

bool PlayerManager::OnClientConnect_Post(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	int client = IndexOfEdict(pEntity);
	bool orig_value = META_RESULT_ORIG_RET(bool);
	CPlayer *pPlayer = &m_Players[client];

	if (orig_value)
	{
		List<IClientListener *>::iterator iter;
		IClientListener *pListener = NULL;
		for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
		{
			pListener = (*iter);
			pListener->OnClientConnected(client);
			if (!pPlayer->IsConnected())
			{
				return true;
			}
		}

		if (!pPlayer->IsFakeClient() 
			&& m_bIsListenServer
			&& strncmp(pszAddress, "127.0.0.1", 9) == 0)
		{
			m_ListenClient = client;
		}

		cell_t res;
		m_clconnect_post->PushCell(client);
		m_clconnect_post->Execute(&res, NULL);
	}
	else
	{
		InvalidatePlayer(pPlayer);
	}

	return true;
}

void PlayerManager::OnClientPutInServer(edict_t *pEntity, const char *playername)
{
	cell_t res;
	int client = IndexOfEdict(pEntity);
	CPlayer *pPlayer = &m_Players[client];

	/* If they're not connected, they're a bot */
	if (!pPlayer->IsConnected())
	{
		/* Run manual connection routines */
		char error[255];

		pPlayer->m_bFakeClient = true;

		/*
		 * While we're already filtered to just bots, we'll do other checks to
		 * make sure that the requisite services are enabled and that the bots
		 * have joined at the expected time.
		 *
		 * Checking playerinfo's IsHLTV and IsReplay would be better and less
		 * error-prone but will always show false until later in the frame,
		 * after PutInServer and Activate, and we want it now!
		 *
		 * These checks are hairy as hell due to differences between engines and games.
		 * 
		 * Most engines use "Replay" and "SourceTV" as bot names for these when they're
		 * created. EP2V, CSS and Nuclear Dawn (but not L4D2) differ from this by now using
		 * replay_/tv_name directly when creating the bot(s). To complicate it slightly
		 * further, the cvar can be empty and the engine's fallback to "unnamed" will be used.
		 * We can maybe just rip out the name checks at some point and rely solely on whether
		 * they're enabled and the join order.
		 */
		
		// This doesn't actually get incremented until OnClientConnect. Fake it to check.
		int newCount = m_PlayersSinceActive + 1;

		int userId = engine->GetPlayerUserId(pEntity);
#if (SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_SDK2013 \
	|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_NUCLEARDAWN  || SOURCE_ENGINE == SE_LEFT4DEAD2)
		static ConVar *tv_name = icvar->FindVar("tv_name");
#endif
#if SOURCE_ENGINE == SE_TF2
		static ConVar *replay_name = icvar->FindVar("replay_name");
#endif

#if SOURCE_ENGINE == SE_TF2
		if (m_bIsReplayActive && newCount == 1
			&& (m_ReplayUserId == userId
				|| (replay_name && strcmp(playername, replay_name->GetString()) == 0) || (replay_name && replay_name->GetString()[0] == 0 && strcmp(playername, "unnamed") == 0)
				)
			)
		{
			pPlayer->m_bIsReplay = true;
			m_ReplayUserId = userId;
		}
#endif

		if (m_bIsSourceTVActive
			&& ((!m_bIsReplayActive && newCount == 1)
				|| (m_bIsReplayActive && newCount == 2))
			&& (m_SourceTVUserId == userId
#if SOURCE_ENGINE == SE_CSGO
				|| strcmp(playername, "GOTV") == 0
#elif SOURCE_ENGINE == SE_BLADE
				|| strcmp(playername, "BBTV") == 0
#elif (SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_SDK2013 \
	|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_NUCLEARDAWN  || SOURCE_ENGINE == SE_LEFT4DEAD2)
				|| (tv_name && strcmp(playername, tv_name->GetString()) == 0) || (tv_name && tv_name->GetString()[0] == 0 && strcmp(playername, "unnamed") == 0)
#else
				|| strcmp(playername, "SourceTV") == 0
#endif
				)
			)
		{
			pPlayer->m_bIsSourceTV = true;
			m_SourceTVUserId = userId;
		}

		if (!OnClientConnect(pEntity, playername, "127.0.0.1", error, sizeof(error)))
		{
			/* :TODO: kick the bot if it's rejected */
			return;
		}
		List<IClientListener *>::iterator iter;
		IClientListener *pListener = NULL;
		for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
		{
			pListener = (*iter);
			pListener->OnClientConnected(client);
			/* See if bot was kicked */
			if (!pPlayer->IsConnected())
			{
				return;
			}
		}

		cell_t res;
		m_clconnect_post->PushCell(client);
		m_clconnect_post->Execute(&res, NULL);

		pPlayer->Authorize();
		
		const char *steamId = pPlayer->GetSteam2Id();

		/* Now do authorization */
		for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
		{
			pListener = (*iter);
			pListener->OnClientAuthorized(client, steamId ? steamId : pPlayer->m_AuthID.c_str());
		}
		/* Finally, tell plugins */
		if (m_clauth->GetFunctionCount())
		{
			m_clauth->PushCell(client);
			/* For legacy reasons, people are expecting the Steam2 id here if using Steam auth */
			m_clauth->PushString(steamId ? steamId : pPlayer->m_AuthID.c_str());
			m_clauth->Execute(NULL);
		}
		pPlayer->Authorize_Post();
	}
#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	else if(m_QueryLang)
	{
		// Not a bot
		pPlayer->m_LanguageCookie = g_ConVarManager.QueryClientConVar(pEntity, "cl_language", NULL, 0);
	}
#endif

	if (playerinfo)
	{
		pPlayer->m_Info = playerinfo->GetPlayerInfo(pEntity);
	}

	pPlayer->Connect();
	m_PlayerCount++;

	List<IClientListener *>::iterator iter;
	IClientListener *pListener = NULL;
	for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnClientPutInServer(client);
	}

	m_clputinserver->PushCell(client);
	m_clputinserver->Execute(&res, NULL);

	if (pPlayer->IsAuthorized())
	{
		pPlayer->DoPostConnectAuthorization();
	}
}

void PlayerManager::OnSourceModLevelEnd()
{
	/* Disconnect all bots still in game */
	for (int i=1; i<=m_maxClients; i++)
	{
		if (m_Players[i].IsConnected())
		{
			OnClientDisconnect(m_Players[i].GetEdict());
			OnClientDisconnect_Post(m_Players[i].GetEdict());
		}
	}
	m_PlayerCount = 0;
}

void PlayerManager::OnServerHibernationUpdate(bool bHibernating)
{
	/* If bots were added at map start, but not fully inited before hibernation, there will
	 * be no OnClientDisconnect for them, despite them getting booted right before this.
	 */

	if (bHibernating)
	{
		for (int i = 1; i <= m_maxClients; i++)
		{
			CPlayer *pPlayer = &m_Players[i];
			if (pPlayer->IsConnected() && pPlayer->IsFakeClient())
			{
#if SOURCE_ENGINE < SE_LEFT4DEAD || SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE || SOURCE_ENGINE == SE_NUCLEARDAWN
				// These games have the bug fixed where hltv/replay was getting kicked on hibernation
				if (pPlayer->IsSourceTV() || pPlayer->IsReplay())
					continue;
#endif
				OnClientDisconnect(m_Players[i].GetEdict());
				OnClientDisconnect_Post(m_Players[i].GetEdict());
			}
		}
	}
}

void PlayerManager::OnClientDisconnect(edict_t *pEntity)
{
	cell_t res;
	int client = IndexOfEdict(pEntity);
	CPlayer *pPlayer = &m_Players[client];

	if (pPlayer->IsConnected())
	{
		m_cldisconnect->PushCell(client);
		m_cldisconnect->Execute(&res, NULL);
	}
	else
	{
		/* We don't care, prevent a double call */
		return;
	}

	if (pPlayer->WasCountedAsInGame())
	{
		m_PlayerCount--;
	}

	List<IClientListener *>::iterator iter;
	IClientListener *pListener = NULL;
	for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnClientDisconnecting(client);
	}
}

void PlayerManager::OnClientDisconnect_Post(edict_t *pEntity)
{
	int client = IndexOfEdict(pEntity);
	CPlayer *pPlayer = &m_Players[client];
	if (!pPlayer->IsConnected())
	{
		/* We don't care, prevent a double call */
		return;
	}

	InvalidatePlayer(pPlayer);

	if (m_ListenClient == client)
	{
		m_ListenClient = 0;
	}

	cell_t res;

	m_cldisconnect_post->PushCell(client);
	m_cldisconnect_post->Execute(&res, NULL);

	List<IClientListener *>::iterator iter;
	IClientListener *pListener = NULL;
	for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnClientDisconnected(client);
	}
}

void PlayerManager::OnClientPrintf(edict_t *pEdict, const char *szMsg)
{
	int client = IndexOfEdict(pEdict);

	CPlayer &player = m_Players[client];
	if (!player.IsConnected())
		RETURN_META(MRES_IGNORED);

	INetChannel *pNetChan = static_cast<INetChannel *>(engine->GetPlayerNetInfo(client));
	if (pNetChan == NULL)
		RETURN_META(MRES_IGNORED);

	size_t nMsgLen = strlen(szMsg);
#if SOURCE_ENGINE == SE_EPISODEONE || SOURCE_ENGINE == SE_DARKMESSIAH
	static const int nNumBitsWritten = 0;
#else
	int nNumBitsWritten = pNetChan->GetNumBitsWritten(false); // SVC_Print uses unreliable netchan
#endif

	// if the msg is bigger than allowed then just let it fail
	if (nMsgLen + 1 >= SVC_Print_BufferSize) // +1 for NETMSG_TYPE_BITS
		RETURN_META(MRES_IGNORED);

	// enqueue msgs if we'd overflow the SVC_Print buffer (+7 as ceil)
	if (!player.m_PrintfBuffer.empty() || (nNumBitsWritten + NETMSG_TYPE_BITS + 7) / 8 + nMsgLen >= SVC_Print_BufferSize)
	{
		// Don't send any more messages for this player until the buffer is empty.
		// Queue up a gameframe hook to empty the buffer (if we haven't already)
		if (player.m_PrintfBuffer.empty())
			g_SourceMod.AddFrameAction(PrintfBuffer_FrameAction, (void *)(uintptr_t)player.GetSerial());

		player.m_PrintfBuffer.push_back(szMsg);

		RETURN_META(MRES_SUPERCEDE);
	}

	RETURN_META(MRES_IGNORED);
}

void PlayerManager::OnPrintfFrameAction(unsigned int serial)
{
	int client = GetClientFromSerial(serial);
	CPlayer &player = m_Players[client];
	if (!player.IsConnected())
	{
		player.ClearNetchannelQueue();
		return;
	}

	INetChannel *pNetChan = static_cast<INetChannel *>(engine->GetPlayerNetInfo(client));
	if (pNetChan == NULL)
	{
		player.ClearNetchannelQueue();
		return;
	}

	while (!player.m_PrintfBuffer.empty())
	{
#if SOURCE_ENGINE == SE_EPISODEONE || SOURCE_ENGINE == SE_DARKMESSIAH
		static const int nNumBitsWritten = 0;
#else
		int nNumBitsWritten = pNetChan->GetNumBitsWritten(false); // SVC_Print uses unreliable netchan
#endif

		std::string &string = player.m_PrintfBuffer.front();

		// stop if we'd overflow the SVC_Print buffer  (+7 as ceil)
		if ((nNumBitsWritten + NETMSG_TYPE_BITS + 7) / 8 + string.length() >= SVC_Print_BufferSize)
			break;

		SH_CALL(engine, &IVEngineServer::ClientPrintf)(player.m_pEdict, string.c_str());

		player.m_PrintfBuffer.pop_front();
	}

	if (!player.m_PrintfBuffer.empty())
	{
		// continue processing it on the next gameframe as buffer is not empty
		g_SourceMod.AddFrameAction(PrintfBuffer_FrameAction, (void *)(uintptr_t)player.GetSerial());
	}
}

void ClientConsolePrint(edict_t *e, const char *fmt, ...)
{
	char buffer[512];

	va_list ap;
	va_start(ap, fmt);
	size_t len = vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	if (len >= sizeof(buffer) - 1)
	{
		buffer[sizeof(buffer) - 2] = '\n';
		buffer[sizeof(buffer) - 1] = '\0';
	} else {
		buffer[len++] = '\n';
		buffer[len] = '\0';
	}

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(IndexOfEdict(e));
	if (!pPlayer)
	{
		return;
	}
	
	pPlayer->PrintToConsole(buffer);
}

void ListExtensionsToClient(CPlayer *player, const CCommand &args)
{
	char buffer[256];
	unsigned int id = 0;
	unsigned int start = 0;

	AutoExtensionList extensions(extsys);
	if (!extensions->size())
	{
		ClientConsolePrint(player->GetEdict(), "[SM] No extensions found.");
		return;
	}

	if (args.ArgC() > 2)
	{
		start = atoi(args.Arg(2));
	}

	size_t i = 0;
	for (; i < extensions->size(); i++)
	{
		IExtension *ext = extensions->at(i);

		char error[255];
		if (!ext->IsRunning(error, sizeof(error)))
		{
			continue;
		}

		id++;
		if (id < start)
		{
			continue;
		}

		if (id - start > 10)
		{
			break;
		}

		IExtensionInterface *api = ext->GetAPI();

		const char *name = api->GetExtensionName();
		const char *version = api->GetExtensionVerString();
		const char *author = api->GetExtensionAuthor();
		const char *description = api->GetExtensionDescription();

		size_t len = ke::SafeSprintf(buffer, sizeof(buffer), " \"%s\"", name);

		if (version != NULL && version[0])
		{
			len += ke::SafeSprintf(&buffer[len], sizeof(buffer)-len, " (%s)", version);
		}

		if (author != NULL && author[0])
		{
			len += ke::SafeSprintf(&buffer[len], sizeof(buffer)-len, " by %s", author);
		}

		if (description != NULL && description[0])
		{
			len += ke::SafeSprintf(&buffer[len], sizeof(buffer)-len, ": %s", description);
		}


		ClientConsolePrint(player->GetEdict(), "%s", buffer);
	}

	for (; i < extensions->size(); i++)
	{
		char error[255];
		if (extensions->at(i)->IsRunning(error, sizeof(error)))
		{
			break;
		}
	}

	if (i < extensions->size())
	{
		ClientConsolePrint(player->GetEdict(), "To see more, type \"sm exts %d\"", id);
	}
}

void ListPluginsToClient(CPlayer *player, const CCommand &args)
{
	char buffer[256];
	unsigned int id = 0;
	edict_t *e = player->GetEdict();
	unsigned int start = 0;

	AutoPluginList plugins(scripts);
	if (!plugins->size())
	{
		ClientConsolePrint(e, "[SM] No plugins found.");
		return;
	}

	if (args.ArgC() > 2)
	{
		start = atoi(args.Arg(2));
	}

	SourceHook::List<SMPlugin *> m_FailList;

	size_t i = 0;
	for (; i < plugins->size(); i++)
	{
		SMPlugin *pl = plugins->at(i);

		if (pl->GetStatus() != Plugin_Running)
		{
			continue;
		}

		/* Count valid plugins */
		id++;
		if (id < start)
		{
			continue;
		}

		if (id - start > 10)
		{
			break;
		}

		size_t len;
		const sm_plugininfo_t *info = pl->GetPublicInfo();
		len = ke::SafeSprintf(buffer, sizeof(buffer), " \"%s\"", (IS_STR_FILLED(info->name)) ? info->name : pl->GetFilename());
		if (IS_STR_FILLED(info->version))
		{
			len += ke::SafeSprintf(&buffer[len], sizeof(buffer)-len, " (%s)", info->version);
		}
		if (IS_STR_FILLED(info->author))
		{
			ke::SafeSprintf(&buffer[len], sizeof(buffer)-len, " by %s", info->author);
		}
		else
		{
			ke::SafeSprintf(&buffer[len], sizeof(buffer)-len, " %s", pl->GetFilename());
		}
		ClientConsolePrint(e, "%s", buffer);
	}

	/* See if we can get more plugins */
	for (; i < plugins->size(); i++)
	{
		if (plugins->at(i)->GetStatus() == Plugin_Running)
		{
			break;
		}
	}

	/* Do we actually have more plugins? */
	if (i < plugins->size())
	{
		ClientConsolePrint(e, "To see more, type \"sm plugins %d\"", id);
	}
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
void PlayerManager::OnClientCommand(edict_t *pEntity, const CCommand &args)
{
#else
void PlayerManager::OnClientCommand(edict_t *pEntity)
{
	CCommand args;
#endif
	
	cell_t res = Pl_Continue;
	int client = IndexOfEdict(pEntity);
	CPlayer *pPlayer = &m_Players[client];

	if (!pPlayer->IsConnected())
	{
		return;
	}

	if (strcmp(args.Arg(0), "sm") == 0)
	{
		if (args.ArgC() > 1 && strcmp(args.Arg(1), "plugins") == 0)
		{
			ListPluginsToClient(pPlayer, args);
			RETURN_META(MRES_SUPERCEDE);
		}
		else if (args.ArgC() > 1 && strcmp(args.Arg(1), "exts") == 0)
		{
			ListExtensionsToClient(pPlayer, args);
			RETURN_META(MRES_SUPERCEDE);
		}
		else if (args.ArgC() > 1 && strcmp(args.Arg(1), "credits") == 0)
		{
			ClientConsolePrint(pEntity,
				"SourceMod would not be possible without:");
			ClientConsolePrint(pEntity,
				" David \"BAILOPAN\" Anderson, Matt \"pRED\" Woodrow");
			ClientConsolePrint(pEntity,
				" Scott \"DS\" Ehlert, Fyren");
			ClientConsolePrint(pEntity,
				" Nicholas \"psychonic\" Hastings, Asher \"asherkin\" Baker");
			ClientConsolePrint(pEntity,
				" Ruben \"Dr!fter\" Gonzalez, Josh \"KyleS\" Allard");
			ClientConsolePrint(pEntity,
				" Michael \"Headline\" Flaherty, Jannik \"Peace-Maker\" Hartung");
			ClientConsolePrint(pEntity,
				" Borja \"faluco\" Ferrer, Pavol \"PM OnoTo\" Marko");
			ClientConsolePrint(pEntity,
				"SourceMod is open source under the GNU General Public License.");
			RETURN_META(MRES_SUPERCEDE);
		}

		ClientConsolePrint(pEntity,
			"SourceMod %s, by AlliedModders LLC", SOURCEMOD_VERSION);
		ClientConsolePrint(pEntity,
			"To see running plugins, type \"sm plugins\"");
		ClientConsolePrint(pEntity,
			"To see credits, type \"sm credits\"");
		ClientConsolePrint(pEntity,
			"Visit https://www.sourcemod.net/");
		RETURN_META(MRES_SUPERCEDE);
	}

	EngineArgs cargs(args);
	AutoEnterCommand autoEnterCommand(&cargs);

	int argcount = args.ArgC() - 1;
	const char *cmd = g_HL2.CurrentCommandName();

	bool result = g_ValveMenuStyle.OnClientCommand(client, cmd, args);
	if (result)
	{
		res = Pl_Handled;
	} else {
		result = g_RadioMenuStyle.OnClientCommand(client, cmd, args);
		if (result)
		{
			res = Pl_Handled;
		}
	}

	if (g_ConsoleDetours.IsEnabled())
	{
		cell_t res2 = g_ConsoleDetours.InternalDispatch(client, &cargs);
		if (res2 >= Pl_Stop)
		{
			RETURN_META(MRES_SUPERCEDE);
		}
		else if (res2 > res)
		{
			res = res2;
		}
	}

	cell_t res2 = Pl_Continue;
	if (pPlayer->IsInGame())
	{
		m_clcommand->PushCell(client);
		m_clcommand->PushCell(argcount);
		m_clcommand->Execute(&res2, NULL);
	}

	if (res2 > res)
	{
		res = res2;
	}

	if (res >= Pl_Stop)
	{
		RETURN_META(MRES_SUPERCEDE);
	}

	res = g_ConCmds.DispatchClientCommand(client, cmd, argcount, (ResultType)res);

	if (res >= Pl_Handled)
	{
		RETURN_META(MRES_SUPERCEDE);
	}
}

#if SOURCE_ENGINE >= SE_EYE
static bool s_LastCCKVAllowed = true;

void PlayerManager::OnClientCommandKeyValues(edict_t *pEntity, KeyValues *pCommand)
{
	int client = IndexOfEdict(pEntity);

	cell_t res = Pl_Continue;
	CPlayer *pPlayer = &m_Players[client];

	if (!pPlayer->IsInGame())
	{
		RETURN_META(MRES_IGNORED);
	}

	KeyValueStack *pStk = new KeyValueStack;
	pStk->pBase = pCommand;
	pStk->pCurRoot.push(pStk->pBase);
	pStk->m_bDeleteOnDestroy = false;

	Handle_t hndl = handlesys->CreateHandle(g_KeyValueType, pStk, g_pCoreIdent, g_pCoreIdent, NULL);

	m_bInCCKVHook = true;
	m_clcommandkv->PushCell(client);
	m_clcommandkv->PushCell(hndl);
	m_clcommandkv->Execute(&res);
	m_bInCCKVHook = false;

	HandleSecurity sec(g_pCoreIdent, g_pCoreIdent);

	// Deletes pStk
	handlesys->FreeHandle(hndl, &sec);

	if (res >= Pl_Handled)
	{
		s_LastCCKVAllowed = false;
		RETURN_META(MRES_SUPERCEDE);
	}

	s_LastCCKVAllowed = true;

	RETURN_META(MRES_IGNORED);
}

void PlayerManager::OnClientCommandKeyValues_Post(edict_t *pEntity, KeyValues *pCommand)
{
	if (!s_LastCCKVAllowed)
	{
		return;
	}

	int client = IndexOfEdict(pEntity);

	CPlayer *pPlayer = &m_Players[client];

	if (!pPlayer->IsInGame())
	{
		return;
	}

	KeyValueStack *pStk = new KeyValueStack;
	pStk->pBase = pCommand;
	pStk->pCurRoot.push(pStk->pBase);
	pStk->m_bDeleteOnDestroy = false;

	Handle_t hndl = handlesys->CreateHandle(g_KeyValueType, pStk, g_pCoreIdent, g_pCoreIdent, NULL);

	m_bInCCKVHook = true;
	m_clcommandkv_post->PushCell(client);
	m_clcommandkv_post->PushCell(hndl);
	m_clcommandkv_post->Execute();
	m_bInCCKVHook = false;

	HandleSecurity sec(g_pCoreIdent, g_pCoreIdent);

	// Deletes pStk
	handlesys->FreeHandle(hndl, &sec);
}
#endif

void PlayerManager::OnClientSettingsChanged(edict_t *pEntity)
{
	cell_t res;
	int client = IndexOfEdict(pEntity);
	CPlayer *pPlayer = &m_Players[client];

	if (!pPlayer->IsConnected())
	{
		return;
	}

	m_clinfochanged->PushCell(client);
	m_clinfochanged->Execute(&res, NULL);

	IPlayerInfo *info = pPlayer->GetPlayerInfo();
	const char *new_name = info ? info->GetName() : engine->GetClientConVarValue(client, "name");
	const char *old_name = pPlayer->m_Name.c_str();

	if (strcmp(old_name, new_name) != 0)
	{
		if (!pPlayer->IsFakeClient())
		{
			AdminId id = adminsys->FindAdminByIdentity("name", new_name);
			if (id != INVALID_ADMIN_ID && pPlayer->GetAdminId() != id)
			{
				if (!CheckSetAdminName(client, pPlayer, id))
				{
					char kickMsg[128];
					logicore.CoreTranslate(kickMsg, sizeof(kickMsg), "%T", 2, NULL, "Name Reserved", &client);
					pPlayer->Kick(kickMsg);
					RETURN_META(MRES_IGNORED);
				}
			}
			else if ((id = adminsys->FindAdminByIdentity("name", old_name)) != INVALID_ADMIN_ID) {
				if (id == pPlayer->GetAdminId())
				{
					/* This player is changing their name; force them to drop admin privileges! */
					pPlayer->SetAdminId(INVALID_ADMIN_ID, false);
				}
			}
		}

		pPlayer->SetName(new_name);
	}
	
	if (!pPlayer->IsFakeClient())
	{
		if (m_PassInfoVar.size() > 0)
		{
			/* Try for a password change */
			const char* old_pass = pPlayer->m_LastPassword.c_str();
			const char* new_pass = engine->GetClientConVarValue(client, m_PassInfoVar.c_str());
			if (strcmp(old_pass, new_pass) != 0)
			{
				pPlayer->m_LastPassword.assign(new_pass);
				if (pPlayer->IsInGame() && pPlayer->IsAuthorized())
				{
					/* If there is already an admin id assigned, this will just bail out. */
					pPlayer->DoBasicAdminChecks();
				}
			}
		}

#if SOURCE_ENGINE >= SE_LEFT4DEAD
		const char* networkid_force;
		if ((networkid_force = engine->GetClientConVarValue(client, "networkid_force")) && networkid_force[0] != '\0')
		{
			unsigned int accountId = pPlayer->GetSteamAccountID();
			logger->LogMessage("\"%s<%d><STEAM_1:%d:%d><>\" has bad networkid (id \"%s\") (ip \"%s\")",
				new_name, pPlayer->GetUserId(), accountId & 1, accountId >> 1, networkid_force, pPlayer->GetIPAddress());

			pPlayer->Kick("NetworkID spoofing detected.");
			RETURN_META(MRES_IGNORED);
		}
#endif
	}

	/* Notify Extensions */
	List<IClientListener *>::iterator iter;
	IClientListener *pListener = NULL;
	for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
	{
		pListener = (*iter);
		if (pListener->GetClientListenerVersion() >= 13)
		{
			pListener->OnClientSettingsChanged(client);
		}
	}
}

void PlayerManager::OnClientLanguageChanged(int client, unsigned int language)
{
	m_cllang->PushCell(client);
	m_cllang->PushCell(language);
	m_cllang->Execute(NULL);
}

int PlayerManager::GetMaxClients()
{
	return m_maxClients;
}

CPlayer *PlayerManager::GetPlayerByIndex(int client) const
{
	if (client > m_maxClients || client < 1)
	{
		return NULL;
	}
	return &m_Players[client];
}

int PlayerManager::GetNumPlayers()
{
	return m_PlayerCount;
}

int PlayerManager::GetClientOfUserId(int userid)
{
	if (userid < 0 || userid > USHRT_MAX)
	{
		return 0;
	}
	
	int client = m_UserIdLookUp[userid];

	/* Verify the userid.  The cache can get messed up with older
	 * Valve engines.  :TODO: If this gets fixed, do an old engine 
	 * check before invoking this backwards compat code.
	 */
	if (client)
	{
		CPlayer *player = GetPlayerByIndex(client);
		if (player && player->IsConnected())
		{
			int realUserId = engine->GetPlayerUserId(player->GetEdict());
			if (realUserId == userid)
			{
				return client;
			}
		}
	}

	/* If we can't verify the userid, we have to do a manual loop */
	CPlayer *player;
	for (int i = 1; i <= m_maxClients; i++)
	{
		player = GetPlayerByIndex(i);
		if (!player || !player->IsConnected())
		{
			continue;
		}
		if (engine->GetPlayerUserId(player->GetEdict()) == userid)
		{
			m_UserIdLookUp[userid] = i;
			return i;
		}
	}

	return 0;
}

void PlayerManager::AddClientListener(IClientListener *listener)
{
	m_hooks.push_back(listener);
}

void PlayerManager::RemoveClientListener(IClientListener *listener)
{
	m_hooks.remove(listener);
}

IGamePlayer *PlayerManager::GetGamePlayer(edict_t *pEdict)
{
	int index = IndexOfEdict(pEdict);
	return GetGamePlayer(index);
}

IGamePlayer *PlayerManager::GetGamePlayer(int client)
{
	return GetPlayerByIndex(client);
}

void PlayerManager::ClearAdminId(AdminId id)
{
	for (int i=1; i<=m_maxClients; i++)
	{
		if (m_Players[i].m_Admin == id)
		{
			m_Players[i].DumpAdmin(true);
		}
	}
}

const char *PlayerManager::GetPassInfoVar()
{
	return m_PassInfoVar.c_str();
}

void PlayerManager::RecheckAnyAdmins()
{
	for (int i=1; i<=m_maxClients; i++)
	{
		if (m_Players[i].IsInGame() && m_Players[i].IsAuthorized())
		{
			m_Players[i].DoBasicAdminChecks();
		}
	}
}

unsigned int PlayerManager::GetReplyTo()
{
	return g_ChatTriggers.GetReplyTo();
}

unsigned int PlayerManager::SetReplyTo(unsigned int reply)
{
	return g_ChatTriggers.SetReplyTo(reply);
}

int PlayerManager::FilterCommandTarget(IGamePlayer *pAdmin, IGamePlayer *pTarget, int flags)
{
	return InternalFilterCommandTarget((CPlayer *)pAdmin, (CPlayer *)pTarget, flags);
}

void PlayerManager::RegisterCommandTargetProcessor(ICommandTargetProcessor *pHandler)
{
	target_processors.push_back(pHandler);
}

void PlayerManager::UnregisterCommandTargetProcessor(ICommandTargetProcessor *pHandler)
{
	target_processors.remove(pHandler);
}

void PlayerManager::InvalidatePlayer(CPlayer *pPlayer)
{
	/**
	* Remove client from auth queue if necessary
	*/
	if (!pPlayer->IsAuthorized())
	{
		for (unsigned int i=1; i<=m_AuthQueue[0]; i++)
		{
			if (m_AuthQueue[i] == (unsigned)pPlayer->m_iIndex)
			{
				/* Move everything ahead of us back by one */
				for (unsigned int j=i+1; j<=m_AuthQueue[0]; j++)
				{
					m_AuthQueue[j-1] = m_AuthQueue[j];
				}
				/* Remove us and break */
				m_AuthQueue[0]--;
				break;
			}
		}
	}
	
	auto userid = engine->GetPlayerUserId(pPlayer->m_pEdict);
	if (userid != -1)
		m_UserIdLookUp[userid] = 0;

	pPlayer->Disconnect();
}

int PlayerManager::InternalFilterCommandTarget(CPlayer *pAdmin, CPlayer *pTarget, int flags)
{
	if ((flags & COMMAND_FILTER_CONNECTED) == COMMAND_FILTER_CONNECTED
		&& !pTarget->IsConnected())
	{
		return COMMAND_TARGET_NONE;
	}
	else if ((flags & COMMAND_FILTER_CONNECTED) != COMMAND_FILTER_CONNECTED && 
		!pTarget->IsInGame())
	{
		return COMMAND_TARGET_NOT_IN_GAME;
	}

	if ((flags & COMMAND_FILTER_NO_BOTS) == COMMAND_FILTER_NO_BOTS
		&& pTarget->IsFakeClient())
	{
		return COMMAND_TARGET_NOT_HUMAN;
	}

	if (pAdmin != NULL)
	{
		if ((flags & COMMAND_FILTER_NO_IMMUNITY) != COMMAND_FILTER_NO_IMMUNITY
			&& !adminsys->CanAdminTarget(pAdmin->GetAdminId(), pTarget->GetAdminId()))
		{
			return COMMAND_TARGET_IMMUNE;
		}
	}

	if ((flags & COMMAND_FILTER_ALIVE) == COMMAND_FILTER_ALIVE 
		&& pTarget->GetLifeState() != PLAYER_LIFE_ALIVE)
	{
		return COMMAND_TARGET_NOT_ALIVE;
	}

	if ((flags & COMMAND_FILTER_DEAD) == COMMAND_FILTER_DEAD
		&& pTarget->GetLifeState() != PLAYER_LIFE_DEAD)
	{
		return COMMAND_TARGET_NOT_DEAD;
	}

	return COMMAND_TARGET_VALID;
}

void PlayerManager::ProcessCommandTarget(cmd_target_info_t *info)
{
	CPlayer *pTarget, *pAdmin;
	int max_clients, total = 0;

	max_clients = GetMaxClients();

	if (info->max_targets < 1)
	{
		info->reason = COMMAND_TARGET_NONE;
		info->num_targets = 0;
	}

	if (info->admin == 0)
	{
		pAdmin = NULL;
	}
	else
	{
		pAdmin = GetPlayerByIndex(info->admin);
	}

	if (info->pattern[0] == '#')
	{
		int userid = atoi(&info->pattern[1]);
		int client = GetClientOfUserId(userid);

		/* See if a valid userid matched */
		if (client > 0)
		{
			IGamePlayer *pTarget = GetPlayerByIndex(client);
			if (pTarget != NULL)
			{
				if ((info->reason = FilterCommandTarget(pAdmin, pTarget, info->flags)) == COMMAND_TARGET_VALID)
				{
					info->targets[0] = client;
					info->num_targets = 1;
					ke::SafeStrcpy(info->target_name, info->target_name_maxlength, pTarget->GetName());
					info->target_name_style = COMMAND_TARGETNAME_RAW;
				}
				else
				{
					info->num_targets = 0;
				}
				return;
			}
		}

		/* Do we need to look for a steam id? */
		int steamIdType = 0;
		if (strncmp(&info->pattern[1], "STEAM_", 6) == 0)
		{
			steamIdType = 2;
		}
		else if (strncmp(&info->pattern[1], "[U:", 3) == 0)
		{
			steamIdType = 3;
		}
		
		if (steamIdType > 0)
		{
			char new_pattern[256];
			if (steamIdType == 2)
			{
				size_t p, len;

				strcpy(new_pattern, "STEAM_");
				len = strlen(&info->pattern[7]);
				for (p = 0; p < len; p++)
				{
					new_pattern[6 + p] = info->pattern[7 + p];
					if (new_pattern[6 + p] == '_')
					{
						new_pattern[6 + p] = ':';
					}
				}
				new_pattern[6 + p] = '\0';
			}
			else
			{
				size_t p = 0;
				char c;
				while ((c = info->pattern[p + 1]) != '\0')
				{
					new_pattern[p] = (c == '_') ? ':' : c;					
					++p;
				}
				
				new_pattern[p] = '\0';
			}

			for (int i = 1; i <= max_clients; i++)
			{
				if ((pTarget = GetPlayerByIndex(i)) == NULL)
				{
					continue;
				}
				if (!pTarget->IsConnected())
				{
					continue;
				}
				
				// We want to make it easy for people to be kicked/banned, so don't require validation for command targets.
				const char *steamId = steamIdType == 2 ? pTarget->GetSteam2Id(false) : pTarget->GetSteam3Id(false);
				if (steamId && strcmp(steamId, new_pattern) == 0)
				{
					if ((info->reason = FilterCommandTarget(pAdmin, pTarget, info->flags))
						== COMMAND_TARGET_VALID)
					{
						info->targets[0] = i;
						info->num_targets = 1;
						ke::SafeStrcpy(info->target_name, info->target_name_maxlength, pTarget->GetName());
						info->target_name_style = COMMAND_TARGETNAME_RAW;
					}
					else
					{
						info->num_targets = 0;
					}
					return;
				}
			}
		}

		/* See if an exact name matches */
		for (int i = 1; i <= max_clients; i++)
		{
			if ((pTarget = GetPlayerByIndex(i)) == NULL)
			{
				continue;
			}
			if  (!pTarget->IsConnected())
			{
				continue;
			}
			if (strcmp(pTarget->GetName(), &info->pattern[1]) == 0)
			{
				if ((info->reason = FilterCommandTarget(pAdmin, pTarget, info->flags))
					== COMMAND_TARGET_VALID)
				{
					info->targets[0] = i;
					info->num_targets = 1;
					ke::SafeStrcpy(info->target_name, info->target_name_maxlength, pTarget->GetName());
					info->target_name_style = COMMAND_TARGETNAME_RAW;
				}
				else
				{
					info->num_targets = 0;
				}
				return;
			}
		}
	}

	if (strcmp(info->pattern, "@me") == 0 && info->admin != 0)
	{
		info->reason = FilterCommandTarget(pAdmin, pAdmin, info->flags);
		if (info->reason == COMMAND_TARGET_VALID)
		{
			info->targets[0] = info->admin;
			info->num_targets = 1;
			ke::SafeStrcpy(info->target_name, info->target_name_maxlength, pAdmin->GetName());
			info->target_name_style = COMMAND_TARGETNAME_RAW;
		}
		else
		{
			info->num_targets = 0;
		}
		return;
	}

	if ((info->flags & COMMAND_FILTER_NO_MULTI) != COMMAND_FILTER_NO_MULTI)
	{
		bool is_multi = false;
		bool bots_only = false;
		int skip_client = -1;

		if (strcmp(info->pattern, "@all") == 0)
		{
			is_multi = true;
			ke::SafeStrcpy(info->target_name, info->target_name_maxlength, "all players");
			info->target_name_style = COMMAND_TARGETNAME_ML;
		}
		else if (strcmp(info->pattern, "@dead") == 0)
		{
			is_multi = true;
			if ((info->flags & COMMAND_FILTER_ALIVE) == COMMAND_FILTER_ALIVE)
			{
				info->num_targets = 0;
				info->reason = COMMAND_TARGET_NOT_ALIVE;
				return;
			}
			info->flags |= COMMAND_FILTER_DEAD;
			ke::SafeStrcpy(info->target_name, info->target_name_maxlength, "all dead players");
			info->target_name_style = COMMAND_TARGETNAME_ML;
		}
		else if (strcmp(info->pattern, "@alive") == 0)
		{
			is_multi = true;
			if ((info->flags & COMMAND_FILTER_DEAD) == COMMAND_FILTER_DEAD)
			{
				info->num_targets = 0;
				info->reason = COMMAND_TARGET_NOT_DEAD;
				return;
			}
			ke::SafeStrcpy(info->target_name, info->target_name_maxlength, "all alive players");
			info->target_name_style = COMMAND_TARGETNAME_ML;
			info->flags |= COMMAND_FILTER_ALIVE;
		}
		else if (strcmp(info->pattern, "@bots") == 0)
		{
			is_multi = true;
			if ((info->flags & COMMAND_FILTER_NO_BOTS) == COMMAND_FILTER_NO_BOTS)
			{
				info->num_targets = 0;
				info->reason = COMMAND_TARGET_NOT_HUMAN;
				return;
			}
			ke::SafeStrcpy(info->target_name, info->target_name_maxlength, "all bots");
			info->target_name_style = COMMAND_TARGETNAME_ML;
			bots_only = true;
		}
		else if (strcmp(info->pattern, "@humans") == 0)
		{
			is_multi = true;
			ke::SafeStrcpy(info->target_name, info->target_name_maxlength, "all humans");
			info->target_name_style = COMMAND_TARGETNAME_ML;
			info->flags |= COMMAND_FILTER_NO_BOTS;
		}
		else if (strcmp(info->pattern, "@!me") == 0)
		{
			is_multi = true;
			ke::SafeStrcpy(info->target_name, info->target_name_maxlength, "all players");
			info->target_name_style = COMMAND_TARGETNAME_ML;
			skip_client = info->admin;
		}

		if (is_multi)
		{
			for (int i = 1; i <= max_clients && total < info->max_targets; i++)
			{
				if ((pTarget = GetPlayerByIndex(i)) == NULL)
				{
					continue;
				}
				if (FilterCommandTarget(pAdmin, pTarget, info->flags) > 0)
				{
					if ((!bots_only || pTarget->IsFakeClient())
						&& skip_client != i)
					{
						info->targets[total++] = i;
					}
				}
			}

			info->num_targets = total;
			info->reason = (info->num_targets) ? COMMAND_TARGET_VALID : COMMAND_TARGET_EMPTY_FILTER;
			return;
		}
	}

	List<ICommandTargetProcessor *>::iterator iter;
	for (iter = target_processors.begin(); iter != target_processors.end(); iter++)
	{
		ICommandTargetProcessor *pProcessor = (*iter);
		if (pProcessor->ProcessCommandTarget(info))
		{
			return;
		}
	}

	/* Check partial names */
	int found_client = 0;
	CPlayer *pFoundClient = NULL;
	for (int i = 1; i <= max_clients; i++)
	{
		if ((pTarget = GetPlayerByIndex(i)) == NULL)
		{
			continue;
		}

		if (logicore.stristr(pTarget->GetName(), info->pattern) != NULL)
		{
			if (found_client)
			{
				info->num_targets = 0;
				info->reason = COMMAND_TARGET_AMBIGUOUS;
				return;
			}
			else
			{
				found_client = i;
				pFoundClient = pTarget;
			}
		}
	}

	if (found_client)
	{
		if ((info->reason = FilterCommandTarget(pAdmin, pFoundClient, info->flags)) 
			== COMMAND_TARGET_VALID)
		{
			info->targets[0] = found_client;
			info->num_targets = 1;
			ke::SafeStrcpy(info->target_name, info->target_name_maxlength, pFoundClient->GetName());
			info->target_name_style = COMMAND_TARGETNAME_RAW;
		}
		else
		{
			info->num_targets = 0;
		}
	}
	else
	{
		info->num_targets = 0;
		info->reason = COMMAND_TARGET_NONE;
	}
}

void PlayerManager::OnSourceModMaxPlayersChanged( int newvalue )
{
	m_maxClients = newvalue;
}

void PlayerManager::MaxPlayersChanged( int newvalue /*= -1*/ )
{
	if (newvalue == -1)
	{
		newvalue = gpGlobals->maxClients;
	}

	if (newvalue == MaxClients())
	{
		return;
	}

	/* Notify the rest of core */
	SMGlobalClass *pBase = SMGlobalClass::head;
	while (pBase)
	{
		pBase->OnSourceModMaxPlayersChanged(newvalue);
		pBase = pBase->m_pGlobalClassNext;
	}

	/* Notify Extensions */
	List<IClientListener *>::iterator iter;
	IClientListener *pListener = NULL;
	for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
	{
		pListener = (*iter);
		if (pListener->GetClientListenerVersion() >= 8)
		{
			pListener->OnMaxPlayersChanged(newvalue);
		}
	}
}

int PlayerManager::GetClientFromSerial(unsigned int serial)
{
	serial_t s;
	s.value = serial;

	int client = s.bits.index;

	IGamePlayer *pPlayer = GetGamePlayer(client);

	if (!pPlayer)
	{
		return 0;
	}

	if (serial == pPlayer->GetSerial())
	{
		return client;
	}

	return 0;
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
void CmdMaxplayersCallback(const CCommand &command)
{
#else
void CmdMaxplayersCallback()
{
#endif

	g_Players.MaxPlayersChanged();
}

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
bool PlayerManager::HandleConVarQuery(QueryCvarCookie_t cookie, int client, EQueryCvarValueStatus result, const char *cvarName, const char *cvarValue)
{
	for (int i = 1; i <= m_maxClients; i++)
	{
		if (m_Players[i].m_LanguageCookie == cookie)
		{
			unsigned int langid;
			unsigned int new_langid = (translator->GetLanguageByName(cvarValue, &langid)) ? langid : translator->GetServerLanguage();
			m_Players[i].m_LangId = new_langid;
			OnClientLanguageChanged(i, new_langid);

			return true;
		}
	}

	return false;
}
#endif


/*******************
 *** PLAYER CODE ***
 *******************/

CPlayer::CPlayer()
{
	m_LastPassword.clear();
	m_Serial.value = -1;
}

void CPlayer::Initialize(const char *name, const char *ip, edict_t *pEntity)
{
	m_IsConnected = true;
	m_Ip.assign(ip);
	m_pEdict = pEntity;
	m_iIndex = IndexOfEdict(pEntity);
	m_LangId = translator->GetServerLanguage();

	m_Serial.bits.index = m_iIndex;
	m_Serial.bits.serial = g_PlayerSerialCount++;

	SetName(name);

	char ip2[24], *ptr;
	ke::SafeStrcpy(ip2, sizeof(ip2), ip);
	if ((ptr = strchr(ip2, ':')) != NULL)
	{
		*ptr = '\0';
	}
	m_IpNoPort.assign(ip2);

#if SOURCE_ENGINE == SE_TF2      \
	|| SOURCE_ENGINE == SE_CSS   \
	|| SOURCE_ENGINE == SE_DODS  \
	|| SOURCE_ENGINE == SE_HL2DM \
	|| SOURCE_ENGINE == SE_BMS   \
	|| SOURCE_ENGINE == SE_INSURGENCY \
	|| SOURCE_ENGINE == SE_DOI   \
	|| SOURCE_ENGINE == SE_BLADE
	m_pIClient = engine->GetIServer()->GetClient(m_iIndex - 1);
#else
  #if SOURCE_ENGINE == SE_SDK2013
	// Source SDK 2013 mods that ship on Steam can be using older engine binaries
	static IVEngineServer *engine22 = (IVEngineServer *)(g_SMAPI->GetEngineFactory()("VEngineServer022", nullptr));
	if (engine22)
	{
		m_pIClient = engine22->GetIServer()->GetClient(m_iIndex - 1);
	}
	else
  #endif
	{
		INetChannel *pNetChan = static_cast<INetChannel *>(engine->GetPlayerNetInfo(m_iIndex));
		if (pNetChan)
		{
			m_pIClient = static_cast<IClient *>(pNetChan->GetMsgHandler());
		}
	}
#endif

	UpdateAuthIds();
}

void CPlayer::Connect()
{
	if (m_IsInGame)
	{
		return;
	}

	m_IsInGame = true;

	const char *var = g_Players.GetPassInfoVar();
	int client = IndexOfEdict(m_pEdict);
	if (var[0] != '\0')
	{
		const char *pass = engine->GetClientConVarValue(client, var);
		m_LastPassword.assign(pass ? pass : "");
	}
	else
	{
		m_LastPassword.assign("");
	}
}

void CPlayer::UpdateAuthIds()
{
	if (m_IsAuthorized || (!SetEngineString() && !SetCSteamID()))
		return;
	
	// Now cache Steam2/3 rendered ids
	if (IsFakeClient())
	{
		m_Steam2Id = "BOT";
		m_Steam3Id = "BOT";
		return;
	}
	
	if (!m_SteamId.IsValid())
	{
		if (g_HL2.IsLANServer())
		{
			m_Steam2Id = "STEAM_ID_LAN";
			m_Steam3Id = "STEAM_ID_LAN";
			return;
		}
		else
		{
			m_Steam2Id = "STEAM_ID_PENDING";
			m_Steam3Id = "STEAM_ID_PENDING";
		}
		
		return;
	}
	
	EUniverse steam2universe = m_SteamId.GetEUniverse();
	const char *keyUseInvalidUniverse = g_pGameConf->GetKeyValue("UseInvalidUniverseInSteam2IDs");
	if (keyUseInvalidUniverse && atoi(keyUseInvalidUniverse) == 1)
	{
		steam2universe = k_EUniverseInvalid;
	}
	
	char szAuthBuffer[64];
	ke::SafeSprintf(szAuthBuffer, sizeof(szAuthBuffer), "STEAM_%u:%u:%u", steam2universe, m_SteamId.GetAccountID() & 1, m_SteamId.GetAccountID() >> 1);
	
	m_Steam2Id = szAuthBuffer;
	
	// TODO: make sure all hl2sdks' steamclientpublic.h have k_unSteamUserDesktopInstance.
	if (m_SteamId.GetUnAccountInstance() == 1 /* k_unSteamUserDesktopInstance */)
	{
		ke::SafeSprintf(szAuthBuffer, sizeof(szAuthBuffer), "[U:%u:%u]", m_SteamId.GetEUniverse(), m_SteamId.GetAccountID());
	}
	else
	{
		ke::SafeSprintf(szAuthBuffer, sizeof(szAuthBuffer), "[U:%u:%u:%u]", m_SteamId.GetEUniverse(), m_SteamId.GetAccountID(), m_SteamId.GetUnAccountInstance());
	}
	
	m_Steam3Id = szAuthBuffer;
}

bool CPlayer::SetEngineString()
{
	const char *authstr = engine->GetPlayerNetworkIDString(m_pEdict);
	if (!authstr || m_AuthID.compare(authstr) == 0)
		return false;

	m_AuthID = authstr;
	SetCSteamID();
	return true;
}

bool CPlayer::SetCSteamID()
{
	if (IsFakeClient())
	{
		m_SteamId = k_steamIDNil;
		return true; /* This is the default value. There's a bug-out branch in the caller function. */
	}

#if SOURCE_ENGINE < SE_ORANGEBOX
	const char *pAuth = GetAuthString();
	/* STEAM_0:1:123123 | STEAM_ID_LAN | STEAM_ID_PENDING */
	if (pAuth && (strlen(pAuth) > 10) && pAuth[8] != '_')
	{
		CSteamID sid = CSteamID(atoi(&pAuth[8]) | (atoi(&pAuth[10]) << 1),
			k_unSteamUserDesktopInstance, k_EUniversePublic, k_EAccountTypeIndividual);

		if (m_SteamId != sid)
		{
			m_SteamId = sid;
			return true;
		}
	}
#else
	const CSteamID *steamId = engine->GetClientSteamID(m_pEdict);
	if (steamId)
	{
		if (m_SteamId != (*steamId))
		{
			m_SteamId = (*steamId);
			return true;
		}
	}
#endif
	return false;
}

// Ensure a valid AuthString is set before calling.
void CPlayer::Authorize()
{
	m_IsAuthorized = true;
}

void CPlayer::Disconnect()
{
	DumpAdmin(false);
	m_IsConnected = false;
	m_IsInGame = false;
	m_IsAuthorized = false;
	m_Name.clear();
	m_Ip.clear();
	m_AuthID = "";
	m_SteamId = k_steamIDNil;
	m_Steam2Id = "";
	m_Steam3Id = "";
	m_pEdict = NULL;
	m_Info = NULL;
	m_pIClient = NULL;
	m_bAdminCheckSignalled = false;
	m_UserId = -1;
	m_bIsInKickQueue = false;
	m_bFakeClient = false;
	m_bIsSourceTV = false;
	m_bIsReplay = false;
	m_Serial.value = -1;
#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	m_LanguageCookie = InvalidQueryCvarCookie;
#endif
	ClearNetchannelQueue();
}

void CPlayer::ClearNetchannelQueue(void)
{
	while (!m_PrintfBuffer.empty())
		m_PrintfBuffer.pop_front();
}

void CPlayer::SetName(const char *name)
{
	// Player names from Steam can get truncated in the engine, leaving
	// a partial, invalid UTF-8 char at the end. Strip it off.

	char szNewName[MAX_PLAYER_NAME_LENGTH];
	ke::SafeStrcpy(szNewName, sizeof(szNewName), name);

	size_t i = 0;
	while (i < sizeof(szNewName))
	{
		if (szNewName[i] == 0)
			break;

		unsigned int cCharBytes = _GetUTF8CharBytes(&szNewName[i]);
		size_t newPos = i + cCharBytes;
		if (newPos > (sizeof(szNewName) - 1))
		{
			szNewName[i] = 0;
			break;
		}

		i = newPos;
	}

	m_Name.assign(szNewName);
}

const char *CPlayer::GetName()
{
	return m_Name.c_str();
}

const char *CPlayer::GetIPAddress()
{
	return m_Ip.c_str();
}

const char *CPlayer::GetAuthString(bool validated)
{
	if (validated && !IsAuthStringValidated())
	{
		return NULL;
	}

	return m_AuthID.c_str();
}

const CSteamID &CPlayer::GetSteamId(bool validated)
{
	if (validated && !IsAuthStringValidated())
	{
		static const CSteamID invalidId = k_steamIDNil;
		return invalidId;
	}
	
	return m_SteamId;
}

const char *CPlayer::GetSteam2Id(bool validated)
{
	if (!m_Steam2Id.length() || (validated && !IsAuthStringValidated()))
	{
		return NULL;
	}

	return m_Steam2Id.c_str();
}

const char *CPlayer::GetSteam3Id(bool validated)
{
	if (!m_Steam3Id.length() || (validated && !IsAuthStringValidated()))
	{
		return NULL;
	}

	return m_Steam3Id.c_str();
}

unsigned int CPlayer::GetSteamAccountID(bool validated)
{
	if (!IsFakeClient() && (!validated || IsAuthStringValidated()))
	{
		const CSteamID &id = GetSteamId(validated);
		if (id.IsValid())
			return id.GetAccountID();
	}
	
	return 0;
}

edict_t *CPlayer::GetEdict()
{
	return m_pEdict;
}

int CPlayer::GetIndex() const
{
	return m_iIndex;
}

bool CPlayer::IsInGame()
{
	return m_IsInGame && (m_pEdict->GetUnknown() != NULL);
}

bool CPlayer::WasCountedAsInGame()
{
	return m_IsInGame;
}

bool CPlayer::IsConnected()
{
	return m_IsConnected;
}

bool CPlayer::IsAuthorized()
{
	return m_IsAuthorized;
}

bool CPlayer::IsAuthStringValidated()
{     
#if SOURCE_ENGINE >= SE_ORANGEBOX
	if (!IsFakeClient() && g_Players.m_bAuthstringValidation && !g_HL2.IsLANServer())
	{
		return engine->IsClientFullyAuthenticated(m_pEdict);
	}
#endif

	return true;
}

IPlayerInfo *CPlayer::GetPlayerInfo()
{
	if (m_pEdict->GetUnknown())
	{
		return m_Info;
	}

	return NULL;
}

bool CPlayer::IsFakeClient()
{
	return m_bFakeClient;
}

bool CPlayer::IsSourceTV() const
{
	return m_bIsSourceTV;
}

bool CPlayer::IsReplay() const
{
	return m_bIsReplay;
}

void CPlayer::SetAdminId(AdminId id, bool temporary)
{
	if (!m_IsConnected)
	{
		return;
	}

	DumpAdmin(false);

	m_Admin = id;
	m_TempAdmin = temporary;
}

AdminId CPlayer::GetAdminId()
{
	return m_Admin;
}

void CPlayer::ClearAdmin()
{
	DumpAdmin(true);
}

void CPlayer::DumpAdmin(bool deleting)
{
	if (m_Admin != INVALID_ADMIN_ID)
	{
		if (m_TempAdmin && !deleting)
		{
			adminsys->InvalidateAdmin(m_Admin);
		}
		m_Admin = INVALID_ADMIN_ID;
		m_TempAdmin = false;
	}
}

void CPlayer::Kick(const char *str)
{
	MarkAsBeingKicked();
	IClient *pClient = GetIClient();
	if (pClient == nullptr)
	{
		int userid = GetUserId();
		if (userid > 0)
		{
			char buffer[255];
			ke::SafeSprintf(buffer, sizeof(buffer), "kickid %d %s\n", userid, str);
			engine->ServerCommand(buffer);
		}
	}
	else
	{
#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
		pClient->Disconnect(str);
#else
		pClient->Disconnect("%s", str);
#endif
	}
}

void CPlayer::Authorize_Post()
{
	if (m_IsInGame)
	{
		DoPostConnectAuthorization();
	}
}

void CPlayer::DoPostConnectAuthorization()
{
	bool delay = false;

	List<IClientListener *>::iterator iter;
	for (iter = g_Players.m_hooks.begin();
		 iter != g_Players.m_hooks.end();
		 iter++)
	{
		IClientListener *pListener = (*iter);
#if defined MIN_API_FOR_ADMINCALLS
		if (pListener->GetClientListenerVersion() < MIN_API_FOR_ADMINCALLS)
		{
			continue;
		}
#endif
		if (!pListener->OnClientPreAdminCheck(m_iIndex))
		{
			delay = true;
		}
	}
	
	cell_t result = 0;
	PreAdminCheck->PushCell(m_iIndex);
	PreAdminCheck->Execute(&result);

	/* Defer, for better or worse */
	if (delay || (ResultType)result >= Pl_Handled)
	{
		return;
	}

	/* Sanity check */
	if (!IsConnected())
	{
		return;
	}

	/* Otherwise, go ahead and do admin checks */
	DoBasicAdminChecks();

	/* Send the notification out */
	NotifyPostAdminChecks();
}

bool CPlayer::RunAdminCacheChecks()
{
	AdminId old_id = GetAdminId();

	DoBasicAdminChecks();

	return (GetAdminId() != old_id);
}

void CPlayer::NotifyPostAdminChecks()
{
	if (m_bAdminCheckSignalled)
	{
		return;
	}

	/* Block beforehand so they can't double-call */
	m_bAdminCheckSignalled = true;

	List<IClientListener *>::iterator iter;
	for (iter = g_Players.m_hooks.begin();
		iter != g_Players.m_hooks.end();
		iter++)
	{
		IClientListener *pListener = (*iter);
#if defined MIN_API_FOR_ADMINCALLS
		if (pListener->GetClientListenerVersion() < MIN_API_FOR_ADMINCALLS)
		{
			continue;
		}
#endif
		pListener->OnClientPostAdminCheck(m_iIndex);
	}

	PostAdminFilter->PushCell(m_iIndex);
	PostAdminFilter->Execute(NULL);

	PostAdminCheck->PushCell(m_iIndex);
	PostAdminCheck->Execute(NULL);
}

void CPlayer::DoBasicAdminChecks()
{
	if (GetAdminId() != INVALID_ADMIN_ID)
	{
		return;
	}

	/* First check the name */
	AdminId id;
	int client = IndexOfEdict(m_pEdict);

	if ((id = adminsys->FindAdminByIdentity("name", GetName())) != INVALID_ADMIN_ID)
	{
		if (!g_Players.CheckSetAdminName(client, this, id))
		{
			int userid = engine->GetPlayerUserId(m_pEdict);
			g_Timers.CreateTimer(&s_KickPlayerTimer, 0.1f, (void *)(intptr_t)userid, 0);
		}
		return;
	}
	
	/* Check IP */
	if ((id = adminsys->FindAdminByIdentity("ip", m_IpNoPort.c_str())) != INVALID_ADMIN_ID)
	{
		if (g_Players.CheckSetAdmin(client, this, id))
		{
			return;
		}
	}

	/* Check steam id */
	if ((id = adminsys->FindAdminByIdentity("steam", m_AuthID.c_str())) != INVALID_ADMIN_ID)
	{
		if (g_Players.CheckSetAdmin(client, this, id))
		{
			return;
		}
	}
}

unsigned int CPlayer::GetLanguageId()
{
	return m_LangId;
}

void CPlayer::SetLanguageId(unsigned int id)
{
	if(m_LangId != id)
	{
		m_LangId = id;
		g_Players.OnClientLanguageChanged(m_iIndex, id);
	}
}

int CPlayer::GetUserId()
{
	if (m_UserId == -1)
	{
		m_UserId = engine->GetPlayerUserId(GetEdict());
	}

	return m_UserId;
}

bool CPlayer::IsInKickQueue()
{
	return m_bIsInKickQueue;
}

void CPlayer::MarkAsBeingKicked()
{
	m_bIsInKickQueue = true;
}

int CPlayer::GetLifeState()
{
	if (lifestate_offset == -1)
	{
		if (!g_pGameConf->GetOffset("m_lifeState", &lifestate_offset))
		{
			lifestate_offset = -2;
		}
	}

	if (lifestate_offset < 0)
	{
		IPlayerInfo *info = GetPlayerInfo();
		if (info == NULL)
		{
			return PLAYER_LIFE_UNKNOWN;
		}
		return info->IsDead() ? PLAYER_LIFE_DEAD : PLAYER_LIFE_ALIVE;
	}

	if (m_pEdict == NULL)
	{
		return PLAYER_LIFE_UNKNOWN;
	}

	CBaseEntity *pEntity;
	IServerUnknown *pUnknown = m_pEdict->GetUnknown();
	if (pUnknown == NULL || (pEntity = pUnknown->GetBaseEntity()) == NULL)
	{
		return PLAYER_LIFE_UNKNOWN;
	}

	if (*((uint8_t *)pEntity + lifestate_offset) == LIFE_ALIVE)
	{
		return PLAYER_LIFE_ALIVE;
	}
	else
	{
		return PLAYER_LIFE_DEAD;
	}
}

IClient *CPlayer::GetIClient() const
{
	return m_pIClient;
}

unsigned int CPlayer::GetSerial()
{
	return m_Serial.value;
}

void CPlayer::PrintToConsole(const char *pMsg)
{
	if (m_IsConnected == false || m_bFakeClient == true)
	{
		return;
	}

	INetChannel *pNetChan = static_cast<INetChannel *>(engine->GetPlayerNetInfo(m_iIndex));
	if (pNetChan == NULL)
	{
		return;
	}

	engine->ClientPrintf(m_pEdict, pMsg);
}
