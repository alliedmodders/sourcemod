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

CDetour *isHolidayDetour = NULL;

IForward *g_isHolidayForward = NULL;

DETOUR_DECL_STATIC1(IsHolidayActive, bool, int, holiday)
{
	bool actualres = DETOUR_STATIC_CALL(IsHolidayActive)(holiday);
	if (!g_isHolidayForward)
	{
		g_pSM->LogMessage(myself, "Invalid Forward");
		return actualres;
	}

	cell_t result = 0;
	cell_t newres = actualres ? 1 : 0;

	g_isHolidayForward->PushCell(holiday);
	g_isHolidayForward->PushCellByRef(&newres);
	g_isHolidayForward->Execute(&result);
	
	if (result > Pl_Continue)
	{
		return (newres == 0) ? false : true;
	}

	return actualres;
}

bool InitialiseIsHolidayDetour()
{
	isHolidayDetour = DETOUR_CREATE_STATIC(IsHolidayActive, "IsHolidayActive");

	if (isHolidayDetour != NULL)
	{
		isHolidayDetour->EnableDetour();
		return true;
	}

	g_pSM->LogError(myself, "IsHolidayActive detour failed");
	return false;
}

void RemoveIsHolidayDetour()
{
	isHolidayDetour->Destroy();
}
