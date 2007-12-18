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

/**
 * Sets an entity's color.
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