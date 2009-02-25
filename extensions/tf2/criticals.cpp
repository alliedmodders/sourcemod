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

DETOUR_DECL_MEMBER0(CalcIsAttackCriticalHelper, bool)
{
	edict_t *pEdict = gameents->BaseEntityToEdict((CBaseEntity *)this);
	
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
	
	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)this + info.actual_offset);
	int index = CheckBaseHandle(hndl);

	g_critForward->PushCell(index); //Client index
	g_critForward->PushCell(engine->IndexOfEdict(pEdict)); // Weapon index
	g_critForward->PushString(pEdict->GetClassName()); //Weapon classname
	g_critForward->PushCellByRef(&returnValue); //return value

	cell_t result = 0;

	g_critForward->Execute(&result);

	if (result)
	{
		return !!returnValue;
	}
	else
	{
		return DETOUR_MEMBER_CALL(CalcIsAttackCriticalHelper)();
	}
	
}

void InitialiseDetours()
{
	calcIsAttackCriticalDetour = DETOUR_CREATE_MEMBER(CalcIsAttackCriticalHelper, "CalcCritical");
	calcIsAttackCriticalMeleeDetour = DETOUR_CREATE_MEMBER(CalcIsAttackCriticalHelper, "CalcCriticalMelee");
	calcIsAttackCriticalKnifeDetour = DETOUR_CREATE_MEMBER(CalcIsAttackCriticalHelper, "CalcCriticalKnife");

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

void RemoveDetours()
{
	calcIsAttackCriticalDetour->Destroy();
	calcIsAttackCriticalMeleeDetour->Destroy();
	calcIsAttackCriticalKnifeDetour->Destroy();
}
