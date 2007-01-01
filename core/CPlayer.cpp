#include "CPlayer.h"

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
