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

#if defined _adt_array_included
 #endinput
#endif
#define _adt_array_included

/**
 * Given a maximum string size (including the null terminator),
 * returns the number of cells required to fit that string.
 *
 * @param size          Number of bytes.
 * @return              Minimum number of cells required to fit the byte count.
 */
stock int ByteCountToCells(int size)
{
	if (!size)
	{
		return 1;
	}
	
	return (size + 3) / 4;
}

methodmap ArrayList < Handle {
	// Creates a dynamic global cell array.  While slower than a normal array,
	// it can be used globally AND dynamically, which is otherwise impossible.
	//
	// The contents of the array are uniform; i.e. storing a string at index X
	// and then retrieving it as an integer is NOT the same as StringToInt()!
	// The "blocksize" determines how many cells each array slot has; it cannot
	// be changed after creation.
	//
	// @param blocksize     The number of cells each member of the array can
	//                      hold.  For example, 32 cells is equivalent to:
	//                      new Array[X][32]
	// @param startsize     Initial size of the array.  Note that data will
	//                      NOT be auto-initialized.
	// @return              New Handle to the array object.
	public native ArrayList(int blocksize=1, int startsize=0);

	// Clears an array of all entries.  This is the same as Resize(0).
	public native void Clear();

	// Clones an array, returning a new handle with the same size and data.
	// This should NOT be confused with CloneHandle. This is a completely new
	// handle with the same data but no relation to the original. It should
	// closed when no longer needed.
	//
	// @return              New handle to the cloned array object
	public native ArrayList Clone();

	// Resizes an array.  If the size is smaller than the current size, the
	// array is truncated.  If the size is larger than the current size,
	// the data at the additional indexes will not be initialized.
	//
	// @param newsize       New size.
	public native void Resize(int newsize);

	// Pushes a value onto the end of an array, adding a new index.
	//
	// This may safely be used even if the array has a blocksize greater
	// than 1.
	//
	// @param value         Value to push.
	// @return              Index of the new entry.
	// @error               Invalid Handle or out of memory.
	public native int Push(any value);

	// Pushes a string onto the end of an array, truncating it if it is too big.
	//
	// @param value         String to push.
	// @return              Index of the new entry.
	public native int PushString(const char[] value);

	// Pushes an array of cells onto the end of an array.  The cells
	// are pushed as a block (i.e. the entire array sits at the index),
	// rather than pushing each cell individually.
	//
	// @param values        Block of values to copy.
	// @param size          If not set, the number of elements copied from the array
	//                      will be equal to the blocksize.  If set higher than the
	//                      blocksize, the operation will be truncated.
	// @return              Index of the new entry.
	public native int PushArray(const any[] values, int size=-1);

	// Retrieves a cell value from an array.
	//
	// @param index         Index in the array.
	// @param block         Optionally specify which block to read from
	//                      (useful if the blocksize > 0).
	// @param asChar        Optionally read as a byte instead of a cell.
	// @return              Value read.
	// @error               Invalid index.
	public native any Get(int index, int block=0, bool asChar=false);

	// Retrieves a string value from an array.
	//
	// @param index         Index in the array.
	// @param buffer        Buffer to copy to.
	// @param maxlength     Maximum size of the buffer.
	// @return              Number of characters copied.
	// @error               Invalid index.
	public native int GetString(int index, char[] buffer, int maxlength);

	// Retrieves an array of cells from an array.
	//
	// @param index         Index in the array.
	// @param buffer        Buffer to store the array in.
	// @param size          If not set, assumes the buffer size is equal to the
	//                      blocksize.  Otherwise, the size passed is used.
	// @return              Number of cells copied.
	// @error               Invalid index.
	public native int GetArray(int index, any[] buffer, int size=-1);

	// Sets a cell value in an array.
	//
	// @param index         Index in the array.
	// @param value         Cell value to set.
	// @param block         Optionally specify which block to write to
	//                      (useful if the blocksize > 0).
	// @param asChar        Optionally set as a byte instead of a cell.
	// @error               Invalid index, or invalid block.
	public native void Set(int index, any value, int block=0, bool asChar=false);

	// Sets a string value in an array.
	//
	// @param index         Index in the array.
	// @param value         String value to set.
	// @return              Number of characters copied.
	// @error               Invalid index.
	public native void SetString(int index, const char[] value);

	// Sets an array of cells in an array.
	//
	// @param index         Index in the array.
	// @param values        Array to copy.
	// @param size          If not set, assumes the buffer size is equal to the
	//                      blocksize.  Otherwise, the size passed is used.
	// @return              Number of cells copied.
	// @error               Invalid index.
	public native void SetArray(int index, const any[] values, int size=-1);

	// Shifts an array up.  All array contents after and including the given
	// index are shifted up by one, and the given index is then "free."
	// After shifting, the contents of the given index is undefined.
	//
	// @param index         Index in the array to shift up from.
	// @error               Invalid index.
	public native void ShiftUp(int index);

	// Removes an array index, shifting the entire array down from that position
	// on.  For example, if item 8 of 10 is removed, the last 3 items will then be
	// (6,7,8) instead of (7,8,9), and all indexes before 8 will remain unchanged.
	//
	// @param index         Index in the array to remove at.
	// @error               Invalid index.
	public native void Erase(int index);

	// Swaps two items in the array.
	//
	// @param index1        First index.
	// @param index2        Second index.
	// @error               Invalid index.
	public native void SwapAt(int index1, int index2);

	// Returns the index for the first occurrence of the provided string. If
	// the string cannot be located, -1 will be returned.
	//
	// @param item          String to search for
	// @return              Array index, or -1 on failure
	public native int FindString(const char[] item);

	// Returns the index for the first occurrence of the provided value. If the
	// value cannot be located, -1 will be returned.
	//
	// @param item          Value to search for
	// @param block         Optionally which block to search in
	// @return              Array index, or -1 on failure
	// @error               Invalid block index
	public native int FindValue(any item, int block=0);

	// Sort an ADT Array. Specify the type as Integer, Float, or String.
	//
	// @param order         Sort order to use, same as other sorts.
	// @param type          Data type stored in the ADT Array
	public native void Sort(SortOrder order, SortType type);

	// Custom sorts an ADT Array. You must pass in a comparison function.
	//
	// @param sortfunc      Sort comparison function to use
	// @param hndl          Optional Handle to pass through the comparison calls.
	public native void SortCustom(SortFuncADTArray sortfunc, Handle hndl=INVALID_HANDLE); 

	// Retrieve the size of the array.
	property int Length {
		public native get();
	}
	
	// Retrieve the blocksize the array was created with.
	property int BlockSize {
		public native get();
	}
};

/**
 * Creates a dynamic global cell array.  While slower than a normal array,
 * it can be used globally AND dynamically, which is otherwise impossible.
 *
 * The contents of the array are uniform; i.e. storing a string at index X
 * and then retrieving it as an integer is NOT the same as StringToInt()!
 * The "blocksize" determines how many cells each array slot has; it cannot
 * be changed after creation.
 *
 * @param blocksize     The number of cells each member of the array can
 *                      hold.  For example, 32 cells is equivalent to:
 *                      new Array[X][32]
 * @param startsize     Initial size of the array.  Note that data will
 *                      NOT be auto-initialized.
 * @return              New Handle to the array object.
 */
native ArrayList CreateArray(int blocksize=1, int startsize=0);

/**
 * Clears an array of all entries.  This is the same as ResizeArray(0).
 *
 * @param array         Array Handle.
 * @error               Invalid Handle.
 */
native void ClearArray(Handle array);

/**
 * Clones an array, returning a new handle with the same size and data. This should NOT
 * be confused with CloneHandle. This is a completely new handle with the same data but
 * no relation to the original. You MUST close it.
 *
 * @param array         Array handle to be cloned
 * @return              New handle to the cloned array object
 * @error               Invalid Handle
 */
native Handle CloneArray(Handle array);

/**
 * Resizes an array.  If the size is smaller than the current size,
 * the array is truncated.  If the size is larger than the current size,
 * the data at the additional indexes will not be initialized.
 *
 * @param array         Array Handle.
 * @param newsize       New size.
 * @error               Invalid Handle or out of memory.
 */
native void ResizeArray(Handle array, int newsize);

/**
 * Returns the array size.
 *
 * @param array         Array Handle.
 * @return              Number of elements in the array.
 * @error               Invalid Handle.
 */
native int GetArraySize(Handle array);

/**
 * Pushes a value onto the end of an array, adding a new index.
 *
 * This may safely be used even if the array has a blocksize
 * greater than 1.
 *
 * @param array         Array Handle.
 * @param value         Value to push.
 * @return              Index of the new entry.
 * @error               Invalid Handle or out of memory.
 */
native int PushArrayCell(Handle array, any value);

/**
 * Pushes a string onto the end of an array, truncating it
 * if it is too big.
 *
 * @param array         Array Handle.
 * @param value         String to push.
 * @return              Index of the new entry.
 * @error               Invalid Handle or out of memory.
 */
native int PushArrayString(Handle array, const char[] value);

/**
 * Pushes an array of cells onto the end of an array.  The cells
 * are pushed as a block (i.e. the entire array sits at the index),
 * rather than pushing each cell individually.
 *
 * @param array         Array Handle.
 * @param values        Block of values to copy.
 * @param size          If not set, the number of elements copied from the array
 *                      will be equal to the blocksize.  If set higher than the
 *                      blocksize, the operation will be truncated.
 * @return              Index of the new entry.
 * @error               Invalid Handle or out of memory.
 */
native int PushArrayArray(Handle array, const any[] values, int size=-1);

/**
 * Retrieves a cell value from an array.
 *
 * @param array         Array Handle.
 * @param index         Index in the array.
 * @param block         Optionally specify which block to read from
 *                      (useful if the blocksize > 0).
 * @param asChar        Optionally read as a byte instead of a cell.
 * @return              Value read.
 * @error               Invalid Handle, invalid index, or invalid block.
 */
native any GetArrayCell(Handle array, int index, int block=0, bool asChar=false);

/**
 * Retrieves a string value from an array.
 *
 * @param array         Array Handle.
 * @param index         Index in the array.
 * @param buffer        Buffer to copy to.
 * @param maxlength     Maximum size of the buffer.
 * @return              Number of characters copied.
 * @error               Invalid Handle or invalid index.
 */
native int GetArrayString(Handle array, int index, char[] buffer, int maxlength);

/**
 * Retrieves an array of cells from an array.
 *
 * @param array         Array Handle.
 * @param index         Index in the array.
 * @param buffer        Buffer to store the array in.
 * @param size          If not set, assumes the buffer size is equal to the
 *                      blocksize.  Otherwise, the size passed is used.
 * @return              Number of cells copied.
 * @error               Invalid Handle or invalid index.
 */
native int GetArrayArray(Handle array, int index, any[] buffer, int size=-1);

/**
 * Sets a cell value in an array.
 *
 * @param array         Array Handle.
 * @param index         Index in the array.
 * @param value         Cell value to set.
 * @param block         Optionally specify which block to write to
 *                      (useful if the blocksize > 0).
 * @param asChar        Optionally set as a byte instead of a cell.
 * @error               Invalid Handle, invalid index, or invalid block.
 */
native void SetArrayCell(Handle array, int index, any value, int block=0, bool asChar=false);

/**
 * Sets a string value in an array.
 *
 * @param array         Array Handle.
 * @param index         Index in the array.
 * @param value         String value to set.
 * @return              Number of characters copied.
 * @error               Invalid Handle or invalid index.
 */
native int SetArrayString(Handle array, int index, const char[] value);

/**
 * Sets an array of cells in an array.
 *
 * @param array         Array Handle.
 * @param index         Index in the array.
 * @param values        Array to copy.
 * @param size          If not set, assumes the buffer size is equal to the
 *                      blocksize.  Otherwise, the size passed is used.
 * @return              Number of cells copied.
 * @error               Invalid Handle or invalid index.
 */
native int SetArrayArray(Handle array, int index, const any[] values, int size=-1);

/**
 * Shifts an array up.  All array contents after and including the given
 * index are shifted up by one, and the given index is then "free."
 * After shifting, the contents of the given index is undefined.
 *
 * @param array         Array Handle.
 * @param index         Index in the array to shift up from.
 * @error               Invalid Handle or invalid index.
 */
native void ShiftArrayUp(Handle array, int index);

/**
 * Removes an array index, shifting the entire array down from that position
 * on.  For example, if item 8 of 10 is removed, the last 3 items will then be
 * (6,7,8) instead of (7,8,9), and all indexes before 8 will remain unchanged.
 *
 * @param array         Array Handle.
 * @param index         Index in the array to remove at.
 * @error               Invalid Handle or invalid index.
 */
native void RemoveFromArray(Handle array, int index);

/**
 * Swaps two items in the array.
 *
 * @param array         Array Handle.
 * @param index1        First index.
 * @param index2        Second index.
 * @error               Invalid Handle or invalid index.
 */
native void SwapArrayItems(Handle array, int index1, int index2);

/**
 * Returns the index for the first occurrence of the provided string. If the string
 * cannot be located, -1 will be returned.
 *
 * @param array         Array Handle.
 * @param item          String to search for
 * @return              Array index, or -1 on failure
 * @error               Invalid Handle
 */
native int FindStringInArray(Handle array, const char[] item);

/**
 * Returns the index for the first occurrence of the provided value. If the value
 * cannot be located, -1 will be returned.
 *
 * @param array         Array Handle.
 * @param item          Value to search for
 * @param block         Optionally which block to search in
 * @return              Array index, or -1 on failure
 * @error               Invalid Handle or invalid block
 */
native int FindValueInArray(Handle array, any item, int block=0);

/**
 * Returns the blocksize the array was created with.
 *
 * @param array         Array Handle.
 * @return              The blocksize of the array.
 * @error               Invalid Handle
 */
native int GetArrayBlockSize(Handle array);
