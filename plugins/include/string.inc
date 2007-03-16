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
 
#if defined _string_included
 #endinput
#endif
#define _string_included

/**
 * @global Unless otherwise noted, all string functions which take in a writable buffer 
 * and maximum length should have the null terminator INCLUDED in the length.  This means 
 * that this is valid: StrCopy(string, sizeof(string), ...)
 */

/**
 * Calculates the length of a string.
 *
 * @param str			String to check.
 * @return				Length of string, in cells (NOT characters).
 */
native strlen(const String:str[]);

/**
 * Tests whether a string is found inside another string.
 *
 * @param str			String to search in.
 * @param substr		Substring to find inside the original string.
 * @param caseSensitive	If true (default), search is case sensitive.
 *						If false, search is case insensitive.
 * @return				-1 on failure (no match found). Any other value
 *						indicates a position in the string where the match starts.
 */
native StrContains(const String:str[], const String:substr[], bool:caseSensitive=true);

/**
 * Compares two strings lexographically.
 *
 * @param str1			First string (left).
 * @param str2			Second string (right).
 * @param caseSensitive	If true (default), comparison is case sensitive.
 *						If false, comparison is case insensitive.
 * @return				-1 if str1 < str2
 *						0 if str1 == str2
 *						1 if str1 > str2
 */
native StrCompare(const String:str1[], const String:str2[], bool:caseSensitive=true);

/**
 * Returns whether two strings are equal.
 *
 *
 * @param str1			First string (left).
 * @param str2			Second string (right).
 * @param caseSensitive	If true (default), comparison is case sensitive.
 * 						If false, comparison is case insensitive.
 * @return				True if equal, false otherwise.
 */
stock bool:StrEqual(const String:str1[], const String:str2[], bool:caseSensitive=true)
{
	return (StrCompare(str1, str2, caseSensitive) == 0);
}

/**
 * Copies one string to another string.
 * @note If the destination buffer is too small to hold the source string, the 
 *       destination will be truncated.
 *
 * @param dest			Destination string buffer to copy to.
 * @param destlen		Destination buffer length (includes null terminator).
 * @param source		Source string buffer to copy from.
 * @return				Number of cells written.
 */
native StrCopy(String:dest[], destLen, const String:source[]);

/**
 * Formats a string according to the SourceMod format rules (see documentation).
 *
 * @param buffer		Destination string buffer.
 * @param maxlength		Maximum length of output string buffer.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @return				Number of cells written.
 */
native Format(String:buffer[], maxlength, const String:format[], {Handle,Float,String,Function,_}:...);

/**
 * Formats a string according to the SourceMod format rules (see documentation).
 * @note This is the same as Format(), except none of the input buffers can overlap the same
 *	     memory as the output buffer.  Since this security check is removed, it is slightly faster.
 *
 * @param buffer		Destination string buffer.
 * @param maxlength		Maximum length of output string buffer.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @return				Number of cells written.
 */
native FormatEx(String:buffer[], maxlength, const String:format[], {Handle,Float,String,Function,_}:...);

/**
 * Formats a string according to the SourceMod format rules (see documentation).
 * @note This is the same as Format(), except it grabs parameters from a parent parameter
 *       stack, rather than a local.  This is useful for implementing your own variable 
 *       argument functions.
 *
 * @param buffer		Destination string buffer.
 * @param maxlength		Maximum length of output string buffer.
 * @param format		Formatting rules.
 * @param varpos		Argument number which contains the '...' symbol.
 *						Note: Arguments start at 1.
 * @return 				Number of bytes written.
 */
native VFormat(String:buffer[], maxlength, const String:format[], varpos);

/**
 * Converts a string to an integer.
 *
 * @param str			String to convert.
 * @param nBase			Numerical base to use.  10 is default.
 * @return				Integer conversion of string, or 0 on failure.
 */
native StringToInt(const String:str[], nBase=10);

/**
 * Converts an integer to a string.
 *
 * @param num			Integer to convert.
 * @param str			Buffer to store string in.
 * @param maxlength		Maximum length of string buffer.
 * @return				Number of cells written to buffer.
 */
native IntToString(num, String:str[], maxlength);

/** 
 * Converts a string to a floating point number.
 *
 * @param str			String to convert to a foat.
 * @return				Floating point result, or 0.0 on error.
 */
native Float:StringToFloat(const String:str[]);

/**
 * Converts a floating point number to a string.
 *
 * @param num			Floating point number to convert.
 * @param str			Buffer to store string in.
 * @param maxlength		Maximum length of string buffer.
 * @return				Number of cells written to buffer.
 */
native FloatToString(Float:num, String:str[], maxlength);
