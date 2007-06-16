/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be used 
 * or modified under the Terms and Conditions of its License Agreement, which is found 
 * in LICENSE.txt.  The Terms and Conditions for making SourceMod extensions/plugins 
 * may change at any time.  To view the latest information, see:
 *   http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#if defined _sdktools_functions_included
  #endinput
#endif
#define _sdktools_functions_included

/**
 * Removes a player's item.
 *
 * @param client		Client index.
 * @param item			CBaseCombatWeapon entity index.
 * @return				True on success, false otherwise.
 * @error				Invalid client or entity, lack of mod support, or client not in 
 *						game.
 */
native bool:RemovePlayerItem(client, item);

/**
 * Gives a named item to a player.
 *
 * @param client		Client index.
 * @param item			Item classname (such as weapon_ak47).
 * @param iSubType		Unknown.
 * @return				Entity index on success, or -1 on failure.
 * @error				Invalid client or client not in game, or lack of mod support.
 */
native GivePlayerItem(client, const String:item[], iSubType=0);

/**
 * Returns the weapon in a player's slot.  
 *
 * @param client		Client index.
 * @param slot			Slot index (mod specific).
 * @return				Entity index on success, -1 if no weapon existed.
 * @error				Invalid client or client not in game, or lack of mod support.
 */
native GetPlayerWeaponSlot(client, slot);

/**
 * Ignites a player on fire.
 *
 * @param client		Client index.
 * @param time			Number of seconds to set on fire.
 * @param npc			True to only affect NPCs.
 * @param size			Unknown.
 * @param level			Unknown.
 * @noreturn
 * @error				Invalid client or client not in game, or lack of mod support.
 */
native IgnitePlayer(client, Float:time, bool:npc=false, Float:size=0.0, bool:level=false);

/**
 * Extinguishes a player that is on fire.
 *
 * @param client		Client index.
 * @noreturn
 * @error				Invalid client or client not in game, or lack of mod support.
 */
native ExtinguishPlayer(client);

/**
 * Teleports a player.
 *
 * @param client		Client index.
 * @param origin		New origin, or NULL_VECTOR for no change.
 * @param angles		New angles, or NULL_VECTOR for no change.
 * @param velocity		New velocity, or NULL_VECTOR for no change.
 * @noreturn
 * @error				Invalid client or client not in game, or lack of mod support.
 */
native TeleportPlayer(client, const Float:origin[3], const Float:angles[3], const Float:velocity[3]);
