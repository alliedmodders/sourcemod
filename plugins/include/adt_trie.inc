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
 
#if defined _adt_trie_included
 #endinput
#endif
#define _adt_trie_included

/**
 * Creates a Trie structure.  A trie is a data storage object that maps any value to a 
 * string of text.  It features very fast lookup and deletion, but grows very slow for 
 * insertion once tens of thousands of items are added.
 *
 * Keys in Tries are unique.  That is, each key may only have one value.  Unlike arrays, 
 * Tries cannot be iterated right now.  Since the contents are known to be unique, to 
 * work around this, you can use ADT Arrays to store a list of keys known to be in a 
 * Trie.
 *
 * @return 			New Trie Handle, which must be freed via CloseHandle().
 */
native Handle:CreateTrie();

/**
 * Sets a value in a Trie, either inserting a new entry or replacing an old one.
 *
 * @param trie		Trie Handle.
 * @param key		Key string.
 * @param value		Value to store at this key.
 * @param replace	If false, operation will fail if the key is already set.
 * @return			True on success, false on failure.
 * @error			Invalid Handle.
 */
native bool:SetTrieValue(Handle:trie, const String:key[], any:value, bool:replace=true);

/**
 * Sets an array value in a Trie, either inserting a new entry or replacing an old one.
 *
 * @param trie		Trie Handle.
 * @param key		Key string.
 * @param array		Array to store.
 * @param num_items	Number of items in the array.
 * @param replace	If false, operation will fail if the key is already set.
 * @return			True on success, false on failure.
 * @error			Invalid Handle.
 */
native bool:SetTrieArray(Handle:trie, const String:key[], const any:array[], num_items, bool:replace=true);

/**
 * Sets a string value in a Trie, either inserting a new entry or replacing an old one.
 *
 * @param trie		Trie Handle.
 * @param key		Key string.
 * @param array		Array to store.
 * @param num_items	Number of items in the array.
 * @param replace	If false, operation will fail if the key is already set.
 * @return			True on success, false on failure.
 * @error			Invalid Handle.
 */
native bool:SetTrieString(Handle:trie, const String:key[], const String:value[], bool:replace=true);

/**
 * Retrieves a value in a Trie.
 *
 * @param trie		Trie Handle.
 * @param key		Key string.
 * @param val		Variable to store value.
 * @return			True on success.  False if the key is not set, or the key is set 
 *					as an array or string (not a value).
 * @error			Invalid Handle.
 */
native bool:GetTrieValue(Handle:trie, const String:key[], &any:value);

/**
 * Retrieves an array in a Trie.
 *
 * @param trie		Trie Handle.
 * @param key		Key string.
 * @param array		Buffer to store array.
 * @param max_size	Maximum size of array buffer.
 * @param size		Optional parameter to store the number of elements written to the buffer.
 * @return			True on success.  False if the key is not set, or the key is set 
 *					as a value or string (not an array).
 * @error			Invalid Handle.
 */
native bool:GetTrieArray(Handle:trie, const String:key[], any:array[], max_size, &size=0);

/**
 * Retrieves a string in a Trie.
 *
 * @param trie		Trie Handle.
 * @param key		Key string.
 * @param value		Buffer to store value.
 * @param max_size	Maximum size of string buffer.
 * @param size		Optional parameter to store the number of bytes written to the buffer.
 * @return			True on success.  False if the key is not set, or the key is set 
 *					as a value or array (not a string).
 * @error			Invalid Handle.
 */
native bool:GetTrieString(Handle:trie, const String:key[], String:value[], max_size, &size=0);

/**
 * Removes a key entry from a Trie.
 *
 * @param trie		Trie Handle.
 * @param key		Key string.
 * @return			True on success, false if the value was never set.
 * @error			Invalid Handle.
 */
native RemoveFromTrie(Handle:trie, const String:key[]);

/**
 * Clears all entries from a Trie.
 *
 * @param trie		Trie Handle.
 * @error			Invalid Handle.
 */
native ClearTrie(Handle:trie);

/**
 * Retrieves the number of elements in a trie.
 *
 * Note that trie items are not enumerable/iteratable.  If you need to 
 * retrieve the elements in a trie, store its keys in an ADT Array. 
 *
 * @param trie		Trie Handle.
 * @return			Number of elements in the trie.
 * @error			Invalid Handle.
 */
native GetTrieSize(Handle:trie);
