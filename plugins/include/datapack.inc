/**
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

#if defined _datapack_included
 #endinput
#endif
#define _datapack_included


/**
 * Creates a new data pack.
 *
 * @return		A Handle to the data pack.  Must be closed with CloseHandle().
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
native WritePackString(Handle:pack, String:str[]);

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
 * @error		Invalid handle.
 */
native ResetPack(Handle:pack, bool:clear=false);

/**
 * Returns the read or write position in a data pack.
 *
 * @param pack		Handle to the data pack.
 * @return			Numerical position in the data pack.
 * @error		Invalid handle.
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
 * @return		True if can be read, false otherwise.
 * @error		Invalid handle.
 */
native bool:IsPackReadable(Handle:pack, bytes);
