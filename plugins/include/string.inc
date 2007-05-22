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
 * @global Unless otherwise noted, all string functions which take in a 
 * writable buffer and maximum length should have the null terminator INCLUDED
 * in the length.  This means that this is valid: 
 * StrCopy(string, sizeof(string), ...)
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
native strcmp(const String:str1[], const String:str2[], bool:caseSensitive=true);

/**
 * Compares two strings parts lexographically.
 *
 * @param str1			First string (left).
 * @param str2			Second string (right).
 * @param num			Number of characters to compare.
 * @param caseSensitive	If true (default), comparison is case sensitive.
 *						If false, comparison is case insensitive.
 * @return				-1 if str1 < str2
 *						0 if str1 == str2
 *						1 if str1 > str2
 */
native strncmp(const String:str1[], const String:str2[], num, bool:caseSensitive=true);

/**
 * Backwards compatible stock - StrCompare is now strcmp
 * @deprecated		Renamed to strcmp
 */
stock StrCompare(const String:str1[], const String:str2[], bool:caseSensitive=true)
{
	return strcmp(str1, str2, caseSensitive);
}

/**
 * Returns whether two strings are equal.
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
native strcopy(String:dest[], destLen, const String:source[]);

/**
 * Backwards compatibility stock - use strcopy
 * @deprecated			Renamed to strcopy
 */
stock StrCopy(String:dest[], destLen, const String:source[])
{
	return strcopy(dest, destLen, source);
}

/**
 * Formats a string according to the SourceMod format rules (see documentation).
 *
 * @param buffer		Destination string buffer.
 * @param maxlength		Maximum length of output string buffer.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @return				Number of cells written.
 */
native Format(String:buffer[], maxlength, const String:format[], any:...);

/**
 * Formats a string according to the SourceMod format rules (see documentation).
 * @note This is the same as Format(), except none of the input buffers can 
 *       overlap the same memory as the output buffer.  Since this security 
 *       check is removed, it is slightly faster.
 *
 * @param buffer		Destination string buffer.
 * @param maxlength		Maximum length of output string buffer.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @return				Number of cells written.
 */
native FormatEx(String:buffer[], maxlength, const String:format[], any:...);

/**
 * Formats a string according to the SourceMod format rules (see documentation).
 * @note This is the same as Format(), except it grabs parameters from a 
 *       parent parameter stack, rather than a local.  This is useful for 
 *       implementing your own variable argument functions.
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

/**
 * Finds the first "argument" in a string; either a set of space
 * terminated characters, or a fully quoted string.  After the 
 * argument is found, whitespace is read until the next portion
 * of the string is reached.  If nothing remains, -1 is returned.
 * Otherwise, the index to the first character is returned.
 *
 * @param source		Source input string.
 * @param arg			Stores argument read from string.
 * @param argLen		Maximum length of argument buffer.
 * @return				Index to next piece of string, or -1 if none.
 */
native BreakString(const String:source[], String:arg[], argLen);

/**
 * Backwards compatibility stock - use BreakString
 * @deprecated			Renamed to BreakString.
 */
stock StrBreak(const String:source[], String:arg[], argLen)
{
	return BreakString(source, arg, argLen);
}

/** 
 * Returns the number of bytes a character is using.  This is
 * for multi-byte characters (UTF-8).  For normal ASCII characters,
 * this will return 1.
 *
 * @param source		Source input string.
 * @return				Number of bytes the current character uses.
 */
native GetCharBytes(const String:source[]);

/**
 * Returns whether a character is an ASCII alphabet character.
 *
 * @note Multi-byte characters will always return false.
 *
 * @param char			Character to test.
 * @return				True if character is alphabetical, otherwise false.
 */
native bool:IsCharAlpha(chr);

/**
 * Returns whether a character is numeric.
 *
 * @note Multi-byte characters will always return false.
 *
 * @param char			Character to test.
 * @return				True if character is numeric, otherwise false.
 */
native bool:IsCharNumeric(chr);

/**
 * Returns whether a character is whitespace.
 *
 * @note Multi-byte characters will always return false.
 *
 * @param char			Character to test.
 * @return				True if character is whitespace, otherwise false.
 */
native bool:IsCharSpace(chr);

/**
 * Returns if a character is multi-byte or not.
 *
 * @param char			Character to test.
 * @return				0 for a normal 7-bit ASCII character,
 *						otherwise number of bytes in multi-byte character.
 */
native IsCharMB(chr);

/**
 * Returns whether an alphabetic character is uppercase.
 *
 * @note Multi-byte characters will always return false.
 *
 * @param char			Character to test.
 * @return				True if character is uppercase, otherwise false.
 */
native bool:IsCharUpper(chr);

/**
 * Returns whether an alphabetic character is lowercase.
 *
 * @note Multi-byte characters will always return false.
 *
 * @param char			Character to test.
 * @return				True if character is lowercase, otherwise false.
 */
native bool:IsCharLower(chr);

/**
 * Returns an uppercase character to a lowercase character.
 *
 * @param chr			Characer to convert.
 * @return				Lowercase character on success, 
 *						no change on failure.
 */
stock CharToUpper(chr)
{
	if (IsCharLower(chr))
	{
		return (chr & ~(1<<5));
	}
	return chr;
}

/**
 * Returns a lowercase character to an uppercase character.
 *
 * @param chr			Characer to convert.
 * @return				Uppercase character on success, 
 *						no change on failure.
 */
stock CharToLower(chr)
{
	if (IsCharUpper(chr))
	{
		return (chr | (1<<5));
	}
	return chr;
}

/**
 * Concatenates one string onto another.
 *
 * @param buffer		String to append to.
 * @param maxlength		Maximum length of entire buffer.
 * @param source		Source string to concatenate.
 * @return				Number of bytes written.
 */
stock StrCat(String:buffer[], maxlength, const String:source[])
{
	new len = strlen(source);
	if (len >= maxlength)
	{
		return 0;
	}
	
	return Format(buffer[len], maxlength-len, "%s", source);
}

/**
 * Removes whitespace characters from the beginning and end of a string.
 *
 * @param str			The string to trim.
 * @return				Number of bytes written (UTF-8 safe).
 */
native TrimString(String:str[]);
