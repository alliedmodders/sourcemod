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

#include "holiday.h"

SH_DECL_MANUALHOOK1(IsHolidayActive, 0, 0, 0, bool, int);
SH_DECL_HOOK0_void(IServerGameDLL, LevelShutdown, SH_NOATTRIB, 0);

HolidayManager g_HolidayManager;

HolidayManager::HolidayManager() :
m_iHookID(0),
m_isHolidayForward(NULL),
m_bInMap(false)
{
}

void HolidayManager::OnSDKLoad(bool bLate)
{
	m_bInMap = bLate;

	plsys->AddPluginsListener(this);
	m_isHolidayForward = forwards->CreateForward("TF2_OnIsHolidayActive", ET_Event, 2, NULL, Param_Cell, Param_CellByRef);

	SH_ADD_HOOK(IServerGameDLL, LevelShutdown, gamedll, SH_MEMBER(this, &HolidayManager::Hook_LevelShutdown), false);
}

void HolidayManager::OnSDKUnload()
{
	UnhookIfNecessary();
	SH_REMOVE_HOOK(IServerGameDLL, LevelShutdown, gamedll, SH_MEMBER(this, &HolidayManager::Hook_LevelShutdown), false);

	plsys->RemovePluginsListener(this);
	forwards->ReleaseForward(m_isHolidayForward);
}

void HolidayManager::OnServerActivated()
{
	m_bInMap = true;

	HookIfNecessary();
}

void HolidayManager::Hook_LevelShutdown()
{
	// GameRules is going away momentarily. Unhook before it does.
	UnhookIfNecessary();

	m_bInMap = false;
}

void HolidayManager::HookIfNecessary()
{
	// Already hooked
	if (m_iHookID)
		return;

	// Nothing wants us
	if (m_isHolidayForward->GetFunctionCount() == 0)
		return;

	void *pGameRules = GetGameRules();
	if (!pGameRules)
	{
		if (m_bInMap)
		{
			g_pSM->LogError(myself, "Gamerules ptr not found. TF2_OnIsHolidayActive will not be available.");
		}
		return;
	}

	static int offset = -1;
	if (offset == -1)
	{
		if (!g_pGameConf->GetOffset("IsHolidayActive", &offset))
		{
			g_pSM->LogError(myself, "IsHolidayActive gamedata offset missing. TF2_OnIsHolidayActive will not be available.");
			return;
		}

		SH_MANUALHOOK_RECONFIGURE(IsHolidayActive, offset, 0, 0);
	}

	m_iHookID = SH_ADD_MANUALHOOK(IsHolidayActive, pGameRules, SH_MEMBER(this, &HolidayManager::Hook_IsHolidayActive), false);
}

void HolidayManager::UnhookIfNecessary()
{
	// Not hooked
	if (!m_iHookID)
		return;

	// We're still wanted
	if (m_isHolidayForward->GetFunctionCount() > 0)
		return;

	SH_REMOVE_HOOK_ID(m_iHookID);
	m_iHookID = 0;
}

static inline void PopulateHolidayVar(IPluginRuntime *pRuntime, const char *pszName)
{
	uint32_t idx;
	if (pRuntime->FindPubvarByName(pszName, &idx) != SP_ERROR_NONE)
		return;

	int varValue = -1;
	const char *key = g_pGameConf->GetKeyValue(pszName);
	if (key)
	{
		varValue = atoi(key);
	}

	sp_pubvar_t *var;
	pRuntime->GetPubvarByIndex(idx, &var);
	*var->offs = varValue;
}

void HolidayManager::OnPluginLoaded(IPlugin *plugin)
{
	HookIfNecessary();

	auto *pRuntime = plugin->GetRuntime();
	PopulateHolidayVar(pRuntime, "TFHoliday_Birthday");
	PopulateHolidayVar(pRuntime, "TFHoliday_Halloween");
	PopulateHolidayVar(pRuntime, "TFHoliday_Christmas");
	PopulateHolidayVar(pRuntime, "TFHoliday_EndOfTheLine");
	PopulateHolidayVar(pRuntime, "TFHoliday_CommunityUpdate");
	PopulateHolidayVar(pRuntime, "TFHoliday_ValentinesDay");
	PopulateHolidayVar(pRuntime, "TFHoliday_MeetThePyro");
	PopulateHolidayVar(pRuntime, "TFHoliday_FullMoon");
	PopulateHolidayVar(pRuntime, "TFHoliday_HalloweenOrFullMoon");
	PopulateHolidayVar(pRuntime, "TFHoliday_HalloweenOrFullMoonOrValentines");
	PopulateHolidayVar(pRuntime, "TFHoliday_AprilFools");
	PopulateHolidayVar(pRuntime, "TFHoliday_Soldier");
}

void HolidayManager::OnPluginUnloaded(IPlugin *plugin)
{
	UnhookIfNecessary();
}

bool HolidayManager::Hook_IsHolidayActive(int holiday)
{
	void *pGameRules = META_IFACEPTR(void *);

	bool actualres = SH_MCALL(pGameRules, IsHolidayActive)(holiday);
	if (!m_isHolidayForward)
	{
		g_pSM->LogMessage(myself, "Invalid Forward");
		RETURN_META_VALUE(MRES_IGNORED, true);
	}

	cell_t result = 0;
	cell_t newres = actualres ? 1 : 0;

	m_isHolidayForward->PushCell(holiday);
	m_isHolidayForward->PushCellByRef(&newres);
	m_isHolidayForward->Execute(&result);

	if (result > Pl_Continue)
	{
		RETURN_META_VALUE(MRES_SUPERCEDE, (newres == 0) ? false : true);
	}

	RETURN_META_VALUE(MRES_IGNORED, true);
}
