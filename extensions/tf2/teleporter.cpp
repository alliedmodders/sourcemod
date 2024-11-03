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

#include "teleporter.h"

CDetour *canPlayerTeleportDetour = NULL;

IForward *g_teleportForward = NULL;

class CTFPlayer;

DETOUR_DECL_MEMBER1(CanPlayerBeTeleported, bool, CTFPlayer *, pPlayer)
{
	bool origCanTeleport = DETOUR_MEMBER_CALL(CanPlayerBeTeleported)(pPlayer);

	cell_t teleporterCell = gamehelpers->EntityToBCompatRef((CBaseEntity *)this);
	cell_t playerCell = gamehelpers->EntityToBCompatRef((CBaseEntity *)pPlayer);

	if (!g_teleportForward)
	{
		g_pSM->LogMessage(myself, "Teleport forward is invalid");
		return origCanTeleport;
	}

	cell_t returnValue = origCanTeleport ? 1 : 0;

	g_teleportForward->PushCell(playerCell); // client
	g_teleportForward->PushCell(teleporterCell); // teleporter
	g_teleportForward->PushCellByRef(&returnValue); // return value

	cell_t result = 0;

	g_teleportForward->Execute(&result);

	if (result > Pl_Continue)
	{
		// plugin wants to override the game (returned something other than Plugin_Continue)
		return returnValue == 1;
	}
	else
	{
		return origCanTeleport; // let the game decide
	}
}

bool InitialiseTeleporterDetour()
{
	canPlayerTeleportDetour = DETOUR_CREATE_MEMBER(CanPlayerBeTeleported, "CanPlayerTeleport");

	if (canPlayerTeleportDetour != NULL)
	{
		canPlayerTeleportDetour->EnableDetour();
		return true;
	}

	g_pSM->LogError(myself, "Teleport forward could not be initialized - Disabled hook");

	return false;
}

void RemoveTeleporterDetour()
{
	if (canPlayerTeleportDetour != NULL)
	{
		canPlayerTeleportDetour->Destroy();
		canPlayerTeleportDetour = NULL;
	}
}
