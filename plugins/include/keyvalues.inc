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
 
#if defined _keyvalues_included
 #endinput
#endif
#define _keyvalues_included

/**
 * KeyValue data value types
 */
enum KvDataTypes
{
	KvData_None = 0,	/**< Type could not be identified, or no type */
	KvData_String,		/**< String value */
	KvData_Int,			/**< Integer value */
	KvData_Float,		/**< Floating point value */
	KvData_Ptr,			/**< Pointer value (sometimes called "long") */
	KvData_WString,		/**< Wide string value */
	KvData_Color,		/**< Color value */
	KvData_UInt64,		/**< Large integer value */
	/* --- */
	KvData_NUMTYPES, 
};

/**
 * Creates a new KeyValues structure.  The Handle must always be closed.
 *
 * @param name			Name of the root section.
 * @param firstKey		If non-empty, specifies the first key value.
 * @param firstValue	If firstKey is non-empty, specifies the first key's value.
 * @return				A Handle to a new KeyValues structure.
 */
native Handle:CreateKeyValues(const String:name[], const String:firstkey[]="", const String:firstValue[]="");

/** 
 * Sets a string value of a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key, or NULL_STRING.
 * @param value			String value.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvSetString(Handle:kv, const String:key[], const String:value[]);

/** 
 * Sets an integer value of a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key, or NULL_STRING.
 * @param value			Value number.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvSetNum(Handle:kv, const String:key[], value);

/** 
 * Sets a large integer value of a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key, or NULL_STRING.
 * @param value			Large integer value (0=High bits, 1=Low bits)
 * @noreturn
 * @error				Invalid Handle.
 */
native KvSetUInt64(Handle:kv, const String:key[], const value[2]);

/** 
 * Sets a floating point value of a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key, or NULL_STRING.
 * @param value			Floating point value.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvSetFloat(Handle:kv, const String:key[], Float:value);

/** 
 * Sets a set of color values of a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key, or NULL_STRING.
 * @param r				Red value.
 * @param g				Green value.
 * @param b				Blue value.
 * @param a				Alpha value.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvSetColor(Handle:kv, const String:key[], r, g, b, a=0);

/** 
 * Sets a vector value of a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key, or NULL_STRING.
 * @param vec			Vector value.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvSetVector(Handle:kv, const String:key[], const Float:vec[3]);

/** 
 * Retrieves a string value from a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key, or NULL_STRING.
 * @param value			Buffer to store key value in.
 * @param maxlength		Maximum length of the value buffer.
 * @param defvalue		Optional default value to use if the key is not found.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvGetString(Handle:kv, const String:key[], String:value[], maxlength, const String:defvalue[]="");

/** 
 * Retrieves an integer value from a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key, or NULL_STRING.
 * @param defvalue		Optional default value to use if the key is not found.
 * @return				Integer value of the key.
 * @error				Invalid Handle.
 */
native KvGetNum(Handle:kv, const String:key[], defvalue=0);

/** 
 * Retrieves a floating point value from a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key, or NULL_STRING.
 * @param defvalue		Optional default value to use if the key is not found.
 * @return				Floating point value of the key.
 * @error				Invalid Handle.
 */
native Float:KvGetFloat(Handle:kv, const String:key[], Float:defvalue=0.0);

/** 
 * Retrieves a set of color values from a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key, or NULL_STRING.
 * @param r				Red value, set by reference.
 * @param g				Green value, set by reference. 
 * @param b				Blue value, set by reference. 
 * @param a				Alpha value, set by reference. 
 * @noreturn
 * @error				Invalid Handle.
 */
native KvGetColor(Handle:kv, const String:key[], &r, &g, &b, &a);

/** 
 * Retrieves a large integer value from a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key, or NULL_STRING.
 * @param value			Array to represent the large integer.
 * @param defvalue		Optional default value to use if the key is not found.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvGetUInt64(Handle:kv, const String:key[], value[2], defvalue[2]={0,0});

/** 
 * Retrieves a vector value from a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key, or NULL_STRING.
 * @param vec			Destination vector to store the value in.
 * @param defvalue		Optional default value to use if the key is not found.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvGetVector(Handle:kv, const String:key[], Float:vec[3], const Float:defvalue[3]={0.0, 0.0, 0.0});

/**
 * Sets the current position in the KeyValues tree to the given key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key.
 * @param create		If true, and the key does not exist, it will be created.
 * @return				True if the key exists, false if it does not and was not created.
 */
native bool:KvJumpToKey(Handle:kv, const String:key[], bool:create=false);

/**
 * Sets the current position in the KeyValues tree to the given key.
 *
 * @param kv			KeyValues Handle.
 * @param id			KeyValues id.
 * @return				True if the key exists, false if it does not.
 */
native bool:KvJumpToKeySymbol(Handle:kv, id);

/**
 * Sets the current position in the KeyValues tree to the first sub key.
 * This native adds to the internal traversal stack.
 *
 * @param kv			KeyValues Handle.
 * @param keyOnly		If false, non-keys will be traversed (values).
 * @return				True on success, false if there was no first sub key.
 * @error				Invalid Handle.
 */
native bool:KvGotoFirstSubKey(Handle:kv, bool:keyOnly=true);

/**
 * Sets the current position in the KeyValues tree to the next sub key.
 * This native does NOT add to the internal traversal stack, and thus
 * KvGoBack() is not needed for each successive call to this function.
 *
 * @param kv			KeyValues Handle.
 * @param keyOnly		If false, non-keys will be traversed (values).
 * @return				True on success, false if there was no next sub key.
 * @error				Invalid Handle.
 */
native bool:KvGotoNextKey(Handle:kv, bool:keyOnly=true);

/**
 * Saves the current position in the traversal stack onto the traversal
 * stack.  This can be useful if you wish to use KvGotoNextKey() and 
 * have the previous key saved for backwards traversal.
 *
 * @param kv			KeyValues Handle.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvSavePosition(Handle:kv);

/**
 * Removes the given key from the current position.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key.
 * @return				True on success, false if key did not exist.
 * @error				Invalid Handle.
 */
native bool:KvDeleteKey(Handle:kv, const String:key[]);

/**
 * Removes the current sub-key and attempts to set the position
 * to the sub-key after the removed one.  If no such sub-key exists,
 * the position will be the parent key in the traversal stack.
 * Given the sub-key having position "N" in the traversal stack, the 
 * removal will always take place from position "N-1."
 *
 * @param kv			KeyValues Handle.
 * @return				1 if removal succeeded and there was another key.
 *						0 if the current node was not contained in the
 *						  previous node, or no previous node exists.
 *						-1 if removal succeeded and there were no more keys,
 *						  thus the state is as if KvGoBack() was called.
 * @error				Invalid Handle.
 */
native KvDeleteThis(Handle:kv);

/**
 * Jumps back to the previous position.  Returns false if there are no
 * previous positions (i.e., at the root node).  This should be called
 * once for each successful Jump call, in order to return to the top node.
 * This function pops one node off the internal traversal stack.
 *
 * @param kv			KeyValues Handle.
 * @return				True on success, false if there is no higher node.
 * @error				Invalid Handle.
 */
native bool:KvGoBack(Handle:kv);

/**
 * Sets the position back to the top node, emptying the entire node 
 * traversal history.  This can be used instead of looping KvGoBack()
 * if recursive iteration is not important.
 *
 * @param kv			KeyValues Handle.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvRewind(Handle:kv);

/**
 * Retrieves the current section name.
 * 
 * @param kv			KeyValues Handle.
 * @param section		Buffer to store the section name.
 * @param maxlength		Maximum length of the name buffer.
 * @return				True on success, false on failure.
 * @error				Invalid Handle.
 */
native bool:KvGetSectionName(Handle:kv, String:section[], maxlength);

/**
 * Sets the current section name.
 * 
 * @param kv			KeyValues Handle.
 * @param section		Section name.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvSetSectionName(Handle:kv, const String:section[]);

/**
 * Returns the data type at a key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Key name.
 * @return				KvDataType value of the key.
 * @error				Invalid Handle.
 */
native KvDataTypes:KvGetDataType(Handle:kv, const String:key[]);

/**
 * Converts a KeyValues tree to a file.  The tree is dumped
 * from the current position.
 *
 * @param kv			KeyValues Handle.
 * @param file			File to dump write to.
 * @return				True on success, false otherwise.
 * @error				Invalid Handle.
 */
native bool:KeyValuesToFile(Handle:kv, const String:file[]);

/**
 * Converts a file to a KeyValues tree.  The file is read into
 * the current position of the tree.
 *
 * @param kv			KeyValues Handle.
 * @param file			File to read from.
 * @return				True on success, false otherwise.
 * @error				Invalid Handle.
 */
native bool:FileToKeyValues(Handle:kv, const String:file[]);

/**
 * Sets whether or not the KeyValues parser will read escape sequences.
 * For example, \n would be read as a literal newline.  This defaults
 * to false for new KeyValues structures.
 *
 * @param kv			KeyValues Handle.
 * @param useEscapes	Whether or not to read escape sequences.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvSetEscapeSequences(Handle:kv, bool:useEscapes);

/**
 * Returns the position in the jump stack; I.e. the number of calls
 * required for KvGoBack to return to the root node.  If at the root node,
 * 0 is returned.
 *
 * @param kv			KeyValues Handle.
 * @return				Number of non-root nodes in the jump stack.
 * @error				Invalid Handle.
 */
native KvNodesInStack(Handle:kv);

/**
 * Makes a new copy of all subkeys in the origin KeyValues to
 * the destination KeyValues.
 * NOTE: All KeyValues are processed from the current location not the root one.
 *
 * @param origin		Origin KeyValues Handle.
 * @param dest			Destination KeyValues Handlee.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvCopySubkeys(Handle:origin, Handle:dest);

/** 
 * Finds a KeyValues name by id.
 *
 * @param kv			KeyValues Handle.
 * @param id			KeyValues id.
 * @param name			Buffer to store the name.
 * @param maxlength		Maximum length of the value buffer.
 * @return			True on success, false if id not found.
 * @error				Invalid Handle.
 */
native bool:KvFindKeyById(Handle:kv, id, String:name[], maxlength);

/** 
 * Finds a KeyValues id inside a KeyValues tree.
 *
 * @param kv			KeyValues Handle.
 * @param key			Key name.
 * @param id			Id of the found KeyValue.
 * @return			True on success, false if key not found.
 * @error				Invalid Handle.
 */
native bool:KvGetNameSymbol(Handle:kv, const String:key[], &id);

/**
 * Retrieves the current section id.
 * 
 * @param kv			KeyValues Handle.
 * @param id			Id of the current section.
 * @return				True on success, false on failure.
 * @error				Invalid Handle.
 */
native bool:KvGetSectionSymbol(Handle:kv, &id);
