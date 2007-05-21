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
 * Property types for entities.
 */
enum PropType
{
	Prop_Send = 0,	/**< This property is networked. */
	Prop_Data = 1,	/**< This property is for saved game files. */
};

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
 * @param changeState	If true, change will be sent over the network.
 * @return				Value at the given memory location.
 * @error				Invalid entity or offset out of reasonable bounds.
 * @noreturn
 */
native SetEntData(entity, offset, value, size=4, bool:changeState=false);

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
 * Peeks into an entity's object data and sets the entity handle info
 * at the given offset.
 * 
 * @param entity		Edict index.
 * @param offset		Offset to use.
 * @param other			Entity index to set, or 0 to clear.
 * @param changeState	If true, change will be sent over the network.
 * @noreturn
 * @error				Invalid entity or offset out of reasonable bounds.
 */
native SetEntDataEnt(entity, offset, other, bool:changeState=false);

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
 * @param cls			Classname.
 * @param prop			Property name.
 * @return				An offset, or -1 on failure.
 */
native FindSendPropOffs(const String:cls[], const String:prop[]);

/**
 * Given an entity, finds a datamap property offset.
 * This information is cached for future calls.
 *
 * @param entity		Entity index.
 * @param prop			Property name.
 * @return				An offset, or -1 on failure.
 */
native FindDataMapOffs(entity, const String:prop[]);

/**
 * Wrapper function for finding a send property for a particular entity.
 *
 * @param ent			Entity index.
 * @param prop			Property name.
 * @return				An offset, or -1 on failure.
 */
stock GetEntSendPropOffs(ent, const String:prop[])
{
	decl String:cls[64];
	
	if (!GetEntityNetClass(ent, cls, sizeof(cls)))
	{
		return -1;
	}
	
	return FindSendPropOffs(cls, prop);
}

/**
 * Gets a network property as an integer; wrapper around GetEntData().
 * 
 * @param entity		Edict index.
 * @param type			Property type.
 * @param prop			Property to use.
 * @param size			Number of bytes to read (valid values are 1, 2, or 4).
 * @return				Value at the given property offset.
 * @error				Invalid entity or property not found.
 */
stock GetEntProp(entity, PropType:type, const String:prop[], size=4)
{
	new offs;

	switch (type)
	{
		case Prop_Send:
		{
			offs = GetEntSendPropOffs(entity, prop);
		}
		case Prop_Data:
		{
			offs = FindDataMapOffs(entity, prop);
		}
		default:
		{
			ThrowError("Invalid Property type %d", type);
		}
	}

	if (offs == -1)
	{
		ThrowError("Property \"%s\" not found for entity %d", prop, entity);
	}

	return GetEntData(entity, offs, size);
}

/**
 * Sets a network property as an integer; wrapper around GetEntData().
 *
 * @param entity		Edict index.
 * @param type			Property type.
 * @param prop			Property to use.
 * @param size			Number of bytes to write (valid values are 1, 2, or 4).
 * @error				Invalid entity or offset out of reasonable bounds.
 * @noreturn
 */
stock SetEntProp(entity, PropType:type, const String:prop[], value, size=4)
{
	new offs;

	switch (type)
	{
		case Prop_Send:
		{
			offs = GetEntSendPropOffs(entity, prop);
		}
		case Prop_Data:
		{
			offs = FindDataMapOffs(entity, prop);
		}
		default:
		{
			ThrowError("Invalid Property type %d", type);
		}
	}

	if (offs == -1)
	{
		ThrowError("Property \"%s\" not found for entity %d", prop, entity);
	}

	return SetEntData(entity, offs, value, size, true);
}

/**
 * Gets a network property as a float; wrapper around GetEntDataFloat().
 * 
 * @param entity		Edict index.
 * @param type			Property type.
 * @param prop			Property to use.
 * @return				Value at the given property offset.
 * @error				Invalid entity or offset out of reasonable bounds.
 */
stock Float:GetEntPropFloat(entity, PropType:type, const String:prop[])
{
	new offs;

	switch (type)
	{
		case Prop_Send:
		{
			offs = GetEntSendPropOffs(entity, prop);
		}
		case Prop_Data:
		{
			offs = FindDataMapOffs(entity, prop);
		}
		default:
		{
			ThrowError("Invalid Property type %d", type);
		}
	}

	if (offs == -1)
	{
		ThrowError("Property \"%s\" not found for entity %d", prop, entity);
	}

	return GetEntDataFloat(entity, offs);
}

/**
 * Sets a network property as a float; wrapper around SetEntDataFloat().
 * 
 * @param entity		Edict index.
 * @param type			Property type.
 * @param prop			Property to use.
 * @param value			Value to set.
 * @noreturn
 * @error				Invalid entity or offset out of reasonable bounds.
 */
stock SetEntPropFloat(entity, PropType:type, const String:prop[], Float:value)
{
	new offs;

	switch (type)
	{
		case Prop_Send:
		{
			offs = GetEntSendPropOffs(entity, prop);
		}
		case Prop_Data:
		{
			offs = FindDataMapOffs(entity, prop);
		}
		default:
		{
			ThrowError("Invalid Property type %d", type);
		}
	}

	if (offs == -1)
	{
		ThrowError("Property \"%s\" not found for entity %d", prop, entity);
	}

	return SetEntDataFloat(entity, offs, value, true);
}

/**
 * Gets a network property as a handle entity; wrapper around GetEntDataEnt().
 * 
 * @param entity		Edict index.
 * @param type			Property type.
 * @param prop			Property to use.
 * @return				Entity index at the given property, or 0 if none.
 * @error				Invalid entity or offset out of reasonable bounds.
 */
stock GetEntPropEnt(entity, PropType:type, const String:prop[])
{
	new offs;

	switch (type)
	{
		case Prop_Send:
		{
			offs = GetEntSendPropOffs(entity, prop);
		}
		case Prop_Data:
		{
			offs = FindDataMapOffs(entity, prop);
		}
		default:
		{
			ThrowError("Invalid Property type %d", type);
		}
	}

	if (offs == -1)
	{
		ThrowError("Property \"%s\" not found for entity %d", prop, entity);
	}

	return GetEntDataEnt(entity, offs);
}

/**
 * Sets a network property as a handle entity; wrapper around SetEntDataEnt().
 * 
 * @param entity		Edict index.
 * @param type			Property type.
 * @param prop			Property to use.
 * @param other			Entity index to set, or 0 to unset.
 * @noreturn
 * @error				Invalid entity or offset out of reasonable bounds.
 */
stock SetEntPropEnt(entity, PropType:type, const String:prop[], other)
{
	new offs;

	switch (type)
	{
		case Prop_Send:
		{
			offs = GetEntSendPropOffs(entity, prop);
		}
		case Prop_Data:
		{
			offs = FindDataMapOffs(entity, prop);
		}
		default:
		{
			ThrowError("Invalid Property type %d", type);
		}
	}

	if (offs == -1)
	{
		ThrowError("Property \"%s\" not found for entity %d", prop, entity);
	}

	return SetEntDataEnt(entity, offs, other, true);
}

/**
 * Gets a network property as a vector; wrapper around GetEntDataVector().
 * @note Both a Vector and a QAngle are three floats.  This is a 
 * convenience function and will work with both types.
 * 
 * @param entity		Edict index.
 * @param type			Property type.
 * @param prop			Property to use.
 * @param vec			Vector buffer to store data in.
 * @noreturn
 * @error				Invalid entity or offset out of reasonable bounds.
 */
stock GetEntPropVector(entity, PropType:type, const String:prop[], Float:vec[3])
{
	new offs;

	switch (type)
	{
		case Prop_Send:
		{
			offs = GetEntSendPropOffs(entity, prop);
		}
		case Prop_Data:
		{
			offs = FindDataMapOffs(entity, prop);
		}
		default:
		{
			ThrowError("Invalid Property type %d", type);
		}
	}

	if (offs == -1)
	{
		ThrowError("Property \"%s\" not found for entity %d", prop, entity);
	}

	return GetEntDataVector(entity, offs, vec);
}

/**
 * Sets a network property as a vector; wrapper around SetEntDataVector().
 * @note Both a Vector and a QAngle are three floats.  This is a 
 * convenience function and will work with both types.
 * 
 * @param entity		Edict index.
 * @param type			Property type.
 * @param prop			Property to use.
 * @param vec			Vector to set.
 * @noreturn
 * @error				Invalid entity or offset out of reasonable bounds.
 */
stock SetEntPropVector(entity, PropType:type, const String:prop[], const Float:vec[3])
{
	new offs;

	switch (type)
	{
		case Prop_Send:
		{
			offs = GetEntSendPropOffs(entity, prop);
		}
		case Prop_Data:
		{
			offs = FindDataMapOffs(entity, prop);
		}
		default:
		{
			ThrowError("Invalid Property type %d", type);
		}
	}

	if (offs == -1)
	{
		ThrowError("Property \"%s\" not found for entity %d", prop, entity);
	}

	return SetEntDataVector(entity, offs, vec, true);
}

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
		array[i] = GetEntData(entity, offset, dataSize)
		offset += dataSize;
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
stock SetEntDataArray(entity, offset, value, const array[], arraySize, dataSize=4, bool:changeState=false)
{
	for (new i=0; i<arraySize; i++)
	{
		SetEntData(entity, real_offset, array[i], dataSize, changeState);
		offset += dataSize;
	}
}
