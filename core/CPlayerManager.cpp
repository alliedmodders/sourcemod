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

#include "CPlayerManager.h"
#include "ForwardSys.h"
#include "ShareSys.h"
#include "AdminCache.h"

CPlayerManager g_Players;

SH_DECL_HOOK5(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, edict_t *, const char *, const char *, char *, int);
SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, const char *);
SH_DECL_HOOK1_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);

CPlayerManager::CPlayerManager()
{
	m_AuthQueue = NULL;
	m_FirstPass = true;
}

CPlayerManager::~CPlayerManager()
{
	delete [] m_AuthQueue;
}

void CPlayerManager::OnSourceModAllInitialized()
{
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientConnect, serverClients, this, &CPlayerManager::OnClientConnect, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientConnect, serverClients, this, &CPlayerManager::OnClientConnect_Post, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, serverClients, this, &CPlayerManager::OnClientPutInServer, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, serverClients, this, &CPlayerManager::OnClientDisconnect, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, serverClients, this, &CPlayerManager::OnClientDisconnect_Post, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientCommand, serverClients, this, &CPlayerManager::OnClientCommand, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, serverClients, this, &CPlayerManager::OnClientSettingsChanged, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, gamedll, this, &CPlayerManager::OnServerActivate, true);

	g_ShareSys.AddInterface(NULL, this);

	ParamType p1[] = {Param_Cell, Param_String, Param_Cell};
	ParamType p2[] = {Param_Cell};
	ParamType p3[] = {Param_Cell, Param_String};

	m_clconnect = g_Forwards.CreateForward("OnClientConnect", ET_Event, 3, p1);
	m_clputinserver = g_Forwards.CreateForward("OnClientPutInServer", ET_Ignore, 1, p2);
	m_cldisconnect = g_Forwards.CreateForward("OnClientDisconnect", ET_Ignore, 1, p2);
	m_cldisconnect_post = g_Forwards.CreateForward("OnClientDisconnect_Post", ET_Ignore, 1, p2);
	m_clcommand = g_Forwards.CreateForward("OnClientCommand", ET_Hook, 1, p2);
	m_clinfochanged = g_Forwards.CreateForward("OnClientSettingsChanged", ET_Ignore, 1, p2);
	m_clauth = g_Forwards.CreateForward("OnClientAuthorized", ET_Ignore, 2, p3);
}

void CPlayerManager::OnSourceModShutdown()
{
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientConnect, serverClients, this, &CPlayerManager::OnClientConnect, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, serverClients, this, &CPlayerManager::OnClientPutInServer, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, serverClients, this, &CPlayerManager::OnClientDisconnect, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, serverClients, this, &CPlayerManager::OnClientDisconnect_Post, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientCommand, serverClients, this, &CPlayerManager::OnClientCommand, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, serverClients, this, &CPlayerManager::OnClientSettingsChanged, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, gamedll, this, &CPlayerManager::OnServerActivate, true);

	/* Release forwards */
	g_Forwards.ReleaseForward(m_clconnect);
	g_Forwards.ReleaseForward(m_clputinserver);
	g_Forwards.ReleaseForward(m_cldisconnect);
	g_Forwards.ReleaseForward(m_cldisconnect_post);
	g_Forwards.ReleaseForward(m_clcommand);
	g_Forwards.ReleaseForward(m_clinfochanged);
	g_Forwards.ReleaseForward(m_clauth);

	delete [] m_Players;
}

void CPlayerManager::OnServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
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
}

void CPlayerManager::RunAuthChecks()
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
			pPlayer->m_AuthID.assign(authstr);
			pPlayer->m_IsAuthorized = true;

			/* Send to extensions */
			List<IClientListener *>::iterator iter;
			IClientListener *pListener;
			for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
			{
				pListener = (*iter);
				pListener->OnClientAuthorized(m_AuthQueue[i], authstr);
			}

			/* Send to plugins */
			if (m_clauth->GetFunctionCount())
			{
				m_clauth->PushCell(m_AuthQueue[i]);
				m_clauth->PushString(authstr);
				m_clauth->Execute(NULL);
			}

			/* Mark as removed from queue */
			m_AuthQueue[i] = 0;
			removed++;
		}
	}

	/* Clean up the queue */
	if (removed)
	{
		/* We don't have to compcat the list if it's empty */
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

bool CPlayerManager::OnClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
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
		m_AuthQueue[++m_AuthQueue[0]] = client;
		g_SourceMod.SetAuthChecking(true);
	}

	return (res) ? true : false;
}

bool CPlayerManager::OnClientConnect_Post(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	int client = engine->IndexOfEdict(pEntity);
	bool orig_value = META_RESULT_ORIG_RET(bool);
	if (orig_value)
	{
		List<IClientListener *>::iterator iter;
		IClientListener *pListener = NULL;
		for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
		{
			pListener = (*iter);
			pListener->OnClientConnected(client);
		}
	}

	return true;
}

void CPlayerManager::OnClientPutInServer(edict_t *pEntity, const char *playername)
{
	cell_t res;
	int client = engine->IndexOfEdict(pEntity);

	CPlayer *pPlayer = GetPlayerByIndex(client);
	if (!pPlayer->IsConnected())
	{
		/* Run manual connection routines */
		char error[255];
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
		}
	}

	List<IClientListener *>::iterator iter;
	IClientListener *pListener = NULL;
	for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnClientPutInServer(client);
	}

	m_Players[client].Connect();
	m_PlayerCount++;
	m_clputinserver->PushCell(client);
	m_clputinserver->Execute(&res, NULL);
}

void CPlayerManager::OnClientAuthorized()
{
	//:TODO:
}

void CPlayerManager::OnClientDisconnect(edict_t *pEntity)
{
	cell_t res;
	int client = engine->IndexOfEdict(pEntity);

	if (m_Players[client].IsConnected())
	{
		m_cldisconnect->PushCell(client);
		m_cldisconnect->Execute(&res, NULL);
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
			if (m_AuthQueue[i] == client)
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
}

void CPlayerManager::OnClientDisconnect_Post(edict_t *pEntity)
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

void CPlayerManager::OnClientCommand(edict_t *pEntity)
{
	cell_t res;

	m_clcommand->PushCell(engine->IndexOfEdict(pEntity));
	m_clcommand->Execute(&res, NULL);
}

void CPlayerManager::OnClientSettingsChanged(edict_t *pEntity)
{
	cell_t res;
	int client = engine->IndexOfEdict(pEntity);

	m_clinfochanged->PushCell(engine->IndexOfEdict(pEntity));
	m_clinfochanged->Execute(&res, NULL);
	m_Players[client].SetName(engine->GetClientConVarValue(client, "name"));
}

int CPlayerManager::GetMaxClients()
{
	return m_maxClients;
}

CPlayer *CPlayerManager::GetPlayerByIndex(int client) const
{
	if (client > m_maxClients || client < 1)
	{
		return NULL;
	}
	return &m_Players[client];
}

int CPlayerManager::GetNumPlayers()
{
	return m_PlayerCount;
}

void CPlayerManager::AddClientListener(IClientListener *listener)
{
	m_hooks.push_back(listener);
}

void CPlayerManager::RemoveClientListener(IClientListener *listener)
{
	m_hooks.remove(listener);
}

IGamePlayer *CPlayerManager::GetGamePlayer(edict_t *pEdict)
{
	int index = engine->IndexOfEdict(pEdict);
	return GetGamePlayer(index);
}

IGamePlayer *CPlayerManager::GetGamePlayer(int client)
{
	return GetPlayerByIndex(client);
}

void CPlayerManager::ClearAdminId(AdminId id)
{
	for (int i=1; i<=m_maxClients; i++)
	{
		if (m_Players[i].m_Admin == id)
		{
			m_Players[i].DumpAdmin(true);
		}
	}
}

void CPlayerManager::ClearAllAdmins()
{
	for (int i=1; i<=m_maxClients; i++)
	{
		m_Players[i].DumpAdmin(true);
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
}

void CPlayer::Initialize(const char *name, const char *ip, edict_t *pEntity)
{
	m_IsConnected = true;
	m_Name.assign(name);
	m_Ip.assign(ip);
	m_pEdict = pEntity;
}

void CPlayer::Connect()
{
	m_IsInGame = true;
}

void CPlayer::Authorize(const char *steamid)
{
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
}

void CPlayer::SetName(const char *name)
{
	m_Name.assign(name);
}

const char *CPlayer::GetName() const
{
	return m_Name.c_str();
}

const char *CPlayer::GetIPAddress() const
{
	return m_Ip.c_str();
}

const char *CPlayer::GetAuthString() const
{
	return m_AuthID.c_str();
}

edict_t *CPlayer::GetEdict() const
{
	return m_pEdict;
}

bool CPlayer::IsInGame() const
{
	return m_IsInGame;
}

bool CPlayer::IsConnected() const
{
	return m_IsConnected;
}

bool CPlayer::IsAuthorized() const
{
	return m_IsAuthorized;
}

bool CPlayer::IsFakeClient() const
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

AdminId CPlayer::GetAdminId() const
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
