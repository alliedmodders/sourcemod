/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Team Fortress 2 Extension
 * Copyright (C) 2004-2011 AlliedModders LLC.  All rights reserved.
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

#include "extension.h"
#include "conditions.h"
#include "util.h"

#include <iplayerinfo.h>

typedef uint64_t condflags_t;

condflags_t g_PlayerConds[MAXPLAYERS+1];

IForward *g_addCondForward = NULL;
IForward *g_removeCondForward = NULL;

int playerCondOffset = -1;
int playerCondExOffset = -1;
int conditionBitsOffset = -1;

bool g_bIgnoreRemove;

inline condflags_t GetPlayerConds(CBaseEntity *pPlayer)
{
	uint32_t playerCond   =  *(uint32_t *)((intptr_t)pPlayer + playerCondOffset);
	uint32_t condBits     =  *(uint32_t *)((intptr_t)pPlayer + conditionBitsOffset);
	uint32_t playerCondEx =  *(uint32_t *)((intptr_t)pPlayer + playerCondExOffset);

	uint64_t playerCondExAdj = playerCondEx;
	playerCondExAdj <<= 32;

	return playerCond|condBits|playerCondExAdj;
}

void Conditions_OnGameFrame(bool simulating)
{
	if (!simulating)
		return;

	condflags_t oldconds;
	condflags_t newconds;
	
	condflags_t addedconds;
	condflags_t removedconds;

	int maxClients = gpGlobals->maxClients;
	for (int i = 1; i <= maxClients; i++)
	{
		IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(i);
		if (!pPlayer || !pPlayer->IsInGame())
			continue;

		IPlayerInfo *info = pPlayer->GetPlayerInfo();
		if (info->IsHLTV() || info->IsReplay())
			continue;

		CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(i);
		oldconds = g_PlayerConds[i];
		newconds = GetPlayerConds(pEntity);

		if (oldconds == newconds)
			continue;

		addedconds = newconds &~ oldconds;
		removedconds = oldconds &~ newconds;

		int j;
		condflags_t bit;

		for (j = 0; (bit = ((condflags_t)1 << j)) <= addedconds; j++)
		{
			if ((addedconds & bit) == bit)
			{
				g_addCondForward->PushCell(i);
				g_addCondForward->PushCell(j);
				g_addCondForward->Execute(NULL, NULL);
			}
		}

		for (j = 0; (bit = ((condflags_t)1 << j)) <= removedconds; j++)
		{
			if ((removedconds & bit) == bit)
			{
				g_removeCondForward->PushCell(i);
				g_removeCondForward->PushCell(j);
				g_removeCondForward->Execute(NULL, NULL);
			}
		}

		g_PlayerConds[i] = newconds;
	}
}

bool InitialiseConditionChecks()
{
	sm_sendprop_info_t prop;
	if (!gamehelpers->FindSendPropInfo("CTFPlayer", "m_nPlayerCond", &prop))
	{
		g_pSM->LogError(myself, "Failed to find m_nPlayerCond prop offset");
		return false;
	}
	
	playerCondOffset = prop.actual_offset;
	
	if (!gamehelpers->FindSendPropInfo("CTFPlayer", "_condition_bits", &prop))
	{
		g_pSM->LogError(myself, "Failed to find _condition_bits prop offset");
		return false;
	}

	conditionBitsOffset = prop.actual_offset;

	if (!gamehelpers->FindSendPropInfo("CTFPlayer", "m_nPlayerCondEx", &prop))
	{
		g_pSM->LogError(myself, "Failed to find m_nPlayerCondEx prop offset");
		return false;
	}
	
	playerCondExOffset = prop.actual_offset;

	if (playerCondOffset == -1 || playerCondExOffset == -1 || conditionBitsOffset == -1)
		return false;
	
	int maxClients = gpGlobals->maxClients;
	for (int i = 1; i <= maxClients; i++)
	{
		IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(i);
		if (!pPlayer || !pPlayer->IsInGame())
			continue;

		g_PlayerConds[i] = GetPlayerConds(gamehelpers->ReferenceToEntity(i));
	}

	g_pSM->AddGameFrameHook(Conditions_OnGameFrame);

	return true;
}

void Conditions_OnClientPutInServer(int client)
{
	g_PlayerConds[client] = 0;
}

void RemoveConditionChecks()
{
	g_pSM->RemoveGameFrameHook(Conditions_OnGameFrame);
}

