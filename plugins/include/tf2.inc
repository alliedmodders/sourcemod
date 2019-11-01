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

#define TF_STUNFLAG_SLOWDOWN        (1 << 0)    /**< activates slowdown modifier */
#define TF_STUNFLAG_BONKSTUCK       (1 << 1)    /**< bonk sound, stuck */
#define TF_STUNFLAG_LIMITMOVEMENT   (1 << 2)    /**< disable forward/backward movement */
#define TF_STUNFLAG_CHEERSOUND      (1 << 3)    /**< cheering sound */
#define TF_STUNFLAG_NOSOUNDOREFFECT (1 << 5)    /**< no sound or particle */
#define TF_STUNFLAG_THIRDPERSON     (1 << 6)    /**< panic animation */
#define TF_STUNFLAG_GHOSTEFFECT     (1 << 7)    /**< ghost particles */
#define TF_STUNFLAG_SOUND           (1 << 8)    /**< sound */

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
	TFCond_Slowed = 0, //0: Revving Minigun, Sniper Rifle. Gives zoomed/revved pose
	TFCond_Zoomed, //1: Sniper Rifle zooming
	TFCond_Disguising, //2: Disguise smoke
	TFCond_Disguised, //3: Disguise
	TFCond_Cloaked, //4: Cloak effect
	TFCond_Ubercharged, //5: Invulnerability, removed when being healed or by another Uber effect
	TFCond_TeleportedGlow, //6: Teleport trail effect
	TFCond_Taunting, //7: Used for taunting, can remove to stop taunting
	TFCond_UberchargeFading, //8: Invulnerability expiration effect
	TFCond_Unknown1, //9
	TFCond_CloakFlicker = 9, //9: Cloak flickering effect
	TFCond_Teleporting, //10: Used for teleporting, does nothing applying
	TFCond_Kritzkrieged, //11: Crit boost, removed when being healed or another Uber effect
	TFCond_Unknown2, //12
	TFCond_TmpDamageBonus = 12, //12: Temporary damage buff, something along with attribute 19
	TFCond_DeadRingered, //13: Dead Ringer damage resistance, gives TFCond_Cloaked
	TFCond_Bonked, //14: Bonk! Atomic Punch effect
	TFCond_Dazed, //15: Slow effect, can remove to remove stun effects
	TFCond_Buffed, //16: Buff Banner mini-crits, icon, and glow
	TFCond_Charging, //17: Forced forward, charge effect
	TFCond_DemoBuff, //18: Eyelander eye glow
	TFCond_CritCola, //19: Mini-crit effect
	TFCond_InHealRadius, //20: Ring effect, rings disappear after a taunt ends
	TFCond_Healing, //21: Used for healing, does nothing applying
	TFCond_OnFire, //22: Ignite sound and vocals, can remove to remove afterburn
	TFCond_Overhealed, //23: Used for overheal, does nothing applying
	TFCond_Jarated, //24: Jarate effect
	TFCond_Bleeding, //25: Bleed effect
	TFCond_DefenseBuffed, //26: Battalion's Backup's defense, icon, and glow
	TFCond_Milked, //27: Mad Milk effect
	TFCond_MegaHeal, //28: Quick-Fix Ubercharge's knockback/stun immunity and visual effect
	TFCond_RegenBuffed, //29: Concheror's speed boost, heal on hit, icon, and glow
	TFCond_MarkedForDeath, //30: Fan o' War marked-for-death effect
	TFCond_NoHealingDamageBuff, //31: Mini-crits, blocks healing, glow, no weapon mini-crit effects
	TFCond_SpeedBuffAlly, //32: Disciplinary Action speed boost
	TFCond_HalloweenCritCandy, //33: Halloween pumpkin crit-boost
	TFCond_CritCanteen, //34: Crit-boost and doubles Sentry Gun fire-rate
	TFCond_CritDemoCharge, //35: Crit glow, adds TFCond_Charging when charge meter is below 75%
	TFCond_CritHype, //36: Soda Popper multi-jump effect
	TFCond_CritOnFirstBlood, //37: Arena first blood crit-boost
	TFCond_CritOnWin, //38: End-of-round crit-boost (May not remove correctly?)
	TFCond_CritOnFlagCapture, //39: Intelligence capture crit-boost
	TFCond_CritOnKill, //40: Crit-boost from crit-on-kill weapons
	TFCond_RestrictToMelee, //41: Prevents switching once melee is out
	TFCond_DefenseBuffNoCritBlock, //42: MvM Bomb Carrier defense buff (TFCond_DefenseBuffed without crit resistance)
	TFCond_Reprogrammed, //43: No longer functions
	TFCond_CritMmmph, //44: Phlogistinator crit-boost
	TFCond_DefenseBuffMmmph, //45: Old Phlogistinator defense buff
	TFCond_FocusBuff, //46: Hitman's Heatmaker no-unscope and faster Sniper charge
	TFCond_DisguiseRemoved, //47: Enforcer damage bonus removed
	TFCond_MarkedForDeathSilent, //48: Marked-for-death without sound effect
	TFCond_DisguisedAsDispenser, //49: Dispenser disguise when crouching, max movement speed, sentries ignore player
	TFCond_Sapped, //50: Sapper sparkle effect in MvM
	TFCond_UberchargedHidden, //51: Out-of-bounds robot invulnerability effect
	TFCond_UberchargedCanteen, //52: Invulnerability effect and Sentry Gun damage resistance
	TFCond_HalloweenBombHead, //53: Bomb head effect (does not explode)
	TFCond_HalloweenThriller, //54: Forced Thriller taunting
	TFCond_RadiusHealOnDamage, //55: Radius healing, adds TFCond_InHealRadius, TFCond_Healing. Removed when a taunt ends, but this condition stays but does nothing
	TFCond_CritOnDamage, //56: Miscellaneous crit-boost
	TFCond_UberchargedOnTakeDamage, //57: Miscellaneous invulnerability
	TFCond_UberBulletResist, //58: Vaccinator Uber bullet resistance
	TFCond_UberBlastResist, //59: Vaccinator Uber blast resistance
	TFCond_UberFireResist, //60: Vaccinator Uber fire resistance
	TFCond_SmallBulletResist, //61: Vaccinator healing bullet resistance
	TFCond_SmallBlastResist, //62: Vaccinator healing blast resistance
	TFCond_SmallFireResist, //63: Vaccinator healing fire resistance
	TFCond_Stealthed, //64: Cloaked until next attack
	TFCond_MedigunDebuff, //65: Unknown
	TFCond_StealthedUserBuffFade, //66: Cloaked, will appear for a few seconds on attack and cloak again
	TFCond_BulletImmune, //67: Full bullet immunity
	TFCond_BlastImmune, //68: Full blast immunity
	TFCond_FireImmune, //69: Full fire immunity
	TFCond_PreventDeath, //70: Survive to 1 health, then the condition is removed
	TFCond_MVMBotRadiowave, //71: Stuns bots and applies radio effect
	TFCond_HalloweenSpeedBoost, //72: Speed boost, non-melee fire rate and reload, infinite air jumps
	TFCond_HalloweenQuickHeal, //73: Healing effect, adds TFCond_Healing along with TFCond_MegaHeal temporarily
	TFCond_HalloweenGiant, //74: Double size, x10 max health increase, ammo regeneration, and forced thirdperson
	TFCond_HalloweenTiny, //75: Half size and increased head size
	TFCond_HalloweenInHell, //76: Applies TFCond_HalloweenGhostMode when the player dies
	TFCond_HalloweenGhostMode, //77: Becomes a ghost unable to attack but can fly
	TFCond_MiniCritOnKill, //78: Mini-crits effect
	TFCond_DodgeChance, //79
	TFCond_ObscuredSmoke = 79, //79: 75% chance to dodge an attack
	TFCond_Parachute, //80: Parachute effect, removed when touching the ground
	TFCond_BlastJumping, //81: Player is blast jumping
	TFCond_HalloweenKart, //82: Player forced into a Halloween kart
	TFCond_HalloweenKartDash, //83: Forced forward if in TFCond_HalloweenKart, zoom in effect, and dash animations
	TFCond_BalloonHead, //84: Big head and lowered gravity
	TFCond_MeleeOnly, //85: Forced melee, along with TFCond_SpeedBuffAlly and TFCond_HalloweenTiny
	TFCond_SwimmingCurse, //86: Swim in the air with Jarate overlay
	TFCond_HalloweenKartNoTurn, //87
	TFCond_FreezeInput = 87, //87: Prevents player from using controls
	TFCond_HalloweenKartCage, //88: Puts a cage around the player if in TFCond_HalloweenKart, otherwise crashes
	TFCond_HasRune, //89: Has a powerup
	TFCond_RuneStrength, //90: Double damage and no damage falloff
	TFCond_RuneHaste, //91: Double fire rate, reload speed, clip and ammo size, and 30% faster movement speed
	TFCond_RuneRegen, //92: Regen ammo, health, and metal
	TFCond_RuneResist, //93: Takes 1/2 damage and critical immunity
	TFCond_RuneVampire, //94: Takes 3/4 damage, gain health on damage, and 40% increase in max health
	TFCond_RuneWarlock, //95: Attacker takes damage and knockback on hitting the player and 50% increase in max health
	TFCond_RunePrecision, //96: Less bullet spread, no damage falloff, 250% faster projectiles, and double damage, faster charge, and faster re-zoom for Sniper Rifles
	TFCond_RuneAgility, //97: Increased movement speed, grappling hook speed, jump height, and instant weapon switch
	TFCond_GrapplingHook, //98: Used when a player fires their grappling hook, no effect applying or removing
	TFCond_GrapplingHookSafeFall, //99: Used when a player is pulled by their grappling hook, no effect applying or removing
	TFCond_GrapplingHookLatched, //100: Used when a player latches onto a wall, no effect applying or removing
	TFCond_GrapplingHookBleeding, //101: Used when a player is hit by attacker's grappling hook
	TFCond_AfterburnImmune, //102: Deadringer afterburn immunity
	TFCond_RuneKnockout, //103: Melee and grappling hook only, increased max health, knockback immunity, x4 more damage against buildings, and knockbacks a powerup off a victim on hit
	TFCond_RuneImbalance, //104: Prevents gaining a crit-boost or Uber powerups
	TFCond_CritRuneTemp, //105: Crit-boost effect
	TFCond_PasstimeInterception, //106: Used when a player intercepts the Jack/Ball
	TFCond_SwimmingNoEffects, //107: Swimming in the air without animations or overlay
	TFCond_EyeaductUnderworld, //108: Refills max health, short Uber, escaped the underworld message on removal
	TFCond_KingRune, //109: Increased max health and applies TFCond_KingAura
	TFCond_PlagueRune, //110: Radius health kit stealing, increased max health, TFCond_Plague on touching a victim
	TFCond_SupernovaRune, //111: Charge meter passively increasing, when charged activiated causes radius Bonk stun
	TFCond_Plague, //112: Plague sound effect and message, blocks King powerup health regen
	TFCond_KingAura, //113: Increased fire rate, reload speed, and health regen to players in a radius
	TFCond_SpawnOutline, //114: Outline and health meter of teammates (and disguised spies)
	TFCond_KnockedIntoAir, //115: Used when a player is airblasted
	TFCond_CompetitiveWinner, //116: Unknown
	TFCond_CompetitiveLoser, //117: Unknown
	TFCond_NoTaunting_DEPRECATED, //118
	TFCond_HealingDebuff = 118, //118: Healing debuff from Medics and dispensers
	TFCond_PasstimePenaltyDebuff, //119: Marked-for-death effect
	TFCond_GrappledToPlayer, //120: Prevents taunting and some Grappling Hook actions
	TFCond_GrappledByPlayer, //121: Unknown
	TFCond_ParachuteDeployed, //122: Parachute deployed, prevents reopening it
	TFCond_Gas, //123: Gas Passer effect
	TFCond_BurningPyro, //124: Dragon's Fury afterburn on Pyros
	TFCond_RocketPack, //125: Thermal Thruster launched effects, prevents reusing
	TFCond_LostFooting, //126: Less ground friction
	TFCond_AirCurrent //127: Reduced air control and friction
};

const float TFCondDuration_Infinite = -1.0;

enum TFHoliday
{
	TFHoliday_Invalid = -1
};

public const TFHoliday TFHoliday_Birthday;
public const TFHoliday TFHoliday_Halloween;
public const TFHoliday TFHoliday_Christmas;
public const TFHoliday TFHoliday_EndOfTheLine;
public const TFHoliday TFHoliday_CommunityUpdate;
public const TFHoliday TFHoliday_ValentinesDay;
public const TFHoliday TFHoliday_MeetThePyro;
public const TFHoliday TFHoliday_FullMoon;
public const TFHoliday TFHoliday_HalloweenOrFullMoon;
public const TFHoliday TFHoliday_HalloweenOrFullMoonOrValentines;
public const TFHoliday TFHoliday_AprilFools;

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
 * Sets a client on fire.
 *
 * @note Fire duration is capped to 10 seconds.
 *
 * @param client        Player's index.
 * @param attacker      Attacker's index.
 * @param duration      Duration of fire (in seconds).
 * @error               Invalid client index, client not in game, or no mod support.
 */
native void TF2_IgnitePlayer(int client, int attacker, float duration=10.0);

/**
 * Respawns a client
 *
 * @param client        Player's index.
 * @error               Invalid client index, client not in game, or no mod support.
 */
native void TF2_RespawnPlayer(int client);

/**
 * Regenerates a client's health and ammunition
 *
 * @param client        Player's index.
 * @error               Invalid client index, client not in game, or no mod support.
 */
native void TF2_RegeneratePlayer(int client);

/**
 * Adds a condition to a player
 *
 * @param client        Player's index.
 * @param condition     Integer identifier of condition to apply.
 * @param duration      Duration of condition (does not apply to all conditions).
 *                      Pass TFCondDuration_Infinite to apply until manually removed.
 * @param inflictor     Condition inflictor's index (0 for no inflictor).
 * @error               Invalid client index, client not in game, or no mod support.
 */
native void TF2_AddCondition(int client, TFCond condition, float duration=TFCondDuration_Infinite, int inflictor=0);

/**
 * Removes a condition from a player
 *
 * @param client        Player's index.
 * @param condition     Integer identifier of condition to remove.
 * @error               Invalid client index, client not in game, or no mod support.
 */
native void TF2_RemoveCondition(int client, TFCond condition);

/**
 * Enables/disables PowerPlay mode on a player.
 *
 * @param client        Player's index.
 * @param enabled       Whether to enable or disable PowerPlay on player.
 * @error               Invalid client index, client not in game, or no mod support.
 */
native void TF2_SetPlayerPowerPlay(int client, bool enabled);

/**
 * Disguises a client to the given model and team. Only has an effect on spies.
 *
 * Note: This only starts the disguise process and a delay occurs before the spy is fully disguised
 *
 * @param client        Player's index.
 * @param team          Team to disguise the player as (only TFTeam_Red and TFTeam_Blue have an effect)
 * @param classType     TFClassType class to disguise the player as
 * @param target        Specific target player to disguise as (0 for any)
 * @error               Invalid client index, client not in game, or no mod support.
 */
native void TF2_DisguisePlayer(int client, TFTeam team, TFClassType classType, int target=0);

/**
 * Removes the current disguise from a client. Only has an effect on spies.
 *
 * @param client        Player's index.
 * @error               Invalid client index, client not in game, or no mod support.
 */
native void TF2_RemovePlayerDisguise(int client);

/**
 * Stuns a client
 *
 * @param client        Player's index.
 * @param duration      Duration of stun.
 * @param slowdown      Slowdown percent (as decimal, 0.00-1.00)
 *                      Ignored if TF_STUNFLAG_SLOWDOWN is not set.
 * @param stunflags     Stun flags.
 * @param attacker      Attacker's index (0 is allowed for world).
 */
native void TF2_StunPlayer(int client, float duration, float slowdown=0.0, int stunflags, int attacker=0);

/**
 * Induces the bleed effect on a client
 *
 * @param client        Player's index.
 * @param attacker      Attacker's index.
 * @param duration      Duration of bleeding (in seconds).
 */
native void TF2_MakeBleed(int client, int attacker, float duration);

/**
 * Retrieves the entity index of the CPlayerResource entity
 *
 * @return              The current resource entity index.
 * @deprecated          Use GetPlayerResourceEntity instead
 */
#pragma deprecated Use GetPlayerResourceEntity instead
native int TF2_GetResourceEntity();

/**
 * Finds the TFClassType for a given class name.
 *
 * @param classname     A classname string such as "sniper" or "demoman"
 * @return              A TFClassType constant.
 */
native TFClassType TF2_GetClass(const char[] classname);

/**
 * Called on weapon fire to decide if the current shot should be critical.
 * Return Plugin_Continue to let the original calculation or return a higher
 * action to override the decision with the value of 'result'
 *
 * @note Since critical shots are also calculated client side any changes made with
 *       this will not show for the shooter. Projectile weapons such as the rocketlauncher
 *       and demoman weapons will show a critical bullet but no critical sound effect.
 *       Bullet hits should appear as expected.
 *
 * @param client        Client Index.
 * @param weapon        Weapon entity Index.
 * @param weaponname    Classname of the weapon.
 * @param result        Buffer param for the result of the decision.
 */
forward Action TF2_CalcIsAttackCritical(int client, int weapon, char[] weaponname, bool &result);

/**
 * @deprecated          No longer called. Use TF2_OnIsHolidayActive.
 */
#pragma deprecated No longer called. Use TF2_OnIsHolidayActive.
forward Action TF2_OnGetHoliday(TFHoliday &holiday);

/**
 * Called at various times when the game checks to see if the given holiday is active.
 * Return Plugin_Continue to let the original calculation or return a higher
 * action to override the decision with the value of 'result'
 *
 * @param holiday       Holiday being checked.
 * @param result        Buffer param for the result of the decision.
 * @return              Plugin_Continue for original calculation, higher value to use 'result'.
 */
forward Action TF2_OnIsHolidayActive(TFHoliday holiday, bool &result);

/**
 * Returns whether or not a holiday is active
 *
 * @param holiday       Holiday being checked.
 * @return              Boolean of whether or not the holiday is active.
 */
native bool TF2_IsHolidayActive(TFHoliday holiday);

/**
 * Returns whether or not a client (Player) is in a duel.
 *
 * @param client    Client Index.
 * @return              Boolean of whether or not the client/player is dueling.
 */
native bool TF2_IsPlayerInDuel(int client);

/**
 * Removes an econ wearable (hat, misc, etc) from a player.
 * This also deletes the wearable entity.
 *
 * @param client        Client index.
 * @param wearable      Index of the wearable entity.
 * @error               Invalid client index, client not in game, invalid wearable entity, or no mod support.
*/
native void TF2_RemoveWearable(int client, int wearable);

/**
 * Called after a condition is added to a player
 *
 * @param client        Index of the client to which the condition is being added.
 * @param condition     Condition that is being added.
 */
forward void TF2_OnConditionAdded(int client, TFCond condition);

/**
 * Called after a condition is removed from a player
 *
 * @param client        Index of the client to which the condition is being removed.
 * @param condition     Condition that is being removed.
 */
forward void TF2_OnConditionRemoved(int client, TFCond condition);

/**
 * Called when the server enters the Waiting for Players round state
 */
forward void TF2_OnWaitingForPlayersStart();

/**
 * Called when the server exits the Waiting for Players round state
 */
forward void TF2_OnWaitingForPlayersEnd();

/**
 * Called when a player attempts to use a teleporter to decide if the player should be allowed to teleport.
 * Return Plugin_Continue to let the original calculation or return a higher
 * action to override the decision with the value of 'result'
 *
 * @param client        Client index.
 * @param teleporter    Teleporter entity index.
 * @param result        Buffer param for the result of the decision.
 *                      This is prepopulated with the game's original decision to let a player teleport.
 * @return              Plugin_Continue for original calculation, higher value to use 'result'.
 */
forward Action TF2_OnPlayerTeleport(int client, int teleporter, bool &result);
 
/**
 * Do not edit below this line!
 */
public Extension __ext_tf2 = 
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
public void __ext_tf2_SetNTVOptional()
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
	MarkNativeAsOptional("TF2_IsHolidayActive");
	MarkNativeAsOptional("TF2_RemoveWearable");
}
#endif
