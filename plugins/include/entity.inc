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
 
#if defined _entity_included
 #endinput
#endif
#define _entity_included

/**
 * Property types for entities.
 */
enum PropType
{
	Prop_Send = 0,	/**< This property is networked. */
	Prop_Data = 1,	/**< This property is for save game data fields. */
};

/**
 * @section For more information on these, see the HL2SDK (public/edict.h)
 */
#define FL_EDICT_CHANGED		(1<<0)	/**< Game DLL sets this when the entity state changes
										 Mutually exclusive with FL_EDICT_PARTIAL_CHANGE. */
#define FL_EDICT_FREE			(1<<1)	/**< this edict if free for reuse */
#define FL_EDICT_FULL			(1<<2)	/**< this is a full server entity */
#define FL_EDICT_FULLCHECK		(0<<0)  /**< call ShouldTransmit() each time, this is a fake flag */
#define FL_EDICT_ALWAYS			(1<<3)	/**< always transmit this entity */
#define FL_EDICT_DONTSEND		(1<<4)	/**< don't transmit this entity */
#define FL_EDICT_PVSCHECK		(1<<5)	/**< always transmit entity, but cull against PVS */
#define FL_EDICT_PENDING_DORMANT_CHECK	(1<<6)
#define FL_EDICT_DIRTY_PVS_INFORMATION	(1<<7)
#define FL_FULL_EDICT_CHANGED		(1<<8)

enum PropFieldType
{
	PropField_Unsupported,		/**< The type is unsupported. */
	PropField_Integer,			/**< Valid for SendProp and Data fields */
	PropField_Float,			/**< Valid for SendProp and Data fields */
	PropField_Entity,			/**< Valid for Data fields only (SendProp shows as int) */
	PropField_Vector,			/**< Valid for SendProp and Data fields */
	PropField_String,			/**< Valid for SendProp and Data fields */
	PropField_String_T,			/**< Valid for Data fields.  Read only.  
									 Note that the size of a string_t is dynamic, and 
									 thus FindDataMapOffs() will return the constant size 
									 of the string_t container (which is 32 bits right now).
								 */
};

/**
 * @endsection
 */

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
 * @param changeState	If true, change will be sent over the network.
 * @return				Value at the given memory location.
 * @error				Invalid entity or offset out of reasonable bounds.
 * @noreturn
 */
native SetEntData(entity, offset, any:value, size=4, bool:changeState=false);

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
 * @param changeState	If true, change will be sent over the network.
 * @return				Value at the given memory location.
 * @error				Invalid entity or offset out of reasonable bounds.
 * @noreturn
 */
native SetEntDataFloat(entity, offset, Float:value, bool:changeState=false);

/**
 * This function is deprecated.  Use GetEntDataEnt2 instead, for 
 * reasons explained in the notes.
 *
 * Note: This function returns 0 on failure, which may be misleading, 
 * as the number 0 is also used for the world entity index.
 *
 * Note: This function makes no attempt to validate the returned 
 * entity, and in fact, it could be garbage or completely unexpected.
 * 
 * @param entity		Edict index.
 * @param offset		Offset to use.
 * @return				Entity index at the given location, or 0 if none.
 * @error				Invalid entity or offset out of reasonable bounds.
 */
#pragma deprecated Use GetEntDataEnt2() instead.
native GetEntDataEnt(entity, offset);

/**
 * This function is deprecated.   Use SetEntDataEnt2 instead, for 
 * reasons explained in the notes.
 *
 * Note: This function uses 0 as an indicator to unset data, but 
 * 0 is also the world entity index.  Thus, the a property cannot 
 * be set to the world entity using this native.
 * 
 * @param entity		Edict index.
 * @param offset		Offset to use.
 * @param other			Entity index to set, or 0 to clear.
 * @param changeState	If true, change will be sent over the network.
 * @noreturn
 * @error				Invalid entity or offset out of reasonable bounds.
 */
#pragma deprecated Use SetEntDataEnt2() instead.
native SetEntDataEnt(entity, offset, other, bool:changeState=false);

/**
 * Peeks into an entity's object data and retrieves the entity index 
 * at the given offset.
 *
 * Note: This will only work on offsets that are stored as "entity 
 * handles" (which usually looks like m_h* in properties).  These 
 * are not SourceMod Handles, but internal Source structures.
 * 
 * @param entity		Edict index.
 * @param offset		Offset to use.
 * @return				Entity index at the given location.  If there is no entity, 
 *						or the stored entity is invalid, then -1 is returned.
 * @error				Invalid input entity, or offset out of reasonable bounds.
 */
native GetEntDataEnt2(entity, offset);

/**
 * Peeks into an entity's object data and sets the entity index at the 
 * given offset.
 *
 * Note: This will only work on offsets that are stored as "entity 
 * handles" (which usually looks like m_h* in properties).  These 
 * are not SourceMod Handles, but internal Source structures.
 * 
 * @param entity		Edict index.
 * @param offset		Offset to use.
 * @param other			Entity index to set, or -1 to clear.
 * @param changeState	If true, change will be sent over the network.
 * @noreturn
 * @error				Invalid input entity, or offset out of reasonable bounds.
 */
native SetEntDataEnt2(entity, offset, other, bool:changeState=false);

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
 * @param changeState	If true, change will be sent over the network.
 * @noreturn
 * @error				Invalid entity or offset out of reasonable bounds.
 */
native SetEntDataVector(entity, offset, const Float:vec[3], bool:changeState=false);

/**
 * Peeks into an entity's object data and retrieves the string at 
 * the given offset.
 * 
 * @param entity		Edict index.
 * @param offset		Offset to use.
 * @param buffer		Destination string buffer.
 * @param maxlen		Maximum length of output string buffer.
 * @return			Number of non-null bytes written.
 * @error				Invalid entity or offset out of reasonable bounds.
 */
native GetEntDataString(entity, offset, String:buffer[], maxlen);

/**
 * Peeks into an entity's object data and sets the string at 
 * the given offset.
 * 
 * @param entity		Edict index.
 * @param offset		Offset to use.
 * @param buffer		String to set.
 * @param maxlen		Maximum length of bytes to write.
 * @param changeState	If true, change will be sent over the network.
 * @return			Number of non-null bytes written.
 * @error				Invalid entity or offset out of reasonable bounds.
 */
native SetEntDataString(entity, offset, const String:buffer[], maxlen, bool:changeState=false);

/**
 * @endsection
 */

/**
 * Given a ServerClass name, finds a networkable send property offset.
 * This information is cached for future calls.
 *
 * Note, this function may return offsets that do not work!
 * If a property is nested beneath a parent object, the resulting offset 
 * will be invalid for direct use with data functions.  Therefore, you 
 * should use FindSendPropInfo() instead.  An example of such a property is 
 * CTFPlayer::DT_LocalPlayer.m_nDisguiseClass on Team Fortress.
 *
 * @param cls			Classname.
 * @param prop			Property name.
 * @return				An offset, or -1 on failure.
 */
native FindSendPropOffs(const String:cls[], const String:prop[]);

/**
 * Given a ServerClass name, finds a networkable send property offset.
 * This information is cached for future calls.
 * 
 * Note: This function will correctly compute nested offsets, unlike 
 * FindSendPropOffs().  YOU SHOULD NOT use this function to self-compute 
 * nested offsets.  For example, it is okay to add indexes for arrays, 
 * but not to add DT_LocalPlayer to m_nDisguiseClass.
 *
 * @param cls			Classname.
 * @param prop			Property name.
 * @param type			Optional parameter to store the type.
 * @param num_bits		Optional parameter to store the number of bits the field 
 *						uses, if applicable (otherwise 0 is stored).  The number 
 *						of bits varies for integers and floats, and is always 0 
 * 						for strings.
 * @param local_offset	Optional parameter to store the local offset, as 
 *						FindSendPropOffs() would return.
 * @return				On success, returns an absolutely computed offset.
 *						If no offset is available, 0 is returned.
 *						If the property is not found, -1 is returned.
 */
native FindSendPropInfo(const String:cls[], 
						const String:prop[], 
						&PropFieldType:type=PropFieldType:0,
						&num_bits=0,
						&local_offset=0);

/**
 * Given an entity, finds a datamap property offset.
 * This information is cached for future calls.
 *
 * @param entity		Entity index.
 * @param prop			Property name.
 * @param type			Optional parameter to store the type.
 * @param num_bits		Optional parameter to store the number of bits the field 
 *						uses.  The bit count will either be 1 (for boolean) or 
 *						divisible by 8 (including 0 if unknown).
 * @return				An offset, or -1 on failure.
 */
native FindDataMapOffs(entity, 
					   const String:prop[],
					   &PropFieldType:type=PropFieldType:0,
					   &num_bits=0);

/**
 * Wrapper function for finding a send property for a particular entity.
 *
 * @param ent			Entity index.
 * @param prop			Property name.
 * @param actual		Defaults to false for backwards compatibility.
 *						If true, the newer FindSendPropInfo() function 
 *						is used instead.
 * @return				An offset, or -1 on failure.
 */
stock GetEntSendPropOffs(ent, const String:prop[], bool:actual=false)
{
	decl String:cls[64];
	
	if (!GetEntityNetClass(ent, cls, sizeof(cls)))
	{
		return -1;
	}
	
	if (actual)
	{
		return FindSendPropInfo(cls, prop);
	}
	else
	{
		return FindSendPropOffs(cls, prop);
	}
}

/**
 * Retrieves an integer value from an entity's property.
 *
 * This function is considered safer and more robust over GetEntData, 
 * because it performs strict offset checking and typing rules.
 * 
 * @param entity		Entity/edict index.
 * @param type			Property type.
 * @param prop			Property name.
 * @param size			Number of bytes to write (valid values are 1, 2, or 4).
 *						This value is auto-detected, and the size parameter is 
 *						only used as a fallback in case detection fails.
 * @return				Value at the given property offset.
 * @error				Invalid entity or property not found.
 */
native GetEntProp(entity, PropType:type, const String:prop[], size=4);

/**
 * Sets an integer value in an entity's property.
 *
 * This function is considered safer and more robust over SetEntData, 
 * because it performs strict offset checking and typing rules.
 * 
 * @param entity		Entity/edict index.
 * @param type			Property type.
 * @param prop			Property name.
 * @param size			Number of bytes to write (valid values are 1, 2, or 4).
 *						This value is auto-detected, and the size parameter is 
 *						only used as a fallback in case detection fails.
 * @error				Invalid entity or offset out of reasonable bounds.
 * @noreturn
 */
native SetEntProp(entity, PropType:type, const String:prop[], any:value, size=4);

/**
 * Retrieves a float value from an entity's property.
 *
 * This function is considered safer and more robust over GetEntDataFloat, 
 * because it performs strict offset checking and typing rules.
 * 
 * @param entity		Entity/edict index.
 * @param type			Property type.
 * @param prop			Property name.
 * @return				Value at the given property offset.
 * @error				Invalid entity or offset out of reasonable bounds.
 */
native Float:GetEntPropFloat(entity, PropType:type, const String:prop[]);

/**
 * Sets a float value in an entity's property.
 *
 * This function is considered safer and more robust over SetEntDataFloat, 
 * because it performs strict offset checking and typing rules.
 * 
 * @param entity		Entity/edict index.
 * @param type			Property type.
 * @param prop			Property name.
 * @param value			Value to set.
 * @noreturn
 * @error				Invalid entity or offset out of reasonable bounds.
 */
native SetEntPropFloat(entity, PropType:type, const String:prop[], Float:value);

/**
 * Retrieves an entity index from an entity's property.
 *
 * This function is considered safer and more robust over GetEntDataEnt*, 
 * because it performs strict offset checking and typing rules.
 * 
 * @param entity		Entity/edict index.
 * @param type			Property type.
 * @param prop			Property name.
 * @return				Entity index at the given property.
 *						If there is no entity, or the entity is not valid, 
 *						then -1 is returned.
 * @error				Invalid entity or offset out of reasonable bounds.
 */
native GetEntPropEnt(entity, PropType:type, const String:prop[]);

/**
 * Sets an entity index in an entity's property.
 *
 * This function is considered safer and more robust over SetEntDataEnt*, 
 * because it performs strict offset checking and typing rules.
 * 
 * @param entity		Entity/edict index.
 * @param type			Property type.
 * @param prop			Property name.
 * @param other			Entity index to set, or -1 to unset.
 * @noreturn
 * @error				Invalid entity or offset out of reasonable bounds.
 */
native SetEntPropEnt(entity, PropType:type, const String:prop[], other);

/**
 * Retrieves a vector of floats from an entity, given a named network property.
 *
 * This function is considered safer and more robust over GetEntDataVector, 
 * because it performs strict offset checking and typing rules.
 * 
 * @param entity		Entity/edict index.
 * @param type			Property type.
 * @param prop			Property name.
 * @param vec			Vector buffer to store data in.
 * @noreturn
 * @error				Invalid entity, property not found, or property not 
 *						actually a vector data type.
 */
native GetEntPropVector(entity, PropType:type, const String:prop[], Float:vec[3]);

/**
 * Sets a vector of floats in an entity, given a named network property.
 *
 * This function is considered safer and more robust over SetEntDataVector, 
 * because it performs strict offset checking and typing rules.
 * 
 * @param entity		Entity/edict index.
 * @param type			Property type.
 * @param prop			Property name.
 * @param vec			Vector to set.
 * @noreturn
 * @error				Invalid entity, property not found, or property not 
 *						actually a vector data type.
 */
native SetEntPropVector(entity, PropType:type, const String:prop[], const Float:vec[3]);

/**
 * Gets a network property as a string.
 * 
 * @param entity		Edict index.
 * @param type			Property type.
 * @param prop			Property to use.
 * @param buffer		Destination string buffer.
 * @param maxlen		Maximum length of output string buffer.		
 * @return				Number of non-null bytes written.
 * @error				Invalid entity, offset out of reasonable bounds, or property is not a valid string.
 */
native GetEntPropString(entity, PropType:type, const String:prop[], String:buffer[], maxlen);

/**
 * Sets a network property as a string.
 *
 * This cannot set property fields of type PropField_String_T (such as "m_target").
 * To set such fields, you should use DispatchKeyValue() from SDKTools.
 * 
 * @param entity		Edict index.
 * @param type			Property type.
 * @param prop			Property to use.
 * @param buffer		String to set.		
 * @return				Number of non-null bytes written.
 * @error				Invalid entity, offset out of reasonable bounds, or property is not a valid string.
 */
native SetEntPropString(entity, PropType:type, const String:prop[], const String:buffer[]);

/**
 * Copies an array of cells from an entity at a given offset.
 *
 * @param entity		Entity index.
 * @param offset		Offset to use.
 * @param array			Array to read into.
 * @param arraySize		Number of values to read.
 * @param dataSize		Size of each value in bytes (1, 2, or 4).
 * @noreturn
 * @error				Invalid entity or offset out of reasonable bounds.
 */
stock GetEntDataArray(entity, offset, array[], arraySize, dataSize=4)
{
	for (new i=0; i<arraySize; i++)
	{
		array[i] = GetEntData(entity, offset + i*dataSize, dataSize)
	}
}

/**
 * Copies an array of cells to an entity at a given offset.
 *
 * @param entity		Entity index.
 * @param offset		Offset to use.
 * @param array			Array of values to copy.
 * @param arraySize		Number of values to copy.
 * @param dataSize		Size of each value in bytes (1, 2, or 4).
 * @param changeState	True to set the network state as changed; false otherwise.
 * @noreturn
 * @error				Invalid entity or offset out of reasonable bounds.
 */
stock SetEntDataArray(entity, offset, const array[], arraySize, dataSize=4, bool:changeState=false)
{
	for (new i=0; i<arraySize; i++)
	{
		SetEntData(entity, offset + i*dataSize, array[i], dataSize, changeState);
	}
}
