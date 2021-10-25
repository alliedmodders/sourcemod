/**
 * vim: set ts=4 sts=8 sw=4 tw=99 noet :
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

#ifndef _INCLUDE_SOURCEMOD_CPLAYERMANAGER_H_
#define _INCLUDE_SOURCEMOD_CPLAYERMANAGER_H_

#include "sm_globals.h"
#include <eiface.h>
#include "sourcemm_api.h"
#include <IForwardSys.h>
#include <IPlayerHelpers.h>
#include <IAdminSystem.h>
#include <ITranslator.h>
#include <sh_string.h>
#include <sh_list.h>
#include <sh_vector.h>
#include <am-string.h>
#include <am-deque.h>
#include "ConVarManager.h"

#include <steam/steamclientpublic.h>

using namespace SourceHook;

class IClient;

#define PLAYER_LIFE_UNKNOWN	0
#define PLAYER_LIFE_ALIVE	1
#define PLAYER_LIFE_DEAD	2

#define MIN_API_FOR_ADMINCALLS		7

union serial_t
{
	uint32_t   value;
	struct
	{
		uint32_t index  :  8;
		uint32_t serial : 24;
	} bits;
};

class CPlayer : public IGamePlayer
{
	friend class PlayerManager;
public:
	CPlayer();
public:
	const char *GetName();
	const char *GetIPAddress();
	const char *GetAuthString(bool validated = true);
	unsigned int GetSteamAccountID(bool validated = true);
	const CSteamID &GetSteamId(bool validated = true);
	uint64_t GetSteamId64(bool validated = true) { return GetSteamId(validated).ConvertToUint64(); }
	const char *GetSteam2Id(bool validated = true);
	const char *GetSteam3Id(bool validated = true);
	edict_t *GetEdict();
	bool IsInGame();
	bool WasCountedAsInGame();
	bool IsConnected();
	bool IsAuthorized();
	bool IsFakeClient();
	bool IsSourceTV() const;
	bool IsReplay() const;
	void SetAdminId(AdminId id, bool temporary);
	AdminId GetAdminId();
	void Kick(const char *str);
	bool IsInKickQueue();
	IPlayerInfo *GetPlayerInfo();
	unsigned int GetLanguageId();
	void SetLanguageId(unsigned int id);
	int GetUserId();
	bool RunAdminCacheChecks();
	void NotifyPostAdminChecks();
	unsigned int GetSerial();
	int GetIndex() const;
	void PrintToConsole(const char *pMsg);
	void ClearAdmin();
public:
	void DoBasicAdminChecks();
	void MarkAsBeingKicked();
	int GetLifeState();

	// This can be NULL for fakeclients due to limitations in our impl
	IClient *GetIClient() const;
private:
	void Initialize(const char *name, const char *ip, edict_t *pEntity);
	void Connect();
	void Disconnect();
	void SetName(const char *name);
	void DumpAdmin(bool deleting);
	void UpdateAuthIds();
	void Authorize();
	void Authorize_Post();
	void DoPostConnectAuthorization();
	bool IsAuthStringValidated();
	bool SetEngineString();
	bool SetCSteamID();
	void ClearNetchannelQueue(void);
private:
	bool m_IsConnected = false;
	bool m_IsInGame = false;
	bool m_IsAuthorized = false;
	bool m_bIsInKickQueue = false;
	String m_Name;
	String m_Ip;
	String m_IpNoPort;
	std::string m_AuthID;
	std::string m_Steam2Id;
	std::string m_Steam3Id;
	AdminId m_Admin = INVALID_ADMIN_ID;
	bool m_TempAdmin = false;
	edict_t *m_pEdict = nullptr;
	IPlayerInfo *m_Info = nullptr;
	IClient *m_pIClient = nullptr;
	String m_LastPassword;
	bool m_bAdminCheckSignalled = false;
	int m_iIndex;
	unsigned int m_LangId = SOURCEMOD_LANGUAGE_ENGLISH;
	int m_UserId = -1;
	bool m_bFakeClient = false;
	bool m_bIsSourceTV = false;
	bool m_bIsReplay = false;
	serial_t m_Serial;
	CSteamID m_SteamId;
#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	QueryCvarCookie_t m_LanguageCookie = InvalidQueryCvarCookie;
#endif
	std::deque<std::string> m_PrintfBuffer;
};

class PlayerManager : 
	public SMGlobalClass,
	public IPlayerManager
{
	friend class CPlayer;
public:
	PlayerManager();
	~PlayerManager();
public: //SMGlobalClass
	void OnSourceModStartup(bool late) override;
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
	void OnSourceModLevelEnd();
	ConfigResult OnSourceModConfigChanged(const char *key, const char *value, ConfigSource source, char *error, size_t maxlength);
	void OnSourceModMaxPlayersChanged(int newvalue);
public:
	CPlayer *GetPlayerByIndex(int client) const;
	void RunAuthChecks();
public:
	bool OnClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
	bool OnClientConnect_Post(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
	void OnClientPutInServer(edict_t *pEntity, char const *playername);
	void OnClientDisconnect(edict_t *pEntity);
	void OnClientDisconnect_Post(edict_t *pEntity);
#if SOURCE_ENGINE >= SE_ORANGEBOX
	void OnClientCommand(edict_t *pEntity, const CCommand &args);
#if SOURCE_ENGINE >= SE_EYE
	void OnClientCommandKeyValues(edict_t *pEntity, KeyValues *pCommand);
	void OnClientCommandKeyValues_Post(edict_t *pEntity, KeyValues *pCommand);
#endif
#else
	void OnClientCommand(edict_t *pEntity);
#endif
	void OnClientSettingsChanged(edict_t *pEntity);
	//void OnClientSettingsChanged_Pre(edict_t *pEntity);
	void OnClientLanguageChanged(int client, unsigned int language);
	void OnServerHibernationUpdate(bool bHibernating);
	void OnClientPrintf(edict_t *pEdict, const char *szMsg);
	void OnPrintfFrameAction(unsigned int serial);
public: //IPlayerManager
	void AddClientListener(IClientListener *listener);
	void RemoveClientListener(IClientListener *listener);
	IGamePlayer *GetGamePlayer(int client);
	IGamePlayer *GetGamePlayer(edict_t *pEdict);
	int GetMaxClients();
	int GetNumPlayers();
	int GetClientOfUserId(int userid);
	bool IsServerActivated();
	int FilterCommandTarget(IGamePlayer *pAdmin, IGamePlayer *pTarget, int flags);
	int InternalFilterCommandTarget(CPlayer *pAdmin, CPlayer *pTarget, int flags);
	void RegisterCommandTargetProcessor(ICommandTargetProcessor *pHandler);
	void UnregisterCommandTargetProcessor(ICommandTargetProcessor *pHandler);
	void ProcessCommandTarget(cmd_target_info_t *info);
	int GetClientFromSerial(unsigned int serial);
	void ClearAdminId(AdminId id);
	void RecheckAnyAdmins();
public:
	inline int MaxClients()
	{
		return m_maxClients;
	}
	inline int NumPlayers()
	{
		return m_PlayerCount;
	}
	inline int ListenClient()
	{
		return m_ListenClient;
	}
	bool CheckSetAdmin(int index, CPlayer *pPlayer, AdminId id);
	bool CheckSetAdminName(int index, CPlayer *pPlayer, AdminId id);
	const char *GetPassInfoVar();
	unsigned int GetReplyTo();
	unsigned int SetReplyTo(unsigned int reply);
	void MaxPlayersChanged(int newvalue = -1);
	inline bool InClientCommandKeyValuesHook()
	{
		return m_bInCCKVHook;
	}
#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	bool HandleConVarQuery(QueryCvarCookie_t cookie, int client, EQueryCvarValueStatus result, const char *cvarName, const char *cvarValue);
#endif
private:
	void OnServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
	void InvalidatePlayer(CPlayer *pPlayer);
private:
	List<IClientListener *> m_hooks;
	IForward *m_clconnect;
	IForward *m_clconnect_post;
	IForward *m_cldisconnect;
	IForward *m_cldisconnect_post;
	IForward *m_clputinserver;
	IForward *m_clcommand;
	IForward *m_clcommandkv;
	IForward *m_clcommandkv_post;
	IForward *m_clinfochanged;
	IForward *m_clauth;
	IForward *m_cllang;
	IForward *m_onActivate;
	IForward *m_onActivate2;
	CPlayer *m_Players;
	int *m_UserIdLookUp;
	int m_maxClients;
	int m_PlayerCount;
	int m_PlayersSinceActive;
	bool m_bServerActivated;
	unsigned int *m_AuthQueue;
	String m_PassInfoVar;
	bool m_QueryLang;
	bool m_bAuthstringValidation; // are we validating admins with steam before authorizing?
	bool m_bIsListenServer;
	int m_ListenClient;
	bool m_bIsSourceTVActive;
	bool m_bIsReplayActive;
	int m_SourceTVUserId;
	int m_ReplayUserId;
	bool m_bInCCKVHook;
private:
	static const int NETMSG_TYPE_BITS = 5; // SVC_Print overhead for netmsg type
	static const int SVC_Print_BufferSize = 2048 - 1; // -1 for terminating \0
};

#if SOURCE_ENGINE >= SE_ORANGEBOX
void CmdMaxplayersCallback(const CCommand &command);
#else
void CmdMaxplayersCallback();
#endif

extern void ClientConsolePrint(edict_t *e, const char *fmt, ...);

extern PlayerManager g_Players;
extern bool g_OnMapStarted;
extern const unsigned int *g_NumPlayersToAuth;

#endif //_INCLUDE_SOURCEMOD_CPLAYERMANAGER_H_
