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
}
#endif
