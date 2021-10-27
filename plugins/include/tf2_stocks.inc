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

#if defined _tf2_stocks_included
 #endinput
#endif
#define _tf2_stocks_included

#include <tf2>
#include <sdktools>

#define TF_CONDFLAG_NONE            0
#define TF_CONDFLAG_SLOWED          (1 << 0)
#define TF_CONDFLAG_ZOOMED          (1 << 1)
#define TF_CONDFLAG_DISGUISING      (1 << 2)
#define TF_CONDFLAG_DISGUISED       (1 << 3)
#define TF_CONDFLAG_CLOAKED         (1 << 4)
#define TF_CONDFLAG_UBERCHARGED     (1 << 5)
#define TF_CONDFLAG_TELEPORTGLOW    (1 << 6)
#define TF_CONDFLAG_TAUNTING        (1 << 7)
#define TF_CONDFLAG_UBERCHARGEFADE  (1 << 8)
#define TF_CONDFLAG_CLOAKFLICKER    (1 << 9)
#define TF_CONDFLAG_TELEPORTING     (1 << 10)
#define TF_CONDFLAG_KRITZKRIEGED    (1 << 11)
#define TF_CONDFLAG_DEADRINGERED    (1 << 13)
#define TF_CONDFLAG_BONKED          (1 << 14)
#define TF_CONDFLAG_DAZED           (1 << 15)
#define TF_CONDFLAG_BUFFED          (1 << 16)
#define TF_CONDFLAG_CHARGING        (1 << 17)
#define TF_CONDFLAG_DEMOBUFF        (1 << 18)
#define TF_CONDFLAG_CRITCOLA        (1 << 19)
#define TF_CONDFLAG_INHEALRADIUS    (1 << 20)
#define TF_CONDFLAG_HEALING         (1 << 21)
#define TF_CONDFLAG_ONFIRE          (1 << 22)
#define TF_CONDFLAG_OVERHEALED      (1 << 23)
#define TF_CONDFLAG_JARATED         (1 << 24)
#define TF_CONDFLAG_BLEEDING        (1 << 25)
#define TF_CONDFLAG_DEFENSEBUFFED   (1 << 26)
#define TF_CONDFLAG_MILKED          (1 << 27)
#define TF_CONDFLAG_MEGAHEAL        (1 << 28)
#define TF_CONDFLAG_REGENBUFFED     (1 << 29)
#define TF_CONDFLAG_MARKEDFORDEATH  (1 << 30)

#define TF_DEATHFLAG_KILLERDOMINATION   (1 << 0)
#define TF_DEATHFLAG_ASSISTERDOMINATION (1 << 1)
#define TF_DEATHFLAG_KILLERREVENGE      (1 << 2)
#define TF_DEATHFLAG_ASSISTERREVENGE    (1 << 3)
#define TF_DEATHFLAG_FIRSTBLOOD         (1 << 4)
#define TF_DEATHFLAG_DEADRINGER         (1 << 5)
#define TF_DEATHFLAG_INTERRUPTED        (1 << 6)
#define TF_DEATHFLAG_GIBBED             (1 << 7)
#define TF_DEATHFLAG_PURGATORY          (1 << 8)
#define TF_DEATHFLAG_MINIBOSS           (1 << 9)
#define TF_DEATHFLAG_AUSTRALIUM         (1 << 10)

// Custom kill identifiers for the customkill property on the player_death event
enum {
	TF_CUSTOM_HEADSHOT = 1,
	TF_CUSTOM_BACKSTAB,
	TF_CUSTOM_BURNING,
	TF_CUSTOM_WRENCH_FIX,
	TF_CUSTOM_MINIGUN,
	TF_CUSTOM_SUICIDE,
	TF_CUSTOM_TAUNT_HADOUKEN,
	TF_CUSTOM_BURNING_FLARE,
	TF_CUSTOM_TAUNT_HIGH_NOON,
	TF_CUSTOM_TAUNT_GRAND_SLAM,
	TF_CUSTOM_PENETRATE_MY_TEAM,
	TF_CUSTOM_PENETRATE_ALL_PLAYERS,
	TF_CUSTOM_TAUNT_FENCING,
	TF_CUSTOM_PENETRATE_HEADSHOT,
	TF_CUSTOM_TAUNT_ARROW_STAB,
	TF_CUSTOM_TELEFRAG,
	TF_CUSTOM_BURNING_ARROW,
	TF_CUSTOM_FLYINGBURN,
	TF_CUSTOM_PUMPKIN_BOMB,
	TF_CUSTOM_DECAPITATION,
	TF_CUSTOM_TAUNT_GRENADE,
	TF_CUSTOM_BASEBALL,
	TF_CUSTOM_CHARGE_IMPACT,
	TF_CUSTOM_TAUNT_BARBARIAN_SWING,
	TF_CUSTOM_AIR_STICKY_BURST,
	TF_CUSTOM_DEFENSIVE_STICKY,
	TF_CUSTOM_PICKAXE,
	TF_CUSTOM_ROCKET_DIRECTHIT,
	TF_CUSTOM_TAUNT_UBERSLICE,
	TF_CUSTOM_PLAYER_SENTRY,
	TF_CUSTOM_STANDARD_STICKY,
	TF_CUSTOM_SHOTGUN_REVENGE_CRIT,
	TF_CUSTOM_TAUNT_ENGINEER_SMASH,
	TF_CUSTOM_BLEEDING,
	TF_CUSTOM_GOLD_WRENCH,
	TF_CUSTOM_CARRIED_BUILDING,
	TF_CUSTOM_COMBO_PUNCH,
	TF_CUSTOM_TAUNT_ENGINEER_ARM,
	TF_CUSTOM_FISH_KILL,
	TF_CUSTOM_TRIGGER_HURT,
	TF_CUSTOM_DECAPITATION_BOSS,
	TF_CUSTOM_STICKBOMB_EXPLOSION,
	TF_CUSTOM_AEGIS_ROUND,
	TF_CUSTOM_FLARE_EXPLOSION,
	TF_CUSTOM_BOOTS_STOMP,
	TF_CUSTOM_PLASMA,
	TF_CUSTOM_PLASMA_CHARGED,
	TF_CUSTOM_PLASMA_GIB,
	TF_CUSTOM_PRACTICE_STICKY,
	TF_CUSTOM_EYEBALL_ROCKET,
	TF_CUSTOM_HEADSHOT_DECAPITATION,
	TF_CUSTOM_TAUNT_ARMAGEDDON,
	TF_CUSTOM_FLARE_PELLET,
	TF_CUSTOM_CLEAVER,
	TF_CUSTOM_CLEAVER_CRIT,
	TF_CUSTOM_SAPPER_RECORDER_DEATH,
	TF_CUSTOM_MERASMUS_PLAYER_BOMB,
	TF_CUSTOM_MERASMUS_GRENADE,
	TF_CUSTOM_MERASMUS_ZAP,
	TF_CUSTOM_MERASMUS_DECAPITATION,
	TF_CUSTOM_CANNONBALL_PUSH,
	TF_CUSTOM_TAUNT_ALLCLASS_GUITAR_RIFF,
	TF_CUSTOM_THROWABLE,
	TF_CUSTOM_THROWABLE_KILL,
	TF_CUSTOM_SPELL_TELEPORT,
	TF_CUSTOM_SPELL_SKELETON,
	TF_CUSTOM_SPELL_MIRV,
	TF_CUSTOM_SPELL_METEOR,
	TF_CUSTOM_SPELL_LIGHTNING,
	TF_CUSTOM_SPELL_FIREBALL,
	TF_CUSTOM_SPELL_MONOCULUS,
	TF_CUSTOM_SPELL_BLASTJUMP,
	TF_CUSTOM_SPELL_BATS,
	TF_CUSTOM_SPELL_TINY,
	TF_CUSTOM_KART,
	TF_CUSTOM_GIANT_HAMMER,
	TF_CUSTOM_RUNE_REFLECT,
	TF_CUSTOM_DRAGONS_FURY_IGNITE,
	TF_CUSTOM_DRAGONS_FURY_BONUS_BURNING,
	TF_CUSTOM_SLAP_KILL,
	TF_CUSTOM_CROC,
	TF_CUSTOM_TAUNTATK_GASBLAST,
	TF_CUSTOM_AXTINGUISHER_BOOSTED
};

// Weapon codes as used in some events, such as player_death
// (not to be confused with Item Definition Indexes)
enum {
	TF_WEAPON_NONE = 0,
	TF_WEAPON_BAT,
	TF_WEAPON_BAT_WOOD,
	TF_WEAPON_BOTTLE,
	TF_WEAPON_FIREAXE,
	TF_WEAPON_CLUB,
	TF_WEAPON_CROWBAR,
	TF_WEAPON_KNIFE,
	TF_WEAPON_FISTS,
	TF_WEAPON_SHOVEL,
	TF_WEAPON_WRENCH,
	TF_WEAPON_BONESAW,
	TF_WEAPON_SHOTGUN_PRIMARY,
	TF_WEAPON_SHOTGUN_SOLDIER,
	TF_WEAPON_SHOTGUN_HWG,
	TF_WEAPON_SHOTGUN_PYRO,
	TF_WEAPON_SCATTERGUN,
	TF_WEAPON_SNIPERRIFLE,
	TF_WEAPON_MINIGUN,
	TF_WEAPON_SMG,
	TF_WEAPON_SYRINGEGUN_MEDIC,
	TF_WEAPON_TRANQ,
	TF_WEAPON_ROCKETLAUNCHER,
	TF_WEAPON_GRENADELAUNCHER,
	TF_WEAPON_PIPEBOMBLAUNCHER,
	TF_WEAPON_FLAMETHROWER,
	TF_WEAPON_GRENADE_NORMAL,
	TF_WEAPON_GRENADE_CONCUSSION,
	TF_WEAPON_GRENADE_NAIL,
	TF_WEAPON_GRENADE_MIRV,
	TF_WEAPON_GRENADE_MIRV_DEMOMAN,
	TF_WEAPON_GRENADE_NAPALM,
	TF_WEAPON_GRENADE_GAS,
	TF_WEAPON_GRENADE_EMP,
	TF_WEAPON_GRENADE_CALTROP,
	TF_WEAPON_GRENADE_PIPEBOMB,
	TF_WEAPON_GRENADE_SMOKE_BOMB,
	TF_WEAPON_GRENADE_HEAL,
	TF_WEAPON_GRENADE_STUNBALL,
	TF_WEAPON_GRENADE_JAR,
	TF_WEAPON_GRENADE_JAR_MILK,
	TF_WEAPON_PISTOL,
	TF_WEAPON_PISTOL_SCOUT,
	TF_WEAPON_REVOLVER,
	TF_WEAPON_NAILGUN,
	TF_WEAPON_PDA,
	TF_WEAPON_PDA_ENGINEER_BUILD,
	TF_WEAPON_PDA_ENGINEER_DESTROY,
	TF_WEAPON_PDA_SPY,
	TF_WEAPON_BUILDER,
	TF_WEAPON_MEDIGUN,
	TF_WEAPON_GRENADE_MIRVBOMB,
	TF_WEAPON_FLAMETHROWER_ROCKET,
	TF_WEAPON_GRENADE_DEMOMAN,
	TF_WEAPON_SENTRY_BULLET,
	TF_WEAPON_SENTRY_ROCKET,
	TF_WEAPON_DISPENSER,
	TF_WEAPON_INVIS,
	TF_WEAPON_FLAREGUN,
	TF_WEAPON_LUNCHBOX,
	TF_WEAPON_JAR,
	TF_WEAPON_COMPOUND_BOW,
	TF_WEAPON_BUFF_ITEM,
	TF_WEAPON_PUMPKIN_BOMB,
	TF_WEAPON_SWORD,
	TF_WEAPON_DIRECTHIT,
	TF_WEAPON_LIFELINE,
	TF_WEAPON_LASER_POINTER,
	TF_WEAPON_DISPENSER_GUN,
	TF_WEAPON_SENTRY_REVENGE,
	TF_WEAPON_JAR_MILK,
	TF_WEAPON_HANDGUN_SCOUT_PRIMARY,
	TF_WEAPON_BAT_FISH,
	TF_WEAPON_CROSSBOW,
	TF_WEAPON_STICKBOMB,
	TF_WEAPON_HANDGUN_SCOUT_SEC,
	TF_WEAPON_SODA_POPPER,
	TF_WEAPON_SNIPERRIFLE_DECAP,
	TF_WEAPON_RAYGUN,
	TF_WEAPON_PARTICLE_CANNON,
	TF_WEAPON_MECHANICAL_ARM,
	TF_WEAPON_DRG_POMSON,
	TF_WEAPON_BAT_GIFTWRAP,
	TF_WEAPON_GRENADE_ORNAMENT,
	TF_WEAPON_RAYGUN_REVENGE,
	TF_WEAPON_PEP_BRAWLER_BLASTER,
	TF_WEAPON_CLEAVER,
	TF_WEAPON_GRENADE_CLEAVER,
	TF_WEAPON_STICKY_BALL_LAUNCHER,
	TF_WEAPON_GRENADE_STICKY_BALL,
	TF_WEAPON_SHOTGUN_BUILDING_RESCUE,
	TF_WEAPON_CANNON,
	TF_WEAPON_THROWABLE,
	TF_WEAPON_GRENADE_THROWABLE,
	TF_WEAPON_PDA_SPY_BUILD,
	TF_WEAPON_GRENADE_WATERBALLOON,
	TF_WEAPON_HARVESTER_SAW,
	TF_WEAPON_SPELLBOOK,
	TF_WEAPON_SPELLBOOK_PROJECTILE,
	TF_WEAPON_SNIPERRIFLE_CLASSIC,
	TF_WEAPON_PARACHUTE,
	TF_WEAPON_GRAPPLINGHOOK,
	TF_WEAPON_PASSTIME_GUN,
	TF_WEAPON_CHARGED_SMG,
	TF_WEAPON_BREAKABLE_SIGN,
	TF_WEAPON_ROCKETPACK,
	TF_WEAPON_SLAP,
	TF_WEAPON_JAR_GAS,
	TF_WEAPON_GRENADE_JAR_GAS,
	TF_WEAPON_FLAME_BALL
};

// TF2 Weapon Loadout Slots
enum
{
	TFWeaponSlot_Primary,
	TFWeaponSlot_Secondary,
	TFWeaponSlot_Melee,
	TFWeaponSlot_Grenade,
	TFWeaponSlot_Building,
	TFWeaponSlot_PDA,
	TFWeaponSlot_Item1,
	TFWeaponSlot_Item2
};

// Identifiers for the eventtype property on the teamplay_flag_event event
enum {
	TF_FLAGEVENT_PICKEDUP = 1,
	TF_FLAGEVENT_CAPTURED,
	TF_FLAGEVENT_DEFENDED,
	TF_FLAGEVENT_DROPPED,
	TF_FLAGEVENT_RETURNED
};

enum TFResourceType
{
	TFResource_Ping,
	TFResource_Score,
	TFResource_Deaths,
	TFResource_TotalScore,
	TFResource_Captures,
	TFResource_Defenses,
	TFResource_Dominations,
	TFResource_Revenge,
	TFResource_BuildingsDestroyed,
	TFResource_Headshots,
	TFResource_Backstabs,
	TFResource_HealPoints,
	TFResource_Invulns,
	TFResource_Teleports,
	TFResource_ResupplyPoints,
	TFResource_KillAssists,
	TFResource_MaxHealth,
	TFResource_PlayerClass
};

static const char TFResourceNames[][] =
{
	"m_iPing",
	"m_iScore",
	"m_iDeaths",
	"m_iTotalScore",
	"m_iCaptures",
	"m_iDefenses",
	"m_iDominations",
	"m_iRevenge",
	"m_iBuildingsDestroyed",
	"m_iHeadshots",
	"m_iBackstabs",
	"m_iHealPoints",
	"m_iInvulns",
	"m_iTeleports",
	"m_iResupplyPoints",
	"m_iKillAssists",
	"m_iMaxHealth",
	"m_iPlayerClass"
};

/**
 * Gets a client's current team.
 *
 * @param client        Client index.
 * @return              Current TFTeam of client.
 * @error               Invalid client index.
 */
stock TFTeam TF2_GetClientTeam(int client)
{
	return view_as<TFTeam>(GetClientTeam(client));
}

/**
 * Changes a client's current team.
 *
 * @param client        Client index.
 * @param team          TFTeam team symbol.
 * @error               Invalid client index.
 */
stock void TF2_ChangeClientTeam(int client, TFTeam team)
{
    ChangeClientTeam(client, view_as<int>(team));
}

/**
 * Gets a client's current class.
 *
 * @param client        Player's index.
 * @return              Current TFClassType of player.
 * @error               Invalid client index.
 */
stock TFClassType TF2_GetPlayerClass(int client)
{
	return view_as<TFClassType>(GetEntProp(client, Prop_Send, "m_iClass"));
}

/**
 * Sets a client's class.
 *
 * Note: If setting player class in a player spawn hook weapons should be set to false.
 *
 * @param client        Player's index.
 * @param classType     TFClassType class symbol.
 * @param weapons       This parameter is ignored.
 * @param persistent    If true, changes the player's desired class so the change stays after death.
 * @error               Invalid client index.
 */
stock void TF2_SetPlayerClass(int client, TFClassType classType, bool weapons=true, bool persistent=true)
{
	SetEntProp(client, Prop_Send, "m_iClass", view_as<int>(classType));

	if (persistent)
	{
		SetEntProp(client, Prop_Send, "m_iDesiredPlayerClass", view_as<int>(classType));
	}
}

/**
 * Retrieves client data from the resource entity
 *
 * @param client		Player's index.
 * @param type			ResourceType constant
 * @return				Value or -1 on failure.
 * @error				Invalid client index, client not in game or failed to find resource entity.
 * @deprecated          Use GetPlayerResourceEntity and GetEntProp instead.
 */
#pragma deprecated Use GetPlayerResourceEntity and GetEntProp instead
stock int TF2_GetPlayerResourceData(int client, TFResourceType type)
{
	if (!IsClientConnected(client))
	{
		return -1;
	}

	int offset = FindSendPropInfo("CTFPlayerResource", TFResourceNames[type]);

	if (offset < 1)
	{
		return -1;
	}

	int entity = GetPlayerResourceEntity();

	if (entity == -1)
	{
		return -1;
	}

	return GetEntData(entity, offset + (client*4));
}

/**
 * Sets client data in the resource entity
 *
 * Note: The game overwrites these values every frame, so changing them will have very little effect.
 *
 * @param client        Player's index.
 * @param type          ResourceType constant
 * @param value         Value to set.
 * @return              Value or -1 on failure.
 * @error               Invalid client index, client not in game or failed to find resource entity.
 * @deprecated          Use GetPlayerResourceEntity and SetEntProp instead.
 */
#pragma deprecated Use GetPlayerResourceEntity and SetEntProp instead
stock bool TF2_SetPlayerResourceData(int client, TFResourceType type, any value)
{
	if (!IsClientConnected(client))
	{
		return false;
	}

	int offset = FindSendPropInfo("CTFPlayerResource", TFResourceNames[type]);

	if (offset < 1)
	{
		return false;
	}

	int entity = GetPlayerResourceEntity();

	if (entity == -1)
	{
		return false;
	}

	SetEntData(entity, offset + (client*4), value);

	return true;
}

/**
 * Removes all weapons from a client's weapon slot
 *
 * @param client        Player's index.
 * @param slot          Slot index (0-5)
 * @error               Invalid client, invalid slot or lack of mod support
 */
stock void TF2_RemoveWeaponSlot(int client, int slot)
{
	int weaponIndex;
	while ((weaponIndex = GetPlayerWeaponSlot(client, slot)) != -1)
	{
		// bug #6206
		// papering over a valve bug where a weapon's extra wearables aren't properly removed from the weapon's owner
		int extraWearable = GetEntPropEnt(weaponIndex, Prop_Send, "m_hExtraWearable");
		if (extraWearable != -1)
		{
			TF2_RemoveWearable(client, extraWearable);
		}

		extraWearable = GetEntPropEnt(weaponIndex, Prop_Send, "m_hExtraWearableViewModel");
		if (extraWearable != -1)
		{
			TF2_RemoveWearable(client, extraWearable);
		}

		RemovePlayerItem(client, weaponIndex);
		AcceptEntityInput(weaponIndex, "Kill");
	}
}

/**
 * Removes all weapons from a client
 *
 * @param client        Player's index.
 */
stock void TF2_RemoveAllWeapons(int client)
{
	for (int i = 0; i <= 5; i++)
	{
		TF2_RemoveWeaponSlot(client, i);
	}
}

/**
 * Gets a player's condition bits
 *
 * @param client        Player's index.
 * @return              Player's condition bits
 * @deprecated          Use TF2_IsPlayerInCondition instead.
 */
#pragma deprecated Use TF2_IsPlayerInCondition instead.
stock int TF2_GetPlayerConditionFlags(int client)
{
	return GetEntProp(client, Prop_Send, "m_nPlayerCond")|GetEntProp(client, Prop_Send, "_condition_bits");
}

/**
 * Check whether or not a condition is set on a player
 *
 * @param client        Player's index.
 * @param cond          TFCond constant
 * @return              True if set, false otherwise
 */
stock bool TF2_IsPlayerInCondition(int client, TFCond cond)
{
	// Conditions are stored across multiple netprops now, one for each 32-bit segment.
	int iCond = view_as<int>(cond);
	switch (iCond / 32)
	{
		case 0:
		{
			int bit = 1 << iCond;
			if ((GetEntProp(client, Prop_Send, "m_nPlayerCond") & bit) == bit)
			{
				return true;
			}

			if ((GetEntProp(client, Prop_Send, "_condition_bits") & bit) == bit)
			{
				return true;
			}
		}
		case 1:
		{
			int bit = (1 << (iCond - 32));
			if ((GetEntProp(client, Prop_Send, "m_nPlayerCondEx") & bit) == bit)
			{
				return true;
			}
		}
		case 2:
		{
			int bit = (1 << (iCond - 64));
			if ((GetEntProp(client, Prop_Send, "m_nPlayerCondEx2") & bit) == bit)
			{
				return true;
			}
		}
		case 3:
		{
			int bit = (1 << (iCond - 96));
			if ((GetEntProp(client, Prop_Send, "m_nPlayerCondEx3") & bit) == bit)
			{
				return true;
			}
		}
		case 4:
		{
			int bit = (1 << (iCond - 128));
			if ((GetEntProp(client, Prop_Send, "m_nPlayerCondEx4") & bit) == bit)
			{
				return true;
			}
		}
		default:
		{
			ThrowError("Invalid TFCond value %d", iCond);
		}
	}

	return false;
}

/**
 * Gets an entity's object type.
 *
 * @param entity        Entity index.
 * @return              Current TFObjectType of entity.
 * @error               Invalid entity index.
 */
stock TFObjectType TF2_GetObjectType(int entity)
{
	int offset = GetEntSendPropOffs(entity, "m_iObjectType");

	if (offset <= 0)
	{
		ThrowError("Entity index %d is not an object", entity);
	}

	return view_as<TFObjectType>(GetEntData(entity, offset));
}

/**
 * Gets an entity's object mode.
 *
 * @param entity        Entity index.
 * @return              Current TFObjectMode of entity.
 * @error               Invalid entity index.
 */
stock TFObjectMode TF2_GetObjectMode(int entity)
{
	int offset = GetEntSendPropOffs(entity, "m_iObjectMode");

	if (offset <= 0)
	{
		ThrowError("Entity index %d is not an object", entity);
	}

	return view_as<TFObjectMode>(GetEntData(entity, offset));
}
