/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
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
 * Ignites an entity on fire.
 *
 * @param entity		Entity index.
 * @param time			Number of seconds to set on fire.
 * @param npc			True to only affect NPCs.
 * @param size			Unknown.
 * @param level			Unknown.
 * @noreturn
 * @error				Invalid entity or client not in game, or lack of mod support.
 */
native IgniteEntity(entity, Float:time, bool:npc=false, Float:size=0.0, bool:level=false);

/**
 * Extinguishes a player that is on fire.
 *
 * @param entity		Entity index.
 * @noreturn
 * @error				Invalid entity or client not in game, or lack of mod support.
 */
native ExtinguishEntity(client);

/**
 * Teleports an entity.
 *
 * @param entity		Client index.
 * @param origin		New origin, or NULL_VECTOR for no change.
 * @param angles		New angles, or NULL_VECTOR for no change.
 * @param velocity		New velocity, or NULL_VECTOR for no change.
 * @noreturn
 * @error				Invalid entity or client not in game, or lack of mod support.
 */
native TeleportEntity(entity, const Float:origin[3], const Float:angles[3], const Float:velocity[3]);

/**
 * Forces a player to commit suicide.
 *
 * @param client		Client index.
 * @noreturn
 * @error				Invalid client or client not in game, or lack of mod support.
 */
native ForcePlayerSuicide(client);

/**
 * Slaps a player in a random direction.
 *
 * @param client		Client index.
 * @param health		Health to subtract.
 * @param sound			False to disable the sound effects.
 * @noreturn
 * @error				Invalid client or client not in game, or lack of mod support.
 */
native SlapPlayer(client, health=5, bool:sound=true);

/**
 * Searches for an entity by classname.
 *
 * @param startEnt		The entity index to begin searching from.
 *						Use -1 to start from the first entity.
 * @param classname		Classname of the entity to find.
 * @return				Entity index >= 0 if found, -1 otherwise.
 * @error				Lack of mod support.
 */
native FindEntityByClassname(startEnt, const String:classname[]);

/**
 * Returns the client's eye angles.
 *
 * @param client		Player's index.
 * @param ang			Destination vector to store the client's eye angles.
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native GetClientEyeAngles(client, Float:ang[3]);

/**
 * Returns if the client is alive or dead.
 *
 * @param client		Player's index.
 * @return			True if the client is alive, false otherwise.
 * @error				Invalid client index, client not in game, or no mod support.
 */
native bool:IsPlayerAlive(client);

/**
 * Creates an entity by string name, but does not spawn it (see DispatchSpawn).
 * If ForceEdictIndex is not -1, then it will use the edict by that index. If the index is 
 *  invalid or there is already an edict using that index, it will error out.
 *
 * @param classname		Entity classname.
 * @param ForceEdictIndex	Edict index used by the created entity.
 * @return			Entity index on success, or -1 on failure.
 * @error			Invalid edict index, or no mod support.
 */
native CreateEntityByName(const String:classname[], ForceEdictIndex=-1);

/**
 * Spawns an entity into the game.
 *
 * @param entity		Entity index of the created entity.
 * @return			True on success, false otherwise.
 * @error			Invalid entity index, or no mod support.
 */
native bool:DispatchSpawn(entity);

/**
 * @deprecated
 */
#pragma deprecated Use IgniteEntity() instead
stock IgnitePlayer(client, Float:time, bool:npc=false, Float:size=0.0, bool:level=false)
{
	return IgniteEntity(client, time, npc, size, level);
}

/**
 * @deprecated
 */
#pragma deprecated Use ExtinguishEntity() instead
stock ExtinguishClient(client)
{
	return ExtinguishEntity(client);
}

/**
 * @deprecated
 */
#pragma deprecated Use TeleportEntity() instead
stock TeleportPlayer(client, const Float:origin[3], const Float:angles[3], const Float:velocity[3])
{
	return TeleportEntity(client, origin, angles, velocity);
}
