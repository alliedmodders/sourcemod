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
#include "util.h"

IServerGameEnts *gameents = NULL;

CDetour *calcIsAttackCriticalDetour = NULL;
CDetour *calcIsAttackCriticalMeleeDetour = NULL;
CDetour *calcIsAttackCriticalBowDetour = NULL;
CDetour *calcIsAttackCriticalKnifeDetour = NULL;
CDetour *calcIsAttackCriticalNoCritsDetour = NULL;
CDetour *calcIsAttackCriticalNoCritsMeleeDetour = NULL;

int nextMeleeCritOffset = -1;

IForward *g_critForward = NULL;

enum DetourResult
{
	Result_Ignore,
	Result_NoCrit,
	Result_Crit,
};

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

DetourResult DetourCallback(CBaseEntity *pEnt, bool bMeleeNoCrits)
{
	edict_t *pEdict = gameents->BaseEntityToEdict((CBaseEntity *)pEnt);

	if (!pEdict)
	{
		g_pSM->LogMessage(myself, "Entity Error");
		return Result_Ignore;
	}

	sm_sendprop_info_t info;

	if (!gamehelpers->FindSendPropInfo(pEdict->GetNetworkable()->GetServerClass()->GetName(), "m_hOwnerEntity", &info))
	{
		g_pSM->LogMessage(myself, "Offset Error");
		return Result_Ignore;
	}

	if (!g_critForward)
	{
		g_pSM->LogMessage(myself, "Invalid Forward");
		return Result_Ignore;
	}

	int returnValue=0;

	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pEnt + info.actual_offset);
	int index = CheckBaseHandle(hndl);

	//Skip CTFWeaponBaseMelee::CalcIsAttackCriticalHelperNoCrits if m_iNextMeleeCrit is not 2
	if (bMeleeNoCrits)
	{
		CBaseEntity *pPlayer = UTIL_GetCBaseEntity(index, true);
		if (pPlayer && *(uint32_t *)((intptr_t)pPlayer + nextMeleeCritOffset) != 2)
			return Result_Ignore;
	}

	g_critForward->PushCell(index); //Client index
	g_critForward->PushCell(engine->IndexOfEdict(pEdict)); // Weapon index
	g_critForward->PushString(pEdict->GetClassName()); //Weapon classname
	g_critForward->PushCellByRef(&returnValue); //return value

	cell_t result = 0;

	g_critForward->Execute(&result);

	if (result)
	{
		if (returnValue)
		{
			return Result_Crit;
		}
		else
		{
			return Result_NoCrit;
		}
	}
	else
	{
		return Result_Ignore;
	}
}

DETOUR_DECL_MEMBER0(CalcIsAttackCriticalHelperMelee, bool)
{
	DetourResult result = DetourCallback((CBaseEntity *)this, false);

	if (result == Result_Ignore)
	{
		return DETOUR_MEMBER_CALL(CalcIsAttackCriticalHelperMelee)();
	}
	else if (result == Result_NoCrit)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

DETOUR_DECL_MEMBER0(CalcIsAttackCriticalHelper, bool)
{
	DetourResult result = DetourCallback((CBaseEntity *)this, false);

	if (result == Result_Ignore)
	{
		return DETOUR_MEMBER_CALL(CalcIsAttackCriticalHelper)();
	}
	else if (result == Result_NoCrit)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

DETOUR_DECL_MEMBER0(CalcIsAttackCriticalHelperBow, bool)
{
	DetourResult result = DetourCallback((CBaseEntity *)this, false);

	if (result == Result_Ignore)
	{
		return DETOUR_MEMBER_CALL(CalcIsAttackCriticalHelperBow)();
	}
	else if (result == Result_NoCrit)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

DETOUR_DECL_MEMBER0(CalcIsAttackCriticalHelperKnife, bool)
{
	DetourResult result = DetourCallback((CBaseEntity *)this, false);

	if (result == Result_Ignore)
	{
		return DETOUR_MEMBER_CALL(CalcIsAttackCriticalHelperKnife)();
	}
	else if (result == Result_NoCrit)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

DETOUR_DECL_MEMBER0(CalcIsAttackCriticalHelperNoCrits, bool)
{
	DetourResult result = DetourCallback((CBaseEntity *)this, false);

	if (result == Result_Ignore)
	{
		return DETOUR_MEMBER_CALL(CalcIsAttackCriticalHelperNoCrits)();
	}
	else if (result == Result_NoCrit)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

DETOUR_DECL_MEMBER0(CalcIsAttackCriticalHelperNoCritsMelee, bool)
{
	DetourResult result = DetourCallback((CBaseEntity *)this, true);

	if (result == Result_Ignore)
	{
		return DETOUR_MEMBER_CALL(CalcIsAttackCriticalHelperNoCritsMelee)();
	}
	else if (result == Result_NoCrit)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

bool InitialiseCritDetours()
{
	sm_sendprop_info_t prop;

	calcIsAttackCriticalDetour = DETOUR_CREATE_MEMBER(CalcIsAttackCriticalHelper, "CalcCritical");
	calcIsAttackCriticalMeleeDetour = DETOUR_CREATE_MEMBER(CalcIsAttackCriticalHelperMelee, "CalcCriticalMelee");
	calcIsAttackCriticalBowDetour = DETOUR_CREATE_MEMBER(CalcIsAttackCriticalHelperBow, "CalcCriticalBow");
	calcIsAttackCriticalKnifeDetour = DETOUR_CREATE_MEMBER(CalcIsAttackCriticalHelperKnife, "CalcCriticalKnife");
	calcIsAttackCriticalNoCritsDetour = DETOUR_CREATE_MEMBER(CalcIsAttackCriticalHelperNoCrits, "CalcCriticalNoCrits");
	calcIsAttackCriticalNoCritsMeleeDetour = DETOUR_CREATE_MEMBER(CalcIsAttackCriticalHelperNoCritsMelee, "CalcCriticalNoCritsMelee");

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

	if (calcIsAttackCriticalBowDetour != NULL)
	{
		calcIsAttackCriticalBowDetour->EnableDetour();
		HookCreated = true;
	}

	if (calcIsAttackCriticalKnifeDetour != NULL)
	{
		calcIsAttackCriticalKnifeDetour->EnableDetour();
		HookCreated = true;
	}

	if (calcIsAttackCriticalNoCritsDetour != NULL)
	{
		calcIsAttackCriticalNoCritsDetour->EnableDetour();
		HookCreated = true;
	}

	if (calcIsAttackCriticalNoCritsMeleeDetour != NULL)
	{
		calcIsAttackCriticalNoCritsMeleeDetour->EnableDetour();
		if (!gamehelpers->FindSendPropInfo("CTFPlayer", "m_iNextMeleeCrit", &prop))
		{
			g_pSM->LogError(myself, "Failed to find m_iNextMeleeCrit prop offset");
			return false;
		}
		nextMeleeCritOffset = prop.actual_offset;
		HookCreated = true;
	}

	if (HookCreated)
	{
		return true;
	}

	g_pSM->LogError(myself, "No critical hit forwards could be initialized - Disabled critical hit hooks");

	return false;
}

void RemoveCritDetours()
{
	if (calcIsAttackCriticalDetour != NULL)
	{
		calcIsAttackCriticalDetour->Destroy();
		calcIsAttackCriticalDetour = NULL;
	}

	if (calcIsAttackCriticalMeleeDetour != NULL)
	{
		calcIsAttackCriticalMeleeDetour->Destroy();
		calcIsAttackCriticalMeleeDetour = NULL;
	}

	if (calcIsAttackCriticalBowDetour != NULL)
	{
		calcIsAttackCriticalBowDetour->Destroy();
		calcIsAttackCriticalBowDetour = NULL;
	}
	
	if (calcIsAttackCriticalKnifeDetour != NULL)
	{
		calcIsAttackCriticalKnifeDetour->Destroy();
		calcIsAttackCriticalKnifeDetour = NULL;
	}

	if (calcIsAttackCriticalNoCritsDetour != NULL)
	{
		calcIsAttackCriticalNoCritsDetour->Destroy();
		calcIsAttackCriticalNoCritsDetour = NULL;
	}

	if (calcIsAttackCriticalNoCritsMeleeDetour != NULL)
	{
		calcIsAttackCriticalNoCritsMeleeDetour->Destroy();
		calcIsAttackCriticalNoCritsMeleeDetour = NULL;
	}
}
