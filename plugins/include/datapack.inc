/**
 * vim: set ts=4 sw=4 tw=99 noet :
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
 * Opaque handle to a datapack position.
 */
enum DataPackPos: {};

// A DataPack allows serializing multiple variables into a single stream.
methodmap DataPack < Handle
{
	// Creates a new data pack.
	public native DataPack();

	// Packs a normal cell into a data pack.
	//
	// @param cell          Cell to add.
	// @param insert        Determines whether mid-pack writes will insert instead of overwrite.
	public native void WriteCell(any cell, bool insert = false);

	// Packs a float into a data pack.
	//
	// @param val           Float to add.
	// @param insert        Determines whether mid-pack writes will insert instead of overwrite.
	public native void WriteFloat(float val, bool insert = false);

	// Packs a string into a data pack.
	//
	// @param str           String to add.
	// @param insert        Determines whether mid-pack writes will insert instead of overwrite.
	public native void WriteString(const char[] str, bool insert = false);

	// Packs a function pointer into a data pack.
	//
	// @param fktptr        Function pointer to add.
	// @param insert        Determines whether mid-pack writes will insert instead of overwrite.
	public native void WriteFunction(Function fktptr, bool insert = false);

	// Packs an array of cells into a data pack.
	//
	// @param array         Array to add.
	// @param count         Number of elements
	// @param insert        Determines whether mid-pack writes will insert instead of overwrite.
	public native void WriteCellArray(const any[] array, int count, bool insert = false);

	// Packs an array of floats into a data pack.
	//
	// @param array         Array to add.
	// @param count         Number of elements
	// @param insert        Determines whether mid-pack writes will insert instead of overwrite.
	public native void WriteFloatArray(const float[] array, int count, bool insert = false);

	// Reads a cell from a data pack.
	//
	// @return		A cell at this position
	public native any ReadCell();

	// Reads a float from a data pack.
	//
	// @return		Float at this position
	public native float ReadFloat();

	// Reads a string from a data pack.
	//
	// @param buffer        Destination string buffer.
	// @param maxlen        Maximum length of output string buffer.
	public native void ReadString(char[] buffer, int maxlen);

	// Reads a function pointer from a data pack.
	//
	// @return              Function pointer.
	public native Function ReadFunction();

	// Reads an array of cells a data pack.
	//
	// @param buffer        Destination buffer.
	// @param count         Maximum length of output buffer.
	public native void ReadCellArray(any[] buffer, int count);

	// Reads an array of floats from a data pack.
	//
	// @param buffer        Destination buffer.
	// @param count         Maximum length of output buffer.
	public native void ReadFloatArray(float[] buffer, int count);

	// Resets the position in a data pack.
	//
	// @param clear         If true, clears the contained data.
	public native void Reset(bool clear=false);

	// Returns whether or not a specified number of bytes from the data pack
	//  position to the end can be read.
	//
	// @param unused        Unused variable. Exists for backwards compatability.
	public native bool IsReadable(int unused = 0);
	
	// The read or write position in a data pack.
	property DataPackPos Position {
		public native get();
		public native set(DataPackPos pos);
	}
};

/**
 * Creates a new data pack.
 *
 * @return              A Handle to the data pack.  Must be closed with CloseHandle().
 */
native DataPack CreateDataPack();

/**
 * Packs a normal cell into a data pack.
 *
 * @param pack          Handle to the data pack.
 * @param cell          Cell to add.
 * @error               Invalid handle.
 */
native void WritePackCell(Handle pack, any cell);

/**
 * Packs a float into a data pack.
 *
 * @param pack          Handle to the data pack.
 * @param val           Float to add.
 * @error               Invalid handle.
 */
native void WritePackFloat(Handle pack, float val);

/**
 * Packs a string into a data pack.
 *
 * @param pack          Handle to the data pack.
 * @param str           String to add.
 * @error               Invalid handle.
 */
native void WritePackString(Handle pack, const char[] str);

/**
 * Packs a function pointer into a data pack.
 *
 * @param pack          Handle to the data pack.
 * @param fktptr        Function pointer to add.
 * @error               Invalid handle.
 */
native void WritePackFunction(Handle pack, Function fktptr);

/**
 * Reads a cell from a data pack.
 *
 * @param pack          Handle to the data pack.
 * @return              Cell value.
 * @error               Invalid handle, or bounds error.
 */
native any ReadPackCell(Handle pack);

/**
 * Reads a float from a data pack.
 *
 * @param pack          Handle to the data pack.
 * @return              Float value.
 * @error               Invalid handle, or bounds error.
 */
native float ReadPackFloat(Handle pack);

/**
 * Reads a string from a data pack.
 *
 * @param pack          Handle to the data pack.
 * @param buffer        Destination string buffer.
 * @param maxlen        Maximum length of output string buffer.
 * @error               Invalid handle, or bounds error.
 */
native void ReadPackString(Handle pack, char[] buffer, int maxlen);

/**
 * Reads a function pointer from a data pack.
 *
 * @param pack          Handle to the data pack.
 * @return              Function pointer.
 * @error               Invalid handle, or bounds error.
 */
native Function ReadPackFunction(Handle pack);

/**
 * Resets the position in a data pack.
 *
 * @param pack          Handle to the data pack.
 * @param clear         If true, clears the contained data.
 * @error               Invalid handle.
 */
native void ResetPack(Handle pack, bool clear=false);

/**
 * Returns the read or write position in a data pack.
 *
 * @param pack          Handle to the data pack.
 * @return              Position in the data pack, only usable with calls to SetPackPosition.
 * @error               Invalid handle.
 */
native DataPackPos GetPackPosition(Handle pack);

/**
 * Sets the read/write position in a data pack.
 *
 * @param pack          Handle to the data pack.
 * @param position      New position to set. Must have been previously retrieved from a call to GetPackPosition.
 * @error               Invalid handle, or position is beyond the pack bounds.
 */
native void SetPackPosition(Handle pack, DataPackPos position);

/**
 * Returns whether or not a specified number of bytes from the data pack
 * position to the end can be read.
 *
 * @param pack          Handle to the data pack.
 * @param bytes         Number of bytes to simulate reading.
 * @return              True if can be read, false otherwise.
 * @error               Invalid handle.
 */
native bool IsPackReadable(Handle pack, int bytes);
