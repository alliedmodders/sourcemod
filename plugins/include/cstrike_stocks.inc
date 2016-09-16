/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
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
 
#if defined _cstrike_stocks_included
 #endinput
#endif
#define _cstrike_stocks_included

#include <cstrike>

/**
 * Gets an entity's speed.
 *
 * @param entity 	Entity index.
 * @return			Entity's m_flLaggedMovementValue value.
 * @error			Invalid entity index, or lack of mod compliance.
 */
stock float CS_GetEntitySpeed(int entity)
{
	static bool gotconfig = false;
	static char datamap[32];
	
	if (!gotconfig)
	{
		Handle gc = LoadGameConfigFile("core.games");
		bool exists = GameConfGetKeyValue(gc, "m_flLaggedMovementValue", datamap, sizeof(datamap));
		CloseHandle(gc);
		
		if (!exists)
		{
			strcopy(datamap, sizeof(datamap), "m_flLaggedMovementValue");
		}
		
		gotconfig = true;
	}
	
	return GetEntPropFloat(entity, Prop_Data, datamap);
}

/**
 * Sets an entity's speed.
 *
 * @param entity	Entity index.
 * @param amount	Speed to set (default = 1.0, half = 0.5, double = 2.0).
 * @error			Invalid entity index, or lack of mod compliance.
 */
stock void CS_SetEntitySpeed(int entity, float amount)
{
	static bool gotconfig = false;
	static char datamap[32];
	
	if (!gotconfig)
	{
		Handle gc = LoadGameConfigFile("core.games");
		bool exists = GameConfGetKeyValue(gc, "m_flLaggedMovementValue", datamap, sizeof(datamap));
		CloseHandle(gc);
		
		if (!exists)
		{
			strcopy(datamap, sizeof(datamap), "m_flLaggedMovementValue");
		}
		
		gotconfig = true;
	}
	
	SetEntPropFloat(entity, Prop_Data, datamap, amount);
}
