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

#if defined _sdktools_functions_included
 #endinput
#endif
#define _sdktools_functions_included

/**
 * Removes a player's item.
 *
 * @param client        Client index.
 * @param item          CBaseCombatWeapon entity index.
 * @return              True on success, false otherwise.
 * @error               Invalid client or entity, lack of mod support, or client not in
 *                      game.
 */
native bool RemovePlayerItem(int client, int item);

/**
 * Gives a named item to a player.
 *
 * @param client        Client index.
 * @param item          Item classname (such as weapon_ak47).
 * @param iSubType      Unknown.
 * @return              Entity index on success, or -1 on failure.
 * @error               Invalid client or client not in game, or lack of mod support.
 */
native int GivePlayerItem(int client, const char[] item, int iSubType=0);

/**
 * Returns the weapon in a player's slot.
 *
 * @param client        Client index.
 * @param slot          Slot index (mod specific).
 * @return              Entity index on success, -1 if no weapon existed.
 * @error               Invalid client or client not in game, or lack of mod support.
 */
native int GetPlayerWeaponSlot(int client, int slot);

/**
 * Ignites an entity on fire.
 *
 * @param entity        Entity index.
 * @param time          Number of seconds to set on fire.
 * @param npc           True to only affect NPCs.
 * @param size          Unknown.
 * @param level         Unknown.
 * @error               Invalid entity or client not in game, or lack of mod support.
 */
native void IgniteEntity(int entity, float time, bool npc=false, float size=0.0, bool level=false);

/**
 * Extinguishes an entity that is on fire.
 *
 * @param entity        Entity index.
 * @error               Invalid entity or client not in game, or lack of mod support.
 */
native void ExtinguishEntity(int entity);

/**
 * Teleports an entity.
 *
 * @param entity        Client index.
 * @param origin        New origin, or NULL_VECTOR for no change.
 * @param angles        New angles, or NULL_VECTOR for no change.
 * @param velocity      New velocity, or NULL_VECTOR for no change.
 * @error               Invalid entity or client not in game, or lack of mod support.
 */
native void TeleportEntity(int entity, const float origin[3] = NULL_VECTOR, const float angles[3] = NULL_VECTOR, const float velocity[3] = NULL_VECTOR);

/**
 * Forces a player to commit suicide.
 *
 * @param client        Client index.
 * @error               Invalid client or client not in game, or lack of mod support.
 */
native void ForcePlayerSuicide(int client);

/**
 * Slaps a player in a random direction.
 *
 * @param client        Client index.
 * @param health        Health to subtract.
 * @param sound         False to disable the sound effects.
 * @error               Invalid client or client not in game, or lack of mod support.
 */
native void SlapPlayer(int client, int health=5, bool sound=true);

/**
 * Searches for an entity by classname.
 *
 * @param startEnt      A valid entity's index after which to begin searching from.
 *                      Use -1 to start from the first entity.
 * @param classname     Classname of the entity to find.
 * @return              Entity index >= 0 if found, -1 otherwise.
 * @error               Invalid start entity or lack of mod support.
 */
native int FindEntityByClassname(int startEnt, const char[] classname);

/**
 * Returns the client's eye angles.
 *
 * @param client        Player's index.
 * @param ang           Destination vector to store the client's eye angles.
 * @return              True on success, false on failure.
 * @error               Invalid client index, client not in game, or lack of mod support.
 */
native bool GetClientEyeAngles(int client, float ang[3]);

/**
 * Creates an entity by string name, but does not spawn it (see DispatchSpawn).
 * If ForceEdictIndex is not -1, then it will use the edict by that index. If the index is
 *  invalid or there is already an edict using that index, it will error out.
 *
 * @param classname         Entity classname.
 * @param ForceEdictIndex   Edict index used by the created entity (ignored on Orangebox and above).
 * @return                  Entity index on success, or -1 on failure.
 * @error                   Invalid edict index, no map is running, or lack of mod support.
 */
native int CreateEntityByName(const char[] classname, int ForceEdictIndex=-1);

/**
 * Spawns an entity into the game.
 *
 * @param entity        Entity index of the created entity.
 * @return              True on success, false otherwise.
 * @error               Invalid entity index or lack of mod support.
 */
native bool DispatchSpawn(int entity);

/**
 * Dispatches a KeyValue into given entity using a string value.
 *
 * @param entity        Destination entity index.
 * @param keyName       Name of the key.
 * @param value         String value.
 * @return              True on success, false otherwise.
 * @error               Invalid entity index or lack of mod support.
 */
native bool DispatchKeyValue(int entity, const char[] keyName, const char[] value);

/**
 * Dispatches a KeyValue into given entity using an integer value.
 *
 * @param entity        Destination entity index.
 * @param keyName       Name of the key.
 * @param value         Integer value.
 * @return              True on success, false otherwise.
 * @error               Invalid entity index or lack of mod support.
 */
stock bool DispatchKeyValueInt(int entity, const char[] keyName, int value)
{
    char str[12];
    FormatEx(str, sizeof(str), "%d", value);

    return DispatchKeyValue(entity, keyName, str);
}

/**
 * Dispatches a KeyValue into given entity using a floating point value.
 *
 * @param entity        Destination entity index.
 * @param keyName       Name of the key.
 * @param value         Floating point value.
 * @return              True on success, false otherwise.
 * @error               Invalid entity index or lack of mod support.
 */
native bool DispatchKeyValueFloat(int entity, const char[] keyName, float value);

/**
 * Dispatches a KeyValue into given entity using a vector value.
 *
 * @param entity        Destination entity index.
 * @param keyName       Name of the key.
 * @param vec           Vector value.
 * @return              True on success, false otherwise.
 * @error               Invalid entity index or lack of mod support.
 */
native bool DispatchKeyValueVector(int entity, const char[] keyName, const float vec[3]);

/**
 * Returns the entity a client is aiming at.
 *
 * @param client        Client performing the aiming.
 * @param only_clients  True to exclude all entities but clients.
 * @return              Entity index being aimed at.
 *                      -1 if no entity is being aimed at.
 *                      -2 if the function is not supported.
 * @error               Invalid client index or client not in game.
 */
native int GetClientAimTarget(int client, bool only_clients=true);

/**
 * Returns the total number of teams in a game.
 * Note: This native should not be called before OnMapStart.
 *
 * @return              Total number of teams.
 */
native int GetTeamCount();

/**
 * Retrieves the team name based on a team index.
 * Note: This native should not be called before OnMapStart.
 *
 * @param index         Team index.
 * @param name          Buffer to store string in.
 * @param maxlength     Maximum length of string buffer.
 * @error               Invalid team index.
 */
native void GetTeamName(int index, char[] name, int maxlength);

/**
 * Returns the score of a team based on a team index.
 * Note: This native should not be called before OnMapStart.
 *
 * @param index         Team index.
 * @return              Score.
 * @error               Invalid team index.
 */
native int GetTeamScore(int index);

/**
 * Sets the score of a team based on a team index.
 * Note: This native should not be called before OnMapStart.
 *
 * @param index         Team index.
 * @param value         New score value.
 * @error               Invalid team index.
 */
native void SetTeamScore(int index, int value);

/**
 * Retrieves the number of players in a certain team.
 * Note: This native should not be called before OnMapStart.
 *
 * @param index         Team index.
 * @return              Number of players in the team.
 * @error               Invalid team index.
 */
native int GetTeamClientCount(int index);

/**
 * Returns the entity index of a team.
 *
 * @param teamIndex     Team index.
 * @return              Entity index of team.
 * @error               Invalid team index.
 */
native int GetTeamEntity(int teamIndex);

/**
 * Sets the model to a given entity.
 *
 * @param entity        Entity index.
 * @param model         Model name.
 * @error               Invalid entity index or lack of mod support.
 */
native void SetEntityModel(int entity, const char[] model);

/**
 * Retrieves the decal file name associated with a given client.
 *
 * @param client        Player's index.
 * @param hex           Buffer to store the logo filename.
 * @param maxlength     Maximum length of string buffer.
 * @return              True on success, otherwise false.
 * @error               Invalid client or client not in game.
 */
native bool GetPlayerDecalFile(int client, char[] hex, int maxlength);

/**
 * Retrieves the jingle file name associated with a given client.
 *
 * @param client        Player's index.
 * @param hex           Buffer to store the jingle filename.
 * @param maxlength     Maximum length of string buffer.
 * @return              True on success, otherwise false.
 * @error               Invalid client or client not in game.
 */
native bool GetPlayerJingleFile(int client, char[] hex, int maxlength);

/**
 * Returns the average server network traffic in bytes/sec.
 *
 * @param in            Buffer to store the input traffic velocity.
 * @param out           Buffer to store the output traffic velocity.
 * @error               Lack of mod support.
 */
native void GetServerNetStats(float &inAmount, float &outAmout);

/**
 * Equip's a player's weapon.
 *
 * @param client        Client index.
 * @param weapon        CBaseCombatWeapon entity index.
 * @error               Invalid client or entity, lack of mod support, or client not in
 *                      game.
 */
native void EquipPlayerWeapon(int client, int weapon);

/**
 * Activates an entity (CBaseAnimating::Activate)
 *
 * @param entity        Entity index.
 * @error               Invalid entity or lack of mod support.
 */
native void ActivateEntity(int entity);

/**
 * Sets values to client info buffer keys and notifies the engine of the change.
 * The change does not get propagated to mods until the next frame.
 *
 * @param client        Player's index.
 * @param key           Key string.
 * @param value         Value string.
 * @error               Invalid client index, client not connected, or lack of mod support.
 */
native void SetClientInfo(int client, const char[] key, const char[] value);

/**
 * Changes a client's name.
 *
 * @param client        Player's index.
 * @param name          New name.
 * @error               Invalid client index, client not connected, or lack of mod support.
 */
native void SetClientName(int client, const char[] name);

/**
 * Gives ammo of a certain type to a player.
 * This natives obeys the maximum amount of ammo a player can carry per ammo type.
 *
 * @param client        The client index.
 * @param amount        Amount of ammo to give. Is capped at ammotype's limit.
 * @param ammotype      Type of ammo to give to player.
 * @param suppressSound If true, don't play the ammo pickup sound.
 * @return              Amount of ammo actually given.
 * @error               Lack of mod support.
 */
native int GivePlayerAmmo(int client, int amount, int ammotype, bool suppressSound=false);

/**
 * Changes an entity's collision group (CBaseEntity::SetCollisionGroup).
 * 
 * @param entity            The entity index.
 * @param collisionGroup    Collision group to use.
 * @error                   Invalid entity or lack of mod support.
 */
native void SetEntityCollisionGroup(int entity, int collisionGroup);

/**
 * Recaculates entity collision rules (CBaseEntity::CollisionRulesChanged).
 * 
 * @param entity            The entity index.
 * @error                   Invalid entity or lack of mod support.
 */
native void EntityCollisionRulesChanged(int entity);

/**
 * Sets an entity's owner (CBaseEntity::SetEntityOwner).
 * 
 * @param entity            The entity index.
 * @param owner             The owner entity index, can be invalid.
 * @error                   Invalid entity or lack of mod support.
 */
native void SetEntityOwner(int entity, int owner=INVALID_ENT_REFERENCE);

/**
 * Returns the index number of a given named attachment.
 * 
 * @param entity        The entity index.
 * @param name          The attachment name.
 * @return              An attachment index, or 0 if the attachment name is invalid or unused.
 * @error               Invalid entity or lack of mod support.
 */
native int LookupEntityAttachment(int entity, const char[] name);

/**
 * Returns the world location and world angles of an attachment.
 * 
 * @param entity        The entity index.
 * @param attachment    The attachment index.
 * @param origin        Destination vector to store the attachment's origin vector.
 * @param angles        Destination vector to store the attachment's position angle.
 * @return              True on success, otherwise false.
 * @error               Invalid entity or lack of mod support.
 */
native bool GetEntityAttachment(int entity, int attachment, float origin[3], float angles[3]);
