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

typedef int condflags_t;
#define CONDITION_TAUNTING 7

IForward *g_addCondForward = NULL;
IForward *g_removeCondForward = NULL;

CDetour *detConditionAdd = NULL;
CDetour *detConditionRemove = NULL;
CDetour *detConditionRemoveAll = NULL;

int playerCondOffset = -1;
int conditionBitsOffset = -1;

bool g_bIgnoreRemove;

inline condflags_t GetPlayerConds(CBaseEntity *pPlayer)
{
	condflags_t cond1 = *(condflags_t *)((intptr_t)pPlayer + playerCondOffset);
	condflags_t cond2 = *(condflags_t *)((intptr_t)pPlayer + conditionBitsOffset);
	return cond1|cond2;
}

DETOUR_DECL_MEMBER2(AddCond, void, int, condition, float, duration)
{
	CBaseEntity *pPlayer = (CBaseEntity *)((intptr_t)this - playerSharedOffset->actual_offset);
	condflags_t oldconds = GetPlayerConds(pPlayer);

	DETOUR_MEMBER_CALL(AddCond)(condition, duration);

	int bit = 1 << condition;

	// The player already had this condition so bug out
	if ((oldconds & bit) == bit)
		return;

	// The condition wasn't actually added (this can happen!)
	if ((GetPlayerConds(pPlayer) & bit) != bit)
		return;
	
	int client = gamehelpers->EntityToBCompatRef(pPlayer);

	g_addCondForward->PushCell(client);
	g_addCondForward->PushCell(condition);
	g_addCondForward->PushFloat(duration);
	g_addCondForward->Execute(NULL, NULL);
}

DETOUR_DECL_MEMBER2(RemoveCond, void, int, condition, bool, bForceRemove)
{
	if (g_bIgnoreRemove)
	{
		DETOUR_MEMBER_CALL(RemoveCond)(condition, bForceRemove);
		return;
	}

	CBaseEntity *pPlayer = (CBaseEntity *)((intptr_t)this - playerSharedOffset->actual_offset);

	condflags_t oldconds = GetPlayerConds(pPlayer);
	int bit = 1 << condition;

	// If we didn't have it, it won't be removed
	if ((oldconds & bit) != bit)
	{
		DETOUR_MEMBER_CALL(RemoveCond)(condition, bForceRemove);
		return;
	}

	DETOUR_MEMBER_CALL(RemoveCond)(condition, bForceRemove);

	// Calling RemoveCond doesn't necessarily remove the cond...
	if ((GetPlayerConds(pPlayer) & bit) == bit)
	{
		return;
	}

	int client = gamehelpers->EntityToBCompatRef(pPlayer);

	DoRemoveCond(client, condition);
}

DETOUR_DECL_MEMBER1(RemoveAllCond, void, CBaseEntity *, unknown)
{
	// We're going to ignore Remove here since it can be called multiple times for the same cond
	g_bIgnoreRemove = true;

	CBaseEntity *pPlayer = (CBaseEntity *)((intptr_t)this - playerSharedOffset->actual_offset);

	int client = gamehelpers->EntityToBCompatRef(pPlayer);
	condflags_t conds = GetPlayerConds(pPlayer);

	// I don't know what the unknown is, but it's always NULL
	DETOUR_MEMBER_CALL(RemoveAllCond)(unknown);
	
	int max = sizeof(condflags_t)*8;
	for (int i = 0; i < max; i++)
	{
		// Taunt flag removal doesn't go through RemoveCond nor OnCondRemoved (or was mangled by compiler?)
		// Better to "documentedly" give no removes rather than only in some cases
		if (i == CONDITION_TAUNTING)
			continue;

		int bit = (1<<i);
		if ((conds & bit) == bit)
		{
			DoRemoveCond(client, i);
		}
	}

	g_bIgnoreRemove = false;
}

void DoRemoveCond(int client, int condition)
{
	g_removeCondForward->PushCell(client);
	g_removeCondForward->PushCell(condition);
	g_removeCondForward->Execute(NULL, NULL);
}

bool InitialiseConditionDetours()
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

	if (playerCondOffset == -1 || conditionBitsOffset == -1)
		return false;

	detConditionAdd = DETOUR_CREATE_MEMBER(AddCond, "AddCondition");
	if (detConditionAdd == NULL)
	{
		g_pSM->LogError(myself, "CTFPlayerShared::AddCond detour failed");
		return false;
	}

	detConditionRemove = DETOUR_CREATE_MEMBER(RemoveCond, "RemoveCondition");
	if (detConditionRemove == NULL)
	{
		g_pSM->LogError(myself, "CTFPlayerShared::RemoveCond detour failed");
		return false;
	}

	detConditionRemoveAll = DETOUR_CREATE_MEMBER(RemoveAllCond, "RemoveAllConditions");
	if (detConditionRemoveAll == NULL)
	{
		g_pSM->LogError(myself, "CTFPlayerShared::RemoveAllCond detour failed");
		return false;
	}

	detConditionAdd->EnableDetour();
	detConditionRemove->EnableDetour();
	detConditionRemoveAll->EnableDetour();

	return true;
}

void RemoveConditionDetours()
{
	detConditionAdd->Destroy();
	detConditionRemove->Destroy();
	detConditionRemoveAll->Destroy();
}

