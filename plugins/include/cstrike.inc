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
}
#endif

