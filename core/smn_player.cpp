/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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
#include "AdminCache.h"
#include "sm_stringutil.h"
#include "HalfLife2.h"
#include "sourcemod.h"
#include "ChatTriggers.h"
#include <inetchannel.h>
#include <iclient.h>

ConVar sm_show_activity("sm_show_activity", "13", FCVAR_SPONLY, "Activity display setting (see sourcemod.cfg)");

static cell_t sm_GetClientCount(IPluginContext *pCtx, const cell_t *params)
{
	if (params[1])
	{
		return g_Players.NumPlayers();
	}

	int maxplayers = g_Players.MaxClients();
	int count = 0;
	for (int i=1; i<=maxplayers; ++i)
	{
		CPlayer *pPlayer = g_Players.GetPlayerByIndex(i);
		if ((pPlayer->IsConnected()) && !(pPlayer->IsInGame()))
		{
			count++;
		}
	}

	return (g_Players.NumPlayers() + count);
}

static cell_t sm_GetMaxClients(IPluginContext *pCtx, const cell_t *params)
{
	return g_Players.MaxClients();
}

static cell_t sm_GetClientName(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];

	if (index == 0)
	{
		static ConVar *hostname = NULL;
		if (!hostname)
		{
			hostname = icvar->FindVar("hostname");
			if (!hostname)
			{
				return pCtx->ThrowNativeError("Could not find \"hostname\" cvar");
			}
		}
		pCtx->StringToLocalUTF8(params[2], static_cast<size_t>(params[3]), hostname->GetString(), NULL);
		return 1;
	}

	if ((index < 1) || (index > g_Players.GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(index);
	if (!pPlayer->IsConnected())
	{
		return pCtx->ThrowNativeError("Client %d is not connected", index);
	}

	pCtx->StringToLocalUTF8(params[2], static_cast<size_t>(params[3]), pPlayer->GetName(), NULL);
	return 1;
}

static cell_t sm_GetClientIP(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_Players.GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(index);
	if (!pPlayer->IsConnected())
	{
		return pCtx->ThrowNativeError("Client %d is not connected", index);
	}

	char buf[64], *ptr;
	strcpy(buf, pPlayer->GetIPAddress());

	if (params[4] && (ptr = strchr(buf, ':')))
	{
		*ptr = '\0';
	}

	pCtx->StringToLocal(params[2], static_cast<size_t>(params[3]), buf);
	return 1;
}

static cell_t sm_GetClientAuthStr(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_Players.GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(index);
	if (!pPlayer->IsConnected())
	{
		return pCtx->ThrowNativeError("Client %d is not connected", index);
	}

	if (!pPlayer->IsAuthorized())
	{
		return 0;
	}

	pCtx->StringToLocal(params[2], static_cast<size_t>(params[3]), pPlayer->GetAuthString());

	return 1;
}

static cell_t sm_IsClientConnected(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_Players.GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	return (g_Players.GetPlayerByIndex(index)->IsConnected()) ? 1 : 0;
}

static cell_t sm_IsClientInGame(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_Players.GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	return (g_Players.GetPlayerByIndex(index)->IsInGame()) ? 1 : 0;
}

static cell_t sm_IsClientAuthorized(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_Players.GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	return (g_Players.GetPlayerByIndex(index)->IsAuthorized()) ? 1 : 0;
}

static cell_t sm_IsClientFakeClient(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_Players.GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(index);
	if (!pPlayer->IsConnected())
	{
		return pCtx->ThrowNativeError("Client %d is not connected", index);
	}

	return (pPlayer->IsFakeClient()) ? 1 : 0;
}

static cell_t IsClientObserver(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	} else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	return pInfo->IsObserver() ? 1 : 0;
}

static cell_t sm_GetClientInfo(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	char *key;
	pContext->LocalToString(params[2], &key);

	const char *val = engine->GetClientConVarValue(client, key);
	if (!val)
	{
		return false;
	}

	pContext->StringToLocalUTF8(params[3], params[4], val, NULL);
	return 1;
}

static cell_t SetUserAdmin(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}
	if (!g_Admins.IsValidAdmin(params[2]) && params[2] != INVALID_ADMIN_ID)
	{
		return pContext->ThrowNativeError("AdminId %x is invalid", params[2]);
	}

	pPlayer->SetAdminId(params[2], params[3] ? true : false);

	return 1;
}

static cell_t GetUserAdmin(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	return pPlayer->GetAdminId();
}

static cell_t AddUserFlags(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	AdminId id;
	if ((id=pPlayer->GetAdminId()) == INVALID_ADMIN_ID)
	{
		id = g_Admins.CreateAdmin(NULL);
		pPlayer->SetAdminId(id, true);
	}

	cell_t *addr;
	for (int i=2; i<=params[0]; i++)
	{
		pContext->LocalToPhysAddr(params[i], &addr);
		g_Admins.SetAdminFlag(id, (AdminFlag)*addr, true);
	}

	return 1;
}

static cell_t RemoveUserFlags(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}
	
	AdminId id;
	if ((id=pPlayer->GetAdminId()) == INVALID_ADMIN_ID)
	{
		return 0;
	}

	cell_t *addr;
	for (int i=2; i<=params[0]; i++)
	{
		pContext->LocalToPhysAddr(params[i], &addr);
		g_Admins.SetAdminFlag(id, (AdminFlag)*addr, false);
	}

	return 1;
}

static cell_t SetUserFlagBits(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	AdminId id;
	if ((id=pPlayer->GetAdminId()) == INVALID_ADMIN_ID)
	{
		id = g_Admins.CreateAdmin(NULL);
		pPlayer->SetAdminId(id, true);
	}

	g_Admins.SetAdminFlags(id, Access_Effective, params[2]);

	return 1;
}

static cell_t GetUserFlagBits(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	AdminId id;
	if ((id=pPlayer->GetAdminId()) == INVALID_ADMIN_ID)
	{
		return 0;
	}

	return g_Admins.GetAdminFlags(id, Access_Effective);
}

static cell_t GetClientUserId(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	return engine->GetPlayerUserId(pPlayer->GetEdict());
}

static cell_t CanUserTarget(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	int target = params[2];

	if (client == 0)
	{
		return 1;
	}

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	} else if (!pPlayer->IsConnected()) {
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	CPlayer *pTarget = g_Players.GetPlayerByIndex(target);
	if (!pTarget)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", target);
	} else if (!pTarget->IsConnected()) {
		return pContext->ThrowNativeError("Client %d is not connected", target);
	}

	return g_Admins.CanAdminTarget(pPlayer->GetAdminId(), pTarget->GetAdminId()) ? 1 : 0;
}

static cell_t GetClientTeam(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	} else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	return pInfo->GetTeamIndex();
}

static cell_t GetFragCount(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	} else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	return pInfo->GetFragCount();
}

static cell_t GetDeathCount(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	} else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	return pInfo->GetDeathCount();
}

static cell_t GetArmorValue(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	} else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	return pInfo->GetArmorValue();
}

static cell_t GetAbsOrigin(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	} else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	cell_t *pVec;
	pContext->LocalToPhysAddr(params[2], &pVec);

	Vector vec = pInfo->GetAbsOrigin();
	pVec[0] = sp_ftoc(vec.x);
	pVec[1] = sp_ftoc(vec.y);
	pVec[2] = sp_ftoc(vec.z);

	return 1;
}

static cell_t GetAbsAngles(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	} else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	cell_t *pAng;
	pContext->LocalToPhysAddr(params[2], &pAng);

	QAngle ang = pInfo->GetAbsAngles();
	pAng[0] = sp_ftoc(ang.x);
	pAng[1] = sp_ftoc(ang.y);
	pAng[2] = sp_ftoc(ang.z);

	return 1;
}

static cell_t GetPlayerMins(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	} else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	cell_t *pVec;
	pContext->LocalToPhysAddr(params[2], &pVec);

	Vector vec = pInfo->GetPlayerMins();
	pVec[0] = sp_ftoc(vec.x);
	pVec[1] = sp_ftoc(vec.y);
	pVec[2] = sp_ftoc(vec.z);

	return 1;
}

static cell_t GetPlayerMaxs(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	} else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	cell_t *pVec;
	pContext->LocalToPhysAddr(params[2], &pVec);

	Vector vec = pInfo->GetPlayerMaxs();
	pVec[0] = sp_ftoc(vec.x);
	pVec[1] = sp_ftoc(vec.y);
	pVec[2] = sp_ftoc(vec.z);

	return 1;
}

static cell_t GetWeaponName(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	} else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	const char *weapon = pInfo->GetWeaponName();
	pContext->StringToLocalUTF8(params[2], static_cast<size_t>(params[3]), weapon ? weapon : "", NULL);

	return 1;
}

static cell_t GetModelName(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	} else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	const char *model = pInfo->GetModelName();
	pContext->StringToLocalUTF8(params[2], static_cast<size_t>(params[3]), model ? model : "", NULL);

	return 1;
}

static cell_t GetHealth(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	} else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	return pInfo->GetHealth();
}

static cell_t GetTimeConnected(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	} else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	} else if (pPlayer->IsFakeClient()) {
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);

	return sp_ftoc(pInfo->GetTimeConnected());
}

static cell_t GetDataRate(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	} else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	} else if (pPlayer->IsFakeClient()) {
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);

	return pInfo->GetDataRate();
}

static cell_t IsTimingOut(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	} else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	} else if (pPlayer->IsFakeClient()) {
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);

	return pInfo->IsTimingOut() ? 1 : 0;
}

static cell_t GetLatency(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	float value;

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}
	else if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);

	if (params[2] == MAX_FLOWS)
	{
		value = pInfo->GetLatency(FLOW_INCOMING) + pInfo->GetLatency(FLOW_OUTGOING);
	}
	else
	{
		value = pInfo->GetLatency(params[2]);
	}

	return sp_ftoc(value);
}

static cell_t GetAvgLatency(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	float value;

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}
	else if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);

	if (params[2] == MAX_FLOWS)
	{
		value = pInfo->GetAvgLatency(FLOW_INCOMING) + pInfo->GetAvgLatency(FLOW_OUTGOING);
	}
	else
	{
		value = pInfo->GetAvgLatency(params[2]);
	}

	return sp_ftoc(value);
}

static cell_t GetAvgLoss(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	float value;

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}
	else if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);

	if (params[2] == MAX_FLOWS)
	{
		value = pInfo->GetAvgLoss(FLOW_INCOMING) + pInfo->GetAvgLoss(FLOW_OUTGOING);
	}
	else
	{
		value = pInfo->GetAvgLoss(params[2]);
	}

	return sp_ftoc(value);
}

static cell_t GetAvgChoke(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	float value;

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}
	else if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);

	if (params[2] == MAX_FLOWS)
	{
		value = pInfo->GetAvgChoke(FLOW_INCOMING) + pInfo->GetAvgChoke(FLOW_OUTGOING);
	}
	else
	{
		value = pInfo->GetAvgChoke(params[2]);
	}

	return sp_ftoc(value);
}

static cell_t GetAvgData(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	float value;

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}
	else if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);

	if (params[2] == MAX_FLOWS)
	{
		value = pInfo->GetAvgData(FLOW_INCOMING) + pInfo->GetAvgData(FLOW_OUTGOING);
	}
	else
	{
		value = pInfo->GetAvgData(params[2]);
	}

	return sp_ftoc(value);
}

static cell_t GetAvgPackets(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	float value;

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}
	else if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);

	if (params[2] == MAX_FLOWS)
	{
		value = pInfo->GetAvgPackets(FLOW_INCOMING) + pInfo->GetAvgPackets(FLOW_OUTGOING);
	}
	else
	{
		value = pInfo->GetAvgPackets(params[2]);
	}

	return sp_ftoc(value);
}

static cell_t GetClientOfUserId(IPluginContext *pContext, const cell_t *params)
{
	return g_Players.GetClientOfUserId(params[1]);
}

static cell_t _ShowActivity(IPluginContext *pContext,
							const cell_t *params,
							const char *tag,
							cell_t fmt_param)
{
	char message[255];
	char buffer[255];
	int value = sm_show_activity.GetInt();
	unsigned int replyto = g_ChatTriggers.GetReplyTo();
	int client = params[1];

	const char *name = "Console";
	const char *sign = "ADMIN";
	bool display_in_chat = false;
	if (client != 0)
	{
		CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
		if (!pPlayer || !pPlayer->IsConnected())
		{
			return pContext->ThrowNativeError("Client index %d is invalid", client);
		}
		name = pPlayer->GetName();
		AdminId id = pPlayer->GetAdminId();
		if (id == INVALID_ADMIN_ID
			|| !g_Admins.GetAdminFlag(id, Admin_Generic, Access_Effective))
		{
			sign = "PLAYER";
		}

		/* Display the message to the client? */
		if (replyto == SM_REPLY_CONSOLE)
		{
			g_SourceMod.SetGlobalTarget(client);
			g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, fmt_param);

			if (pContext->GetLastNativeError() != SP_ERROR_NONE)
			{
				return 0;
			}

			UTIL_Format(message, sizeof(message), "%s%s\n", tag, buffer);
			engine->ClientPrintf(pPlayer->GetEdict(), message);
			display_in_chat = true;
		}
	}
	else
	{
		g_SourceMod.SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);
		g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, fmt_param);

		if (pContext->GetLastNativeError() != SP_ERROR_NONE)
		{
			return 0;
		}

		UTIL_Format(message, sizeof(message), "%s%s\n", tag, buffer);
		META_CONPRINT(message);
	}

	if (!value)
	{
		return 1;
	}

	int maxClients = g_Players.GetMaxClients();
	for (int i=1; i<=maxClients; i++)
	{
		CPlayer *pPlayer = g_Players.GetPlayerByIndex(i);
		if (!pPlayer->IsInGame() 
			|| pPlayer->IsFakeClient()
			|| (display_in_chat && i == client))
		{
			continue;
		}
		AdminId id = pPlayer->GetAdminId();
		g_SourceMod.SetGlobalTarget(i);
		if (id == INVALID_ADMIN_ID
			|| !g_Admins.GetAdminFlag(id, Admin_Generic, Access_Effective))
		{
			/* Treat this as a normal user */
			if ((value & 1) || (value & 2))
			{
				const char *newsign = sign;
				if ((value & 2) || (i == client))
				{
					newsign = name;
				}
				g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, fmt_param);

				if (pContext->GetLastNativeError() != SP_ERROR_NONE)
				{
					return 0;
				}

				UTIL_Format(message, sizeof(message), "%s%s: %s", tag, newsign, buffer);
				g_HL2.TextMsg(i, HUD_PRINTTALK, message);
			}
		}
		else
		{
			/* Treat this as an admin user */
			bool is_root = g_Admins.GetAdminFlag(id, Admin_Root, Access_Effective);
			if ((value & 4) 
				|| (value & 8)
				|| ((value & 16) && is_root))
			{
				const char *newsign = sign;
				if ((value & 8) || ((value & 16) && is_root) || (i == client))
				{
					newsign = name;
				}
				g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, fmt_param);

				if (pContext->GetLastNativeError() != SP_ERROR_NONE)
				{
					return 0;
				}

				UTIL_Format(message, sizeof(message), "%s%s: %s", tag, newsign, buffer);
				g_HL2.TextMsg(i, HUD_PRINTTALK, message);
			}
		}
	}

	return 1;
}

static cell_t _ShowActivity2(IPluginContext *pContext,
							 const cell_t *params,
							 const char *tag,
							 cell_t fmt_param)
{
	char message[255];
	char buffer[255];
	int value = sm_show_activity.GetInt();
	unsigned int replyto = g_ChatTriggers.GetReplyTo();
	int client = params[1];

	const char *name = "Console";
	const char *sign = "ADMIN";
	if (client != 0)
	{
		CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
		if (!pPlayer || !pPlayer->IsConnected())
		{
			return pContext->ThrowNativeError("Client index %d is invalid", client);
		}
		name = pPlayer->GetName();
		AdminId id = pPlayer->GetAdminId();
		if (id == INVALID_ADMIN_ID
			|| !g_Admins.GetAdminFlag(id, Admin_Generic, Access_Effective))
		{
			sign = "PLAYER";
		}

		g_SourceMod.SetGlobalTarget(client);
		g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, fmt_param);

		if (pContext->GetLastNativeError() != SP_ERROR_NONE)
		{
			return 0;
		}

		/* We don't display directly to the console because the chat text 
		 * simply gets added to the console, so we don't want it to print 
		 * twice.
		 */
		if (replyto == SM_REPLY_CONSOLE)
		{
#if 0
			UTIL_Format(message, sizeof(message), "%s%s\n", tag, buffer);
			engine->ClientPrintf(pPlayer->GetEdict(), message);
#endif
			UTIL_Format(message, sizeof(message), "%s%s", tag, buffer);
			g_HL2.TextMsg(client, HUD_PRINTTALK, message);
		}
		else
		{
			UTIL_Format(message, sizeof(message), "%s%s", tag, buffer);
			g_HL2.TextMsg(client, HUD_PRINTTALK, message);
		}
	}
	else
	{
		g_SourceMod.SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);
		g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, fmt_param);

		if (pContext->GetLastNativeError() != SP_ERROR_NONE)
		{
			return 0;
		}

		UTIL_Format(message, sizeof(message), "%s%s\n", tag, buffer);
		META_CONPRINT(message);
	}

	if (!value)
	{
		return 1;
	}

	int maxClients = g_Players.GetMaxClients();
	for (int i=1; i<=maxClients; i++)
	{
		CPlayer *pPlayer = g_Players.GetPlayerByIndex(i);
		if (!pPlayer->IsInGame() 
			|| pPlayer->IsFakeClient()
			|| i == client)
		{
			continue;
		}
		AdminId id = pPlayer->GetAdminId();
		g_SourceMod.SetGlobalTarget(i);
		if (id == INVALID_ADMIN_ID
			|| !g_Admins.GetAdminFlag(id, Admin_Generic, Access_Effective))
		{
			/* Treat this as a normal user */
			if ((value & 1) || (value & 2))
			{
				const char *newsign = sign;
				if ((value & 2))
				{
					newsign = name;
				}
				g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, fmt_param);

				if (pContext->GetLastNativeError() != SP_ERROR_NONE)
				{
					return 0;
				}

				UTIL_Format(message, sizeof(message), "%s%s: %s", tag, newsign, buffer);
				g_HL2.TextMsg(i, HUD_PRINTTALK, message);
			}
		}
		else
		{
			/* Treat this as an admin user */
			bool is_root = g_Admins.GetAdminFlag(id, Admin_Root, Access_Effective);
			if ((value & 4) 
				|| (value & 8)
				|| ((value & 16) && is_root))
			{
				const char *newsign = sign;
				if ((value & 8) || ((value & 16) && is_root))
				{
					newsign = name;
				}
				g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, fmt_param);

				if (pContext->GetLastNativeError() != SP_ERROR_NONE)
				{
					return 0;
				}

				UTIL_Format(message, sizeof(message), "%s%s: %s", tag, newsign, buffer);
				g_HL2.TextMsg(i, HUD_PRINTTALK, message);
			}
		}
	}

	return 1;
}

static cell_t ShowActivity(IPluginContext *pContext, const cell_t *params)
{
	return _ShowActivity(pContext, params, "[SM] ", 2);
}

static cell_t ShowActivityEx(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	pContext->LocalToString(params[2], &str);

	return _ShowActivity(pContext, params, str, 3);
}

static cell_t ShowActivity2(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	pContext->LocalToString(params[2], &str);

	return _ShowActivity2(pContext, params, str, 3);
}

static cell_t KickClient(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	/* Ignore duplicate kicks */
	if (pPlayer->IsInKickQueue())
	{
		return 1;
	}

	pPlayer->MarkAsBeingKicked();

	if (pPlayer->IsFakeClient())
	{
		char kickcmd[40];
		UTIL_Format(kickcmd, sizeof(kickcmd), "kick %s\n", pPlayer->GetName());

		engine->ServerCommand(kickcmd);
		return 1;
	}

	g_SourceMod.SetGlobalTarget(client);

	char buffer[256];
	g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, 2);

	if (pContext->GetLastNativeError() != SP_ERROR_NONE)
	{
		return 0;
	}

	g_HL2.AddDelayedKick(client, pPlayer->GetUserId(), buffer);

	return 1;
}

static cell_t KickClientEx(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	/* Ignore duplicate kicks */
	if (pPlayer->IsInKickQueue())
	{
		return 1;
	}

	pPlayer->MarkAsBeingKicked();

	if (pPlayer->IsFakeClient())
	{
		char kickcmd[40];
		UTIL_Format(kickcmd, sizeof(kickcmd), "kick %s\n", pPlayer->GetName());

		engine->ServerCommand(kickcmd);
		return 1;
	}

	g_SourceMod.SetGlobalTarget(client);

	char buffer[256];
	g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, 2);

	if (pContext->GetLastNativeError() != SP_ERROR_NONE)
	{
		return 0;
	}

	pPlayer->Kick(buffer);

	return 1;
}

static cell_t ChangeClientTeam(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	pInfo->ChangeTeam(params[2]);

	return 1;
}

static cell_t RunAdminCacheChecks(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}
	else if (!pPlayer->IsAuthorized())
	{
		return pContext->ThrowNativeError("Client %d is not authorized", client);
	}

	AdminId id = pPlayer->GetAdminId();
	pPlayer->DoBasicAdminChecks();

	return (id != pPlayer->GetAdminId()) ? 1 : 0;
}

static cell_t NotifyPostAdminCheck(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}
	else if (!pPlayer->IsAuthorized())
	{
		return pContext->ThrowNativeError("Client %d is not authorized", client);
	}

	pPlayer->NotifyPostAdminChecks();

	return 1;
}

static cell_t IsClientInKickQueue(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	return pPlayer->IsInKickQueue() ? 1 : 0;
}

static cell_t ProcessTargetString(IPluginContext *pContext, const cell_t *params)
{
	cmd_target_info_t info;

	pContext->LocalToString(params[1], (char **)&info.pattern);
	info.admin = params[2];
	pContext->LocalToPhysAddr(params[3], &info.targets);
	info.max_targets = params[4];
	info.flags = params[5];
	pContext->LocalToString(params[6], &info.target_name);
	info.target_name_maxlength = params[7];

	cell_t *tn_is_ml;
	pContext->LocalToPhysAddr(params[8], &tn_is_ml);

	g_Players.ProcessCommandTarget(&info);

	if (info.target_name_style == COMMAND_TARGETNAME_ML)
	{
		*tn_is_ml = 1;
	}
	else
	{
		*tn_is_ml = 0;
	}

	if (info.num_targets == 0)
	{
		return info.reason;
	}
	else
	{
		return info.num_targets;
	}
}

static cell_t FormatActivitySource(IPluginContext *pContext, const cell_t *params)
{
	int value;
	int client;
	int target;
	CPlayer *pTarget;
	AdminId aidTarget;
	const char *identity[2] = {"Console", "ADMIN"};

	client = params[1];
	target = params[2];

	if ((pTarget = g_Players.GetPlayerByIndex(target)) == NULL)
	{
		return pContext->ThrowNativeError("Invalid client index %d", target);
	}
	if (!pTarget->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d not connected", target);
	}

	value = sm_show_activity.GetInt();

	if (client != 0)
	{
		CPlayer *pPlayer;

		if ((pPlayer = g_Players.GetPlayerByIndex(client)) == NULL)
		{
			return pContext->ThrowNativeError("Invalid client index %d", client);
		}
		if (!pPlayer->IsConnected())
		{
			return pContext->ThrowNativeError("Client %d not connected", client);
		}

		identity[0] = pPlayer->GetName();

		AdminId id = pPlayer->GetAdminId();
		if (id == INVALID_ADMIN_ID
			|| !g_Admins.GetAdminFlag(id, Admin_Generic, Access_Effective))
		{
			identity[1] = "PLAYER";
		}
	}

	int mode = 1;
	bool bShowActivity = false;

	if ((aidTarget = pTarget->GetAdminId()) == INVALID_ADMIN_ID
		|| !g_Admins.GetAdminFlag(aidTarget, Admin_Generic, Access_Effective))
	{
		/* Treat this as a normal user */
		if ((value & 1) || (value & 2))
		{
			if ((value & 2) || (target == client))
			{
				mode = 0;
			}
			bShowActivity = true;
		}
	}
	else
	{
		/* Treat this as an admin user */
		bool is_root = g_Admins.GetAdminFlag(aidTarget, Admin_Root, Access_Effective);
		if ((value & 4) 
			|| (value & 8)
			|| ((value & 16) && is_root))
		{
			if ((value & 8) || ((value & 16) && is_root) || (target == client))
			{
				mode = 0;
			}
			bShowActivity = true;
		}
	}

	/* Otherwise, send it back to the script. */
	pContext->StringToLocalUTF8(params[3], params[4], identity[mode], NULL);

	return bShowActivity ? 1 : 0;
}

static cell_t sm_GetClientSerial(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}

	return pPlayer->GetSerial();
}

static cell_t sm_GetClientFromSerial(IPluginContext *pContext, const cell_t *params)
{
	return g_Players.GetClientFromSerial(params[1]);
}

REGISTER_NATIVES(playernatives)
{
	{"AddUserFlags",			AddUserFlags},
	{"CanUserTarget",			CanUserTarget},
	{"ChangeClientTeam",		ChangeClientTeam},
	{"GetClientAuthString",		sm_GetClientAuthStr},
	{"GetClientCount",			sm_GetClientCount},
	{"GetClientInfo",			sm_GetClientInfo},
	{"GetClientIP",				sm_GetClientIP},
	{"GetClientName",			sm_GetClientName},
	{"GetClientTeam",			GetClientTeam},
	{"GetClientUserId",			GetClientUserId},
	{"GetMaxClients",			sm_GetMaxClients},
	{"GetUserAdmin",			GetUserAdmin},
	{"GetUserFlagBits",			GetUserFlagBits},
	{"IsClientAuthorized",		sm_IsClientAuthorized},
	{"IsClientConnected",		sm_IsClientConnected},
	{"IsFakeClient",			sm_IsClientFakeClient},
	{"IsClientInGame",			sm_IsClientInGame},
	{"IsClientObserver",		IsClientObserver},
	{"RemoveUserFlags",			RemoveUserFlags},
	{"SetUserAdmin",			SetUserAdmin},
	{"SetUserFlagBits",			SetUserFlagBits},
	{"GetClientDeaths",			GetDeathCount},
	{"GetClientFrags",			GetFragCount},
	{"GetClientArmor",			GetArmorValue},
	{"GetClientAbsOrigin",		GetAbsOrigin},
	{"GetClientAbsAngles",		GetAbsAngles},
	{"GetClientMins",			GetPlayerMins},
	{"GetClientMaxs",			GetPlayerMaxs},
	{"GetClientWeapon",			GetWeaponName},
	{"GetClientModel",			GetModelName},
	{"GetClientHealth",			GetHealth},
	{"GetClientTime",			GetTimeConnected},
	{"GetClientDataRate",		GetDataRate},
	{"IsClientTimingOut",		IsTimingOut},
	{"GetClientLatency",		GetLatency},
	{"GetClientAvgLatency",		GetAvgLatency},
	{"GetClientAvgLoss",		GetAvgLoss},
	{"GetClientAvgChoke",		GetAvgChoke},
	{"GetClientAvgData",		GetAvgData},
	{"GetClientAvgPackets",		GetAvgPackets},
	{"GetClientOfUserId",		GetClientOfUserId},
	{"ShowActivity",			ShowActivity},
	{"ShowActivityEx",			ShowActivityEx},
	{"ShowActivity2",			ShowActivity2},
	{"KickClient",				KickClient},
	{"KickClientEx",			KickClientEx},
	{"RunAdminCacheChecks",		RunAdminCacheChecks},
	{"NotifyPostAdminCheck",	NotifyPostAdminCheck},
	{"IsClientInKickQueue",		IsClientInKickQueue},
	{"ProcessTargetString",		ProcessTargetString},
	{"FormatActivitySource",	FormatActivitySource},
	{"GetClientSerial",			sm_GetClientSerial},
	{"GetClientFromSerial",		sm_GetClientFromSerial},
	{NULL,						NULL}
};

