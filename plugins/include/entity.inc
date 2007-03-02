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
 
#if defined _entity_included
 #endinput
#endif
#define _entity_included

/**
 * Returns the maximum number of entities.
 *
 * @return				Maximum number of entities.
 */
native GetMaxEntities();

/**
 * Returns the number of entities in the server.
 *
 * @return				Number of entities in the server.
 */
native GetEntityCount();

/**
 * Returns whether or not an entity is valid.  Returns false
 * if there is no matching CBaseEntity for this edict index.
 *
 * @param edict			Index of the entity.
 * @return				True if valid, false otherwise.
 */
native bool:IsValidEntity(edict);

/**
 * Returns whether or not an edict index is valid.
 *
 * @param edict			Index of the edict.
 * @return				True if valid, false otherwise.
 */
native bool:IsValidEdict(edict);

/**
 * Creates a new edict (the basis of a networkable entity)
 *
 * @return				Index of the entity, 0 on failure.
 */
native CreateEdict();

/** 
 * Removes an edict from the world.
 *
 * @param edict			Index of the entity.
 * @noreturn
 * @error				Invalid entity index.
 */
native RemoveEdict(edict);
