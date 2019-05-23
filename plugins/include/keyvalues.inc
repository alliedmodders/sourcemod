/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod (C)2004-2014 AlliedModders LLC.  All rights reserved.
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
	KvData_None = 0,    /**< Type could not be identified, or no type */
	KvData_String,      /**< String value */
	KvData_Int,         /**< Integer value */
	KvData_Float,       /**< Floating point value */
	KvData_Ptr,         /**< Pointer value (sometimes called "long") */
	KvData_WString,     /**< Wide string value */
	KvData_Color,       /**< Color value */
	KvData_UInt64,      /**< Large integer value */
	/* --- */
	KvData_NUMTYPES
};

methodmap KeyValues < Handle
{
	// Creates a new KeyValues structure.  The Handle must be closed with
	// CloseHandle() or delete.
	//
	// @param name          Name of the root section.
	// @param firstKey      If non-empty, specifies the first key value.
	// @param firstValue    If firstKey is non-empty, specifies the first key's value.
	public native KeyValues(const char[] name, const char[] firstKey="", const char[] firstValue="");

	// Exports a KeyValues tree to a file. The tree is dumped from the current position.
	//
	// @param file          File to dump write to.
	// @return              True on success, false otherwise.
	public native bool ExportToFile(const char[] file);

	// Exports a KeyValues tree to a string. The string is dumped from the current position.
	//
	// @param buffer        Buffer to write to.
	// @param maxlength     Max length of buffer.
	// @return              Number of bytes that can be written to buffer.
	public native int ExportToString(char[] buffer, int maxlength);

	// Amount of bytes written by ExportToFile & ExportToString.
	property int ExportLength {
		public native get();
	}

	// Imports a file in KeyValues format. The file is read into the current
	// position of the tree.
	//
	// @param file          File to read from.
	// @return              True on success, false otherwise.
	public native bool ImportFromFile(const char[] file);

	// Converts a given string to a KeyValues tree.  The string is read into
	// the current postion of the tree.
	//
	// @param buffer        String buffer to load into the KeyValues.
	// @param resourceName  The resource name of the KeyValues, used for error tracking purposes.
	// @return              True on success, false otherwise.
	public native bool ImportFromString(const char[] buffer, const char[] resourceName="StringToKeyValues");

	// Imports subkeys in the given KeyValues, at the current position in that
	// KeyValues, into the current position in this KeyValues. Note that this
	// copies keys; it does not embed a reference to them.
	//
	// @param other         Origin KeyValues Handle.
	public native void Import(KeyValues other);

	// Sets a string value of a KeyValues key.
	//
	// @param kv            KeyValues Handle.
	// @param key           Name of the key, or NULL_STRING.
	// @param value         String value.
	public native void SetString(const char[] key, const char[] value);

	// Sets an integer value of a KeyValues key.
	//
	// @param key           Name of the key, or NULL_STRING.
	// @param value         Value number.
	public native void SetNum(const char[] key, int value);

	// Sets a large integer value of a KeyValues key.
	//
	// @param key           Name of the key, or NULL_STRING.
	// @param value         Large integer value (0=High bits, 1=Low bits)
	public native void SetUInt64(const char[] key, const int value[2]);

	// Sets a floating point value of a KeyValues key.
	//
	// @param key           Name of the key, or NULL_STRING.
	// @param value         Floating point value.
	public native void SetFloat(const char[] key, float value);

	// Sets a set of color values of a KeyValues key.
	//
	// @param key           Name of the key, or NULL_STRING.
	// @param r             Red value.
	// @param g             Green value.
	// @param b             Blue value.
	// @param a             Alpha value.
	public native void SetColor(const char[] key, int r, int g, int b, int a=0);

	// Sets a set of color values of a KeyValues key.
	//
	// @param key           Name of the key, or NULL_STRING.
	// @param color         Red, green, blue and alpha channels.
	public void SetColor4(const char[] key, const int color[4]) {
		this.SetColor(key, color[0], color[1], color[2], color[3]);
	}

	// Sets a vector value of a KeyValues key.
	//
	// @param key           Name of the key, or NULL_STRING.
	// @param vec           Vector value.
	public native void SetVector(const char[] key, const float vec[3]);

	// Retrieves a string value from a KeyValues key.
	//
	// @param key           Name of the key, or NULL_STRING.
	// @param value         Buffer to store key value in.
	// @param maxlength     Maximum length of the value buffer.
	// @param defvalue      Optional default value to use if the key is not found.
	public native void GetString(const char[] key, char[] value, int maxlength, const char[] defvalue="");

	// Retrieves an integer value from a KeyValues key.
	//
	// @param key           Name of the key, or NULL_STRING.
	// @param defvalue      Optional default value to use if the key is not found.
	// @return              Integer value of the key.
	public native int GetNum(const char[] key, int defvalue=0);

	// Retrieves a floating point value from a KeyValues key.
	//
	// @param key           Name of the key, or NULL_STRING.
	// @param defvalue      Optional default value to use if the key is not found.
	// @return              Floating point value of the key.
	public native float GetFloat(const char[] key, float defvalue=0.0);

	// Retrieves a set of color values from a KeyValues key.
	//
	// @param key           Name of the key, or NULL_STRING.
	// @param r             Red value, set by reference.
	// @param g             Green value, set by reference.
	// @param b             Blue value, set by reference.
	// @param a             Alpha value, set by reference.
	public native void GetColor(const char[] key, int &r, int &g, int &b, int &a);

	// Retrieves a set of color values from a KeyValues key.
	//
	// @param key           Name of the key, or NULL_STRING.
	// @param color         Red, green, blue, and alpha channels.
	public void GetColor4(const char[] key, int color[4]) {
		int r, g, b, a;
		this.GetColor(key, r, g, b, a);
		color[0] = r;
		color[1] = g;
		color[2] = b;
		color[3] = a;
	}

	// Retrieves a large integer value from a KeyValues key.
	//
	// @param key           Name of the key, or NULL_STRING.
	// @param value         Array to represent the large integer.
	// @param defvalue      Optional default value to use if the key is not found.
	public native void GetUInt64(const char[] key, int value[2], int defvalue[2]={0,0});

	// Retrieves a vector value from a KeyValues key.
	//
	// @param key           Name of the key, or NULL_STRING.
	// @param vec           Destination vector to store the value in.
	// @param defvalue      Optional default value to use if the key is not found.
	public native void GetVector(const char[] key, float vec[3], const float defvalue[3]={0.0, 0.0, 0.0});

	// Sets the current position in the KeyValues tree to the given key.
	//
	// @param key           Name of the key.
	// @param create        If true, and the key does not exist, it will be created.
	// @return              True if the key exists, false if it does not and was not created.
	public native bool JumpToKey(const char[] key, bool create=false);

	// Sets the current position in the KeyValues tree to the given key.
	//
	// @param id            KeyValues id.
	// @return              True if the key exists, false if it does not.
	public native bool JumpToKeySymbol(int id);

	// Sets the current position in the KeyValues tree to the first sub key.
	// This native adds to the internal traversal stack.
	//
	// @param keyOnly       If false, non-keys will be traversed (values).
	// @return              True on success, false if there was no first sub key.
	public native bool GotoFirstSubKey(bool keyOnly=true);

	// Sets the current position in the KeyValues tree to the next sub key.
	// This native does NOT add to the internal traversal stack, and thus
	// GoBack() is not needed for each successive call to this function.
	//
	// @param keyOnly       If false, non-keys will be traversed (values).
	// @return              True on success, false if there was no next sub key.
	public native bool GotoNextKey(bool keyOnly=true);

	// Saves the current position in the traversal stack onto the traversal
	// stack.  This can be useful if you wish to use KvGotoNextKey() and
	// have the previous key saved for backwards traversal.
	//
	// @param kv            KeyValues Handle.
	public native void SavePosition();

	// Jumps back to the previous position.  Returns false if there are no
	// previous positions (i.e., at the root node).  This should be called
	// once for each successful Jump call, in order to return to the top node.
	// This function pops one node off the internal traversal stack.
	//
	// @return              True on success, false if there is no higher node.
	public native bool GoBack();

	// Removes the given key from the current position.
	//
	// @param key           Name of the key.
	// @return              True on success, false if key did not exist.
	public native bool DeleteKey(const char[] key);

	// Removes the current sub-key and attempts to set the position
	// to the sub-key after the removed one.  If no such sub-key exists,
	// the position will be the parent key in the traversal stack.
	// Given the sub-key having position "N" in the traversal stack, the
	// removal will always take place from position "N-1."
	//
	// @param kv            KeyValues Handle.
	// @return              1 if removal succeeded and there was another key.
	//                      0 if the current node was not contained in the
	//                        previous node, or no previous node exists.
	//                      -1 if removal succeeded and there were no more keys,
	//                        thus the state is as if KvGoBack() was called.
	public native int DeleteThis();

	// Sets the position back to the top node, emptying the entire node
	// traversal history.  This can be used instead of looping KvGoBack()
	// if recursive iteration is not important.
	//
	// @param kv            KeyValues Handle.
	public native void Rewind();

	// Retrieves the current section name.
	//
	// @param section       Buffer to store the section name.
	// @param maxlength     Maximum length of the name buffer.
	// @return              True on success, false on failure.
	public native bool GetSectionName(char[] section, int maxlength);

	// Sets the current section name.
	//
	// @param section       Section name.
	public native void SetSectionName(const char[] section);

	// Returns the data type at a key.
	//
	// @param key           Key name.
	// @return              KvDataType value of the key.
	public native KvDataTypes GetDataType(const char[] key);

	// Sets whether or not the KeyValues parser will read escape sequences.
	// For example, \n would be read as a literal newline.  This defaults
	// to false for new KeyValues structures.
	//
	// @param useEscapes    Whether or not to read escape sequences.
	public native void SetEscapeSequences(bool useEscapes);

	// Returns the position in the jump stack; I.e. the number of calls
	// required for KvGoBack to return to the root node.  If at the root node,
	// 0 is returned.
	//
	// @return              Number of non-root nodes in the jump stack.
	public native int NodesInStack();

	// Finds a KeyValues name by id.
	//
	// @param id            KeyValues id.
	// @param name          Buffer to store the name.
	// @param maxlength     Maximum length of the value buffer.
	// @return              True on success, false if id not found.
	public native bool FindKeyById(int id, char[] name, int maxlength);

	// Finds a KeyValues id inside a KeyValues tree.
	//
	// @param key           Key name.
	// @param id            Id of the found KeyValue.
	// @return              True on success, false if key not found.
	public native bool GetNameSymbol(const char[] key, int &id);

	// Retrieves the current section id.
	//
	// @param kv            KeyValues Handle.
	// @param id            Id of the current section.
	// @return              True on success, false on failure.
	public native bool GetSectionSymbol(int &id);
};

/**
 * Creates a new KeyValues structure.  The Handle must always be closed.
 *
 * @param name          Name of the root section.
 * @param firstKey      If non-empty, specifies the first key value.
 * @param firstValue    If firstKey is non-empty, specifies the first key's value.
 * @return              A Handle to a new KeyValues structure.
 */
native KeyValues CreateKeyValues(const char[] name, const char[] firstKey="", const char[] firstValue="");

/**
 * Sets a string value of a KeyValues key.
 *
 * @param kv            KeyValues Handle.
 * @param key           Name of the key, or NULL_STRING.
 * @param value         String value.
 * @error               Invalid Handle.
 */
native void KvSetString(Handle kv, const char[] key, const char[] value);

/**
 * Sets an integer value of a KeyValues key.
 *
 * @param kv            KeyValues Handle.
 * @param key           Name of the key, or NULL_STRING.
 * @param value         Value number.
 * @error               Invalid Handle.
 */
native void KvSetNum(Handle kv, const char[] key, int value);

/**
 * Sets a large integer value of a KeyValues key.
 *
 * @param kv            KeyValues Handle.
 * @param key           Name of the key, or NULL_STRING.
 * @param value         Large integer value (0=High bits, 1=Low bits)
 * @error               Invalid Handle.
 */
native void KvSetUInt64(Handle kv, const char[] key, const int value[2]);

/**
 * Sets a floating point value of a KeyValues key.
 *
 * @param kv            KeyValues Handle.
 * @param key           Name of the key, or NULL_STRING.
 * @param value         Floating point value.
 * @error               Invalid Handle.
 */
native void KvSetFloat(Handle kv, const char[] key, float value);

/**
 * Sets a set of color values of a KeyValues key.
 *
 * @param kv            KeyValues Handle.
 * @param key           Name of the key, or NULL_STRING.
 * @param r             Red value.
 * @param g             Green value.
 * @param b             Blue value.
 * @param a             Alpha value.
 * @error               Invalid Handle.
 */
native void KvSetColor(Handle kv, const char[] key, int r, int g, int b, int a=0);

/**
 * Sets a vector value of a KeyValues key.
 *
 * @param kv            KeyValues Handle.
 * @param key           Name of the key, or NULL_STRING.
 * @param vec           Vector value.
 * @error               Invalid Handle.
 */
native void KvSetVector(Handle kv, const char[] key, const float vec[3]);

/**
 * Retrieves a string value from a KeyValues key.
 *
 * @param kv            KeyValues Handle.
 * @param key           Name of the key, or NULL_STRING.
 * @param value         Buffer to store key value in.
 * @param maxlength     Maximum length of the value buffer.
 * @param defvalue      Optional default value to use if the key is not found.
 * @error               Invalid Handle.
 */
native void KvGetString(Handle kv, const char[] key, char[] value, int maxlength, const char[] defvalue="");

/**
 * Retrieves an integer value from a KeyValues key.
 *
 * @param kv            KeyValues Handle.
 * @param key           Name of the key, or NULL_STRING.
 * @param defvalue      Optional default value to use if the key is not found.
 * @return              Integer value of the key.
 * @error               Invalid Handle.
 */
native int KvGetNum(Handle kv, const char[] key, int defvalue=0);

/**
 * Retrieves a floating point value from a KeyValues key.
 *
 * @param kv            KeyValues Handle.
 * @param key           Name of the key, or NULL_STRING.
 * @param defvalue      Optional default value to use if the key is not found.
 * @return              Floating point value of the key.
 * @error               Invalid Handle.
 */
native float KvGetFloat(Handle kv, const char[] key, float defvalue=0.0);

/**
 * Retrieves a set of color values from a KeyValues key.
 *
 * @param kv            KeyValues Handle.
 * @param key           Name of the key, or NULL_STRING.
 * @param r             Red value, set by reference.
 * @param g             Green value, set by reference.
 * @param b             Blue value, set by reference.
 * @param a             Alpha value, set by reference.
 * @error               Invalid Handle.
 */
native void KvGetColor(Handle kv, const char[] key, int &r, int &g, int &b, int &a);

/**
 * Retrieves a large integer value from a KeyValues key.
 *
 * @param kv            KeyValues Handle.
 * @param key           Name of the key, or NULL_STRING.
 * @param value         Array to represent the large integer.
 * @param defvalue      Optional default value to use if the key is not found.
 * @error               Invalid Handle.
 */
native void KvGetUInt64(Handle kv, const char[] key, int value[2], int defvalue[2]={0,0});

/**
 * Retrieves a vector value from a KeyValues key.
 *
 * @param kv            KeyValues Handle.
 * @param key           Name of the key, or NULL_STRING.
 * @param vec           Destination vector to store the value in.
 * @param defvalue      Optional default value to use if the key is not found.
 * @error               Invalid Handle.
 */
native void KvGetVector(Handle kv, const char[] key, float vec[3], const float defvalue[3]={0.0, 0.0, 0.0});

/**
 * Sets the current position in the KeyValues tree to the given key.
 *
 * @param kv            KeyValues Handle.
 * @param key           Name of the key.
 * @param create        If true, and the key does not exist, it will be created.
 * @return              True if the key exists, false if it does not and was not created.
 */
native bool KvJumpToKey(Handle kv, const char[] key, bool create=false);

/**
 * Sets the current position in the KeyValues tree to the given key.
 *
 * @param kv            KeyValues Handle.
 * @param id            KeyValues id.
 * @return              True if the key exists, false if it does not.
 */
native bool KvJumpToKeySymbol(Handle kv, int id);

/**
 * Sets the current position in the KeyValues tree to the first sub key.
 * This native adds to the internal traversal stack.
 *
 * @param kv            KeyValues Handle.
 * @param keyOnly       If false, non-keys will be traversed (values).
 * @return              True on success, false if there was no first sub key.
 * @error               Invalid Handle.
 */
native bool KvGotoFirstSubKey(Handle kv, bool keyOnly=true);

/**
 * Sets the current position in the KeyValues tree to the next sub key.
 * This native does NOT add to the internal traversal stack, and thus
 * KvGoBack() is not needed for each successive call to this function.
 *
 * @param kv            KeyValues Handle.
 * @param keyOnly       If false, non-keys will be traversed (values).
 * @return              True on success, false if there was no next sub key.
 * @error               Invalid Handle.
 */
native bool KvGotoNextKey(Handle kv, bool keyOnly=true);

/**
 * Saves the current position in the traversal stack onto the traversal
 * stack.  This can be useful if you wish to use KvGotoNextKey() and
 * have the previous key saved for backwards traversal.
 *
 * @param kv            KeyValues Handle.
 * @error               Invalid Handle.
 */
native void KvSavePosition(Handle kv);

/**
 * Removes the given key from the current position.
 *
 * @param kv            KeyValues Handle.
 * @param key           Name of the key.
 * @return              True on success, false if key did not exist.
 * @error               Invalid Handle.
 */
native bool KvDeleteKey(Handle kv, const char[] key);

/**
 * Removes the current sub-key and attempts to set the position
 * to the sub-key after the removed one.  If no such sub-key exists,
 * the position will be the parent key in the traversal stack.
 * Given the sub-key having position "N" in the traversal stack, the
 * removal will always take place from position "N-1."
 *
 * @param kv            KeyValues Handle.
 * @return              1 if removal succeeded and there was another key.
 *                      0 if the current node was not contained in the
 *                        previous node, or no previous node exists.
 *                     -1 if removal succeeded and there were no more keys,
 *                        thus the state is as if KvGoBack() was called.
 * @error               Invalid Handle.
 */
native int KvDeleteThis(Handle kv);

/**
 * Jumps back to the previous position.  Returns false if there are no
 * previous positions (i.e., at the root node).  This should be called
 * once for each successful Jump call, in order to return to the top node.
 * This function pops one node off the internal traversal stack.
 *
 * @param kv            KeyValues Handle.
 * @return              True on success, false if there is no higher node.
 * @error               Invalid Handle.
 */
native bool KvGoBack(Handle kv);

/**
 * Sets the position back to the top node, emptying the entire node
 * traversal history.  This can be used instead of looping KvGoBack()
 * if recursive iteration is not important.
 *
 * @param kv            KeyValues Handle.
 * @error               Invalid Handle.
 */
native void KvRewind(Handle kv);

/**
 * Retrieves the current section name.
 *
 * @param kv            KeyValues Handle.
 * @param section       Buffer to store the section name.
 * @param maxlength     Maximum length of the name buffer.
 * @return              True on success, false on failure.
 * @error               Invalid Handle.
 */
native bool KvGetSectionName(Handle kv, char[] section, int maxlength);

/**
 * Sets the current section name.
 *
 * @param kv            KeyValues Handle.
 * @param section       Section name.
 * @error               Invalid Handle.
 */
native void KvSetSectionName(Handle kv, const char[] section);

/**
 * Returns the data type at a key.
 *
 * @param kv            KeyValues Handle.
 * @param key           Key name.
 * @return              KvDataType value of the key.
 * @error               Invalid Handle.
 */
native KvDataTypes KvGetDataType(Handle kv, const char[] key);

/**
 * Converts a KeyValues tree to a file.  The tree is dumped
 * from the current position.
 *
 * @param kv            KeyValues Handle.
 * @param file          File to dump write to.
 * @return              True on success, false otherwise.
 * @error               Invalid Handle.
 */
native bool KeyValuesToFile(Handle kv, const char[] file);

/**
 * Converts a file to a KeyValues tree.  The file is read into
 * the current position of the tree.
 *
 * @param kv            KeyValues Handle.
 * @param file          File to read from.
 * @return              True on success, false otherwise.
 * @error               Invalid Handle.
 */
native bool FileToKeyValues(Handle kv, const char[] file);

/**
 * Converts a given string to a KeyValues tree.  The string is read into
 * the current postion of the tree.
 *
 * @param kv            KeyValues Handle.
 * @param buffer        String buffer to load into the KeyValues.
 * @param resourceName  The resource name of the KeyValues, used for error tracking purposes.
 * @return              True on success, false otherwise.
 * @error               Invalid Handle.
 */
native bool StringToKeyValues(Handle kv, const char[] buffer, const char[] resourceName="StringToKeyValues");

/**
 * Sets whether or not the KeyValues parser will read escape sequences.
 * For example, \n would be read as a literal newline.  This defaults
 * to false for new KeyValues structures.
 *
 * @param kv            KeyValues Handle.
 * @param useEscapes    Whether or not to read escape sequences.
 * @error               Invalid Handle.
 */
native void KvSetEscapeSequences(Handle kv, bool useEscapes);

/**
 * Returns the position in the jump stack; I.e. the number of calls
 * required for KvGoBack to return to the root node.  If at the root node,
 * 0 is returned.
 *
 * @param kv            KeyValues Handle.
 * @return              Number of non-root nodes in the jump stack.
 * @error               Invalid Handle.
 */
native int KvNodesInStack(Handle kv);

/**
 * Makes a new copy of all subkeys in the origin KeyValues to
 * the destination KeyValues.
 * NOTE: All KeyValues are processed from the current location not the root one.
 *
 * @param origin        Origin KeyValues Handle.
 * @param dest          Destination KeyValues Handle.
 * @error               Invalid Handle.
 */
native void KvCopySubkeys(Handle origin, Handle dest);

/**
 * Finds a KeyValues name by id.
 *
 * @param kv            KeyValues Handle.
 * @param id            KeyValues id.
 * @param name          Buffer to store the name.
 * @param maxlength     Maximum length of the value buffer.
 * @return              True on success, false if id not found.
 * @error               Invalid Handle.
 */
native bool KvFindKeyById(Handle kv, int id, char[] name, int maxlength);

/**
 * Finds a KeyValues id inside a KeyValues tree.
 *
 * @param kv            KeyValues Handle.
 * @param key           Key name.
 * @param id            Id of the found KeyValue.
 * @return              True on success, false if key not found.
 * @error               Invalid Handle.
 */
native bool KvGetNameSymbol(Handle kv, const char[] key, int &id);

/**
 * Retrieves the current section id.
 *
 * @param kv            KeyValues Handle.
 * @param id            Id of the current section.
 * @return              True on success, false on failure.
 * @error               Invalid Handle.
 */
native bool KvGetSectionSymbol(Handle kv, int &id);
