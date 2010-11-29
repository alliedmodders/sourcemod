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

#include "gameplayrules.h"

CDetour *setInWaitingForPlayersDetour = NULL;

IForward *g_waitingPlayersStartForward = NULL;
IForward *g_waitingPlayersEndForward = NULL;

DETOUR_DECL_MEMBER1(SetInWaitingForPlayers, void, bool, bWaitingForPlayers)
{
	DETOUR_MEMBER_CALL(SetInWaitingForPlayers)(bWaitingForPlayers);

	if (bWaitingForPlayers)
	{
		if (!g_waitingPlayersStartForward)
		{
			g_pSM->LogMessage(myself, "Invalid Forward");
		}
		else
		{
			g_waitingPlayersStartForward->Execute(NULL);
		}
	}
	else
	{
		if (!g_waitingPlayersEndForward)
		{
			g_pSM->LogMessage(myself, "Invalid Forward");
		}
		else
		{
			g_waitingPlayersEndForward->Execute(NULL);
		}
	}
}

bool InitialiseRulesDetours()
{
	setInWaitingForPlayersDetour = DETOUR_CREATE_MEMBER(SetInWaitingForPlayers, "SetInWaitingForPlayers");

	if (setInWaitingForPlayersDetour != NULL)
	{
		setInWaitingForPlayersDetour->EnableDetour();
		return true;
	}

	g_pSM->LogError(myself, "No Gameplay Rules detours could be initialized - Disabled Gameplay Rules functions");
	return false;
}

void RemoveRulesDetours()
{
	setInWaitingForPlayersDetour->Destroy();
}
