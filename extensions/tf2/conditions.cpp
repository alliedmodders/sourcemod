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

#include <bitvec.h>

const int TF_MAX_CONDITIONS = 128;

typedef CBitVec<TF_MAX_CONDITIONS> condbitvec_t;
condbitvec_t g_PlayerActiveConds[SM_MAXPLAYERS + 1];

IForward *g_addCondForward = NULL;
IForward *g_removeCondForward = NULL;

int playerCondOffset = -1;
int playerCondExOffset = -1;
int playerCondEx2Offset = -1;
int playerCondEx3Offset = -1;
int conditionBitsOffset = -1;

bool g_bIgnoreRemove;

#define MAX_CONDS (sizeof(uint64_t) * 8)

inline void GetPlayerConds(CBaseEntity *pPlayer, condbitvec_t *pOut)
{
	uint32_t tmp =  *(uint32_t *)((intptr_t)pPlayer + playerCondOffset);
	         tmp |= *(uint32_t *)((intptr_t)pPlayer + conditionBitsOffset);
	pOut->SetDWord(0, tmp);
	         tmp =  *(uint32_t *)((intptr_t)pPlayer + playerCondExOffset);
	pOut->SetDWord(1, tmp);
	         tmp =  *(uint32_t *)((intptr_t)pPlayer + playerCondEx2Offset);
	pOut->SetDWord(2, tmp);
	         tmp =  *(uint32_t *)((intptr_t) pPlayer + playerCondEx3Offset);
	pOut->SetDWord(3, tmp);
}

inline void CondBitVecAndNot(const condbitvec_t &src, const condbitvec_t &addStr, condbitvec_t *out)
{
	static_assert(TF_MAX_CONDITIONS == 128, "CondBitVecAndNot hack is hardcoded for 128-bit bitvec.");

	// CBitVec has And and Not, but not a simple, combined AndNot.
	// We'll also treat the halves as two 64-bit ints instead of four 32-bit ints
	// as a minor optimization (maybe?) that the compiler is not making itself.
	uint64 *pDest           = (uint64 *)out->Base();
	const uint64 *pOperand1 = (const uint64 *) src.Base();
	const uint64 *pOperand2 = (const uint64 *) addStr.Base();

	pDest[0] = pOperand1[0] & ~pOperand2[0];
	pDest[1] = pOperand1[1] & ~pOperand2[1];
}

void Conditions_OnGameFrame(bool simulating)
{
	if (!simulating)
		return;

	static condbitvec_t newconds;
	
	static condbitvec_t addedconds;
	static condbitvec_t removedconds;

	int maxClients = gpGlobals->maxClients;
	for (int i = 1; i <= maxClients; i++)
	{
		IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(i);
		if (!pPlayer->IsInGame() || pPlayer->IsSourceTV() || pPlayer->IsReplay())
			continue;

		CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(i);
		condbitvec_t &oldconds = g_PlayerActiveConds[i];
		GetPlayerConds(pEntity, &newconds);

		if (oldconds == newconds)
			continue;

		CondBitVecAndNot(newconds, oldconds, &addedconds);
		CondBitVecAndNot(oldconds, newconds, &removedconds);

		int bit;
		bit = -1;
		while ((bit = addedconds.FindNextSetBit(bit + 1)) != -1)
		{
			g_addCondForward->PushCell(i);
			g_addCondForward->PushCell(bit);
			g_addCondForward->Execute(NULL, NULL);
		}

		bit = -1;
		while ((bit = removedconds.FindNextSetBit(bit + 1)) != -1)
		{
			g_removeCondForward->PushCell(i);
			g_removeCondForward->PushCell(bit);
			g_removeCondForward->Execute(NULL, NULL);
		}

		g_PlayerActiveConds[i] = newconds;
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

	if (!gamehelpers->FindSendPropInfo("CTFPlayer", "m_nPlayerCondEx3", &prop))
	{
		g_pSM->LogError(myself, "Failed to find m_nPlayerCondEx3 prop offset");
		return false;
	}

	playerCondEx3Offset = prop.actual_offset;

	if (playerCondOffset == -1 || playerCondExOffset == -1 || conditionBitsOffset == -1 || playerCondEx2Offset == -1 || playerCondEx3Offset == -1)
		return false;
	
	int maxClients = gpGlobals->maxClients;
	for (int i = 1; i <= maxClients; i++)
	{
		IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(i);
		if (!pPlayer || !pPlayer->IsInGame())
			continue;

		GetPlayerConds(gamehelpers->ReferenceToEntity(i), &g_PlayerActiveConds[i]);
	}

	g_pSM->AddGameFrameHook(Conditions_OnGameFrame);

	return true;
}

void Conditions_OnClientPutInServer(int client)
{
	g_PlayerActiveConds[client].ClearAll();
}

void RemoveConditionChecks()
{
	g_pSM->RemoveGameFrameHook(Conditions_OnGameFrame);
}

