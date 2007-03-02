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
 * For more information on these, see the HL2SDK (public/edict.h)
 */
#define FL_EDICT_CHANGED	(1<<0)	/**< Game DLL sets this when the entity state changes
										 Mutually exclusive with FL_EDICT_PARTIAL_CHANGE. */
#define FL_EDICT_FREE		(1<<1)	/**< this edict if free for reuse */
#define FL_EDICT_FULL		(1<<2)	/**< this is a full server entity */
#define FL_EDICT_FULLCHECK	(0<<0)  /**< call ShouldTransmit() each time, this is a fake flag */
#define FL_EDICT_ALWAYS		(1<<3)	/**< always transmit this entity */
#define FL_EDICT_DONTSEND	(1<<4)	/**< don't transmit this entity */
#define FL_EDICT_PVSCHECK	(1<<5)	/**< always transmit entity, but cull against PVS */
#define FL_EDICT_PENDING_DORMANT_CHECK	(1<<6)
#define FL_EDICT_DIRTY_PVS_INFORMATION	(1<<7)
#define FL_FULL_EDICT_CHANGED			(1<<8)

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
 * @param edict			Index of the entity/edict.
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
 * Returns whether or not an entity is a valid networkable edict.
 *
 * @param edict			Index of the edict.
 * @return				True if networkable, false if invalid or not networkable.
 */
native bool:IsEntNetworkable(edict);

/**
 * Creates a new edict (the basis of a networkable entity)
 *
 * @return				Index of the edict, 0 on failure.
 */
native CreateEdict();

/** 
 * Removes an edict from the world.
 *
 * @param edict			Index of the edict.
 * @noreturn
 * @error				Invalid edict index.
 */
native RemoveEdict(edict);

/**
 * Returns the flags on an edict.  These are not the same as entity flags.
 *
 * @param edict			Index of the entity.
 * @return				Edict flags.
 * @error				Invalid edict index.
 */
native GetEdictFlags(edict);

/**
 * Sets the flags on an edict.  These are not the same as entity flags.
 *
 * @param edict			Index of the entity.
 * @param flags			Flags to set.
 * @noreturn
 * @error				Invalid edict index.
 */
native SetEdictFlags(edict, flags);

/**
 * Retrieves an edict classname.
 *
 * @param edict			Index of the entity.
 * @param clsname		Buffer to store the classname.
 * @param maxlength		Maximum length of the buffer.
 * @return				True on success, false if there is no classname set.
 */
native bool:GetEdictClassname(edict, String:clsname[], maxlength);

/**
 * Retrieves an entity's networkable serverclass name.
 * This is not the same as the classname and is used for networkable state changes.
 *
 * @param edict			Index of the entity.
 * @param clsname		Buffer to store the serverclass name.
 * @param maxlength		Maximum lnegth of the buffer.
 * @return				True on success, false if the edict is not networkable.
 * @error				Invalid edict index.
 */
native bool:GetEntityNetClass(edict, String:clsname[], maxlength);

/**
 * @section Entity offset functions
 *
 * Offsets should be specified in byte distance from the CBaseEntity 
 * structure, not short (double byte) or integer (four byte) multiples.  
 * It is somewhat common practice to use offsets aligned to their final 
 * type, and thus make sure you are not falling to this error in SourceMod.
 * For example, if your "integer-aligned" offset was 119, your byte-aligned
 * offset is 119*4, or 476.
 
 * Specifying incorrect offsets or the incorrect data type for an offset
 * can have fatal consequences.  If you are hardcoding offsets, and the 
 * layout of CBaseEntity does not match, you can easily crash the server.
 *
 * The reasonable bounds for offsets is greater than or equal to 0 and 
 * below 32768.  Offsets out of these bounds will throw an error.  However,
 * this does not represent any real range, it is simply a sanity check for
 * illegal values.  Any range outside of the CBaseEntity structure's private
 * size will cause undefined behaviour or even crash.
 */
 
/**
 * Marks an entity as state changed.  This can be useful if you set an offset 
 * and wish for it to be immediately changed over the network.  By default this 
 * is not done for offset setting functions.  
 *
 * @param edict			Index to the edict.
 * @param offset		Offset to mark as changed.  If 0, 
 *						the entire edict is marked as changed.
 * @noreturn
 * @error				Invalid entity or offset out of bounds.
 */
native ChangeEdictState(edict, offset = 0);

/**
 * Peeks into an entity's object data and retrieves the integer value at 
 * the given offset.
 * 
 * @param entity		Edict index.
 * @param offset		Offset to use.
 * @param size			Number of bytes to read (valid values are 1, 2, or 4).
 * @return				Value at the given memory location.
 * @error				Invalid entity or offset out of reasonable bounds.
 */
native GetEntData(entity, offset, size=4);

/**
 * Peeks into an entity's object data and sets the integer value at 
 * the given offset.
 *
 * @param entity		Edict index.
 * @param offset		Offset to use.
 * @param size			Number of bytes to write (valid values are 1, 2, or 4).
 * @return				Value at the given memory location.
 * @error				Invalid entity or offset out of reasonable bounds.
 * @noreturn
 */
native SetEntData(entity, offset, value, size=4);

/**
 * Peeks into an entity's object data and retrieves the float value at 
 * the given offset.
 * 
 * @param entity		Edict index.
 * @param offset		Offset to use.
 * @return				Value at the given memory location.
 * @error				Invalid entity or offset out of reasonable bounds.
 */
native Float:GetEntDataFloat(entity, offset);

/**
 * Peeks into an entity's object data and sets the integer value at 
 * the given offset.
 *
 * @param entity		Edict index.
 * @param offset		Offset to use.
 * @return				Value at the given memory location.
 * @error				Invalid entity or offset out of reasonable bounds.
 * @noreturn
 */
native SetEntDataFloat(entity, offset, Float:value);

/**
 * Peeks into an entity's object data and retrieves the entity handle
 * info at the given offset.
 * 
 * @param entity		Edict index.
 * @param offset		Offset to use.
 * @return				Entity index at the given location, or 0 if none.
 * @error				Invalid entity or offset out of reasonable bounds.
 */
native GetEntDataEnt(entity, offset);

/**
 * Peeks into an entity's object data and sest the entity handle info
 * at the given offset.
 * 
 * @param entity		Edict index.
 * @param offset		Offset to use.
 * @param other			Entity index to set, or 0 to clear.
 * @noreturn
 * @error				Invalid entity or offset out of reasonable bounds.
 */
native SetEntDataEnt(entity, offset, other);

/**
 * Peeks into an entity's object data and retrieves the vector at the 
 * given offset.
 * @note Both a Vector and a QAngle are three floats.  This is a 
 * convenience function and will work with both types.
 * 
 * @param entity		Edict index.
 * @param offset		Offset to use.
 * @param vec			Vector buffer to store data in.
 * @noreturn
 * @error				Invalid entity or offset out of reasonable bounds.
 */
native GetEntDataVector(entity, offset, Float:vec[3]);

/**
 * Peeks into an entity's object data and sets the vector at the given 
 * offset.
 * @note Both a Vector and a QAngle are three floats.  This is a 
 * convenience function and will work with both types.
 * 
 * @param entity		Edict index.
 * @param offset		Offset to use.
 * @param vec			Vector to set.
 * @noreturn
 * @error				Invalid entity or offset out of reasonable bounds.
 */
native SetEntDataVector(entity, offset, const Float:vec[3]);
