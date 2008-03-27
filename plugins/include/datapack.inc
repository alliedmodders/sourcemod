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

#if defined _datapack_included
 #endinput
#endif
#define _datapack_included

/**
 * Creates a new data pack.
 *
 * @return			A Handle to the data pack.  Must be closed with CloseHandle().
 */
native Handle:CreateDataPack();

/**
 * Packs a normal cell into a data pack.
 *
 * @param pack		Handle to the data pack.
 * @param cell		Cell to add.
 * @noreturn
 * @error			Invalid handle.
 */
native WritePackCell(Handle:pack, cell);

/**
 * Packs a float into a data pack.
 *
 * @param pack		Handle to the data pack.
 * @param val		Float to add.
 * @noreturn
 * @error			Invalid handle.
 */
native WritePackFloat(Handle:pack, Float:val);

/**
 * Packs a string into a data pack.
 *
 * @param pack		Handle to the data pack.
 * @param str		String to add.
 * @noreturn
 * @error			Invalid handle.
 */
native WritePackString(Handle:pack, const String:str[]);

/**
 * Reads a cell from a data pack.
 *
 * @param pack		Handle to the data pack.
 * @return			Cell value.
 * @error			Invalid handle, or bounds error.
 */
native ReadPackCell(Handle:pack);

/**
 * Reads a float from a data pack.
 *
 * @param pack		Handle to the data pack.
 * @return			Float value.
 * @error			Invalid handle, or bounds error.
 */
native Float:ReadPackFloat(Handle:pack);

/**
 * Reads a string from a data pack.
 *
 * @param pack		Handle to the data pack.
 * @param buffer	Destination string buffer.
 * @param maxlen	Maximum length of output string buffer.
 * @noreturn
 * @error			Invalid handle, or bounds error.
 */
native ReadPackString(Handle:pack, String:buffer[], maxlen);

/**
 * Resets the position in a data pack.
 *
 * @param pack		Handle to the data pack.
 * @param clear		If true, clears the contained data.
 * @noreturn
 * @error			Invalid handle.
 */
native ResetPack(Handle:pack, bool:clear=false);

/**
 * Returns the read or write position in a data pack.
 *
 * @param pack		Handle to the data pack.
 * @return			Numerical position in the data pack.
 * @error			Invalid handle.
 */
native GetPackPosition(Handle:pack);

/**
 * Sets the read/write position in a data pack.
 *
 * @param pack		Handle to the data pack.
 * @param position	New position to set.
 * @noreturn
 * @error			Invalid handle, or position is beyond the pack bounds.
 */
native SetPackPosition(Handle:pack, position);

/**
 * Returns whether or not a specified number of bytes from the data pack
 *  position to the end can be read.
 *
 * @param pack		Handle to the data pack.
 * @param bytes		Number of bytes to simulate reading.
 * @return			True if can be read, false otherwise.
 * @error			Invalid handle.
 */
native bool:IsPackReadable(Handle:pack, bytes);
