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

#define TF_STUNFLAG_SLOWDOWN        (1 << 0)	// activates slowdown modifier
#define TF_STUNFLAG_BONKSTUCK       (1 << 1)	// bonk sound, stuck
#define TF_STUNFLAG_LIMITMOVEMENT   (1 << 2)	// disable forward/backward movement
#define TF_STUNFLAG_CHEERSOUND      (1 << 3)	// cheering sound
#define TF_STUNFLAG_NOSOUNDOREFFECT (1 << 5)	// no sound or particle
#define TF_STUNFLAG_THIRDPERSON     (1 << 6)	// panic animation
#define TF_STUNFLAG_GHOSTEFFECT     (1 << 7)	// ghost particles

#define TF_STUNFLAGS_LOSERSTATE     TF_STUNFLAG_SLOWDOWN|TF_STUNFLAG_NOSOUNDOREFFECT|TF_STUNFLAG_THIRDPERSON
#define TF_STUNFLAGS_GHOSTSCARE     TF_STUNFLAG_GHOSTEFFECT|TF_STUNFLAG_THIRDPERSON
#define TF_STUNFLAGS_SMALLBONK      TF_STUNFLAG_THIRDPERSON|TF_STUNFLAG_SLOWDOWN
#define TF_STUNFLAGS_NORMALBONK     TF_STUNFLAG_BONKSTUCK
#define TF_STUNFLAGS_BIGBONK        TF_STUNFLAG_CHEERSOUND|TF_STUNFLAG_BONKSTUCK

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

enum TFCond
{
	TFCond_Slowed = 0,
	TFCond_Zoomed,
	TFCond_Disguising,
	TFCond_Disguised,
	TFCond_Cloaked,
	TFCond_Ubercharged,
	TFCond_TeleportedGlow,
	TFCond_Taunting,
	TFCond_UberchargeFading,
	TFCond_Unknown1, //9
	TFCond_CloakFlicker = 9,
	TFCond_Teleporting,
	TFCond_Kritzkrieged,
	TFCond_Unknown2, //12
	TFCond_TmpDamageBonus = 12,
	TFCond_DeadRingered,
	TFCond_Bonked,
	TFCond_Dazed,
	TFCond_Buffed,
	TFCond_Charging,
	TFCond_DemoBuff,
	TFCond_CritCola,
	TFCond_InHealRadius,
	TFCond_Healing,
	TFCond_OnFire,
	TFCond_Overhealed,
	TFCond_Jarated,
	TFCond_Bleeding,
	TFCond_DefenseBuffed,
	TFCond_Milked,
	TFCond_MegaHeal,
	TFCond_RegenBuffed,
	TFCond_MarkedForDeath,
	TFCond_NoHealingDamageBuff,
	TFCond_SpeedBuffAlly,
	TFCond_HalloweenCritCandy,
	
	TFCond_CritHype = 36,
	TFCond_CritOnFirstBlood,
	TFCond_CritOnWin,
	TFCond_CritOnFlagCapture,
	TFCond_CritOnKill,
	TFCond_RestrictToMelee,
	
	TFCond_CritMmmph = 44,
	TFCond_DefenseBuffMmmph,
};

enum TFHoliday
{
	TFHoliday_Birthday = 1,
	TFHoliday_Halloween,
	TFHoliday_Christmas,
	TFHoliday_MeetThePyro,
	TFHoliday_FullMoon,
	TFHoliday_HalloweenOrFullMoon,
};

enum TFObjectType
{
	TFObject_CartDispenser = 0,
	TFObject_Dispenser = 0,
	TFObject_Teleporter = 1,
	TFObject_Sentry = 2,
	TFObject_Sapper = 3
};

enum TFObjectMode
{
	TFObjectMode_None = 0,
	TFObjectMode_Entrance = 0,
	TFObjectMode_Exit = 1
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
 * Regenerates a client's health and ammunition
 *
 * @param client		Player's index.
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native TF2_RegeneratePlayer(client);

/**
 * Adds a condition to a player
 *
 * @param client		Player's index.
 * @param condition		Integer identifier of condition to apply.
 * @param duration		Duration of condition (does not apply to all conditions).
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native TF2_AddCondition(client, TFCond:condition, Float:duration);

/**
 * Removes a condition from a player
 *
 * @param client		Player's index.
 * @param condition		Integer identifier of condition to remove.
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native TF2_RemoveCondition(client, TFCond:condition);

/**
 * Enables/disables PowerPlay mode on a player.
 *
 * @param client		Player's index.
 * @param enabled		Whether to enable or disable PowerPlay on player.
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native TF2_SetPlayerPowerPlay(client, bool:enabled);

/**
 * Disguises a client to the given model and team. Only has an effect on spies.
 *
 * Note: This only starts the disguise process and a delay occurs before the spy is fully disguised
 *
 * @param client		Player's index.
 * @param team			Team to disguise the player as (only TFTeam_Red and TFTeam_Blue have an effect)
 * @param class			TFClassType class to disguise the player as
 * @param target		Specific target player to disguise as (0 for any)
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native TF2_DisguisePlayer(client, TFTeam:team, TFClassType:class, target=0);

/**
 * Removes the current disguise from a client. Only has an effect on spies.
 *
 * @param client		Player's index.
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native TF2_RemovePlayerDisguise(client);

/**
 * Stuns a client
 *
 * @param client		Player's index.
 * @param float			Duration of stun.
 * @param float			Slowdown percent (as decimal, 0.00-1.00)
 *						(ignored if TF_STUNFLAG_SLOWDOWN is not set.
 * @param int			Stun flags.
 * @param attacker		Attacker's index (0 is allowed for world).
 * @noreturn
 */
native TF2_StunPlayer(client, Float:duration, Float:slowdown=0.0, stunflags, attacker=0);

/**
 * Induces the bleed effect on a client
 *
 * @param client		Player's index.
 * @param attacker		Attacker's index.
 * @param float			Duration of bleeding (in seconds).
 * @noreturn
 */
native TF2_MakeBleed(client, attacker, Float:duration);

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
 * @deprecated		No longer called. Use TF2_OnIsHolidayActive.
 */
#pragma deprecated No longer called. Use TF2_OnIsHolidayActive.
forward Action:TF2_OnGetHoliday(&TFHoliday:holiday);

/**
 * Called at various times when the game checks to see if the given holiday is active.
 * Return Plugin_Continue to let the original calculation or return a higher
 * action to override the decision with the value of 'result'
 *
 * @param holiday		Holiday being checked.
 * @param result		Buffer param for the result of the decision.
 * @return				Plugin_Continue for original calculation, higher value to use 'result'.
 */
forward Action:TF2_OnIsHolidayActive(TFHoliday:holiday, &bool:result);

/**
 * Returns whether or not a client (Player) is in a duel.
 *
 * @param client    Client Index.
 * @return          Boolean of whether or not the client/player is dueling.
 */
native bool:TF2_IsPlayerInDuel(client);

/**
 * Called after a condition is added to a player
 *
 * @param client		Index of the client to which the conditon is being added.
 * @param condition     Condition that is being added.
 * @noreturn
 */
forward TF2_OnConditionAdded(client, TFCond:condition);

/**
 * Called after a condition is removed from a player
 *
 * @param client		Index of the client to which the condition is being removed.
 * @param condition		Condition that is being removed.
 * @noreturn
 */
forward TF2_OnConditionRemoved(client, TFCond:condition);

/**
 * Called when the server enters the Waiting for Players round state
 *
 * @noreturn
 */
forward TF2_OnWaitingForPlayersStart();

/**
 * Called when the server exits the Waiting for Players round state
 *
 * @noreturn
 */
forward TF2_OnWaitingForPlayersEnd();

/**
 * Do not edit below this line!
 */
public Extension:__ext_tf2 = 
{
	name = "TF2 Tools",
	file = "game.tf2.ext",
	autoload = 0,
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public __ext_tf2_SetNTVOptional()
{
	MarkNativeAsOptional("TF2_IgnitePlayer");
	MarkNativeAsOptional("TF2_RespawnPlayer");
	MarkNativeAsOptional("TF2_RegeneratePlayer");
	MarkNativeAsOptional("TF2_AddCondition");
	MarkNativeAsOptional("TF2_RemoveCondition");
	MarkNativeAsOptional("TF2_SetPlayerPowerPlay");
	MarkNativeAsOptional("TF2_DisguisePlayer");
	MarkNativeAsOptional("TF2_RemovePlayerDisguise");
	MarkNativeAsOptional("TF2_StunPlayer");
	MarkNativeAsOptional("TF2_MakeBleed");
	MarkNativeAsOptional("TF2_GetResourceEntity");
	MarkNativeAsOptional("TF2_GetClass");
	MarkNativeAsOptional("TF2_IsPlayerInDuel");
}
#endif
