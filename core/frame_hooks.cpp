/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
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
#include "frame_hooks.h"
#include "TimerSys.h"
#include "Database.h"
#include "HalfLife2.h"
#include "MenuStyle_Valve.h"
#include "MenuStyle_Radio.h"
#include "PlayerManager.h"

float g_LastMenuTime = 0.0f;
float g_LastAuthCheck = 0.0f;

void RunFrameHooks(bool simulating)
{
	/* Frame based hooks */
	g_DBMan.RunFrame();
	g_HL2.ProcessFakeCliCmdQueue();
	g_SourceMod.ProcessGameFrameHooks(simulating);

	float curtime = *g_pUniversalTime;

	if (curtime - g_LastMenuTime >= 1.0f)
	{
		g_ValveMenuStyle.ProcessWatchList();
		g_RadioMenuStyle.ProcessWatchList();
		g_LastMenuTime = curtime;
	}

	if (*g_NumPlayersToAuth && curtime - g_LastAuthCheck >= 0.7f)
	{
		g_Players.RunAuthChecks();
		g_LastAuthCheck = curtime;
	}
}
