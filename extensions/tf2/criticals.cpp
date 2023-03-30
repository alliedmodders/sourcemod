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

SH_DECL_MANUALHOOK0(CalcIsAttackCriticalHelper, 0, 0, 0, bool);
SH_DECL_MANUALHOOK0(CalcIsAttackCriticalHelperNoCrits, 0, 0, 0, bool);

const char TF_WEAPON_DATATABLE[] = "DT_TFWeaponBase";

CritManager::CritManager() :
	m_enabled(false),
	m_hooksSetup(false)
{
	m_entsHooked.Init();
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

		SH_MANUALHOOK_RECONFIGURE(CalcIsAttackCriticalHelper, offset, 0, 0);

		if (!g_pGameConf->GetOffset("CalcIsAttackCriticalHelperNoCrits", &offset))
		{
			g_pSM->LogError(myself, "Failed to find CalcIsAttackCriticalHelperNoCrits offset");
			return false;
		}

		SH_MANUALHOOK_RECONFIGURE(CalcIsAttackCriticalHelperNoCrits, offset, 0, 0);

		m_hooksSetup = true;
	}

	for (size_t i = playerhelpers->GetMaxClients() + 1; i < MAX_EDICTS; ++i)
	{
		CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(i);
		if (pEntity == NULL)
			continue;

		IServerUnknown *pUnknown = (IServerUnknown *)pEntity;
		IServerNetworkable *pNetworkable = pUnknown->GetNetworkable();
		if (!pNetworkable)
			continue;

		if (!UTIL_ContainsDataTable(pNetworkable->GetServerClass()->m_pTable, TF_WEAPON_DATATABLE))
			continue;

		SH_ADD_MANUALHOOK(CalcIsAttackCriticalHelper, pEntity, SH_MEMBER(&g_CritManager, &CritManager::Hook_CalcIsAttackCriticalHelper), false);
		SH_ADD_MANUALHOOK(CalcIsAttackCriticalHelperNoCrits, pEntity, SH_MEMBER(&g_CritManager, &CritManager::Hook_CalcIsAttackCriticalHelperNoCrits), false);

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
		SH_REMOVE_MANUALHOOK(CalcIsAttackCriticalHelper, pEntity, SH_MEMBER(&g_CritManager, &CritManager::Hook_CalcIsAttackCriticalHelper), false);
		SH_REMOVE_MANUALHOOK(CalcIsAttackCriticalHelperNoCrits, pEntity, SH_MEMBER(&g_CritManager, &CritManager::Hook_CalcIsAttackCriticalHelperNoCrits), false);

		m_entsHooked.Set(i, false);
	}

	m_enabled = false;
}

void CritManager::OnEntityCreated(CBaseEntity *pEntity, const char *classname)
{
	if (!m_enabled)
		return;

	IServerUnknown *pUnknown = (IServerUnknown *)pEntity;
	IServerNetworkable *pNetworkable = pUnknown->GetNetworkable();
	if (!pNetworkable)
		return;

	if (!UTIL_ContainsDataTable(pNetworkable->GetServerClass()->m_pTable, TF_WEAPON_DATATABLE))
		return;

	SH_ADD_MANUALHOOK(CalcIsAttackCriticalHelper, pEntity, SH_MEMBER(&g_CritManager, &CritManager::Hook_CalcIsAttackCriticalHelper), false);
	SH_ADD_MANUALHOOK(CalcIsAttackCriticalHelperNoCrits, pEntity, SH_MEMBER(&g_CritManager, &CritManager::Hook_CalcIsAttackCriticalHelperNoCrits), false);

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

	SH_REMOVE_MANUALHOOK(CalcIsAttackCriticalHelper, pEntity, SH_MEMBER(&g_CritManager, &CritManager::Hook_CalcIsAttackCriticalHelper), false);
	SH_REMOVE_MANUALHOOK(CalcIsAttackCriticalHelperNoCrits, pEntity, SH_MEMBER(&g_CritManager, &CritManager::Hook_CalcIsAttackCriticalHelperNoCrits), false);

	m_entsHooked.Set(index, false);
}

bool CritManager::Hook_CalcIsAttackCriticalHelper()
{
	return Hook_CalcIsAttackCriticalHelpers(false);
}

bool CritManager::Hook_CalcIsAttackCriticalHelperNoCrits()
{
	return Hook_CalcIsAttackCriticalHelpers(true);
}

bool CritManager::Hook_CalcIsAttackCriticalHelpers(bool noCrits)
{
	CBaseEntity *pWeapon = META_IFACEPTR(CBaseEntity);
	
	// If there's an invalid ent or invalid networkable here, we've got issues elsewhere.

	IServerNetworkable *pNetWeapon = ((IServerUnknown *)pWeapon)->GetNetworkable();
	ServerClass *pServerClass = pNetWeapon->GetServerClass();
	if (!pServerClass)
	{
		g_pSM->LogError(myself, "Invalid server class on weapon.");
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	sm_sendprop_info_t info;
	if (!gamehelpers->FindSendPropInfo(pServerClass->GetName(), "m_hOwnerEntity", &info))
	{
		g_pSM->LogError(myself, "Could not find m_hOwnerEntity on %s", pServerClass->GetName());
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	int returnValue;
	if (noCrits)
	{
		returnValue = SH_MCALL(pWeapon, CalcIsAttackCriticalHelperNoCrits)() ? 1 : 0;
	}
	else
	{
		returnValue = SH_MCALL(pWeapon, CalcIsAttackCriticalHelper)() ? 1 : 0;
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
		RETURN_META_VALUE(MRES_SUPERCEDE, returnValue);
	}
	
	RETURN_META_VALUE(MRES_SUPERCEDE, origReturnValue);
}
