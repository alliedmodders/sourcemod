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
	friend class PlayerManager;
public:
	CPlayer();
public:
	const char *GetName();
	const char *GetIPAddress();
	const char *GetAuthString();
	edict_t *GetEdict();
	bool IsInGame();
	bool IsConnected();
	bool IsAuthorized();
	bool IsFakeClient();
	void SetAdminId(AdminId id, bool temporary);
	AdminId GetAdminId();
	void Kick(const char *str);
public:
	IPlayerInfo *GetPlayerInfo();
private:
	void Initialize(const char *name, const char *ip, edict_t *pEntity);
	void Connect();
	void Disconnect();
	void SetName(const char *name);
	void DumpAdmin(bool deleting);
	void Authorize(const char *auth);
	void Authorize_Post();
	void DoBasicAdminChecks();
private:
	bool m_IsConnected;
	bool m_IsInGame;
	bool m_IsAuthorized;
	String m_Name;
	String m_Ip;
	String m_IpNoPort;
	String m_AuthID;
	AdminId m_Admin;
	bool m_TempAdmin;
	edict_t *m_pEdict;
	IPlayerInfo *m_Info;
	String m_LastPassword;
};

class PlayerManager : 
	public SMGlobalClass,
	public IPlayerManager
{
public:
	PlayerManager();
	~PlayerManager();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
	void OnSourceModLevelEnd();
	ConfigResult OnSourceModConfigChanged(const char *key, const char *value, ConfigSource source, char *error, size_t maxlength);
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
	//void OnClientSettingsChanged_Pre(edict_t *pEntity);
public: //IPlayerManager
	void AddClientListener(IClientListener *listener);
	void RemoveClientListener(IClientListener *listener);
	IGamePlayer *GetGamePlayer(int client);
	IGamePlayer *GetGamePlayer(edict_t *pEdict);
	int GetMaxClients();
	int GetNumPlayers();
	int GetClientOfUserId(int userid);
public:
	inline int MaxClients()
	{
		return m_maxClients;
	}
	inline int NumPlayers()
	{
		return m_PlayerCount;
	}
	bool CheckSetAdmin(int index, CPlayer *pPlayer, AdminId id);
	bool CheckSetAdminName(int index, CPlayer *pPlayer, AdminId id);
	const char *GetPassInfoVar();
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
	IForward *m_onActivate;
	IForward *m_onActivate2;
	CPlayer *m_Players;
	int *m_UserIdLookUp;
	int m_maxClients;
	int m_PlayerCount;
	bool m_FirstPass;
	unsigned int *m_AuthQueue;
	String m_PassInfoVar;
};

extern PlayerManager g_Players;
extern bool g_OnMapStarted;

#endif //_INCLUDE_SOURCEMOD_CPLAYERMANAGER_H_
