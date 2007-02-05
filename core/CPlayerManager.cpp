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

CPlayerManager g_PlayerManager;

SH_DECL_HOOK5(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, edict_t *, const char *, const char *, char *, int);
SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, const char *);
SH_DECL_HOOK1_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);

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

	/* Register OnClientConnect */
	ParamType p1[] = {Param_Cell, Param_String, Param_Cell};
	m_clconnect = g_Forwards.CreateForward("OnClientConnect", ET_Event, 3, p1);

	/* Register OnClientPutInServer */
	ParamType p2[] = {Param_Cell};
	m_clputinserver = g_Forwards.CreateForward("OnClientPutInServer", ET_Ignore, 1, p2);

	/* Register OnClientDisconnect */
	m_cldisconnect = g_Forwards.CreateForward("OnClientDisconnect", ET_Ignore, 1, p2);

	/* Register OnClientDisconnect_Post */
	m_cldisconnect_post = g_Forwards.CreateForward("OnClientDisconnect_Post", ET_Ignore, 1, p2);

	/* Register OnClientCommand */
	m_clcommand = g_Forwards.CreateForward("OnClientCommand", ET_Hook, 1, p2);

	/* Register OnClientSettingsChanged */
	m_clinfochanged = g_Forwards.CreateForward("OnClientSettingsChanged", ET_Ignore, 1, p2);

	/* Register OnClientAuthorized */
	//:TODO:
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
		m_FirstPass = false;
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

	//:TODO: res should be evaluated here for something, DOCUMENT this in the INC FILE!
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
	if (client > m_maxClients || client < 0)
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

/*******************
 *** PLAYER CODE ***
 *******************/

CPlayer::CPlayer()
{
	m_IsConnected = false;
	m_IsInGame = false;
	m_IsAuthorized = false;
	m_PlayerEdict = NULL;
}

void CPlayer::Initialize(const char *name, const char *ip, edict_t *pEntity)
{
	m_IsConnected = true;
	m_Name.assign(name);
	m_Ip.assign(ip);
	m_PlayerEdict = pEntity;
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
	m_IsConnected = false;
	m_IsInGame = false;
	m_IsAuthorized = false;
	m_Name.clear();
	m_Ip.clear();
	m_AuthID.clear();
	m_PlayerEdict = NULL;
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
	return m_PlayerEdict;
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
