/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Counter-Strike:Source Extension
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

#include "extension.h"
#include "timeleft.h"

TimeLeftEvents g_TimeLeftEvents;
bool get_new_timeleft_offset = false;
bool round_end_found = false;

bool TimeLeftEvents::LevelInit(char const *pMapName,
							   char const *pMapEntities,
							   char const *pOldLevel,
							   char const *pLandmarkName,
							   bool loadGame,
							   bool background)
{
	round_end_found	= true;
	get_new_timeleft_offset = false;

	return true;
}

void TimeLeftEvents::FireGameEvent(IGameEvent *event)
{
	const char *name = event->GetName();
	
	if (strcmp(name, "round_start") == 0)
	{
		if (get_new_timeleft_offset || !round_end_found)
		{
			get_new_timeleft_offset = false;

			float flGameStartTime = gpGlobals->curtime;
			uintptr_t gamerules = (uintptr_t)g_pSDKTools->GetGameRules();
			if (gamerules)
			{
				sm_sendprop_info_t info;
				if (gamehelpers->FindSendPropInfo("CCSGameRulesProxy", "m_flGameStartTime", &info))
				{
					flGameStartTime = *(float *)(gamerules + info.actual_offset);
				}
			}

			timersys->NotifyOfGameStart(flGameStartTime - gpGlobals->curtime);
			timersys->MapTimeLeftChanged();
		}
		round_end_found = false;
	}
	else if (strcmp(name, "round_end") == 0)
	{
#if SOURCE_ENGINE == SE_CSGO
		if (event->GetInt("reason") == 16)
#else
		if (event->GetInt("reason") == 15)
#endif
		{
			get_new_timeleft_offset = true;
		}
		round_end_found = true;
	}
}
