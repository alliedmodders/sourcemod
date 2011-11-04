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

#define CS_TEAM_NONE		0	/**< No team yet. */
#define CS_TEAM_SPECTATOR	1	/**< Spectators. */
#define CS_TEAM_T 			2	/**< Terrorists. */
#define CS_TEAM_CT			3	/**< Counter-Terrorists. */

#define CS_SLOT_PRIMARY		0	/**< Primary weapon slot. */
#define CS_SLOT_SECONDARY	1	/**< Secondary weapon slot. */
#define CS_SLOT_GRENADE		3	/**< Grenade slot (will only return one grenade). */
#define CS_SLOT_C4			4	/**< C4 slot. */

enum CSRoundEndReason
{
	CSRoundEnd_TargetBombed = 0,           // Target Successfully Bombed!
	CSRoundEnd_VIPEscaped,                 // The VIP has escaped!
	CSRoundEnd_VIPKilled,                  // VIP has been assassinated!
	CSRoundEnd_TerroristsEscaped,          // The terrorists have escaped!
	CSRoundEnd_CTStoppedEscape,            // The CTs have prevented most of the terrorists from escaping!
	CSRoundEnd_TerroristsStopped,          // Escaping terrorists have all been neutralized!
	CSRoundEnd_BombDefused,                // The bomb has been defused!
	CSRoundEnd_CTWin,                      // Counter-Terrorists Win!
	CSRoundEnd_TerroristWin,               // Terrorists Win!
	CSRoundEnd_Draw,                       // Round Draw!
	CSRoundEnd_HostagesRescued,            // All Hostages have been rescued!
	CSRoundEnd_TargetSaved,                // Target has been saved!
	CSRoundEnd_HostagesNotRescued,         // Hostages have not been rescued!
	CSRoundEnd_TerroristsNotEscaped,       // Terrorists have not escaped!
	CSRoundEnd_VIPNotEscaped,              // VIP has not escaped!
	CSRoundEnd_GameStart                   // Game Commencing!
};

enum CSWeaponID
{
    CSWeapon_NONE,
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
    CSWeapon_NIGHTVISION
};
/**
 * Called when a player attempts to purchase an item.
 * Return Plugin_Continue to allow the purchase or return a
 * higher action to deny.
 *
 * @param client	Client index
 * @param weapon	User input for weapon name
 */
forward Action:CS_OnBuyCommand(client, const String:weapon[]);

/**
 * Called when CSWeaponDrop is called
 * Return Plugin_Continue to allow the call or return a
 * higher action to deny.
 *
 * @param client	Client index
 * @param weapon	Weapon index
 */
forward Action:CS_OnCSWeaponDrop(client, weaponIndex);

/**
 * Called when game retrieves a weapon's price for a player.
 * Return Plugin_Continue to use default value or return a higher
 * action to use a newly-set price.
 * 
 * @note This can be called multiple times per weapon purchase
 * 
 * @param client	Client index
 * @param weapon	Weapon classname
 * @param price		Buffer param for the price of the weapon
 *
 * @note			Not all "weapons" call GetWeaponPrice. Example: c4, knife, vest, vest helmet, night vision.
 */
forward Action:CS_OnGetWeaponPrice(client, const String:weapon[], &price);

/**
 * Called when TerminateRound is called.
 * Return Plugin_Continue to ignore, return Plugin_Changed to continue,
 * using the given delay and reason, or return Plugin_Handled or a higher
 * action to block TerminateRound from firing.
 *
 * @param delay		Time (in seconds) until new round starts
 * @param reason	Reason for round end
 */
forward Action:CS_OnTerminateRound(&Float:delay, &CSRoundEndReason:reason);

/**
 * Respawns a player.
 *
 * @param client		Player's index.
 * @noreturn
 * @error			Invalid client index, client not in game.
 */
native CS_RespawnPlayer(client);

/**
 * Switches the player's team.
 *
 * @param client		Player's index.
 * @param team			Team index.
 * @noreturn
 * @error			Invalid client index, client not in game.
 */
native CS_SwitchTeam(client, team);

/**
 * Forces a player to drop or toss their weapon
 *
 * @param client		Player's index.
 * @param weaponIndex	Index of weapon to drop.
 * @param toss			True to toss weapon (with velocity) or false to just drop weapon
 * @param blockhook		Set to true to stop the corresponding CS_OnCSWeaponDrop
 * 
 * @noreturn
 * @error				Invalid client index, client not in game, or invalid weapon index.
 */
native CS_DropWeapon(client, weaponIndex, bool:toss, bool:blockhook = false);

/**
 * Forces round to end with a reason
 *
 * @param delay			Time (in seconds) to delay before new round starts
 * @param reason		Reason for the round ending
 * @param blockhook		Set to true to stop the corresponding CS_OnTerminateRound
 *						forward from being called.
 * @noreturn
 */
native CS_TerminateRound(Float:delay, CSRoundEndReason:reason, bool:blockhook = false);
 
/**
 * Gets a weapon name from a weapon alias
 *
 * @param alias			Weapons alias to get weapon name for.
 * @param weapon		Buffer to store weapons name
 * @param size			Size of buffer to store the weapons name.
 * @noreturn
 *
 * @note				Will set the buffer to the original alias if it is not an alias to a weapon.
 */
native CS_GetTranslatedWeaponAlias(const String:alias[], String:weapon[], size);
 
/**
 * Gets a weapon's price
 *
 * @param client		Client to check weapon price for.
 * @param id			Weapon id for the weapon to check
 * @param defaultprice	Set to true to get defaultprice.
 * @return				Returns price of the weapon (even if modified)
 *
 * @error				Invalid client, failing to get weapon info, or failing to get price offset.
 * @note				c4, knife and shield will always return 0. vest, vest helmet and night vision will always return default price.
 */
native CS_GetWeaponPrice(client, CSWeaponID:id, bool:defaultprice = false);
 
/**
 * Gets a clients clan tag
 * @param client		Client index to get clan tag for.
 * @param buffer		Buffer to store clients clan tag in.
 * @param size			Size of the buffer.
 * @return				Number of non-null bytes written.
 *
 * @error				Invalid client.
 */
native CS_GetClientClanTag(client, String:buffer[], size);

/**
 * Sets a clients clan tag
 * @param client		Client index to set clan tag for.
 * @param tag			Tag to set clients clan tag as.
 * @noreturn
 *
 * @error				Invalid client.
 */
native CS_SetClientClanTag(client, const String:tag[]);
 
/**
 * Do not edit below this line!
 */
public Extension:__ext_cstrike = 
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
public __ext_cstrike_SetNTVOptional()
{
	MarkNativeAsOptional("CS_RespawnPlayer");
	MarkNativeAsOptional("CS_SwitchTeam");
	MarkNativeAsOptional("CS_DropWeapon");
	MarkNativeAsOptional("CS_TerminateRound");
	MarkNativeAsOptional("CS_GetTranslatedWeaponAlias");
	MarkNativeAsOptional("CS_GetWeaponPrice");
	MarkNativeAsOptional("CS_GetClientClanTag");
	MarkNativeAsOptional("CS_SetClientClanTag");
}
#endif

