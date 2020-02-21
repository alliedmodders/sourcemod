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


#if defined _sorting_included
 #endinput
#endif
#define _sorting_included

/**
 * Contains sorting orders.
 */
enum SortOrder
{
	Sort_Ascending = 0,     /**< Ascending order */
	Sort_Descending = 1,    /**< Descending order */
	Sort_Random = 2         /**< Random order */
};

/**
 * Data types for ADT Array Sorts
 */
enum SortType
{
	Sort_Integer = 0,
	Sort_Float,
	Sort_String
};

/**
 * Sorts an array of integers.
 *
 * @param array         Array of integers to sort in-place.
 * @param array_size    Size of the array.
 * @param order         Sorting order to use.
 */
native void SortIntegers(int[] array, int array_size, SortOrder order = Sort_Ascending);

/**
 * Sorts an array of float point numbers.
 *
 * @param array         Array of floating point numbers to sort in-place.
 * @param array_size    Size of the array.
 * @param order         Sorting order to use.
 */
native void SortFloats(float[] array, int array_size, SortOrder order = Sort_Ascending);

/**
 * Sorts an array of strings.
 *
 * @param array         Array of strings to sort in-place.
 * @param array_size    Size of the array.
 * @param order         Sorting order to use.
 */
native void SortStrings(char[][] array, int array_size, SortOrder order = Sort_Ascending);

/**
 * Sort comparison function for 1D array elements.
 * @note You may need to use explicit tags in order to use data properly.
 *
 * @param elem1         First element to compare.
 * @param elem2         Second element to compare.
 * @param array         Array that is being sorted (order is undefined).
 * @param hndl          Handle optionally passed in while sorting.
 * @return              -1 if first should go before second
 *                      0 if first is equal to second
 *                      1 if first should go after second
 */
typedef SortFunc1D = function int (int elem1, int elem2, const int[] array, Handle hndl);

/**
 * Sorts a custom 1D array.  You must pass in a comparison function.
 *
 * @param array         Array to sort.
 * @param array_size    Size of the array to sort.
 * @param sortfunc      Sort function.
 * @param hndl          Optional Handle to pass through the comparison calls.
 */
native void SortCustom1D(int[] array, int array_size, SortFunc1D sortfunc, Handle hndl=INVALID_HANDLE);

/**
 * Sort comparison function for 2D array elements (sub-arrays).
 * @note You may need to use explicit tags in order to use data properly.
 *
 * @param elem1         First array to compare.
 * @param elem2         Second array to compare.
 * @param array         Array that is being sorted (order is undefined).
 * @param hndl          Handle optionally passed in while sorting.
 * @return              -1 if first should go before second
 *                      0 if first is equal to second
 *                      1 if first should go after second
 */
typeset SortFunc2D
{
	function int (int[] elem1, int[] elem2, const int[][] array, Handle hndl);
	function int (char[] elem1, char[] elem2, const char[][] array, Handle hndl);
};

/**
 * Sorts a custom 2D array.  You must pass in a comparison function.
 *
 * @param array         Array to sort.
 * @param array_size    Size of the major array to sort (first index, outermost).
 * @param sortfunc      Sort comparison function to use.
 * @param hndl          Optional Handle to pass through the comparison calls.
 */
native void SortCustom2D(any[][] array, int array_size, SortFunc2D sortfunc, Handle hndl=INVALID_HANDLE);

/**
 * Sort an ADT Array. Specify the type as Integer, Float, or String.
 *
 * @param array         Array Handle to sort
 * @param order         Sort order to use, same as other sorts.
 * @param type          Data type stored in the ADT Array
 */
native void SortADTArray(Handle array, SortOrder order, SortType type);

/**
 * Sort comparison function for ADT Array elements. Function provides you with
 * indexes currently being sorted, use ADT Array functions to retrieve the
 * index values and compare.
 *
 * @param index1        First index to compare.
 * @param index2        Second index to compare.
 * @param array         Array that is being sorted (order is undefined).
 * @param hndl          Handle optionally passed in while sorting.
 * @return              -1 if first should go before second
 *                      0 if first is equal to second
 *                      1 if first should go after second
 */
typedef SortFuncADTArray = function int (int index1, int index2, Handle array, Handle hndl);

/**
 * Custom sorts an ADT Array. You must pass in a comparison function.
 *
 * @param array         Array Handle to sort
 * @param sortfunc      Sort comparison function to use
 * @param hndl          Optional Handle to pass through the comparison calls.
 */
native void SortADTArrayCustom(Handle array, SortFuncADTArray sortfunc, Handle hndl=INVALID_HANDLE);
