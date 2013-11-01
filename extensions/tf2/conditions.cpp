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

typedef struct condflags_s {
	uint64_t upper;
	uint64_t lower;
} condflags_t;
condflags_t condflags_empty = { 0, 0 };

condflags_t g_PlayerConds[MAXPLAYERS+1];

IForward *g_addCondForward = NULL;
IForward *g_removeCondForward = NULL;

int playerCondOffset = -1;
int playerCondExOffset = -1;
int playerCondEx2Offset = -1;
int conditionBitsOffset = -1;

bool g_bIgnoreRemove;

#define MAX_CONDS (sizeof(uint64_t) * 8)

inline condflags_t GetPlayerConds(CBaseEntity *pPlayer)
{
	condflags_t result;
	uint32_t playerCond    =  *(uint32_t *)((intptr_t)pPlayer + playerCondOffset);
	uint32_t condBits      =  *(uint32_t *)((intptr_t)pPlayer + conditionBitsOffset);
	uint32_t playerCondEx  =  *(uint32_t *)((intptr_t)pPlayer + playerCondExOffset);
	uint32_t playerCondEx2 =  *(uint32_t *)((intptr_t)pPlayer + playerCondEx2Offset);

	uint64_t playerCondExAdj = playerCondEx;
	playerCondExAdj <<= 32;

	result.lower = playerCond|condBits|playerCondExAdj;
	result.upper = playerCondEx2;
	return result;
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

		if (oldconds.lower == newconds.lower && oldconds.upper == newconds.upper)
			continue;

		addedconds.lower = newconds.lower &~ oldconds.lower;
		removedconds.lower = oldconds.lower &~ newconds.lower;
		addedconds.upper = newconds.upper &~ oldconds.upper;
		removedconds.upper = oldconds.upper &~ newconds.upper;

		uint64_t j;
		uint64_t bit;

		uint64_t maxbit = MAX(addedconds.lower, addedconds.upper);
		for (j = 0; j < MAX_CONDS && (bit = ((uint64_t)1 << j)) <= maxbit; j++)
		{
			if ((addedconds.lower & bit) == bit)
			{
				g_addCondForward->PushCell(i);
				g_addCondForward->PushCell(j & 0xFFFFFFFF);
				g_addCondForward->Execute(NULL, NULL);
			}
			if ((addedconds.upper & bit) == bit)
			{
				g_addCondForward->PushCell(i);
				g_addCondForward->PushCell((j+64) & 0xFFFFFFFF);
				g_addCondForward->Execute(NULL, NULL);
			}
		}

		maxbit = MAX(removedconds.lower, removedconds.upper);
		for (j = 0; j < MAX_CONDS && (bit = ((uint64_t)1 << j)) <= maxbit; j++)
		{
			if ((removedconds.lower & bit) == bit)
			{
				g_removeCondForward->PushCell(i);
				g_removeCondForward->PushCell(j & 0xFFFFFFFF);
				g_removeCondForward->Execute(NULL, NULL);
			}
			if ((removedconds.upper & bit) == bit)
			{
				g_removeCondForward->PushCell(i);
				g_removeCondForward->PushCell((j+64) & 0xFFFFFFFF);
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

	if (!gamehelpers->FindSendPropInfo("CTFPlayer", "m_nPlayerCondEx2", &prop))
	{
		g_pSM->LogError(myself, "Failed to find m_nPlayerCondEx2 prop offset");
		return false;
	}
	
	playerCondEx2Offset = prop.actual_offset;

	if (playerCondOffset == -1 || playerCondExOffset == -1 || conditionBitsOffset == -1 || playerCondEx2Offset == -1)
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
	g_PlayerConds[client] = condflags_empty;
}

void RemoveConditionChecks()
{
	g_pSM->RemoveGameFrameHook(Conditions_OnGameFrame);
}

