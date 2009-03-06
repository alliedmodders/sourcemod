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

// These defines are for client button presses.
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
#define IN_MOVERIGHT		(1 << 10)
#define IN_ATTACK2		(1 << 11)
#define IN_RUN			(1 << 12)
#define IN_RELOAD		(1 << 13)
#define IN_ALT1			(1 << 14)
#define IN_ALT2			(1 << 15)
#define IN_SCORE		(1 << 16)   	// Used by client.dll for when scoreboard is held down
#define IN_SPEED		(1 << 17)	// Player is holding the speed key
#define IN_WALK			(1 << 18)	// Player holding walk key
#define IN_ZOOM			(1 << 19)	// Zoom key for HUD zoom
#define IN_WEAPON1		(1 << 20)	// weapon defines these bits
#define IN_WEAPON2		(1 << 21)	// weapon defines these bits
#define IN_BULLRUSH		(1 << 22)
#define IN_GRENADE1		(1 << 23)	// grenade 1
#define IN_GRENADE2		(1 << 24)	// grenade 2

// CBaseEntity::m_fFlags
// PLAYER SPECIFIC FLAGS FIRST BECAUSE WE USE ONLY A FEW BITS OF NETWORK PRECISION
#define	FL_ONGROUND			(1 << 0)	// At rest / on the ground
#define FL_DUCKING			(1 << 1)	// Player flag -- Player is fully crouched
#define	FL_WATERJUMP			(1 << 2)	// player jumping out of water
#define FL_ONTRAIN			(1 << 3)	// Player is _controlling_ a train, so movement commands should be ignored on client during prediction.
#define FL_INRAIN			(1 << 4)	// Indicates the entity is standing in rain
#define FL_FROZEN			(1 << 5)	// Player is frozen for 3rd person camera
#define FL_ATCONTROLS			(1 << 6)	// Player can't move, but keeps key inputs for controlling another entity
#define	FL_CLIENT			(1 << 7)	// Is a player
#define FL_FAKECLIENT			(1 << 8)	// Fake client, simulated server side; don't send network messages to them
// NOTE if you move things up, make sure to change this value
#define PLAYER_FLAG_BITS		9
// NON-PLAYER SPECIFIC (i.e., not used by GameMovement or the client .dll ) -- Can still be applied to players, though
#define	FL_INWATER			(1 << 9)	// In water
#define	FL_FLY				(1 << 10)	// Changes the SV_Movestep() behavior to not need to be on ground
#define	FL_SWIM				(1 << 11)	// Changes the SV_Movestep() behavior to not need to be on ground (but stay in water)
#define	FL_CONVEYOR			(1 << 12)
#define	FL_NPC				(1 << 13)
#define	FL_GODMODE			(1 << 14)
#define	FL_NOTARGET			(1 << 15)
#define	FL_AIMTARGET			(1 << 16)	// set if the crosshair needs to aim onto the entity
#define	FL_PARTIALGROUND		(1 << 17)	// not all corners are valid
#define FL_STATICPROP			(1 << 18)	// Eetsa static prop!		
#define FL_GRAPHED			(1 << 19)	// worldgraph has this ent listed as something that blocks a connection
#define FL_GRENADE			(1 << 20)
#define FL_STEPMOVEMENT			(1 << 21)	// Changes the SV_Movestep() behavior to not do any processing
#define FL_DONTTOUCH			(1 << 22)	// Doesn't generate touch functions, generates Untouch() for anything it was touching when this flag was set
#define FL_BASEVELOCITY			(1 << 23)	// Base velocity has been applied this frame (used to convert base velocity into momentum)
#define FL_WORLDBRUSH			(1 << 24)	// Not moveable/removeable brush entity (really part of the world, but represented as an entity for transparency or something)
#define FL_OBJECT			(1 << 25)	// Terrible name. This is an object that NPCs should see. Missiles, for example.
#define FL_KILLME			(1 << 26)	// This entity is marked for death -- will be freed by game DLL
#define FL_ONFIRE			(1 << 27)	// You know...
#define FL_DISSOLVING			(1 << 28)	// We're dissolving!
#define FL_TRANSRAGDOLL			(1 << 29)	// In the process of turning into a client side ragdoll.
#define FL_UNBLOCKABLE_BY_PLAYER	(1 << 30)	// pusher that can't be blocked by the player
// END m_fFlags #defines

/**
 * Get an entity's flags.
 *
 * @param entity	Entity index.
 * @return			Entity's flags, see m_fFlag defines above
 * @error			Invalid entity index, or lack of mod compliance.
 */
stock GetEntityFlags(entity)
{
	static bool:gotconfig = false;
	static String:datamap[32];
	
	if (!gotconfig)
	{
		new Handle:gc = LoadGameConfigFile("core.games");
		new bool:exists = GameConfGetKeyValue(gc, "m_fFlags", datamap, sizeof(datamap));
		CloseHandle(gc);
		
		if (!exists)
		{
			strcopy(datamap, sizeof(datamap), "m_fFlags");
		}
		
		gotconfig = true;
	}
	
	return GetEntProp(entity, Prop_Data, datamap);
}


/**
 * Gets an entity's movetype.
 *
 * @param entity	Entity index.
 * @return			Movetype, see enum above.
 * @error			Invalid entity index, or lack of mod compliance.
 */
stock MoveType:GetEntityMoveType(entity)
{
	static bool:gotconfig = false;
	static String:datamap[32];
	
	if (!gotconfig)
	{
		new Handle:gc = LoadGameConfigFile("core.games");
		new bool:exists = GameConfGetKeyValue(gc, "m_MoveType", datamap, sizeof(datamap));
		CloseHandle(gc);
		
		if (!exists)
		{
			strcopy(datamap, sizeof(datamap), "m_MoveType");
		}
		
		gotconfig = true;
	}
	
	return MoveType:GetEntProp(entity, Prop_Data, datamap);
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
	static bool:gotconfig = false;
	static String:datamap[32];
	
	if (!gotconfig)
	{
		new Handle:gc = LoadGameConfigFile("core.games");
		new bool:exists = GameConfGetKeyValue(gc, "m_MoveType", datamap, sizeof(datamap));
		CloseHandle(gc);
		
		if (!exists)
		{
			strcopy(datamap, sizeof(datamap), "m_MoveType");
		}
		
		gotconfig = true;
	}
	
	SetEntProp(entity, Prop_Data, datamap, mt);
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
	static bool:gotconfig = false;
	static String:prop[32];
	
	if (!gotconfig)
	{
		new Handle:gc = LoadGameConfigFile("core.games");
		new bool:exists = GameConfGetKeyValue(gc, "m_nRenderMode", prop, sizeof(prop));
		CloseHandle(gc);
		
		if (!exists)
		{
			strcopy(prop, sizeof(prop), "m_nRenderMode");
		}
		
		gotconfig = true;
	}
	
	return RenderMode:GetEntProp(entity, Prop_Send, prop, 1);
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
	static bool:gotconfig = false;
	static String:prop[32];
	
	if (!gotconfig)
	{
		new Handle:gc = LoadGameConfigFile("core.games");
		new bool:exists = GameConfGetKeyValue(gc, "m_nRenderMode", prop, sizeof(prop));
		CloseHandle(gc);
		
		if (!exists)
		{
			strcopy(prop, sizeof(prop), "m_nRenderMode");
		}
		
		gotconfig = true;
	}
	
	SetEntProp(entity, Prop_Send, prop, mode, 1);
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
	static bool:gotconfig = false;
	static String:prop[32];
	
	if (!gotconfig)
	{
		new Handle:gc = LoadGameConfigFile("core.games");
		new bool:exists = GameConfGetKeyValue(gc, "m_nRenderFX", prop, sizeof(prop));
		CloseHandle(gc);
		
		if (!exists)
		{
			strcopy(prop, sizeof(prop), "m_nRenderFX");
		}
		
		gotconfig = true;
	}
	
	return RenderFx:GetEntProp(entity, Prop_Send, prop, 1);
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
	static bool:gotconfig = false;
	static String:prop[32];
	
	if (!gotconfig)
	{
		new Handle:gc = LoadGameConfigFile("core.games");
		new bool:exists = GameConfGetKeyValue(gc, "m_nRenderFX", prop, sizeof(prop));
		CloseHandle(gc);
		
		if (!exists)
		{
			strcopy(prop, sizeof(prop), "m_nRenderFX");
		}
		
		gotconfig = true;
	}
	
	SetEntProp(entity, Prop_Send, prop, fx, 1);
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
	static bool:gotconfig = false;
	static String:prop[32];
	
	if (!gotconfig)
	{
		new Handle:gc = LoadGameConfigFile("core.games");
		new bool:exists = GameConfGetKeyValue(gc, "m_clrRender", prop, sizeof(prop));
		CloseHandle(gc);
		
		if (!exists)
		{
			strcopy(prop, sizeof(prop), "m_clrRender");
		}
		
		gotconfig = true;
	}
	
	new offset = GetEntSendPropOffs(entity, prop);
	
	if (offset <= 0)
	{
		ThrowError("SetEntityRenderColor not supported by this mod");
	}
	
	SetEntData(entity, offset, r, 1, true);
	SetEntData(entity, offset + 1, g, 1, true);
	SetEntData(entity, offset + 2, b, 1, true);
	SetEntData(entity, offset + 3, a, 1, true);
}

/**
 * Gets an entity's gravity.
 *
 * @param entity 	Entity index.
 * @return			Entity's m_flGravity value.
 * @error			Invalid entity index, or lack of mod compliance.
 */
stock Float:GetEntityGravity(entity)
{
	static bool:gotconfig = false;
	static String:datamap[32];
	
	if (!gotconfig)
	{
		new Handle:gc = LoadGameConfigFile("core.games");
		new bool:exists = GameConfGetKeyValue(gc, "m_flGravity", datamap, sizeof(datamap));
		CloseHandle(gc);
		
		if (!exists)
		{
			strcopy(datamap, sizeof(datamap), "m_flGravity");
		}
		
		gotconfig = true;
	}
	
	return GetEntPropFloat(entity, Prop_Data, datamap);
}

/**
 * Sets an entity's gravity.
 *
 * @param entity	Entity index.
 * @param amount	Gravity to set (default = 1.0, half = 0.5, double = 2.0).
 * @noreturn
 * @error			Invalid entity index, or lack of mod compliance.
 */
stock SetEntityGravity(entity, Float:amount)
{
	static bool:gotconfig = false;
	static String:datamap[32];
	
	if (!gotconfig)
	{
		new Handle:gc = LoadGameConfigFile("core.games");
		new bool:exists = GameConfGetKeyValue(gc, "m_flGravity", datamap, sizeof(datamap));
		CloseHandle(gc);
		
		if (!exists)
		{
			strcopy(datamap, sizeof(datamap), "m_flGravity");
		}
		
		gotconfig = true;
	}
	
	SetEntPropFloat(entity, Prop_Data, datamap, amount);
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
	static bool:gotconfig = false;
	static String:prop[32];
	
	if (!gotconfig)
	{
		new Handle:gc = LoadGameConfigFile("core.games");
		new bool:exists = GameConfGetKeyValue(gc, "m_iHealth", prop, sizeof(prop));
		CloseHandle(gc);
		
		if (!exists)
		{
			strcopy(prop, sizeof(prop), "m_iHealth");
		}
		
		gotconfig = true;
	}
	
	decl String:cls[64];
	new PropFieldType:type;
	new offset;
	
	if (!GetEntityNetClass(entity, cls, sizeof(cls)))
	{
		ThrowError("SetEntityHealth not supported by this mod: Could not get serverclass name");
		return;
	}
	
	offset = FindSendPropInfo(cls, prop, type);
	
	if (offset <= 0)
	{
		ThrowError("SetEntityHealth not supported by this mod");
		return;
	}
	
	/* Dark Messiah uses a float for the health instead an integer */
	if (type == PropField_Float)
	{
		SetEntDataFloat(entity, offset, float(amount));
	}
	else
	{
		SetEntProp(entity, Prop_Send, prop, amount);
	}
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
	static bool:gotconfig = false;
	static String:datamap[32];
	
	if (!gotconfig)
	{
		new Handle:gc = LoadGameConfigFile("core.games");
		new bool:exists = GameConfGetKeyValue(gc, "m_nButtons", datamap, sizeof(datamap));
		CloseHandle(gc);
		
		if (!exists)
		{
			strcopy(datamap, sizeof(datamap), "m_nButtons");
		}
		
		gotconfig = true;
	}
	
	return GetEntProp(client, Prop_Data, datamap);
}
