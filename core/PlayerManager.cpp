/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
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

#include "PlayerManager.h"
#include "ForwardSys.h"
#include "ShareSys.h"
#include "AdminCache.h"
#include "ConCmdManager.h"
#include "MenuStyle_Valve.h"
#include "MenuStyle_Radio.h"
#include "sm_stringutil.h"
#include "CoreConfig.h"
#include <inetchannel.h>
#include <iclient.h>
#include "TimerSys.h"
#include "Translator.h"
#include "Logger.h"

PlayerManager g_Players;
bool g_OnMapStarted = false;
IForward *PreAdminCheck = NULL;
IForward *PostAdminCheck = NULL;

SH_DECL_HOOK5(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, edict_t *, const char *, const char *, char *, int);
SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, const char *);
SH_DECL_HOOK1_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);

class KickPlayerTimer : public ITimedEvent
{
public:
	ResultType OnTimer(ITimer *pTimer, void *pData)
	{
		int userid = (int)pData;
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
	m_FirstPass = true;

	m_UserIdLookUp = new int[USHRT_MAX];
	memset(m_UserIdLookUp, 0, sizeof(int) * USHRT_MAX);
}

PlayerManager::~PlayerManager()
{
	delete [] m_AuthQueue;
	delete [] m_UserIdLookUp;
}

void PlayerManager::OnSourceModAllInitialized()
{
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientConnect, serverClients, this, &PlayerManager::OnClientConnect, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientConnect, serverClients, this, &PlayerManager::OnClientConnect_Post, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, serverClients, this, &PlayerManager::OnClientPutInServer, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, serverClients, this, &PlayerManager::OnClientDisconnect, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, serverClients, this, &PlayerManager::OnClientDisconnect_Post, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientCommand, serverClients, this, &PlayerManager::OnClientCommand, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, serverClients, this, &PlayerManager::OnClientSettingsChanged, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, gamedll, this, &PlayerManager::OnServerActivate, true);

	g_ShareSys.AddInterface(NULL, this);

	ParamType p1[] = {Param_Cell, Param_String, Param_Cell};
	ParamType p2[] = {Param_Cell};

	m_clconnect = g_Forwards.CreateForward("OnClientConnect", ET_Event, 3, p1);
	m_clputinserver = g_Forwards.CreateForward("OnClientPutInServer", ET_Ignore, 1, p2);
	m_cldisconnect = g_Forwards.CreateForward("OnClientDisconnect", ET_Ignore, 1, p2);
	m_cldisconnect_post = g_Forwards.CreateForward("OnClientDisconnect_Post", ET_Ignore, 1, p2);
	m_clcommand = g_Forwards.CreateForward("OnClientCommand", ET_Hook, 2, NULL, Param_Cell, Param_Cell);
	m_clinfochanged = g_Forwards.CreateForward("OnClientSettingsChanged", ET_Ignore, 1, p2);
	m_clauth = g_Forwards.CreateForward("OnClientAuthorized", ET_Ignore, 2, NULL, Param_Cell, Param_String);
	m_onActivate = g_Forwards.CreateForward("OnServerLoad", ET_Ignore, 0, NULL);
	m_onActivate2 = g_Forwards.CreateForward("OnMapStart", ET_Ignore, 0, NULL);

	PreAdminCheck = g_Forwards.CreateForward("OnClientPreAdminCheck", ET_Event, 1, p1);
	PostAdminCheck = g_Forwards.CreateForward("OnClientPostAdminCheck", ET_Ignore, 1, p1);
}

void PlayerManager::OnSourceModShutdown()
{
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientConnect, serverClients, this, &PlayerManager::OnClientConnect, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, serverClients, this, &PlayerManager::OnClientPutInServer, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, serverClients, this, &PlayerManager::OnClientDisconnect, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, serverClients, this, &PlayerManager::OnClientDisconnect_Post, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientCommand, serverClients, this, &PlayerManager::OnClientCommand, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, serverClients, this, &PlayerManager::OnClientSettingsChanged, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, gamedll, this, &PlayerManager::OnServerActivate, true);

	/* Release forwards */
	g_Forwards.ReleaseForward(m_clconnect);
	g_Forwards.ReleaseForward(m_clputinserver);
	g_Forwards.ReleaseForward(m_cldisconnect);
	g_Forwards.ReleaseForward(m_cldisconnect_post);
	g_Forwards.ReleaseForward(m_clcommand);
	g_Forwards.ReleaseForward(m_clinfochanged);
	g_Forwards.ReleaseForward(m_clauth);
	g_Forwards.ReleaseForward(m_onActivate);
	g_Forwards.ReleaseForward(m_onActivate2);

	g_Forwards.ReleaseForward(PreAdminCheck);
	g_Forwards.ReleaseForward(PostAdminCheck);

	delete [] m_Players;
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
			UTIL_Format(error, maxlength, "Invalid value: must be \"on\" or \"off\"");
			return ConfigResult_Reject;
		}
		return ConfigResult_Accept;
	}
	return ConfigResult_Ignore;
}

void PlayerManager::OnServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	if (m_FirstPass)
	{
		/* Initialize all players */
		m_maxClients = clientMax;
		m_PlayerCount = 0;
		m_Players = new CPlayer[m_maxClients + 1];
		m_AuthQueue = new unsigned int[m_maxClients + 1];
		m_FirstPass = false;

		memset(m_AuthQueue, 0, sizeof(unsigned int) * (m_maxClients + 1));
	}
	m_onActivate->Execute(NULL);
	m_onActivate2->Execute(NULL);

	g_OnMapStarted = true;

	SM_ExecuteAllConfigs();
}

bool PlayerManager::CheckSetAdmin(int index, CPlayer *pPlayer, AdminId id)
{
	const char *password = g_Admins.GetAdminPassword(id);
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
	const char *password = g_Admins.GetAdminPassword(id);
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
		pPlayer = GetPlayerByIndex(m_AuthQueue[i]);
		authstr = engine->GetPlayerNetworkIDString(pPlayer->m_pEdict);
		if (authstr && authstr[0] != '\0'
			&& (strcmp(authstr, "STEAM_ID_PENDING") != 0))
		{
			/* Set authorization */
			pPlayer->Authorize(authstr);

			/* Mark as removed from queue */
			unsigned int client = m_AuthQueue[i];
			m_AuthQueue[i] = 0;
			removed++;

			/* Send to extensions */
			List<IClientListener *>::iterator iter;
			IClientListener *pListener;
			for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
			{
				pListener = (*iter);
				pListener->OnClientAuthorized(client, authstr);
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
				m_clauth->PushString(authstr);
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
			g_SourceMod.SetAuthChecking(false);
		}
	}
}

bool PlayerManager::OnClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	int client = engine->IndexOfEdict(pEntity);

	List<IClientListener *>::iterator iter;
	IClientListener *pListener = NULL;
	for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
	{
		pListener = (*iter);
		if (!pListener->InterceptClientConnect(client, reject, maxrejectlen))
		{
			return false;
		}
	}

	cell_t res = 1;

	m_Players[client].Initialize(pszName, pszAddress, pEntity);
	m_clconnect->PushCell(client);
	m_clconnect->PushStringEx(reject, maxrejectlen, SM_PARAM_STRING_UTF8, SM_PARAM_COPYBACK);
	m_clconnect->PushCell(maxrejectlen);
	m_clconnect->Execute(&res, NULL);

	if (res)
	{
		if (!m_Players[client].IsAuthorized())
		{
			m_AuthQueue[++m_AuthQueue[0]] = client;
			g_SourceMod.SetAuthChecking(true);
		}
	} else {
		RETURN_META_VALUE(MRES_SUPERCEDE, false);
	}

	m_UserIdLookUp[engine->GetPlayerUserId(pEntity)] = client;

	return true;
}

bool PlayerManager::OnClientConnect_Post(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	int client = engine->IndexOfEdict(pEntity);
	bool orig_value = META_RESULT_ORIG_RET(bool);
	CPlayer *pPlayer = GetPlayerByIndex(client);

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
				break;
			}
		}
	}

	return true;
}

void PlayerManager::OnClientPutInServer(edict_t *pEntity, const char *playername)
{
	cell_t res;
	int client = engine->IndexOfEdict(pEntity);

	CPlayer *pPlayer = GetPlayerByIndex(client);
	/* If they're not connected, they're a bot */
	if (!pPlayer->IsConnected())
	{
		/* Run manual connection routines */
		char error[255];
		const char *authid = engine->GetPlayerNetworkIDString(pEntity);
		pPlayer->Authorize(authid);
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
		/* Now do authorization */
		for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
		{
			pListener = (*iter);
			pListener->OnClientAuthorized(client, authid);
		}
		/* Finally, tell plugins */
		if (m_clauth->GetFunctionCount())
		{
			m_clauth->PushCell(client);
			m_clauth->PushString(authid);
			m_clauth->Execute(NULL);
		}
		pPlayer->Authorize_Post();
	}

	if (playerinfo)
	{
		pPlayer->m_Info = playerinfo->GetPlayerInfo(pEntity);
	}

	/* Get the client's language */
	if (m_QueryLang)
	{
		const char *name;
		if (!pPlayer->IsFakeClient() && (name=engine->GetClientConVarValue(client, "cl_language")))
		{
			unsigned int langid;
			pPlayer->m_LangId = (g_Translator.GetLanguageByName(name, &langid)) ? langid : g_Translator.GetServerLanguage();
		} else {
			pPlayer->m_LangId = g_Translator.GetServerLanguage();
		}
	}

	m_Players[client].Connect();
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
}

void PlayerManager::OnSourceModLevelEnd()
{
	/* Disconnect all bots still in game */
	for (int i=1; i<=m_maxClients; i++)
	{
		if (m_Players[i].IsConnected())
		{
			OnClientDisconnect(m_Players[i].GetEdict());
		}
	}
}

void PlayerManager::OnClientDisconnect(edict_t *pEntity)
{
	cell_t res;
	int client = engine->IndexOfEdict(pEntity);

	if (m_Players[client].IsConnected())
	{
		m_cldisconnect->PushCell(client);
		m_cldisconnect->Execute(&res, NULL);
	} else {
		/* We don't care, prevent a double call */
		return;
	}

	if (m_Players[client].IsInGame())
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

	/**
	 * Remove client from auth queue if necessary
	 */
	if (!m_Players[client].IsAuthorized())
	{
		for (unsigned int i=1; i<=m_AuthQueue[0]; i++)
		{
			if (m_AuthQueue[i] == (unsigned)client)
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

	m_Players[client].Disconnect();
	m_UserIdLookUp[engine->GetPlayerUserId(pEntity)] = 0;
}

void PlayerManager::OnClientDisconnect_Post(edict_t *pEntity)
{
	cell_t res;
	int client = engine->IndexOfEdict(pEntity);

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

void PlayerManager::OnClientCommand(edict_t *pEntity)
{
	/**
	 * We cache this because the engine is not re-entrant.
	 */
	char cmd[300];
	int args = engine->Cmd_Argc() - 1;
	strncopy(cmd, engine->Cmd_Argv(0), sizeof(cmd));

	int client = engine->IndexOfEdict(pEntity);
	cell_t res = Pl_Continue;

	bool result = g_ValveMenuStyle.OnClientCommand(client, cmd);
	if (result)
	{
		res = Pl_Handled;
	} else {
		result = g_RadioMenuStyle.OnClientCommand(client, cmd);
		if (result)
		{
			res = Pl_Handled;
		}
	}

	cell_t res2 = Pl_Continue;
	m_clcommand->PushCell(client);
	m_clcommand->PushCell(args);
	m_clcommand->Execute(&res2, NULL);

	if (res2 > res)
	{
		res = res2;
	}

	if (res >= Pl_Stop)
	{
		RETURN_META(MRES_SUPERCEDE);
	}

	res = g_ConCmds.DispatchClientCommand(client, cmd, args, (ResultType)res);

	if (res >= Pl_Handled)
	{
		RETURN_META(MRES_SUPERCEDE);
	}
}

void PlayerManager::OnClientSettingsChanged(edict_t *pEntity)
{
	cell_t res;
	int client = engine->IndexOfEdict(pEntity);

	m_clinfochanged->PushCell(engine->IndexOfEdict(pEntity));
	m_clinfochanged->Execute(&res, NULL);

	IPlayerInfo *info = m_Players[client].GetPlayerInfo();
	const char *new_name = info ? info->GetName() : engine->GetClientConVarValue(client, "name");
	const char *old_name = m_Players[client].m_Name.c_str();

	if (strcmp(old_name, new_name) != 0)
	{
		AdminId id = g_Admins.FindAdminByIdentity("name", new_name);
		if (id != INVALID_ADMIN_ID && m_Players[client].GetAdminId() != id)
		{
			if (!CheckSetAdminName(client, &m_Players[client], id))
			{
				m_Players[client].Kick("Your name is reserved by SourceMod; set your password to use it.");
				RETURN_META(MRES_IGNORED);
			}
		} else if ((id = g_Admins.FindAdminByIdentity("name", old_name)) != INVALID_ADMIN_ID) {
			if (id == m_Players[client].GetAdminId())
			{
				/* This player is changing their name; force them to drop admin privileges! */
				m_Players[client].SetAdminId(INVALID_ADMIN_ID, false);
			}
		}
		m_Players[client].SetName(new_name);
	}
	
	if (m_PassInfoVar.size() > 0)
	{
		/* Try for a password change */
		const char *old_pass = m_Players[client].m_LastPassword.c_str();
		const char *new_pass = engine->GetClientConVarValue(client, m_PassInfoVar.c_str());
		if (strcmp(old_pass, new_pass) != 0)
		{
			m_Players[client].m_LastPassword.assign(new_pass);
			if (m_Players[client].IsInGame() && m_Players[client].IsAuthorized())
			{
				/* If there is already an admin id assigned, this will just bail out. */
				m_Players[client].DoBasicAdminChecks();
			}
		}
	}
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
	int index = engine->IndexOfEdict(pEdict);
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

void PlayerManager::ClearAllAdmins()
{
	for (int i=1; i<=m_maxClients; i++)
	{
		m_Players[i].DumpAdmin(true);
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

/*******************
 *** PLAYER CODE ***
 *******************/

CPlayer::CPlayer()
{
	m_IsConnected = false;
	m_IsInGame = false;
	m_IsAuthorized = false;
	m_pEdict = NULL;
	m_Admin = INVALID_ADMIN_ID;
	m_TempAdmin = false;
	m_Info = NULL;
	m_bAdminCheckSignalled = false;
	m_LastPassword.clear();
	m_LangId = LANGUAGE_ENGLISH;
}

void CPlayer::Initialize(const char *name, const char *ip, edict_t *pEntity)
{
	m_IsConnected = true;
	m_Name.assign(name);
	m_Ip.assign(ip);
	m_pEdict = pEntity;
	m_iIndex = engine->IndexOfEdict(pEntity);
	m_LangId = g_Translator.GetServerLanguage();

	char ip2[24], *ptr;
	strncopy(ip2, ip, sizeof(ip2));
	if ((ptr = strchr(ip2, ':')) != NULL)
	{
		*ptr = '\0';
	}
	m_IpNoPort.assign(ip2);
}

void CPlayer::Connect()
{
	if (m_IsInGame)
	{
		return;
	}

	m_IsInGame = true;

	const char *var = g_Players.GetPassInfoVar();
	int client = engine->IndexOfEdict(m_pEdict);
	if (var[0] != '\0')
	{
		const char *pass = engine->GetClientConVarValue(client, var);
		m_LastPassword.assign(pass ? pass : "");
	} else {
		m_LastPassword.assign("");
	}

	if (m_IsAuthorized)
	{
		DoPostConnectAuthorization();
	}
}

void CPlayer::Authorize(const char *steamid)
{
	if (m_IsAuthorized)
	{
		return;
	}

	m_IsAuthorized = true;
	m_AuthID.assign(steamid);
}

void CPlayer::Disconnect()
{
	DumpAdmin(false);
	m_IsConnected = false;
	m_IsInGame = false;
	m_IsAuthorized = false;
	m_Name.clear();
	m_Ip.clear();
	m_AuthID.clear();
	m_pEdict = NULL;
	m_Info = NULL;
	m_bAdminCheckSignalled = false;
	m_UserId = -1;
}

void CPlayer::SetName(const char *name)
{
	m_Name.assign(name);
}

const char *CPlayer::GetName()
{
	if (m_Info && m_pEdict->GetUnknown())
	{
		return m_Info->GetName();
	}
	
	return m_Name.c_str();
}

const char *CPlayer::GetIPAddress()
{
	return m_Ip.c_str();
}

const char *CPlayer::GetAuthString()
{
	return m_AuthID.c_str();
}

edict_t *CPlayer::GetEdict()
{
	return m_pEdict;
}

bool CPlayer::IsInGame()
{
	return m_IsInGame && (m_pEdict->GetUnknown() != NULL);
}

bool CPlayer::IsConnected()
{
	return m_IsConnected;
}

bool CPlayer::IsAuthorized()
{
	return m_IsAuthorized;
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
	return (strcmp(m_AuthID.c_str(), "BOT") == 0);
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

void CPlayer::DumpAdmin(bool deleting)
{
	if (m_Admin != INVALID_ADMIN_ID)
	{
		if (m_TempAdmin && !deleting)
		{
			g_Admins.InvalidateAdmin(m_Admin);
		}
		m_Admin = INVALID_ADMIN_ID;
		m_TempAdmin = false;
	}
}

void CPlayer::Kick(const char *str)
{
	INetChannel *pNetChan = static_cast<INetChannel *>(engine->GetPlayerNetInfo(m_iIndex));
	IClient *pClient = static_cast<IClient *>(pNetChan->GetMsgHandler());
	pClient->Disconnect("%s", str);
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
	cell_t result = 0;
	PreAdminCheck->PushCell(m_iIndex);
	PreAdminCheck->Execute(&result);

	/* Defer, for better or worse */
	if ((ResultType)result >= Pl_Handled)
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

void CPlayer::NotifyPostAdminChecks()
{
	if (m_bAdminCheckSignalled)
	{
		return;
	}

	/* Block beforehand so they can't double-call */
	m_bAdminCheckSignalled = true;

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
	int client = engine->IndexOfEdict(m_pEdict);

	if ((id = g_Admins.FindAdminByIdentity("name", GetName())) != INVALID_ADMIN_ID)
	{
		if (!g_Players.CheckSetAdminName(client, this, id))
		{
			int userid = engine->GetPlayerUserId(m_pEdict);
			g_Timers.CreateTimer(&s_KickPlayerTimer, 0.1f, (void *)userid, 0);
		}
		return;
	}
	
	/* Check IP */
	if ((id = g_Admins.FindAdminByIdentity("ip", m_IpNoPort.c_str())) != INVALID_ADMIN_ID)
	{
		if (g_Players.CheckSetAdmin(client, this, id))
		{
			return;
		}
	}

	/* Check IP address */
	if ((id = g_Admins.FindAdminByIdentity("steam", m_AuthID.c_str())) != INVALID_ADMIN_ID)
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

int CPlayer::GetUserId()
{
	if (m_UserId == -1)
	{
		m_UserId = engine->GetPlayerUserId(GetEdict());
	}

	return m_UserId;
}
