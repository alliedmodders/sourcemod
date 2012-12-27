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

#include "bitvec.h"

#include <iplayerinfo.h>

typedef CBitVec<96> condbits_t;

condbits_t g_PlayerConds[MAXPLAYERS+1];

IForward *g_addCondForward = NULL;
IForward *g_removeCondForward = NULL;

int playerCondOffset = -1;
int playerCondExOffset = -1;
int playerCondEx2Offset = -1;
int conditionBitsOffset = -1;

bool g_bIgnoreRemove;

inline void GetPlayerConds(CBaseEntity *pPlayer, condbits_t *pOut)
{
	uint32_t playerCond    =  *(uint32_t *)((intptr_t)pPlayer + playerCondOffset);
	uint32_t condBits      =  *(uint32_t *)((intptr_t)pPlayer + conditionBitsOffset);
	uint32_t playerCondEx  =  *(uint32_t *)((intptr_t)pPlayer + playerCondExOffset);
	uint32_t playerCondEx2 =  *(uint32_t *)((intptr_t)pPlayer + playerCondEx2Offset);

	pOut->SetDWord(0, playerCond);
	pOut->Set(0u, condBits);
	pOut->SetDWord(1, playerCondEx);
	pOut->SetDWord(2, playerCondEx2);
}

void Conditions_OnGameFrame(bool simulating)
{
	if (!simulating)
		return;

	condbits_t *oldconds;
	condbits_t newconds;
	
	condbits_t *addedconds;
	condbits_t *removedconds;

	int i, j;

	int maxClients = gpGlobals->maxClients;
	for (i = 1; i <= maxClients; i++)
	{
		IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(i);
		if (!pPlayer || !pPlayer->IsInGame())
			continue;

		IPlayerInfo *info = pPlayer->GetPlayerInfo();
		if (info->IsHLTV() || info->IsReplay())
			continue;

		CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(i);
		oldconds = &g_PlayerConds[i];
		GetPlayerConds(pEntity, &newconds);

		if (*oldconds == newconds)
			continue;

		addedconds = &newconds;
		for (j = 0; j < addedconds->GetNumDWords(); ++j)
		{
			addedconds->Clear(j * sizeof(int), oldconds->GetDWord(j));
		}

		removedconds = oldconds;
		for (j = 0; j < removedconds->GetNumDWords(); ++j)
		{
			removedconds->Clear(j * sizeof(int), newconds.GetDWord(j));
		}

		for (j = 0; (j = addedconds->FindNextSetBit( j )) != -1; ++j)
		{
			g_addCondForward->PushCell(i);
			g_addCondForward->PushCell(j);
			g_addCondForward->Execute(NULL, NULL);
		}

		for (j = 0; (j = addedconds->FindNextSetBit( j )) != -1; ++j)
		{
			g_removeCondForward->PushCell(i);
			g_removeCondForward->PushCell(j);
			g_removeCondForward->Execute(NULL, NULL);
		}

		g_PlayerConds[i].Copy( newconds );
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

	if (playerCondOffset == -1
		|| playerCondExOffset == -1
		|| playerCondEx2Offset == -1
		|| conditionBitsOffset == -1)
	{
		return false;
	}
	
	int maxClients = gpGlobals->maxClients;
	for (int i = 1; i <= maxClients; i++)
	{
		IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(i);
		if (!pPlayer || !pPlayer->IsInGame())
			continue;

		GetPlayerConds(gamehelpers->ReferenceToEntity(i), &g_PlayerConds[i]);
	}

	g_pSM->AddGameFrameHook(Conditions_OnGameFrame);

	return true;
}

void Conditions_OnClientPutInServer(int client)
{
	g_PlayerConds[client].Init();
}

void RemoveConditionChecks()
{
	g_pSM->RemoveGameFrameHook(Conditions_OnGameFrame);
}

