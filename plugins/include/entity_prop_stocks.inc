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

enum MoveType
{
	MOVETYPE_NONE = 0,			/**< never moves */
	MOVETYPE_ISOMETRIC,			/**< For players */
	MOVETYPE_WALK,				/**< Player only - moving on the ground */
	MOVETYPE_STEP,				/**< gravity, special edge handling -- monsters use this */
	MOVETYPE_FLY,				/**< No gravity, but still collides with stuff */
	MOVETYPE_FLYGRAVITY,		/**< flies through the air + is affected by gravity */
	MOVETYPE_VPHYSICS,			/**< uses VPHYSICS for simulation */
	MOVETYPE_PUSH,				/**< no clip to world, push and crush */
	MOVETYPE_NOCLIP,			/**< No gravity, no collisions, still do velocity/avelocity */
	MOVETYPE_LADDER,			/**< Used by players only when going onto a ladder */
	MOVETYPE_OBSERVER,			/**< Observer movement, depends on player's observer mode */
	MOVETYPE_CUSTOM,			/**< Allows the entity to describe its own physics */
};

enum RenderMode
{	
	RENDER_NORMAL,			/**< src */
	RENDER_TRANSCOLOR,		/**< c*a+dest*(1-a) */
	RENDER_TRANSTEXTURE,	/**< src*a+dest*(1-a) */
	RENDER_GLOW,			/**< src*a+dest -- No Z buffer checks -- Fixed size in screen space */
	RENDER_TRANSALPHA,		/**< src*srca+dest*(1-srca) */
	RENDER_TRANSADD,		/**< src*a+dest */
	RENDER_ENVIRONMENTAL,	/**< not drawn, used for environmental effects */
	RENDER_TRANSADDFRAMEBLEND, /**< use a fractional frame value to blend between animation frames */
	RENDER_TRANSALPHAADD,	/**< src + dest*(1-a) */
	RENDER_WORLDGLOW,		/**< Same as kRenderGlow but not fixed size in screen space */
	RENDER_NONE,			/**< Don't render. */
};

enum RenderFx
{	
	RENDERFX_NONE = 0, 
	RENDERFX_PULSE_SLOW, 
	RENDERFX_PULSE_FAST, 
	RENDERFX_PULSE_SLOW_WIDE, 
	RENDERFX_PULSE_FAST_WIDE, 
	RENDERFX_FADE_SLOW, 
	RENDERFX_FADE_FAST, 
	RENDERFX_SOLID_SLOW, 
	RENDERFX_SOLID_FAST, 	   
	RENDERFX_STROBE_SLOW, 
	RENDERFX_STROBE_FAST, 
	RENDERFX_STROBE_FASTER, 
	RENDERFX_FLICKER_SLOW, 
	RENDERFX_FLICKER_FAST,
	RENDERFX_NO_DISSIPATION,
	RENDERFX_DISTORT,			/**< Distort/scale/translate flicker */
	RENDERFX_HOLOGRAM,			/**< kRenderFxDistort + distance fade */
	RENDERFX_EXPLODE,			/**< Scale up really big! */
	RENDERFX_GLOWSHELL,			/**< Glowing Shell */
	RENDERFX_CLAMP_MIN_SCALE,	/**< Keep this sprite from getting very small (SPRITES only!) */
	RENDERFX_ENV_RAIN,			/**< for environmental rendermode, make rain */
	RENDERFX_ENV_SNOW,			/**<  "        "            "    , make snow */
	RENDERFX_SPOTLIGHT,			/**< TEST CODE for experimental spotlight */
	RENDERFX_RAGDOLL,			/**< HACKHACK: TEST CODE for signalling death of a ragdoll character */
	RENDERFX_PULSE_FAST_WIDER,
	RENDERFX_MAX
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
 * @param entity	Entity index.
 * @return			Movetype, see enum above.
 * @error			Invalid entity index, or lack of mod compliance.
 */
stock MoveType:GetEntityMoveType(entity)
{
	new offset = GetEntSendPropOffs(entity, "movetype");
	return MoveType:GetEntData(entity, offset, 1);
}

/**
 * Sets an entity's movetype.
 *
 * @param entity	Entity index.
 * @param mt		Movetype, see enum above.
 * @noreturn
 * @error			Invalid entity index, or lack of mod compliance.
 */
stock SetEntityMoveType(entity, MoveType:mt)
{
	new offset = GetEntSendPropOffs(entity, "movetype");
	SetEntData(entity, offset, mt, 1, true);
}

/**
 * Gets an entity's render mode.
 *
 * @param entity	Entity index.
 * @return			RenderMode value.
 * @error			Invalid entity index, or lack of mod compliance.
 */
stock RenderMode:GetEntityRenderMode(entity)
{
	return RenderMode:GetEntProp(entity, Prop_Send, "m_nRenderMode", 1);
}

/**
 * Sets an entity's render mode.
 *
 * @param entity	Entity index.
 * @param mode		RenderMode value.
 * @noreturn
 * @error			Invalid entity index, or lack of mod compliance.
 */
stock SetEntityRenderMode(entity, RenderMode:mode)
{
	SetEntProp(entity, Prop_Send, "m_nRenderMode", mode, 1);
}

/**
 * Gets an entity's render Fx.
 *
 * @param entity	Entity index.
 * @return			RenderFx value.
 * @error			Invalid entity index, or lack of mod compliance.
 */
stock RenderFx:GetEntityRenderFx(entity)
{
	return RenderFx:GetEntProp(entity, Prop_Send, "m_nRenderFX", 1);
}

/**
 * Sets an entity's render Fx.
 *
 * @param entity	Entity index.
 * @param fx		RenderFx value.
 * @noreturn
 * @error			Invalid entity index, or lack of mod compliance.
 */
stock SetEntityRenderFx(entity, RenderFx:fx)
{
	SetEntProp(entity, Prop_Send, "m_nRenderFX", fx, 1);
}

/**
 * Sets an entity's color.
 *
 * @param entity	Entity index
 * @param r 		Amount of red (0-255)
 * @param g 		Amount of green (0-255)
 * @param b 		Amount of blue (0-255)
 * @param a 		Amount of alpha (0-255)
 * @noreturn
 * @error			Invalid entity index, or lack of mod compliance.
 */
stock SetEntityRenderColor(entity, r=255, g=255, b=255, a=255)
{
	new offset = GetEntSendPropOffs(entity, "m_clrRender");
	SetEntData(entity, offset, r, 1, true);
	SetEntData(entity, offset + 1, g, 1, true);
	SetEntData(entity, offset + 2, b, 1, true);
	SetEntData(entity, offset + 3, a, 1, true);
}

/* GuessSDKVersion */

/**
 * Gets an entity's gravity.
 *
 * @param entity 	Entity index.
 * @return			Entity's m_flGravity value.
 * @error			Invalid entity index, or lack of mod compliance.
 */
stock Float:GetEntityGravity(entity)
{
	return GetEntPropFloat(entity, Prop_Data, "m_flGravity");
}

/**
 * Sets an entity's gravity.
 *
 * @param entity	Entity index.
 * @param amount	Gravity to set (default = 800).
 * @noreturn
 * @error			Invalid entity index, or lack of mod compliance.
 */
stock SetEntityGravity(entity, Float:amount)
{
	SetEntPropFloat(entity, Prop_Data, "m_flGravity", amount);
}

/**
 * Sets an entity's health
 *
 * @param entity	Entity index.
 * @param amount	Health amount.
 * @noreturn
 * @error			Invalid entity index, or lack of mod compliance.
 */
stock SetEntityHealth(entity, amount)
{
	SetEntProp(entity, Prop_Send, "m_iHealth", amount);
}

/**
 * Get's a users current pressed buttons
 *
 * @param client	Client index
 * @return			Bitsum of buttons
 * @error			Invalid client index, client not in game, 
 *					or lack of mod compliance.
 */
stock GetClientButtons(client)
{
	return GetEntProp(client, Prop_Data, "m_nButtons");
}

#pragma deprecated Case change, use MoveType version
stock SetEntityMovetype(entity, any:mt)
{
	SetEntityMoveType(entity, MoveType:mt);
}

#pragma deprecated Case change, use MoveType version
stock GetEntityMovetype(entity)
{
	return _:GetEntityMoveType(entity);
}
