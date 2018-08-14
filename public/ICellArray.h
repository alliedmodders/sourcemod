/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2016 AlliedModders LLC.  All rights reserved.
 * =============================================================================
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

#ifndef _INCLUDE_SOURCEMOD_INTERFACE_CELLARRAY_H_
#define _INCLUDE_SOURCEMOD_INTERFACE_CELLARRAY_H_

/**
 * @file ICellArray.h
 * @brief Contains functions for dynamic arrays in plugins.  The wrappers
 * for creating these are contained in ISourceMod.h
 */

namespace SourceMod
{
	/**
	 * @brief Specifies a dynamic array data structure used in plugins.
	 */
	class ICellArray
	{
	public:
		/**
		 * @brief Retrieve the size of the array.
		 *
		 * @return       The size of the array.
		 */
		virtual size_t size() const = 0;

		/**
		 * @brief Increases the size of the array by one and returns
		 *        a pointer to the newly added item at the end of the array.
		 *
		 * @return       A pointer to the new item added at the end
		 *               or NULL if growing the array failed.
		 */
		virtual cell_t *push() = 0;

		/**
		 * @brief Retrieve a pointer to the memory at a given index.
		 *
		 * @return       A pointer to the memory for item at given index.
		 */
		virtual cell_t *at(size_t index) const = 0;

		/**
		 * @brief Retrieve the block size set while creating the CellArray.
		 *        It determines how many cells each array slot has.
		 *
		 * @return          The block size of the array.
		 */
		virtual size_t blocksize() const = 0;

		/**
		 * @brief Clears an array of all entries.
		 *        This is the same as Resize(0).
		 */
		virtual void clear() = 0;

		/**
		 * @brief Swaps two items in the array.
		 *
		 * @param item1     First index.
		 * @param item2     Second index.
		 * @return          True if items were swapped, false otherwise.
		 */
		virtual bool swap(size_t item1, size_t item2) = 0;

		/**
		 * @brief Removes an array index, shifting the entire array down from that position
		 *        on.  For example, if item 8 of 10 is removed, the last 3 items will then be
		 *        (6,7,8) instead of (7,8,9), and all indexes before 8 will remain unchanged.
		 *
		 * @param index     Index in the array to remove at.
		 */
		virtual void remove(size_t index) = 0;

		/**
		 * @brief Shifts items at the given index and following up by one
		 *        to make space for a new item at the index.
		 *
		 * @param index     Index in the array to insert at.
		 * @return          Pointer to item space at the given index or NULL if shifting failed.
		 */
		virtual cell_t *insert_at(size_t index) = 0;

		/**
		 * @brief Resizes an array. If the size is smaller than the current size, the
		 *        array is truncated.
		 *
		 * @param newsize   New size
		 * @return          True if resized, false otherwise.
		 */
		virtual bool resize(size_t newsize) = 0;

		/**
		 * @brief Clones an array, returning a new object
		 *        with the same size and data.
		 *
		 * @return          Pointer to cloned array instance.
		 */
		virtual ICellArray *clone() = 0;

		/**
		 * @brief Retrieve a pointer to the array base.
		 *
		 * @return          Pointer to the array base.
		 */
		virtual cell_t *base() = 0;

		/**
		 * @brief Retrieve the amount of memory used by this array in bytes.
		 *
		 * @return          Amount of memory used in bytes.
		 */
		virtual size_t mem_usage() = 0;
	};
}

#endif //_INCLUDE_SOURCEMOD_INTERFACE_CELLARRAY_H_
