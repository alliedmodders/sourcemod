#ifndef _INCLUDE_SOURCEMOD_CPLAYERMANAGER_H_
#define _INCLUDE_SOURCEMOD_CPLAYERMANAGER_H_

#include "sm_globals.h"
#include <eiface.h>
#include "sourcemm_api.h"
#include <IForwardSys.h>
#include "CPlayer.h"

class CPlayer;

class CPlayerManager : public SMGlobalClass
{
public:
	CPlayerManager() : m_FirstPass(true) {}
public: //SMGlobalClass
	virtual void OnSourceModAllInitialized();
	virtual void OnSourceModShutdown();
public:
	int GetMaxClients() const;
	CPlayer *GetPlayerByIndex(int client) const;
	int GetPlayerCount() const;
public:
	bool OnClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
	void OnClientPutInServer(edict_t *pEntity, char const *playername);
	void OnClientDisconnect(edict_t *pEntity);
	void OnClientDisconnect_Post(edict_t *pEntity);
	void OnClientAuthorized(); //:TODO: any args needed?
	void OnClientCommand(edict_t *pEntity);
	void OnClientSettingsChanged(edict_t *pEntity);
private:
	void OnServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
private:
	IForward *m_clconnect;
	IForward *m_cldisconnect;
	IForward *m_cldisconnect_post;
	IForward *m_clputinserver;
	IForward *m_clcommand;
	IForward *m_clinfochanged;
	CPlayer *m_Players;
	int m_maxClients;
	int m_PlayerCount;
	bool m_FirstPass;
};

extern CPlayerManager g_PlayerManager;

inline int CPlayerManager::GetMaxClients() const
{
	return m_maxClients;
}

inline CPlayer *CPlayerManager::GetPlayerByIndex(int client) const
{
	if (client > m_maxClients || client < 0)
	{
		return NULL;
	}
	return &m_Players[client];
}

inline int CPlayerManager::GetPlayerCount() const
{
	return m_PlayerCount;
}

#endif //_INCLUDE_SOURCEMOD_CPLAYERMANAGER_H_
