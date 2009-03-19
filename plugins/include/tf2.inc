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

#if defined _tf2_included
 #endinput
#endif
#define _tf2_included

enum TFClassType
{
	TFClass_Unknown = 0,
	TFClass_Scout,
	TFClass_Sniper,
	TFClass_Soldier,
	TFClass_DemoMan,
	TFClass_Medic,
	TFClass_Heavy,
	TFClass_Pyro,
	TFClass_Spy,
	TFClass_Engineer
};

enum TFTeam
{
	TFTeam_Unassigned = 0,
	TFTeam_Spectator = 1,
	TFTeam_Red = 2,
	TFTeam_Blue = 3	
};

/**
 * Sets a client on fire for 10 seconds.
 *
 * @param client		Player's index.
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native TF2_IgnitePlayer(client, target);

/**
 * Respawns a client
 *
 * @param client		Player's index.
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native TF2_RespawnPlayer(client);

/**
 * Disguises a client to the given model and team. Only has an effect on spies.
 *
 * Note: This only starts the disguise process and a delay occurs before the spy is fully disguised
 *
 * @param client		Player's index.
 * @param team			Team to disguise the player as (only TFTeam_Red and TFTeam_Blue have an effect)
 * @param class			TFClassType class to disguise the player as
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native TF2_DisguisePlayer(client, TFTeam:team, TFClassType:class);

/**
 * Removes the current disguise from a client. Only has an effect on spies.
 *
 * @param client		Player's index.
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native TF2_RemovePlayerDisguise(client);

/**
 * Retrieves the entity index of the CPlayerResource entity
 *
 * @return				The current resource entity index.
 */
native TF2_GetResourceEntity();

/**
 * Finds the TFClassType for a given class name.
 *
 * @param classname		A classname string such as "sniper" or "demoman"
 * @return				A TFClassType constant.
 */
native TFClassType:TF2_GetClass(const String:classname[]);

/**
 * Called on weapon fire to decide if the current shot should be critical.
 * Return Plugin_Continue to let the original calculation or return a higher
 * action to override the decision with the value of 'result'
 *
 * @note Since critical shots are also calculated client side any changes made with
 *		 this will not show for the shooter. Projectile weapons such as the rocketlauncher
 *		 and demoman weapons will show a critical bullet but no critical sound effect.
 *		 Bullet hits should appear as expected.
 *
 * @param client		Client Index.
 * @param weapon		Weapon entity Index.
 * @param weaponname	Classname of the weapon.
 * @param result		Buffer param for the result of the decision.
 */
forward Action:TF2_CalcIsAttackCritical(client, weapon, String:weaponname[], &bool:result);

/**
 * Do not edit below this line!
 */
public Extension:__ext_tf2 = 
{
	name = "TF2 Tools",
	file = "game.tf2.ext",
	autoload = 1,
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};
