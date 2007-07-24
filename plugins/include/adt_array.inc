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
 
#if defined _adt_array_included
 #endinput
#endif
#define _adt_array_included

/**
 * Given a maximum string size (including the null terminator), 
 * returns the number of cells required to fit that string.
 *
 * @param size			Number of bytes.
 * @return				Minimum number of cells required to fit the byte count.
 */
stock ByteCountToCells(size)
{
	if (!size)
	{
		return 1;
	}
	
	return (size + 3) / 4;
}

/**
 * Creates a dynamic global cell array.  While slower than a normal array,
 * it can be used globally AND dynamically, which is otherwise impossible.
 * 
 * The contents of the array are uniform; i.e. storing a string at index X 
 * and then retrieving it as an integer is NOT the same as StringToInt()!
 * The "blocksize" determines how many cells each array slot has; it cannot
 * be changed after creation.
 *
 * @param blocksize		The number of cells each member of the array can 
 *						hold.  For example, 32 cells is equivalent to:
 *						new Array[X][32]
 * @param startsize		Initial size of the array.  Note that data will 
 *						NOT be auto-intialized.
 * @return				New Handle to the array object.
 */
native Handle:CreateArray(blocksize=1, startsize=0);

/**
 * Clears an array of all entries.  This is the same as ResizeArray(0).
 *
 * @param array			Array Handle.
 * @noreturn
 * @error				Invalid Handle.
 */
native ClearArray(Handle:array);

/**
 * Resizes an array.  If the size is smaller than the current size,
 * the array is truncated.
 *
 * @param array			Array Handle.
 * @param newsize		New size.
 * @noreturn
 * @error				Invalid Handle or out of memory.
 */
native bool:ResizeArray(Handle:array, newsize);

/**
 * Returns the array size.
 *
 * @param array			Array Handle.
 * @return				Number of elements in the array.
 * @error				Invalid Handle.
 */
native GetArraySize(Handle:array);

/**
 * Pushes a value onto the end of an array, adding a new index.
 *
 * This may safely be used even if the array has a blocksize
 * greater than 1.
 *
 * @param array			Array Handle.
 * @param value			Value to push.
 * @return				Index of the new entry.
 * @error				Invalid Handle or out of memory.
 */
native PushArrayCell(Handle:array, any:value);

/**
 * Pushes a string onto the end of an array, truncating it
 * if it is too big.
 *
 * @param array			Array Handle.
 * @param value			String to push.
 * @return				Index of the new entry.
 * @error				Invalid Handle or out of memory.
 */
native PushArrayString(Handle:array, const String:value[]);

/**
 * Pushes an array of cells onto the end of an array.  The cells
 * are pushed as a block (i.e. the entire array sits at the index),
 * rather than pushing each cell individually.
 *
 * @param array			Array Handle.
 * @param values		Block of values to copy.
 * @param size			If not set, the number of elements copied from the array
 *						will be equal to the blocksize.  If set higher than the 
 *						blocksize, the operation will be truncated.
 * @return				Index of the new entry.
 * @error				Invalid Handle or out of memory.
 */
native PushArrayArray(Handle:array, const values[], size=-1);

/**
 * Retrieves a cell value from an array.
 *
 * @param array			Array Handle.
 * @param index			Index in the array.
 * @param block			Optionally specify which block to read from
 *						(useful if the blocksize > 0).
 * @param asChar		Optionally read as a byte instead of a cell.
 * @return				Value read.
 * @error				Invalid Handle, invalid index, or invalid block.
 */
native any:GetArrayCell(Handle:array, index, block=0, bool:asChar=false);

/**
 * Retrieves a string value from an array.
 *
 * @param array			Array Handle.
 * @param index			Index in the array.
 * @param buffer		Buffer to copy to.
 * @param maxlength		Maximum size of the buffer.
 * @return				Number of characters copied.
 * @error				Invalid Handle or invalid index.
 */
native GetArrayString(Handle:array, index, String:buffer[], maxlength);

/**
 * Retrieves an array of cells from an array.
 *
 * @param array			Array Handle.
 * @param index			Index in the array.
 * @param buffer		Buffer to store the array in.
 * @param size			If not set, assumes the buffer size is equal to the
 *						blocksize.  Otherwise, the size passed is used.
 * @return				Number of cells copied.
 * @error				Invalid Handle or invalid index.
 */
native GetArrayArray(Handle:array, index, buffer[], size=-1);

/**
 * Sets a cell value in an array.
 *
 * @param array			Array Handle.
 * @param index			Index in the array.
 * @param value			Cell value to set.
 * @param block			Optionally specify which block to write to
 *						(useful if the blocksize > 0).
 * @param asChar		Optionally set as a byte instead of a cell.
 * @noreturn
 * @error				Invalid Handle, invalid index, or invalid block.
 */
native any:SetArrayCell(Handle:array, index, any:value, block=0, bool:asChar=false);

/**
 * Sets a string value in an array.
 *
 * @param array			Array Handle.
 * @param index			Index in the array.
 * @param value			String value to set.
 * @return				Number of characters copied.
 * @error				Invalid Handle or invalid index.
 */
native SetArrayString(Handle:array, index, const String:buffer[]);

/**
 * Sets an array of cells in an array.
 *
 * @param array			Array Handle.
 * @param index			Index in the array.
 * @param buffer		Array to copy.
 * @param size			If not set, assumes the buffer size is equal to the
 *						blocksize.  Otherwise, the size passed is used.
 * @return				Number of cells copied.
 * @error				Invalid Handle or invalid index.
 */
native SetArrayArray(Handle:array, index, const values[], size=-1);

/**
 * Shifts an array up.  All array contents after and including the given 
 * index are shifted up by one, and the given index is then "free."  
 * After shifting, the contents of the given index is undefined.
 *
 * @param array			Array Handle.
 * @param index			Index in the array to shift up from.
 * @noreturn
 * @error				Invalid Handle or invalid index.
 */
native ShiftArrayUp(Handle:array, index);

/**
 * Removes an array index, shifting the entire array down from that position 
 * on.  For example, if item 8 of 10 is removed, the last 3 items will then be
 * (6,7,8) instead of (7,8,9), and all indexes before 8 will remain unchanged.
 *
 * @param array			Array Handle.
 * @param index			Index in the array to remove at.
 * @noreturn
 * @error				Invalid Handle or invalid index.
 */
native RemoveFromArray(Handle:array, index);

/**
 * Swaps two items in the array.
 *
 * @param array			Array Handle.
 * @param index1		First index.
 * @param index2		Second index.
 * @noreturn
 * @error				Invalid Handle or invalid index.
 */
native SwapArrayItems(Handle:array, index1, index2);
