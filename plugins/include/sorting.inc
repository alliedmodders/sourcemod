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


#if defined _sorting_included
  #endinput
#endif
#define _sorting_included

/**
 * Contains sorting orders.
 */
enum SortOrder
{
	Sort_Ascending = 0,		/**< Ascending order */
	Sort_Descending = 1,	/**< Descending order */
};

/**
 * Sorts an array of integers.
 *
 * @param array			Array of integers to sort in-place.
 * @param array_size	Size of the array.
 * @param order			Sorting order to use.
 * @noreturn
 */
native SortIntegers(array[], array_size, SortOrder:order = Sort_Ascending);

/**
 * Sorts an array of float point numbers.
 *
 * @param array			Array of floating point numbers to sort in-place.
 * @param array_size	Size of the array.
 * @param order			Sorting order to use.
 * @noreturn
 */
native SortFloats(Float:array[], array_size, SortOrder:order = Sort_Ascending);

/**
 * Sorts an array of strings.
 *
 * @param array			Array of strings to sort in-place.
 * @param array_size	Size of the array.
 * @param order			Sorting order to use.
 * @noreturn
 */
native SortStrings(String:array[][], num_strings, SortOrder:order = Sort_Ascending);

/**
 * Sort comparison function for 1D array elements.
 * @note You may need to use explicit tags in order to use data properly.
 *
 * @param elem1			First element to compare.
 * @param elem2			Second element to compare.
 * @param array			Array that is being sorted (order is undefined).
 * @param hndl			Handle optionally passed in while sorting.
 * @return				-1 if first should go before second
 *						0 if first is equal to second
 *						1 if first should go after second
 */
functag SortFunc1D public(elem1, elem2, const array[], Handle:hndl);

/** 
 * Sorts a custom 1D array.  You must pass in a comparison function.
 *
 * @param array			Array to sort.
 * @param array_size	Size of the array to sort.
 * @param sortfunc		Sort function.
 * @param hndl			Optional Handle to pass through the comparison calls.
 * @noreturn
 */
native SortCustom1D(array[], array_size, SortFunc1D:sortfunc, Handle:hndl=INVALID_HANDLE);

/**
 * Sort comparison function for 2D array elements (sub-arrays).
 * @note You may need to use explicit tags in order to use data properly.
 *
 * @param elem1			First array to compare.
 * @param elem2			Second array to compare.
 * @param array			Array that is being sorted (order is undefined).
 * @param hndl			Handle optionally passed in while sorting.
 * @return				-1 if first should go before second
 *						0 if first is equal to second
 *						1 if first should go after second
 */
funcenum SortFunc2D
{
	public(array[], array[], const array[][], Handle:hndl),
	public(String:array[], String:array[], const String:array[][], Handle:hndl),
};

/** 
 * Sorts a custom 2D array.  You must pass in a comparison function.
 *
 * @param array			Array to sort.
 * @param array_size	Size of the major array to sort (first index, outermost).
 * @param sortfunc		Sort comparison function to use.
 * @param hndl			Optional Handle to pass through the comparison calls.
 * @noreturn
 */
native SortCustom2D(array[][], array_size, SortFunc2D:sortfunc, Handle:hndl=INVALID_HANDLE);