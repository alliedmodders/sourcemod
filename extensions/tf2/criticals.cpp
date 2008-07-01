/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Team Fortress 2 Extension
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

#include "criticals.h"

IServerGameEnts *gameents = NULL;

CDetour *calcIsAttackCriticalDetour = NULL;
CDetour *calcIsAttackCriticalMeleeDetour = NULL;
CDetour *calcIsAttackCriticalKnifeDetour = NULL;

IForward *g_critForward = NULL;

void InitialiseDetours()
{
	calcIsAttackCriticalDetour = CDetourManager::CreateDetour((void *)&TempDetour, 0, "CalcCritical");
	calcIsAttackCriticalMeleeDetour = CDetourManager::CreateDetour((void *)&TempDetour, 0, "CalcCriticalMelee");
	calcIsAttackCriticalKnifeDetour = CDetourManager::CreateDetour((void *)&TempDetour, 0, "CalcCriticalKnife");

	bool HookCreated = false;

	if (calcIsAttackCriticalDetour != NULL)
	{
		calcIsAttackCriticalDetour->EnableDetour();
		HookCreated = true;
	}

	if (calcIsAttackCriticalMeleeDetour != NULL)
	{
		calcIsAttackCriticalMeleeDetour->EnableDetour();
		HookCreated = true;
	}

	if (calcIsAttackCriticalKnifeDetour != NULL)
	{
		calcIsAttackCriticalKnifeDetour->EnableDetour();
		HookCreated = true;
	}

	if (!HookCreated)
	{
		g_pSM->LogError(myself, "No critical hit forwards could be initialized - Disabled critical hit hooks");
		return;
	}

}

int CheckBaseHandle(CBaseHandle &hndl)
{
	if (!hndl.IsValid())
	{
		return -1;
	}

	int index = hndl.GetEntryIndex();

	edict_t *pStoredEdict;

	pStoredEdict = engine->PEntityOfEntIndex(index);

	if (pStoredEdict == NULL)
	{
		return -1;
	}

	IServerEntity *pSE = pStoredEdict->GetIServerEntity();

	if (pSE == NULL)
	{
		return -1;
	}

	if (pSE->GetRefEHandle() != hndl)
	{
		return -1;
	}

	return index;
}

DetourReturn TempDetour(void *pWeapon)
{
	edict_t *pEdict = gameents->BaseEntityToEdict((CBaseEntity *)pWeapon);
	
	if (!pEdict)
	{
		g_pSM->LogMessage(myself, "Entity Error");
		return false;
	}

	sm_sendprop_info_t info;

	if (!gamehelpers->FindSendPropInfo(pEdict->GetNetworkable()->GetServerClass()->GetName(), "m_hOwnerEntity", &info))
	{
		g_pSM->LogMessage(myself, "Offset Error");
		return false;
	}

	if (!g_critForward)
	{
		g_pSM->LogMessage(myself, "Invalid Forward");
		return false;
	}

	int returnValue=0;
	
	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pWeapon + info.actual_offset);
	int index = CheckBaseHandle(hndl);

	g_critForward->PushCell(index); //Client index
	g_critForward->PushCell(engine->IndexOfEdict(pEdict)); // Weapon index
	g_critForward->PushString(pEdict->GetClassName()); //Weapon classname
	g_critForward->PushCellByRef(&returnValue); //return value

	cell_t result = 0;

	g_critForward->Execute(&result);

	if (result)
	{
		RETURN_DETOUR_VALUE(DETOUR_RESULT_OVERRIDE, returnValue);
	}
	else
	{
		RETURN_DETOUR_VALUE(DETOUR_RESULT_IGNORED, returnValue);
	}
	
}

void RemoveDetours()
{
	CDetourManager::DeleteDetour(calcIsAttackCriticalDetour);
	CDetourManager::DeleteDetour(calcIsAttackCriticalMeleeDetour);
	CDetourManager::DeleteDetour(calcIsAttackCriticalKnifeDetour);
}
