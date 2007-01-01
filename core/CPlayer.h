#ifndef _INCLUDE_SOURCEMOD_CPLAYER_H_
#define _INCLUDE_SOURCEMOD_CPLAYER_H_

#include <sh_string.h>
#include <eiface.h>

using namespace SourceHook;

class CPlayer
{
	friend class CPlayerManager;
public:
	CPlayer();
public:
	const char *PlayerName() const;
	const char *PlayerIP() const;
	const char *PlayerAuthString() const;
	edict_t *GetPlayerEdict() const;
	bool IsPlayerInGame() const;
	bool IsPlayerConnected() const;
	bool IsPlayerAuthorized() const;
	bool IsPlayerFakeClient() const;
	//:TODO: is user alive function
private:
	void Initialize(const char *name, const char *ip, edict_t *pEntity);
	void Connect();
	void Authorize(const char *steamid);
	void Disconnect();
	void SetName(const char *name);
private:
	bool m_IsConnected;
	bool m_IsInGame;
	bool m_IsAuthorized;
	String m_Name;
	String m_Ip;
	String m_AuthID;
	edict_t *m_PlayerEdict;
};

inline const char *CPlayer::PlayerName() const
{
	return m_Name.c_str();
}

inline const char *CPlayer::PlayerIP() const
{
	return m_Ip.c_str();
}

inline const char *CPlayer::PlayerAuthString() const
{
	return m_AuthID.c_str();
}

inline edict_t *CPlayer::GetPlayerEdict() const
{
	return m_PlayerEdict;
}

inline bool CPlayer::IsPlayerInGame() const
{
	return m_IsInGame;
}

inline bool CPlayer::IsPlayerConnected() const
{
	return m_IsConnected;
}

inline bool CPlayer::IsPlayerAuthorized() const
{
	return m_IsAuthorized;
}

inline bool CPlayer::IsPlayerFakeClient() const
{
	return (strcmp(m_AuthID.c_str(), "BOT") == 0);
}

#endif // _INCLUDE_SOURCEMOD_CPLAYER_H_
