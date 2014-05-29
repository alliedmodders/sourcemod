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
#include "sm_stringutil.h"
#include "HalfLife2.h"
#include "sourcemod.h"
#include "ChatTriggers.h"
#include <inetchannel.h>
#include <iclient.h>

static cell_t sm_GetMaxHumanPlayers(IPluginContext *pCtx, const cell_t *params)
{
	int maxHumans = -1;

#if SOURCE_ENGINE >= SE_LEFT4DEAD
	maxHumans = serverClients->GetMaxHumanPlayers();
#endif

	if( maxHumans == -1 )
	{
		return g_Players.MaxClients();
	}
	
	return maxHumans;
}

static cell_t GetTimeConnected(IPluginContext *pContext, const cell_t *params)
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
	else if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);
	if (pInfo == NULL)
	{
		return 0;
	}

	return sp_ftoc(pInfo->GetTimeConnected());
}

static cell_t GetDataRate(IPluginContext *pContext, const cell_t *params)
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
	else if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);
	if (pInfo == NULL)
	{
		return 0;
	}

	return pInfo->GetDataRate();
}

static cell_t IsTimingOut(IPluginContext *pContext, const cell_t *params)
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
	else if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);
	if (pInfo == NULL)
	{
		return 1;
	}

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
	else if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}
	else if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);
	if (pInfo == NULL)
	{
		return sp_ftoc(-1);
	}

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
	else if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}
	else if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);
	if (pInfo == NULL)
	{
		return sp_ftoc(-1);
	}

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
	else if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}
	else if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);
	if (pInfo == NULL)
	{
		return sp_ftoc(-1);
	}

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
	else if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}
	else if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);
	if (pInfo == NULL)
	{
		return sp_ftoc(-1);
	}

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
	else if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}
	else if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);
	if (pInfo == NULL)
	{
		return 0;
	}

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
	else if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}
	else if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is a bot", client);
	}

	INetChannelInfo *pInfo = engine->GetPlayerNetInfo(client);
	if (pInfo == NULL)
	{
		return 0;
	}

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

REGISTER_NATIVES(playernatives)
{
	{"GetMaxHumanPlayers",		sm_GetMaxHumanPlayers},
	{"GetClientTime",			GetTimeConnected},
	{"GetClientDataRate",		GetDataRate},
	{"IsClientTimingOut",		IsTimingOut},
	{"GetClientLatency",		GetLatency},
	{"GetClientAvgLatency",		GetAvgLatency},
	{"GetClientAvgLoss",		GetAvgLoss},
	{"GetClientAvgChoke",		GetAvgChoke},
	{"GetClientAvgData",		GetAvgData},
	{"GetClientAvgPackets",		GetAvgPackets},
	{"RunAdminCacheChecks",		RunAdminCacheChecks},
	{NULL,						NULL}
};

