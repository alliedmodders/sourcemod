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

#ifndef _INCLUDE_SOURCEMOD_CPLAYERMANAGER_H_
#define _INCLUDE_SOURCEMOD_CPLAYERMANAGER_H_

#include "sm_globals.h"
#include <eiface.h>
#include "sourcemm_api.h"
#include <IForwardSys.h>
#include <IPlayerHelpers.h>
#include <IAdminSystem.h>
#include <sh_string.h>
#include <sh_list.h>
#include <sh_vector.h>

using namespace SourceHook;

class CPlayer : public IGamePlayer
{
	friend class CPlayerManager;
public:
	CPlayer();
public:
	const char *GetName() const;
	const char *GetIPAddress() const;
	const char *GetAuthString() const;
	edict_t *GetEdict() const;
	bool IsInGame() const;
	bool IsConnected() const;
	bool IsAuthorized() const;
	bool IsFakeClient() const;
	void SetAdminId(AdminId id, bool temporary);
	AdminId GetAdminId() const;
private:
	void Initialize(const char *name, const char *ip, edict_t *pEntity);
	void Connect();
	void Authorize(const char *steamid);
	void Disconnect();
	void SetName(const char *name);
	void DumpAdmin(bool deleting);
private:
	bool m_IsConnected;
	bool m_IsInGame;
	bool m_IsAuthorized;
	String m_Name;
	String m_Ip;
	String m_AuthID;
	AdminId m_Admin;
	bool m_TempAdmin;
	edict_t *m_pEdict;
};

class CPlayerManager : 
	public SMGlobalClass,
	public IPlayerManager
{
public:
	CPlayerManager();
	~CPlayerManager();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
public:
	CPlayer *GetPlayerByIndex(int client) const;
	void RunAuthChecks();
	void ClearAdminId(AdminId id);
	void ClearAllAdmins();
public:
	bool OnClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
	bool OnClientConnect_Post(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
	void OnClientPutInServer(edict_t *pEntity, char const *playername);
	void OnClientDisconnect(edict_t *pEntity);
	void OnClientDisconnect_Post(edict_t *pEntity);
	void OnClientCommand(edict_t *pEntity);
	void OnClientSettingsChanged(edict_t *pEntity);
public: //IPlayerManager
	void AddClientListener(IClientListener *listener);
	void RemoveClientListener(IClientListener *listener);
	IGamePlayer *GetGamePlayer(int client);
	IGamePlayer *GetGamePlayer(edict_t *pEdict);
	int GetMaxClients();
	int GetNumPlayers();
public:
	inline int MaxClients()
	{
		return m_maxClients;
	}
	inline int NumPlayers()
	{
		return m_PlayerCount;
	}
private:
	void OnServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
private:
	List<IClientListener *> m_hooks;
	IForward *m_clconnect;
	IForward *m_cldisconnect;
	IForward *m_cldisconnect_post;
	IForward *m_clputinserver;
	IForward *m_clcommand;
	IForward *m_clinfochanged;
	IForward *m_clauth;
	CPlayer *m_Players;
	int m_maxClients;
	int m_PlayerCount;
	bool m_FirstPass;
	unsigned int *m_AuthQueue;
};

extern CPlayerManager g_Players;

#endif //_INCLUDE_SOURCEMOD_CPLAYERMANAGER_H_
