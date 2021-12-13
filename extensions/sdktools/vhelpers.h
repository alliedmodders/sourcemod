/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
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

#ifndef _INCLUDE_SDKTOOLS_VHELPERS_H_
#define _INCLUDE_SDKTOOLS_VHELPERS_H_

#include <sh_list.h>
#include <eiface.h>
#include <IBinTools.h>

using namespace SourceMod;

struct CallHelper
{
	CallHelper() : call(NULL), supported(false), setup(false)
	{
	}
	void Shutdown()
	{
		if (call)
		{
			call->Destroy();
			call = NULL;
			supported = false;
		}
	}
	ICallWrapper *call;
	bool supported;
	bool setup;
};

void Teleport(CBaseEntity *pEntity, Vector *origin, QAngle *ang, Vector *velocity);
bool IsTeleportSupported();

void GetVelocity(CBaseEntity *pEntity, Vector *velocity, AngularImpulse *angvelocity);
bool IsGetVelocitySupported();

bool GetEyeAngles(CBaseEntity *pEntity, QAngle *pAngles);
bool IsEyeAnglesSupported();

int GetClientAimTarget(edict_t *pEdict, bool only_players);

bool GetPlayerInfo(int client, player_info_t *info);

void ShutdownHelpers();

bool FindNestedDataTable(SendTable *pTable, const char *name);

bool CollisionRulesChanged(CBaseEntity *pEntity);

#endif //_INCLUDE_SDKTOOLS_VHELPERS_H_
