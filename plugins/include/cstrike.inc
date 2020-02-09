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

#if defined _cstrike_included
 #endinput
#endif
#define _cstrike_included

#define CS_TEAM_NONE        0   /**< No team yet. */
#define CS_TEAM_SPECTATOR   1   /**< Spectators. */
#define CS_TEAM_T           2   /**< Terrorists. */
#define CS_TEAM_CT          3   /**< Counter-Terrorists. */

#define CS_SLOT_PRIMARY     0   /**< Primary weapon slot. */
#define CS_SLOT_SECONDARY   1   /**< Secondary weapon slot. */
#define CS_SLOT_KNIFE       2   /**< Knife slot. */
#define CS_SLOT_GRENADE     3   /**< Grenade slot (will only return one grenade). */
#define CS_SLOT_C4          4   /**< C4 slot. */

#define CS_DMG_HEADSHOT     (1 << 30)    /**< Headshot */

enum CSRoundEndReason
{
	CSRoundEnd_TargetBombed = 0,           /**< Target Successfully Bombed! */
	CSRoundEnd_VIPEscaped,                 /**< The VIP has escaped! - Doesn't exist on CS:GO */
	CSRoundEnd_VIPKilled,                  /**< VIP has been assassinated! - Doesn't exist on CS:GO */
	CSRoundEnd_TerroristsEscaped,          /**< The terrorists have escaped! */
	CSRoundEnd_CTStoppedEscape,            /**< The CTs have prevented most of the terrorists from escaping! */
	CSRoundEnd_TerroristsStopped,          /**< Escaping terrorists have all been neutralized! */
	CSRoundEnd_BombDefused,                /**< The bomb has been defused! */
	CSRoundEnd_CTWin,                      /**< Counter-Terrorists Win! */
	CSRoundEnd_TerroristWin,               /**< Terrorists Win! */
	CSRoundEnd_Draw,                       /**< Round Draw! */
	CSRoundEnd_HostagesRescued,            /**< All Hostages have been rescued! */
	CSRoundEnd_TargetSaved,                /**< Target has been saved! */
	CSRoundEnd_HostagesNotRescued,         /**< Hostages have not been rescued! */
	CSRoundEnd_TerroristsNotEscaped,       /**< Terrorists have not escaped! */
	CSRoundEnd_VIPNotEscaped,              /**< VIP has not escaped! - Doesn't exist on CS:GO */
	CSRoundEnd_GameStart,                  /**< Game Commencing! */
	
	// The below only exist on CS:GO
	CSRoundEnd_TerroristsSurrender,        /**< Terrorists Surrender */
	CSRoundEnd_CTSurrender,                /**< CTs Surrender */
	CSRoundEnd_TerroristsPlanted,          /**< Terrorists Planted the bomb */
	CSRoundEnd_CTsReachedHostage           /**< CTs Reached the hostage */
};

enum CSWeaponID
{
	CSWeapon_NONE = 0,
	CSWeapon_P228,
	CSWeapon_GLOCK,
	CSWeapon_SCOUT,
	CSWeapon_HEGRENADE,
	CSWeapon_XM1014,
	CSWeapon_C4,
	CSWeapon_MAC10,
	CSWeapon_AUG,
	CSWeapon_SMOKEGRENADE,
	CSWeapon_ELITE,
	CSWeapon_FIVESEVEN,
	CSWeapon_UMP45,
	CSWeapon_SG550,
	CSWeapon_GALIL,
	CSWeapon_FAMAS,
	CSWeapon_USP,
	CSWeapon_AWP,
	CSWeapon_MP5NAVY,
	CSWeapon_M249,
	CSWeapon_M3,
	CSWeapon_M4A1,
	CSWeapon_TMP,
	CSWeapon_G3SG1,
	CSWeapon_FLASHBANG,
	CSWeapon_DEAGLE,
	CSWeapon_SG552,
	CSWeapon_AK47,
	CSWeapon_KNIFE,
	CSWeapon_P90,
	CSWeapon_SHIELD,
	CSWeapon_KEVLAR,
	CSWeapon_ASSAULTSUIT,
	CSWeapon_NIGHTVISION, //Anything below is CS:GO ONLY
	CSWeapon_GALILAR,
	CSWeapon_BIZON,
	CSWeapon_MAG7,
	CSWeapon_NEGEV,
	CSWeapon_SAWEDOFF,
	CSWeapon_TEC9,
	CSWeapon_TASER,
	CSWeapon_HKP2000,
	CSWeapon_MP7,
	CSWeapon_MP9,
	CSWeapon_NOVA,
	CSWeapon_P250,
	CSWeapon_SCAR17,
	CSWeapon_SCAR20,
	CSWeapon_SG556,
	CSWeapon_SSG08,
	CSWeapon_KNIFE_GG,
	CSWeapon_MOLOTOV,
	CSWeapon_DECOY,
	CSWeapon_INCGRENADE,
	CSWeapon_DEFUSER,
	CSWeapon_HEAVYASSAULTSUIT,
	//The rest are actual item definition indexes for CS:GO
	CSWeapon_CUTTERS = 56,
	CSWeapon_HEALTHSHOT = 57,
	CSWeapon_KNIFE_T = 59,
	CSWeapon_M4A1_SILENCER = 60,
	CSWeapon_USP_SILENCER = 61,
	CSWeapon_CZ75A = 63,
	CSWeapon_REVOLVER = 64,
	CSWeapon_TAGGRENADE = 68,
	CSWeapon_FISTS = 69,
	CSWeapon_BREACHCHARGE = 70,
	CSWeapon_TABLET = 72,
	CSWeapon_MELEE = 74,
	CSWeapon_AXE = 75,
	CSWeapon_HAMMER = 76,
	CSWeapon_SPANNER = 78,
	CSWeapon_KNIFE_GHOST = 80,
	CSWeapon_FIREBOMB = 81,
	CSWeapon_DIVERSION = 82,
	CSWeapon_FRAGGRENADE = 83,
	CSWeapon_SNOWBALL = 84,
	CSWeapon_BUMPMINE = 85,
	CSWeapon_MAX_WEAPONS_NO_KNIFES, // Max without the knife item defs, useful when treating all knives as a regular knife.
	CSWeapon_BAYONET = 500,
	CSWeapon_KNIFE_FLIP = 505,
	CSWeapon_KNIFE_GUT = 506,
	CSWeapon_KNIFE_KARAMBIT = 507,
	CSWeapon_KNIFE_M9_BAYONET = 508,
	CSWeapon_KNIFE_TATICAL = 509,
	CSWeapon_KNIFE_FALCHION = 512,
	CSWeapon_KNIFE_SURVIVAL_BOWIE = 514,
	CSWeapon_KNIFE_BUTTERFLY = 515,
	CSWeapon_KNIFE_PUSH = 516,
	CSWeapon_KNIFE_CORD = 517,
	CSWeapon_KNIFE_CANIS = 518,
	CSWeapon_KNIFE_URSUS = 519,
	CSWeapon_KNIFE_GYPSY_JACKKNIFE = 520,
	CSWeapon_KNIFE_OUTDOOR = 521,
	CSWeapon_KNIFE_STILETTO = 522,
	CSWeapon_KNIFE_WIDOWMAKER = 523,
	CSWeapon_KNIFE_SKELETON = 525,
	CSWeapon_MAX_WEAPONS //THIS MUST BE LAST, EASY WAY TO CREATE LOOPS. When looping, do CS_IsValidWeaponID(i), to check.
};

/**
 * Called when a player attempts to purchase an item.
 * Return Plugin_Continue to allow the purchase or return a
 * higher action to deny.
 *
 * @param client        Client index
 * @param weapon        User input for weapon name
 */
forward Action CS_OnBuyCommand(int client, const char[] weapon);

/**
 * Called when CSWeaponDrop is called
 * Return Plugin_Continue to allow the call or return a
 * higher action to block.
 *
 * @param client        Client index
 * @param weaponIndex   Weapon index
 */
forward Action CS_OnCSWeaponDrop(int client, int weaponIndex);

/**
 * Called when game retrieves a weapon's price for a player.
 * Return Plugin_Continue to use default value or return a higher
 * action to use a newly-set price.
 * 
 * @note This can be called multiple times per weapon purchase
 * 
 * @param client        Client index
 * @param weapon        Weapon classname
 * @param price         Buffer param for the price of the weapon
 *
 * @note Not all "weapons" call GetWeaponPrice. Example: c4, knife, vest, vest helmet, night vision.
 */
forward Action CS_OnGetWeaponPrice(int client, const char[] weapon, int &price);

/**
 * Called when TerminateRound is called.
 * Return Plugin_Continue to ignore, return Plugin_Changed to continue,
 * using the given delay and reason, or return Plugin_Handled or a higher
 * action to block TerminateRound from firing.
 *
 * @param delay         Time (in seconds) until new round starts
 * @param reason        Reason for round end
 */
forward Action CS_OnTerminateRound(float &delay, CSRoundEndReason &reason);

/**
 * Respawns a player.
 *
 * @param client        Player's index.
 * @error               Invalid client index, client not in game.
 */
native void CS_RespawnPlayer(int client);

/**
 * Switches the player's team.
 *
 * @param client        Player's index.
 * @param team          Team index.
 * @error               Invalid client index, client not in game.
 */
native void CS_SwitchTeam(int client, int team);

/**
 * Forces a player to drop or toss their weapon
 *
 * @param client        Player's index.
 * @param weaponIndex   Index of weapon to drop.
 * @param toss          True to toss weapon (with velocity) or false to just drop weapon
 * @param blockhook     Set to true to stop the corresponding CS_OnCSWeaponDrop
 * @error               Invalid client index, client not in game, or invalid weapon index.
 */
native void CS_DropWeapon(int client, int weaponIndex, bool toss, bool blockhook = false);

/**
 * Forces round to end with a reason
 *
 * @param delay         Time (in seconds) to delay before new round starts
 * @param reason        Reason for the round ending
 * @param blockhook     Set to true to stop the corresponding CS_OnTerminateRound
 *                      forward from being called.
 */
native void CS_TerminateRound(float delay, CSRoundEndReason reason, bool blockhook = false);
 
/**
 * Gets a weapon name from a weapon alias
 *
 * @param alias         Weapons alias to get weapon name for.
 * @param weapon        Buffer to store weapons name
 * @param size          Size of buffer to store the weapons name.
 *
 * @note Will set the buffer to the original alias if it is not an alias to a weapon.
 */
native void CS_GetTranslatedWeaponAlias(const char[] alias, char[] weapon, int size);
 
/**
 * Gets a weapon's price
 *
 * @param client        Client to check weapon price for.
 * @param id            Weapon id for the weapon to check
 * @param defaultprice  Set to true to get defaultprice.
 * @return              Returns price of the weapon (even if modified)
 * @error               Invalid client, failing to get weapon info, or failing to get price offset.
 *
 * @note c4, knife and shield will always return 0. vest, vest helmet and night vision will always return default price.
 */
native int CS_GetWeaponPrice(int client, CSWeaponID id, bool defaultprice = false);
 
/**
 * Gets a clients clan tag
 *
 * @param client        Client index to get clan tag for.
 * @param buffer        Buffer to store clients clan tag in.
 * @param size          Size of the buffer.
 * @return              Number of non-null bytes written.
 * @error               Invalid client.
 */
native int CS_GetClientClanTag(int client, char[] buffer, int size);

/**
 * Sets a clients clan tag
 *
 * @param client        Client index to set clan tag for.
 * @param tag           Tag to set clients clan tag as.
 * @error               Invalid client.
 */
native void CS_SetClientClanTag(int client, const char[] tag);

/**
 * Gets a team's score
 *
 * @param team          Team index to get score for.
 * @return              Returns the internal team score.
 * @error               Invalid team index.
 */
native int CS_GetTeamScore(int team);

/**
 * Sets a team's score
 *
 * @param team          Team index to set score for.
 * @param value         Value to set teams score as.
 * @error               Invalid team index.
 *
 * @note This will update the scoreboard only after the scoreboard update function is called.
 *       Use SetTeamScore plus this to update the scoreboard instantly and save values correctly.
 */
native void CS_SetTeamScore(int team, int value);

/**
 * Gets a client's mvp count
 *
 * @param client        Client index to get mvp count of.
 * @return              Returns the client's internal MVP count.
 * @error               Invalid client.
 */
native int CS_GetMVPCount(int client);

/**
 * Sets a client's mvp count
 *
 * @param client        Client index to set mvp count for.
 * @param value         Value to set client's mvp count as.
 * @error               Invalid client.
 */
native void CS_SetMVPCount(int client, int value);

/**
 * Gets a client's contribution score (CS:GO only)
 *
 * @param client        Client index to get score of.
 * @return              Returns the client's score.
 * @error               Invalid client.
 */
native int CS_GetClientContributionScore(int client);

/**
 * Sets a client's contribution score (CS:GO only)
 *
 * @param client        Client index to set score for.
 * @param value         Value to set client's score as.
 * @error               Invalid client.
 */
native void CS_SetClientContributionScore(int client, int value);

/**
 * Gets a client's assists (CS:GO only)
 *
 * @param client        Client index to get assists of.
 * @return              Returns the client's assists.
 * @error               Invalid client.
 */
native int CS_GetClientAssists(int client);

/**
 * Sets a client's assists (CS:GO only)
 *
 * @param client        Client index to set assists for.
 * @param value         Value to set client's assists as.
 * @error               Invalid client.
 */
native void CS_SetClientAssists(int client, int value);

/**
 * Gets a weaponID from a alias
 *
 * @param alias         Weapon alias to attempt to get an id for.
 * @return              Returns a weapon id or 0 if failed to find a match.
 *
 * @note For best results use CS_GetTranslatedWeaponAlias on the weapon name before passing it.
 */
native CSWeaponID CS_AliasToWeaponID(const char[] alias);

/**
 * Gets a alias from a weaponID
 *
 * @param weaponID      WeaponID to get alias for.
 * @param destination   Destination string to hold the weapon alias.
 * @param len           Length of the destination array.
 * @return              Returns number of cells written.
 */
native int CS_WeaponIDToAlias(CSWeaponID weaponID, char[] destination, int len);

/**
 * Returns weather a WeaponID is valid on the current mod (css or csgo)
 *
 * @param weaponID      WeaponID to check
 * @return              Returns true if its a valid WeaponID false otherwise.
 *
 * @note This will return false always for CSWeapon_NONE. Should only be called after OnMapStart since weapon info isnt intialized before.
 */
native bool CS_IsValidWeaponID(CSWeaponID id);

/**
 * Sets a player's model based on their current class
 *
 * @param client        Player's index.
 * @error               Invalid client index, client not in game.
 */
native void CS_UpdateClientModel(int client);

/**
 * Returns a CSWeaponID equivalent based on the item definition index.
 *
 * @param iDefIndex     Definition index to get the CSWeaponID value for.
 * @return              Returns CSWeaponID value for the definition index.
 * @error               Invalid definition index.
 *
 * @note In most cases the id will be the item definition index. Works for CS:GO ONLY.
 */
native CSWeaponID CS_ItemDefIndexToID(int iDefIndex);

/**
 * Returns a item definition index equivalent based on the CSWeaponID.
 *
 * @param id            CSWeaponID to get the item definition for.
 * @return              Returns item definition index value for the weapon id.
 * @error               Invalid weapon id.
 *
 * @note In most cases the item deinition index will be the id. Works for CS:GO ONLY.
 */
native int CS_WeaponIDToItemDefIndex(CSWeaponID id);

/**
 * Do not edit below this line!
 */
public Extension __ext_cstrike = 
{
	name = "cstrike",
	file = "games/game.cstrike.ext",
	autoload = 0,
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public void __ext_cstrike_SetNTVOptional()
{
	MarkNativeAsOptional("CS_RespawnPlayer");
	MarkNativeAsOptional("CS_SwitchTeam");
	MarkNativeAsOptional("CS_DropWeapon");
	MarkNativeAsOptional("CS_TerminateRound");
	MarkNativeAsOptional("CS_GetTranslatedWeaponAlias");
	MarkNativeAsOptional("CS_GetWeaponPrice");
	MarkNativeAsOptional("CS_GetClientClanTag");
	MarkNativeAsOptional("CS_SetClientClanTag");
	MarkNativeAsOptional("CS_GetTeamScore");
	MarkNativeAsOptional("CS_SetTeamScore");
	MarkNativeAsOptional("CS_GetMVPCount");
	MarkNativeAsOptional("CS_SetMVPCount");
	MarkNativeAsOptional("CS_GetClientContributionScore");
	MarkNativeAsOptional("CS_SetClientContributionScore");
	MarkNativeAsOptional("CS_GetClientAssists");
	MarkNativeAsOptional("CS_SetClientAssists");
	MarkNativeAsOptional("CS_AliasToWeaponID");
	MarkNativeAsOptional("CS_WeaponIDToAlias");
	MarkNativeAsOptional("CS_IsValidWeaponID");
	MarkNativeAsOptional("CS_UpdateClientModel");
	MarkNativeAsOptional("CS_ItemDefIndexToID");
	MarkNativeAsOptional("CS_WeaponIDToItemDefIndex");
}
#endif
