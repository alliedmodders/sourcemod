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
 
#if defined _adt_stack_included
 #endinput
#endif
#define _adt_stack_included

/**
 * Creates a stack structure.  A stack is a LIFO (last in, first out) 
 * vector (array) of items.  It has O(1) insertion and O(1) removal.
 *
 * Stacks have two operations: Push (adding an item) and Pop (removes 
 * items in reverse-push order).
 * 
 * The contents of the stack are uniform; i.e. storing a string and then 
 * retrieving it as an integer is NOT the same as StringToInt()!
 *
 * The "blocksize" determines how many cells each slot has; it cannot
 * be changed after creation.
 *
 * @param blocksize		The number of cells each entry in the stack can 
 *						hold.  For example, 32 cells is equivalent to:
 *						new Array[X][32]
 * @return				New stack Handle.
 */
native Handle:CreateStack(blocksize=1);

/**
 * Pushes a value onto the end of the stack, adding a new index.
 *
 * This may safely be used even if the stack has a blocksize
 * greater than 1.
 *
 * @param stack			Stack Handle.
 * @param value			Value to push.
 * @noreturn
 * @error				Invalid Handle or out of memory.
 */
native PushStackCell(Handle:stack, any:value);

/**
 * Pushes a string onto the end of a stack, truncating it if it is 
 * too big.
 *
 * @param stack			Stack Handle.
 * @param value			String to push.
 * @noreturn
 * @error				Invalid Handle or out of memory.
 */
native PushStackString(Handle:stack, const String:value[]);

/**
 * Pushes an array of cells onto the end of a stack.  The cells
 * are pushed as a block (i.e. the entire array takes up one stack slot),
 * rather than pushing each cell individually.
 *
 * @param stack			Stack Handle.
 * @param values		Block of values to copy.
 * @param size			If not set, the number of elements copied from the array
 *						will be equal to the blocksize.  If set higher than the 
 *						blocksize, the operation will be truncated.
 * @noreturn
 * @error				Invalid Handle or out of memory.
 */
native PushStackArray(Handle:stack, const any:values[], size=-1);

/**
 * Pops a cell value from a stack.
 *
 * @param stack			Stack Handle.
 * @param value			Variable to store the value.
 * @param block			Optionally specify which block to read from
 *						(useful if the blocksize > 0).
 * @param asChar		Optionally read as a byte instead of a cell.
 * @return				True on success, false if the stack is empty.
 * @error				Invalid Handle.
 */
native bool:PopStackCell(Handle:stack, &any:value, block=0, bool:asChar=false);

/**
 * Pops a string value from a stack.
 *
 * @param stack			Stack Handle.
 * @param buffer		Buffer to store string.
 * @param maxlength		Maximum size of the buffer.
 * @return				True on success, false if the stack is empty.
 * @error				Invalid Handle.
 */
native bool:PopStackString(Handle:stack, String:buffer[], maxlength, &written=0);

/**
 * Pops an array of cells from a stack.
 *
 * @param stack			Stack Handle.
 * @param buffer		Buffer to store the array in.
 * @param size			If not set, assumes the buffer size is equal to the
 *						blocksize.  Otherwise, the size passed is used.
 * @return				True on success, false if the stack is empty.
 * @error				Invalid Handle.
 */
native bool:PopStackArray(Handle:stack, any:buffer[], size=-1);

/**
 * Checks if a stack is empty.
 *
 * @param stack			Stack Handle.
 * @return				True if empty, false if not empty.
 * @error				Invalid Handle.
 */
native bool:IsStackEmpty(Handle:stack);

/**
 * Pops a value off a stack, ignoring it completely.
 *
 * @param stack			Stack Handle.
 * @return				True if something was popped, false otherwise.
 * @error				Invalid Handle.
 */
stock PopStack(Handle:stack)
{
	new value;
	
	return PopStackCell(stack, value);
}
