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

#if defined _sdktools_stringtables_included
  #endinput
#endif
#define _sdktools_stringtables_included

#define INVALID_STRING_TABLE -1		/**< An invalid string table index */
#define INVALID_STRING_INDEX -1		/**< An invalid string index in a table */

/**
 * Searches for a string table.
 *
 * @param name			Name of string table to find.
 * @return				A string table index number if found, INVALID_STRING_TABLE otherwise.
 */
native FindStringTable(const String:name[]);

/**
 * Returns the number of string tables that currently exist.
 *
 * @return				Number of string tables that currently exist.
 */
native GetNumStringTables();

/**
 * Returns the number of strings that currently exist in a given string table.
 *
 * @param tableidx		A string table index.
 * @return				Number of strings that currently exist.
 * @error				Invalid string table index.
 */
native GetStringTableNumStrings(tableidx);

/**
 * Returns the maximum number of strings that are allowed in a given string table.
 *
 * @param tableidx		A string table index.
 * @return				Maximum number of strings allowed.
 * @error				Invalid string table index.
 */
native GetStringTableMaxStrings(tableidx);

/**
 * Retrieves the name of a string table.
 *
 * @param tableidx		A string table index.
 * @param name			Buffer to store the name of the string table.
 * @param maxlength		Maximum length of string buffer.
 * @return				Number of bytes written to the buffer (UTF-8 safe).
 * @error				Invalid string table index.
 */
native GetStringTableName(tableidx, String:name[], maxlength);

/**
 * Searches for the index of a given string in a string table.
 *
 * @param tableidx		A string table index.
 * @param string		String to find.
 * @return				String index if found, INVALID_STRING_INDEX otherwise.
 * @error				Invalid string table index.
 */
native FindStringIndex(tableidx, const String:str[]);

/**
 * Retrieves the string at a given index of a string table.
 *
 * @param tableidx		A string table index.
 * @param stringidx		A string index.
 * @param name			Buffer to store the string value.
 * @param maxlength		Maximum length of string buffer.
 * @return				Number of bytes written to the buffer (UTF-8 safe).
 * @error				Invalid string table index or string index.
 */
native ReadStringTable(tableidx, stringIdx, String:str[], maxlength);

/**
 * Returns the length of the user data associated with a given string index.
 *
 * @param tableidx		A string table index.
 * @param stringidx		A string index.
 * @return				Length of user data. This will be 0 if there is no user data.
 * @error				Invalid string table index or string index.
 */
native GetStringTableDataLength(tableidx, stringidx);

/**
 * Retrieves the user data associated with a given string index.
 *
 * @param tableidx		A string table index.
 * @param stringidx		A string index.
 * @param userdata		Buffer to store the user data. This will be set to "" if there is no user data.
 * @param maxlength		Maximum length of string buffer.
 * @return				Number of bytes written to the buffer (UTF-8 safe).
 * @error				Invalid string table index or string index.
 */
native GetStringTableData(tableidx, stringIdx, String:userdata[], maxlength);

/**
 * Sets the user data associated with a given string index.
 *
 * @param tableidx		A string table index.
 * @param stringidx		A string index.
 * @param userdata		User data string that will be set.
 * @param length		Length of user data string. This should include the null terminator.
 * @return				Number of bytes written to the buffer (UTF-8 safe).
 * @error				Invalid string table index or string index.
 */
native SetStringTableData(tableidx, stringIdx, const String:userdata[], length);

/**
 * Adds a string to a given string table.
 *
 * @param tableidx		A string table index.
 * @param string		String to add.
 * @param userdata		An optional user data string.
 * @param length		Length of user data string. This should include the null terminator.
 *						If set to -1, then user data will be not be altered if the specified string
 *						already exists in the string table.
 */
native AddToStringTable(tableidx, const String:str[], const String:userdata[]="", length=-1);

/**
 * Locks or unlocks the network string tables.
 *
 * @param lock			Determines whether network string tables should be locked.
 *						True means the tables should be locked for writing; false means unlocked.
 * @return				Previous lock state.
 */
native bool:LockStringTables(bool:lock);

/**
 * Adds a file to the downloadables network string table.
 * This forces a client to download the file if they do not already have it.
 *
 * @param filename		File that will be added to downloadables table.
 */
stock AddFileToDownloadsTable(const String:filename[])
{
	static table = INVALID_STRING_TABLE;
	
	if (table == INVALID_STRING_TABLE)
	{
		table = FindStringTable("downloadables");
	}
	
	new bool:save = LockStringTables(false);
	AddToStringTable(table, filename);
	LockStringTables(save);
}
