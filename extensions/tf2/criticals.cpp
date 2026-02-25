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

CritManager g_CritManager;

IForward *g_critForward = NULL;

KHook::Virtual<CBaseEntity, bool> g_HookCalcIsAttackCriticalHelper;
KHook::Virtual<CBaseEntity, bool> g_HookCalcIsAttackCriticalHelperNoCrits;

const char TF_WEAPON_DATATABLE[] = "DT_TFWeaponBase";

CritManager::CritManager() :
	m_enabled(false),
	m_hooksSetup(false)
{
	m_entsHooked.Init();

	g_HookCalcIsAttackCriticalHelper.AddContext(this, &CritManager::Hook_CalcIsAttackCriticalHelper, nullptr);
	g_HookCalcIsAttackCriticalHelperNoCrits.AddContext(this, &CritManager::Hook_CalcIsAttackCriticalHelperNoCrits, nullptr);
}

bool CritManager::TryEnable()
{
	if (!m_hooksSetup)
	{
		int offset;

		if (!g_pGameConf->GetOffset("CalcIsAttackCriticalHelper", &offset))
		{
			g_pSM->LogError(myself, "Failed to find CalcIsAttackCriticalHelper offset");
			return false;
		}

		g_HookCalcIsAttackCriticalHelper.Configure(offset);

		if (!g_pGameConf->GetOffset("CalcIsAttackCriticalHelperNoCrits", &offset))
		{
			g_pSM->LogError(myself, "Failed to find CalcIsAttackCriticalHelperNoCrits offset");
			return false;
		}

		g_HookCalcIsAttackCriticalHelperNoCrits.Configure(offset);

		m_hooksSetup = true;
	}

	for (size_t i = playerhelpers->GetMaxClients() + 1; i < MAX_EDICTS; ++i)
	{
		CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(i);
		if (pEntity == nullptr)
		{
			continue;
		}

		ServerClass *pServerClass = gamehelpers->FindEntityServerClass(pEntity);
		if (pServerClass == nullptr)
		{
			continue;
		}

		if (!UTIL_ContainsDataTable(pServerClass->m_pTable, TF_WEAPON_DATATABLE))
		{
			continue;
		}

		g_HookCalcIsAttackCriticalHelper.Add(pEntity);
		g_HookCalcIsAttackCriticalHelperNoCrits.Add(pEntity);

		m_entsHooked.Set(i);
	}

	m_enabled = true;

	return true;
}

void CritManager::Disable()
{
	int i = m_entsHooked.FindNextSetBit(playerhelpers->GetMaxClients() + 1);
	for (i; i != -1; i = m_entsHooked.FindNextSetBit(i))
	{
		CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(i);
		g_HookCalcIsAttackCriticalHelper.Remove(pEntity);
		g_HookCalcIsAttackCriticalHelperNoCrits.Remove(pEntity);
		m_entsHooked.Set(i, false);
	}

	m_enabled = false;
}

void CritManager::OnEntityCreated(CBaseEntity *pEntity, const char *classname)
{
	if (!m_enabled)
	{
		return;
	}

	ServerClass *pServerClass = gamehelpers->FindEntityServerClass(pEntity);
	if (pServerClass == nullptr)
	{
		return;
	}

	if (!UTIL_ContainsDataTable(pServerClass->m_pTable, TF_WEAPON_DATATABLE))
	{
		return;
	}

	g_HookCalcIsAttackCriticalHelper.Add(pEntity);
	g_HookCalcIsAttackCriticalHelperNoCrits.Add(pEntity);
	
	m_entsHooked.Set(gamehelpers->EntityToBCompatRef(pEntity));
}

void CritManager::OnEntityDestroyed(CBaseEntity *pEntity)
{
	if (!m_enabled)
		return;

	int index = gamehelpers->EntityToBCompatRef(pEntity);
	if (index < 0 || index >= MAX_EDICTS)
		return;
	
	if (!m_entsHooked.IsBitSet(index))
		return;

	g_HookCalcIsAttackCriticalHelper.Remove(pEntity);
	g_HookCalcIsAttackCriticalHelperNoCrits.Remove(pEntity);

	m_entsHooked.Set(index, false);
}

KHook::Return<bool> CritManager::Hook_CalcIsAttackCriticalHelper(CBaseEntity* ent)
{
	return Hook_CalcIsAttackCriticalHelpers(ent, false);
}

KHook::Return<bool> CritManager::Hook_CalcIsAttackCriticalHelperNoCrits(CBaseEntity* ent)
{
	return Hook_CalcIsAttackCriticalHelpers(ent, true);
}

KHook::Return<bool> CritManager::Hook_CalcIsAttackCriticalHelpers(CBaseEntity* pWeapon, bool noCrits)
{
	// If there's an invalid ent or invalid server class here, we've got issues elsewhere.
	ServerClass *pServerClass = gamehelpers->FindEntityServerClass(pWeapon);
	if (pServerClass == nullptr)
	{
		g_pSM->LogError(myself, "Invalid server class on weapon.");
		return { KHook::Action::Ignore };
	}

	sm_sendprop_info_t info;
	if (!gamehelpers->FindSendPropInfo(pServerClass->GetName(), "m_hOwnerEntity", &info))
	{
		g_pSM->LogError(myself, "Could not find m_hOwnerEntity on %s", pServerClass->GetName());
		return { KHook::Action::Ignore };
	}

	int returnValue;
	if (noCrits)
	{
		returnValue = g_HookCalcIsAttackCriticalHelperNoCrits.CallOriginal(pWeapon) ? 1 : 0;
	}
	else
	{
		returnValue = g_HookCalcIsAttackCriticalHelper.CallOriginal(pWeapon) ? 1 : 0;
	}

	int origReturnValue = returnValue;
	int ownerIndex = -1;
	CBaseHandle &hndl = *(CBaseHandle *) ((intptr_t)pWeapon + info.actual_offset);
	CBaseEntity *pHandleEntity = gamehelpers->ReferenceToEntity(hndl.GetEntryIndex());

	if (pHandleEntity != NULL && hndl == reinterpret_cast<IHandleEntity *>(pHandleEntity)->GetRefEHandle())
	{
		ownerIndex = hndl.GetEntryIndex();
	}

	g_critForward->PushCell(ownerIndex); //Client index
	g_critForward->PushCell(gamehelpers->EntityToBCompatRef(pWeapon)); // Weapon index
	g_critForward->PushString(gamehelpers->GetEntityClassname(pWeapon)); //Weapon classname
	g_critForward->PushCellByRef(&returnValue); //return value

	cell_t result = 0;

	g_critForward->Execute(&result);

	if (result > Pl_Continue)
	{
		return { KHook::Action::Supersede, returnValue };
	}
	
	return { KHook::Action::Supersede, origReturnValue };
}
