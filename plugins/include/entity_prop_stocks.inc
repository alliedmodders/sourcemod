/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
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
 
#if defined _entity_prop_stocks_included
 #endinput
#endif
#define _entity_prop_stocks_included

enum
{
	MOVETYPE_NONE = 0,			/** never moves */
	MOVETYPE_ISOMETRIC,			/** For players */
	MOVETYPE_WALK,				/** Player only - moving on the ground */
	MOVETYPE_STEP,				/** gravity, special edge handling -- monsters use this */
	MOVETYPE_FLY,				/** No gravity, but still collides with stuff */
	MOVETYPE_FLYGRAVITY,			/** flies through the air + is affected by gravity */
	MOVETYPE_VPHYSICS,			/** uses VPHYSICS for simulation */
	MOVETYPE_PUSH,				/** no clip to world, push and crush */
	MOVETYPE_NOCLIP,			/** No gravity, no collisions, still do velocity/avelocity */
	MOVETYPE_LADDER,			/** Used by players only when going onto a ladder */
	MOVETYPE_OBSERVER,			/** Observer movement, depends on player's observer mode */
	MOVETYPE_CUSTOM,			/** Allows the entity to describe its own physics */
};

#define IN_ATTACK		(1 << 0)
#define IN_JUMP			(1 << 1)
#define IN_DUCK			(1 << 2)
#define IN_FORWARD		(1 << 3)
#define IN_BACK			(1 << 4)
#define IN_USE			(1 << 5)
#define IN_CANCEL		(1 << 6)
#define IN_LEFT			(1 << 7)
#define IN_RIGHT		(1 << 8)
#define IN_MOVELEFT		(1 << 9)
#define IN_MOVERIGHT	(1 << 10)
#define IN_ATTACK2		(1 << 11)
#define IN_RUN			(1 << 12)
#define IN_RELOAD		(1 << 13)
#define IN_ALT1			(1 << 14)
#define IN_ALT2			(1 << 15)
#define IN_SCORE		(1 << 16)   // Used by client.dll for when scoreboard is held down
#define IN_SPEED		(1 << 17)	// Player is holding the speed key
#define IN_WALK			(1 << 18)	// Player holding walk key
#define IN_ZOOM			(1 << 19)	// Zoom key for HUD zoom
#define IN_WEAPON1		(1 << 20)	// weapon defines these bits
#define IN_WEAPON2		(1 << 21)	// weapon defines these bits
#define IN_BULLRUSH		(1 << 22)
#define IN_GRENADE1		(1 << 23)	// grenade 1
#define IN_GRENADE2		(1 << 24)	// grenade 2

/**
 * Gets an entity's movetype.
 *
 * @param index		Entity index
 * @return		Movetype, see enum above
 */
stock GetEntityMovetype(index)
{
	new offset = GetEntSendPropOffs(index, "movetype");
	return GetEntData(index, offset, 1);
}

/**
 * Sets an entity's movetype.
 *
 * @param index		Entity index
 * @param mt		Movetype, see enum above
 * @noreturn
 */
stock SetEntityMovetype(index, mt)
{
	new offset = GetEntSendPropOffs(index, "movetype");
	SetEntData(index, offset, mt, 1, true);
}

/**
 * Sets an entity's color.
 *
 * @param index		Entity index
 * @param r 		Amount of red (0-255)
 * @param g 		Amount of green (0-255)
 * @param b 		Amount of blue (0-255)
 * @param a 		Amount of alpha (0-255)
 * @noreturn
 */
stock SetEntityRenderColor(index, r=255, g=255, b=255, a=255)
{
	new offset = GetEntSendPropOffs(index, "m_clrRender");
	SetEntData(index, offset, r, 1, true);
	SetEntData(index, offset + 1, g, 1, true);
	SetEntData(index, offset + 2, b, 1, true);
	SetEntData(index, offset + 3, a, 1, true);
}

/**
 * Gets an entity's gravity.
 *
 * @param index		Entity index
 * @return		Entity's m_flGravity value
 */
stock Float:GetEntityGravity(index)
{
	new offset = FindDataMapOffs(index, "m_flGravity");
	return GetEntDataFloat(index, offset);	
}

/**
 * Sets an entity's gravity.
 *
 * @param index		Entity index
 * @param amount	Gravity to set. Default = 800
 * @noreturn
 */
stock SetEntityGravity(index, Float:amount)
{
	new offset = FindDataMapOffs(index, "m_flGravity");
	SetEntDataFloat(index, offset, amount, true);	
}

/**
 * Sets an entity's health
 *
 * @param index		Entity index
 * @param amount	Health
 * @noreturn
 */
stock SetEntityHealth(index, amount)
{
	SetEntProp(client, Prop_Send, "m_iHealth", amount, 4)
	SetEntProp(client, Prop_Data, "m_iHealth", amount, 4)
}

/**
 * Get's a users current pressed buttons
 *
 * @param client	Client index
 * @return			Bitsum of buttons
 */
stock GetClientButtons(client)
{
	new offset = FindDataMapOffs(client, "m_nButtons");
	return GetEntData(client, offset, 4);
}