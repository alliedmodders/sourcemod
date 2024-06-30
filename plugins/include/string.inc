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
 
#if defined _string_included
 #endinput
#endif
#define _string_included

/**
 * @global Unless otherwise noted, all string functions which take in a 
 * writable buffer and maximum length should have the null terminator INCLUDED
 * in the length.  This means that this is valid: 
 * strcopy(string, sizeof(string), ...)
 */
 
/**
 * Calculates the length of a string.
 *
 * @param str           String to check.
 * @return              Number of valid character bytes in the string.
 */
native int strlen(const char[] str);

/**
 * Tests whether a string is found inside another string.
 *
 * @param str           String to search in.
 * @param substr        Substring to find inside the original string.
 * @param caseSensitive If true (default), search is case sensitive.
 *                      If false, search is case insensitive.
 * @return              -1 on failure (no match found). Any other value
 *                      indicates a position in the string where the match starts.
 */
native int StrContains(const char[] str, const char[] substr, bool caseSensitive=true);

/**
 * Compares two strings lexographically.
 *
 * @param str1          First string (left).
 * @param str2          Second string (right).
 * @param caseSensitive If true (default), comparison is case sensitive.
 *                      If false, comparison is case insensitive.
 * @return              -1 if str1 < str2
 *                      0 if str1 == str2
 *                      1 if str1 > str2
 */
native int strcmp(const char[] str1, const char[] str2, bool caseSensitive=true);

/**
 * Compares two strings parts lexographically.
 *
 * @param str1          First string (left).
 * @param str2          Second string (right).
 * @param num           Number of characters to compare.
 * @param caseSensitive If true (default), comparison is case sensitive.
 *                      If false, comparison is case insensitive.
 * @return              -1 if str1 < str2
 *                      0 if str1 == str2
 *                      1 if str1 > str2
 */
native int strncmp(const char[] str1, const char[] str2, int num, bool caseSensitive=true);

/**
 * Backwards compatible stock - StrCompare is now strcmp
 * @deprecated          Renamed to strcmp
 */
#pragma deprecated Use strcmp() instead
stock int StrCompare(const char[] str1, const char[] str2, bool caseSensitive=true)
{
	return strcmp(str1, str2, caseSensitive);
}

/**
 * Returns whether two strings are equal.
 *
 * @param str1          First string (left).
 * @param str2          Second string (right).
 * @param caseSensitive If true (default), comparison is case sensitive.
 *                      If false, comparison is case insensitive.
 * @return              True if equal, false otherwise.
 */
stock bool StrEqual(const char[] str1, const char[] str2, bool caseSensitive=true)
{
	return (strcmp(str1, str2, caseSensitive) == 0);
}

/**
 * Copies one string to another string.
 * @note If the destination buffer is too small to hold the source string, the 
 *       destination will be truncated.
 *
 * @param dest          Destination string buffer to copy to.
 * @param destLen       Destination buffer length (includes null terminator).
 * @param source        Source string buffer to copy from.
 * @return              Number of characters written to the buffer,
 *                      not including the null terminator.
 */
native int strcopy(char[] dest, int destLen, const char[] source);

/**
 * Backwards compatibility stock - use strcopy
 * @deprecated          Renamed to strcopy
 */
#pragma deprecated Use strcopy() instead
stock int StrCopy(char[] dest, int destLen, const char[] source)
{
	return strcopy(dest, destLen, source);
}

/**
 * Formats a string according to the SourceMod format rules (see documentation).
 *
 * @param buffer        Destination string buffer.
 * @param maxlength     Maximum length of output string buffer,
 *                      including the null terminator.
 * @param format        Formatting rules.
 * @param ...           Variable number of format parameters.
 * @return              Number of characters written to the buffer,
 *                      not including the null terminator.
 */
native int Format(char[] buffer, int maxlength, const char[] format, any ...);

/**
 * Formats a string according to the SourceMod format rules (see documentation).
 * @note This is the same as Format(), except none of the input buffers can 
 *       overlap the same memory as the output buffer.  Since this security 
 *       check is removed, it is slightly faster.
 *
 * @param buffer        Destination string buffer.
 * @param maxlength     Maximum length of output string buffer,
 *                      including the null terminator.
 * @param format        Formatting rules.
 * @param ...           Variable number of format parameters.
 * @return              Number of characters written to the buffer,
 *                      not including the null terminator.
 */
native int FormatEx(char[] buffer, int maxlength, const char[] format, any ...);

/**
 * Formats a string according to the SourceMod format rules (see documentation).
 * @note This is the same as Format(), except it grabs parameters from a 
 *       parent parameter stack, rather than a local.  This is useful for 
 *       implementing your own variable argument functions.
 *
 * @param buffer        Destination string buffer.
 * @param maxlength     Maximum length of output string buffer,
 *                      including the null terminator.
 * @param format        Formatting rules.
 * @param varpos        Argument number which contains the '...' symbol.
 *                      Note: Arguments start at 1.
 * @return              Number of bytes written.
 * @error               Invalid argument index.
 */
native int VFormat(char[] buffer, int maxlength, const char[] format, int varpos);

/**
 * Converts a string to an integer.
 *
 * @param str           String to convert.
 * @param nBase         Numerical base to use.  10 is default.
 * @return              Integer conversion of string, or 0 on failure.
 */
native int StringToInt(const char[] str, int nBase=10);

/**
 * Converts a string to an integer with some more options.
 *
 * @param str           String to convert.
 * @param result        Variable to store the result in.
 * @param nBase         Numerical base to use.  10 is default.
 * @return              Number of characters consumed.
 */
native int StringToIntEx(const char[] str, int &result, int nBase=10);

/**
 * Converts a string to a 64-bit integer.
 *
 * @param str           String to convert.
 * @param result        Array to store the upper and lower
 *                      32-bits of the 64-bit integer.
 * @param nBase         Numerical base to use.  10 is default.
 * @return              Number of characters consumed.
 */
native int StringToInt64(const char[] str, int result[2], int nBase=10);

/**
 * Converts an integer to a string.
 *
 * @param num           Integer to convert.
 * @param str           Buffer to store string in.
 * @param maxlength     Maximum length of string buffer.
 * @return              Number of characters written to the buffer,
 *                      not including the null terminator.
 */
native int IntToString(int num, char[] str, int maxlength);

/**
 * Converts a 64-bit integer to a string.
 *
 * @param num           Array containing the upper and lower
 *                      32-bits of a 64-bit integer.
 * @param str           Buffer to store string in.
 * @param maxlength     Maximum length of string buffer.
 * @return              Number of characters written to the buffer,
 *                      not including the null terminator.
 */
native int Int64ToString(const int num[2], char[] str, int maxlength);

/** 
 * Converts a string to a floating point number.
 *
 * @param str           String to convert to a float.
 * @return              Floating point result, or 0.0 on error.
 */
native float StringToFloat(const char[] str);

/** 
 * Converts a string to a floating point number with some more options.
 *
 * @param str           String to convert to a float.
 * @param result        Variable to store result in.
 * @return              Number of characters consumed.
 */
native int StringToFloatEx(const char[] str, float &result);

/**
 * Converts a floating point number to a string.
 *
 * @param num           Floating point number to convert.
 * @param str           Buffer to store string in.
 * @param maxlength     Maximum length of string buffer.
 * @return              Number of characters written to the buffer,
 *                      not including the null terminator.
 */
native int FloatToString(float num, char[] str, int maxlength);

/**
 * Finds the first "argument" in a string; either a set of space
 * terminated characters, or a fully quoted string.  After the 
 * argument is found, whitespace is read until the next portion
 * of the string is reached.  If nothing remains, -1 is returned.
 * Otherwise, the index to the first character is returned.
 *
 * @param source        Source input string.
 * @param arg           Stores argument read from string.
 * @param argLen        Maximum length of argument buffer.
 * @return              Index to next piece of string, or -1 if none.
 */
native int BreakString(const char[] source, char[] arg, int argLen);

/**
 * Backwards compatibility stock - use BreakString
 * @deprecated          Renamed to BreakString.
 */
#pragma deprecated Use BreakString() instead
stock int StrBreak(const char[] source, char[] arg, int argLen)
{
	return BreakString(source, arg, argLen);
}

/**
 * Removes whitespace characters from the beginning and end of a string.
 *
 * @param str           The string to trim.
 * @return              Number of bytes written (UTF-8 safe).
 */
native int TrimString(char[] str);

/**
 * Returns text in a string up until a certain character sequence is reached.
 *
 * @param source        Source input string.
 * @param split         A string which specifies a search point to break at.
 * @param part          Buffer to store string part.
 * @param partLen       Maximum length of the string part buffer.
 * @return              -1 if no match was found; otherwise, an index into source
 *                      marking the first index after the searched text.  The
 *                      index is always relative to the start of the input string.
 */
native int SplitString(const char[] source, const char[] split, char[] part, int partLen);

/**
 * Given a string, replaces all occurrences of a search string with a 
 * replacement string.
 *
 * @param text          String to perform search and replacements on.
 * @param maxlength     Maximum length of the string buffer.
 * @param search        String to search for.
 * @param replace       String to replace the search string with.
 * @param caseSensitive If true (default), search is case sensitive.
 * @return              Number of replacements that were performed.
 * @error               'search' parameter is empty.
 */
native int ReplaceString(char[] text, int maxlength, const char[] search, const char[] replace, bool caseSensitive=true);

/**
 * Given a string, replaces the first occurrence of a search string with a 
 * replacement string.
 *
 * @param text          String to perform search and replacements on.
 * @param maxlength     Maximum length of the string buffer.
 * @param search        String to search for.
 * @param replace       String to replace the search string with.
 * @param searchLen     If higher than -1, its value will be used instead of
 *                      a strlen() call on the search parameter.
 * @param replaceLen    If higher than -1, its value will be used instead of
 *                      a strlen() call on the replace parameter.
 * @param caseSensitive If true (default), search is case sensitive.
 * @return              Index into the buffer (relative to the start) from where
 *                      the last replacement ended, or -1 if no replacements were
 *                      made.
 * @error               'search' parameter is empty.
 */
native int ReplaceStringEx(char[] text, int maxlength, const char[] search, const char[] replace, int searchLen=-1, int replaceLen=-1, bool caseSensitive=true);

/** 
 * Returns the number of bytes a character is using.  This is
 * for multi-byte characters (UTF-8).  For normal ASCII characters,
 * this will return 1.
 *
 * @param source        Source input string.
 * @return              Number of bytes the current character uses.
 */
native int GetCharBytes(const char[] source);

/**
 * Returns whether a character is an ASCII alphabet character.
 *
 * @note Multi-byte characters will always return false.
 *
 * @param chr           Character to test.
 * @return              True if character is alphabetical, otherwise false.
 */
native bool IsCharAlpha(int chr);

/**
 * Returns whether a character is numeric.
 *
 * @note Multi-byte characters will always return false.
 *
 * @param chr           Character to test.
 * @return              True if character is numeric, otherwise false.
 */
native bool IsCharNumeric(int chr);

/**
 * Returns whether a character is whitespace.
 *
 * @note Multi-byte characters will always return false.
 *
 * @param chr           Character to test.
 * @return              True if character is whitespace, otherwise false.
 */
native bool IsCharSpace(int chr);

/**
 * Returns if a character is multi-byte or not.
 *
 * @param chr           Character to test.
 * @return              0 for a normal 7-bit ASCII character,
 *                      otherwise number of bytes in multi-byte character.
 */
native int IsCharMB(int chr);

/**
 * Returns whether an alphabetic character is uppercase.
 *
 * @note Multi-byte characters will always return false.
 *
 * @param chr           Character to test.
 * @return              True if character is uppercase, otherwise false.
 */
native bool IsCharUpper(int chr);

/**
 * Returns whether an alphabetic character is lowercase.
 *
 * @note Multi-byte characters will always return false.
 *
 * @param chr           Character to test.
 * @return              True if character is lowercase, otherwise false.
 */
native bool IsCharLower(int chr);

/**
 * Strips a quote pair off a string if it exists.  That is, the following 
 * replace rule is applied once:  ^"(.*)"$ -> ^\1$
 *
 * Note that the leading and trailing quotes will only be removed if both 
 * exist.  Otherwise, the string is left unmodified.  This function should 
 * be considered O(k) (all characters get shifted down).
 *
 * @param text          String to modify (in place).
 * @return              True if string was modified, false if there was no 
 *                      set of quotes.
 */
native bool StripQuotes(char[] text);

/**
 * Converts a lowercase character to its uppercase counterpart.
 *
 * @param chr           Character to convert.
 * @return              Uppercase character on success, 
 *                      no change on failure.
 */
stock int CharToUpper(int chr)
{
	if (IsCharLower(chr))
	{
		return (chr & ~(1<<5));
	}

	return chr;
}

/**
 * Converts an uppercase character to its lowercase counterpart.
 *
 * @param chr           Character to convert.
 * @return              Lowercase character on success, 
 *                      no change on failure.
 */
stock int CharToLower(int chr)
{
	if (IsCharUpper(chr))
	{
		return (chr | (1<<5));
	}
	
	return chr;
}

/**
 * Finds the first occurrence of a character in a string.
 *
 * @param str           String.
 * @param c             Character to search for.
 * @param reverse       False (default) to search forward, true to search 
 *                      backward.
 * @return              The index of the first occurrence of the character 
 *                      in the string, or -1 if the character was not found.
 */
stock int FindCharInString(const char[] str, char c, bool reverse = false)
{
	int len = strlen(str);
	
	if (!reverse)
	{
		for (int i = 0; i < len; i++)
		{
			if (str[i] == c)
			{
				return i;
			}
		}
	}
	else
	{
		for (int i = len - 1; i >= 0; i--)
		{
			if (str[i] == c)
			{
				return i;
			}
		}
	}

	return -1;
}

/**
 * Concatenates one string onto another.
 *
 * @param buffer        String to append to.
 * @param maxlength     Maximum length of entire buffer.
 * @param source        Source string to concatenate.
 * @return              Number of bytes written.
 */
stock int StrCat(char[] buffer, int maxlength, const char[] source)
{
	int len = strlen(buffer);
	if (len >= maxlength)
	{
		return 0;
	}
	
	return Format(buffer[len], maxlength-len, "%s", source);
}

/**
 * Breaks a string into pieces and stores each piece into an array of buffers.
 *
 * @param text              The string to split.
 * @param split             The string to use as a split delimiter.
 * @param buffers           An array of string buffers (2D array).
 * @param maxStrings        Number of string buffers (first dimension size).
 * @param maxStringLength   Maximum length of each string buffer.
 * @param copyRemainder     False (default) discard excess pieces, true to ignore
 *                          delimiters after last piece.
 * @return                  Number of strings retrieved.
 */
stock int ExplodeString(const char[] text, const char[] split, char[][] buffers, int maxStrings,
                    int maxStringLength, bool copyRemainder = false)
{
	int reloc_idx, idx, total;

	if (maxStrings < 1 || !split[0])
	{
		return 0;
	}

	while ((idx = SplitString(text[reloc_idx], split, buffers[total], maxStringLength)) != -1)
	{
		reloc_idx += idx;
		if (++total == maxStrings)
		{
			if (copyRemainder)
			{
				strcopy(buffers[total-1], maxStringLength, text[reloc_idx-idx]);
			}
			return total;
		}
	}

	strcopy(buffers[total++], maxStringLength, text[reloc_idx]);

	return total;
}

/**
 * Joins an array of strings into one string, with a "join" string inserted in
 * between each given string.  This function complements ExplodeString.
 *
 * @param strings       An array of strings.
 * @param numStrings    Number of strings in the array.
 * @param join          The join string to insert between each string.
 * @param buffer        Output buffer to write the joined string to.
 * @param maxLength     Maximum length of the output buffer.
 * @return              Number of bytes written to the output buffer.
 */
stock int ImplodeStrings(const char[][] strings, int numStrings, const char[] join, char[] buffer, int maxLength)
{
	int total, length, part_length;
	int join_length = strlen(join);
	for (int i=0; i<numStrings; i++)
	{
		length = strcopy(buffer[total], maxLength-total, strings[i]);
		total += length;
		if (length < part_length)
		{
			break;
		}
		if (i != numStrings - 1)
		{
			length = strcopy(buffer[total], maxLength-total, join);
			total += length;
			if (length < join_length)
			{
				break;
			}
		}
	}
	return total;
}






// =======================================================================================================================
// New funcs below







stock bool IsStringInteger(const char[] str, int nBase=10)
{
	int dummy;
	return StringToIntStrict(str, dummy, nBase);
}

stock bool IsStringDecimal(const char[] str)
{
	float dummy;
	return StringToFloatStrict(str, dummy);
}

stock void StringToLower(char[] str)
{
	for (int i = 0; str[i] != '\0'; ++i)
	{
		str[i] = CharToLower(str[i]);
	}
}

stock void StringToUpper(char[] str)
{
	for (int i = 0; str[i] != '\0'; ++i)
	{
		str[i] = CharToUpper(str[i]);
	}
}

stock bool IsStringUpper(const char[] str, bool lettersOnly=false)  // <<<<<<<<<<<<<<<<<<<<<<<<<<< Is lettersOnly worth it/implemented right?
{
	if (str[0] == '\0')
	{
		return false;                                               // <<<<<<<<<<<<<<<<<<<<<<<<<<< Should empty string return false?
	}

	for (int i = 0; str[i] != '\0'; ++i)
	{
		if (!IsCharAlpha(str[i]))
		{
			if (lettersOnly)
			{
				return false;
			}
			
			continue;
		}

		if (!IsCharUpper(str[i]))
		{
			return false;
		}
	}

	return true;                                                    // <<<<<<<<<<<<<<<<<<<<<<<<<<< Is this return logic correct?
}

stock bool IsStringLower(const char[] str, bool lettersOnly=false)  // <<<<<<<<<<<<<<<<<<<<<<<<<<< Is lettersOnly worth it/implemented right?
{
	if (str[0] == '\0')
	{
		return false;                                               // <<<<<<<<<<<<<<<<<<<<<<<<<<< Should empty string return false?
	}

	for (int i = 0; str[i] != '\0'; ++i)
	{
		if (!IsCharAlpha(str[i]))
		{
			if (lettersOnly)
			{
				return false;
			}
			
			continue;
		}

		if (!IsCharLower(str[i]))
		{
			return false;
		}
	}

	return true;                                                    // <<<<<<<<<<<<<<<<<<<<<<<<<<< Is this return logic correct?
}

stock bool IsStringTitle(const char[] str)
{
	if (str[0] == '\0')
	{
		return false;                                               // <<<<<<<<<<<<<<<<<<<<<<<<<<< Should empty string return false?
	}

	bool wasLastCharSpace = true;
	bool isStartOfWord = true;

	for (int i = 0; str[i] != '\0'; ++i)
	{
		bool isSpace = IsCharSpace(str[i]);
		isStartOfWord = wasLastCharSpace && !isSpace;

		if (isStartOfWord && IsCharAlpha(str[i]) && !IsCharUpper(str[i]))
		{
			return false;
		}

		wasLastCharSpace = isSpace;
	}

	return true;
}

stock bool IsStringASCII(const char[] str)
{
	for (int i = 0; str[i] != '\0'; ++i)
	{
		if (str[i] & 0x80)
		{
			return false;
		}
	}

	return true;                                                    // <<<<<<<<<<<<<<<<<<<<<<<<<<< Should empty strings return true?
}

stock bool IsStringUTF8(const char[] str)                           // <<<<<<<<<<<<<<<<<<<<<<<<<<< Is this func needed? And is this correct?
{
	return !IsStringASCII(str);
}

stock bool IsStringAlpha(const char[] str, bool allowSpace=false)   // <<<<<<<<<<<<<<<<<<<<<<<<<<< Is allowSpace a good idea?
{
	if (str[0] == '\0')
	{
		return false;                                               // <<<<<<<<<<<<<<<<<<<<<<<<<<< Should empty string return false?
	}

	for (int i = 0; str[i] != '\0'; ++i)
	{
		if (IsCharSpace(str[i]))
		{
			if (allowSpace)
			{
				continue;
			}
			
			return false;
		}
		
		if (!IsCharAlpha(str[i]))
		{
			return false;
		}
	}

	return true;
}

stock bool IsStringAlphaNumeric(const char[] str, bool allowSpace=false)    // <<<<<<<<<<<<<<<<<<<<<<<<<<< Is allowSpace a good idea?
{
	if (str[0] == '\0')
	{
		return false;                                               // <<<<<<<<<<<<<<<<<<<<<<<<<<< Should empty string return ?
	}

	for (int i = 0; str[i] != '\0'; ++i)
	{
		if (IsCharSpace(str[i]))
		{
			if (allowSpace)
			{
				continue;
			}

			return false;
		}
		
		if (!IsCharAlphaNumeric(str[i]))
		{
			return false;
		}
	}

	return true;
}

stock bool IsCharAlphaNumeric(char c)                               // <<<<<<<<<<<<<<<<<<<<<<<<<<< Is this worth adding? It is cleaner.
{
	return IsCharAlpha(c) || IsCharNumeric(c);
}

stock bool IsCharHex(char c)
{
	if (c >= '0' && c <= '9')
	{
		return true;
	}

	if (c >= 'A' && c <= 'F')
	{
		return true;
	}

	if (c >= 'a' && c <= 'f')
	{
		return true;
	}

	return false;
}

stock bool IsStringHex(const char[] str, bool allowSpace=false) // <<<<<<<<<<<<<<<<<<<<<<<<<<< Is allowSpace a good idea?
{
	if (str[0] == '\0')
	{
		return false;                                           // <<<<<<<<<<<<<<<<<<<<<<<<<<< Should empty string return false
	}

	for (int i = 0; str[i] != '\0'; ++i)
	{
		if (IsCharSpace(str[i]))
		{
			if (allowSpace)
			{
				continue;
			}
			
			return false;
		}

		if (!IsCharHex(str[i]))
		{
			return false;
		}
	}

	return true; 
}

stock bool IsCharVisible(char c, bool allowSpace=false)
{
	// Non-space, non-control ASCII chars
	if (c >= 0x21 && c <= 0x7E)
	{
		return true;
	}
	
	if (IsCharSpace(c))
	{
		return allowSpace;
	}
	
	if (IsCharMB(c))                                                // <<<<<<<<<<<<<<<<<<<<<<<<<<< How [in]correct is this condition?
	{
		return true;
	}

	return false;
}

stock bool IsStringVisible(const char[] str, bool allowSpace=false) // <<<<<<<<<<<<<<<<<<<<<<<<<<< Is allowSpace a good idea?
{
	if (str[0] == '\0')
	{
		return false;
	}

	for (int i = 0; str[i] != '\0'; ++i)
	{
		if (!IsCharVisible(str[i], allowSpace))
		{
			return false;
		}
	}

	return true;
}

stock bool IsStringWhitespace(const char[] str)                     // <<<<<<<<<<<<<<<<<<<<<<<<<<< Should name be IsStringSpace to match IsCharSpace
{
	if (str[0] == '\0')
	{
		return false;                                               // <<<<<<<<<<<<<<<<<<<<<<<<<<< Should empty string return false?
	}

	for (int i = 0; str[i] != '\0'; ++i)
	{
		if (!IsCharSpace(str[i]))
		{
			return false;
		}
	}

	return true;
}

stock bool StringEndsWith(const char[] str, const char[] substring, bool caseSensitive=true)
{
	if (str[0] == '\0')
	{
		return substring[0] == '\0';                                // <<<<<<<<<<<<<<<<<<<<<<<<<<< Does an empty string contain an empty substring?
	}
	
	int len = strlen(str);
	int subLen = strlen(substring);
	
	if (subLen > len)
	{
		return false;
	}

	return strcmp(str[len - subLen], substring, caseSensitive) == 0;
}

stock bool StringStartsWith(const char[] str, const char[] substring, bool caseSensitive=true)
{
	if (str[0] == '\0')
	{
		return substring[0] == '\0';                                // <<<<<<<<<<<<<<<<<<<<<<<<<<< Does an empty string contain an empty substring?
	}

	return StrContains(str, substring, caseSensitive) == 0;
}

stock int TrimStringEx(char[] str, const char[] charset)            // <<<<<<<<<<<<<<<<<<<<<<<<<<< This whole function needs checking. I don't trust it and it looks bad.
{
	int len = strlen(str);
	int start = 0;
	int stop = len - 1;
	
	// Trim left
	for ( ; start < len; ++start)
	{
		if (FindCharInString(charset, str[start]) == -1)
		{
			break;
		}
	}
	
	// No valid characters; trim everything
	if (start == len)
	{
		str[0] = '\0';
		return 0;
	}
	
	// Trim right
	for ( ; stop > start; --stop)
	{
		if (FindCharInString(charset, str[stop]) == -1)
		{
			break;
		}
	}

	len = stop - start + 1;

	// Output untrimmed range
	for (int i = 0; start <= stop; ++i)
	{
		str[i] = str[start++];
	}
	
	str[len] = '\0';
	return len;
}

stock int TrimStringLeft(char[] str)
{
	int len = strlen(str);
	if (!len)
	{
		return 0;
	}

	int index = 0;
	while (index < len)
	{
		if (!IsCharSpace(str[index]))
		{
			break;
		}
		
		++index;
	}
	
	return strcopy(str, len - index, str[index]);
}

stock int TrimStringRight(char[] str)
{
	int len = strlen(str);
	if (!len)
	{
		return 0;
	}

	int index = len - 1;
	while (index >= 0)
	{
		if (!IsCharSpace(str[index]))
		{
			break;
		}
		
		--index;
	}
	
	str[++index] = '\0';
	return index;
}

stock int TrimStringSuffix(char[] str, const char[] suffix, bool caseSensitive=true)
{
	if (!str[0] || !suffix[0])
	{
		return 0;                                                   // <<<<<<<<<<<<<<<<<<<<<<<<<<< Returns 0 if suffix is empty, so non-zero return value means a trim was performed.
	}
	
	int len = strlen(str);
	int suffixLen = strlen(suffix);
	
	if (suffixLen > len)
	{
		return 0;
	}
	
	len -= suffixLen;

	if (strcmp(str[len], suffix, caseSensitive) == 0)
	{
		str[len] = '\0';
		return len;
	}
	
	return 0;
}

stock int TrimStringPrefix(char[] str, const char[] prefix, bool caseSensitive=true)
{
	if (!str[0] || !prefix[0])
	{
		return 0;                                                   // <<<<<<<<<<<<<<<<<<<<<<<<<<< Returns 0 if prefix is empty, so non-zero return value means a trim was performed.
	}
	
	int len = strlen(str);
	int prefixLen = strlen(prefix);
	
	if (prefixLen > len)
	{
		return 0;
	}
	
	if (strncmp(str, prefix, prefixLen, caseSensitive) == 0)
	{
		return strcopy(str, len - prefixLen, str[prefixLen]);
	}
	
	return 0;
}

stock int FilterString(char[] str, const char[] charset)
{
	int writeIndex = 0;
	for (int i = 0; str[i] != '\0'; ++i)
	{
		if (FindCharInString(charset, str[i]) == -1)
		{
			str[writeIndex++] = str[i];
		}
	}
	
	str[writeIndex] = '\0';
	return writeIndex;
}

stock int NormalizeString(char[] str)
{
	int start = 0;
	while (str[start] == '\"' || str[start] == '\'' || IsCharSpace(str[start]))
	{
		++start;
	}

	int end = strlen(str) - 1;
	while (str[end] == '\"' || str[end] == '\'' || IsCharSpace(str[end]))
	{
		--end;
	}

	int writeIndex = 0;
	while (start <= end)
	{
		str[writeIndex++] = CharToLower(str[start]);
		++start;
	}
	
	str[writeIndex] = '\0';
	return writeIndex;
}

stock int TruncateString(const char[] source, char[] dest, int maxlength, const char[] clip="...", bool breakWords=false)
{
	if (maxlength <= 0)
	{
		return 0;
	}
	
	int len = strlen(source);
	if (len < maxlength)
	{
		return strcopy(dest, maxlength, source);
	}
	
	int clipLen = strlen(clip);
	int maxlengthWithClip = maxlength - clipLen - 1;
	
	if (maxlengthWithClip < 0)
	{
		return 0;
	}
	
	if (breakWords)
	{
		len = strcopy(dest, maxlengthWithClip, source);
		len += strcopy(dest[maxlengthWithClip], clipLen, clip);
		return len;
	}
	
	bool wasLastCharWord = false;
	int boundaryIndex = maxlengthWithClip;
	
	for (int i = 0; i <= maxlengthWithClip && source[i] != '\0'; ++i)
	{
		bool isSpace = IsCharSpace(source[i]);
		
		if (wasLastCharWord && isSpace)
		{
			boundaryIndex = i;
		}
		
		wasLastCharWord = !isSpace;
	}

	len = strcopy(dest, boundaryIndex, source);
	len += strcopy(dest[boundaryIndex], clipLen, clip);
	return len;
}

stock void StringToTitle(char[] str)
{
	if (str[0] == '\0')
	{
		return;
	}

	bool wasLastCharSpace = true;

	for (int i = 0; str[i] != '\0'; ++i)
	{
		bool isSpace = IsCharSpace(str[i]);

		if (wasLastCharSpace && !isSpace)
		{
			str[i] = CharToUpper(str[i]);
		}
		
		wasLastCharSpace = isSpace;
	}
}

stock void StringLeftPad(char[] str, int maxlength, char padding=' ')
{
	int len = strlen(str);
	int indexOffset = maxlength - len - 1;
	
	if (indexOffset < 0)
	{
		indexOffset = 0;
	}
	
	strcopy(str[indexOffset], len + 1, str);
	for (int i = 0; i < indexOffset; ++i)
	{
		str[i] = padding;
	}
}

stock void StringRightPad(char[] str, int maxlength, char padding=' ')
{
	int len = strlen(str);
	
	for ( ; len < maxlength; ++len)
	{
		str[len] = padding;
	}
	
	str[len] = '\0';
}

stock void StringCenterPad(char[] str, int maxlength, char padding=' ')
{
	int len = strlen(str);
	if (len >= maxlength)
	{
		return;
	}

	int available = maxlength - len;
	int left = available / 2;
	
	strcopy(str[left], len, str);
	
	for (int i = 0; i < left; ++i)
	{
		str[i] = padding;
	}
	
	for (int i = left + len; i < maxlength; ++i)
	{
		str[i] = padding;
	}
	
	str[maxlength] = '\0';
}

stock int StringRepeat(const char[] source, int count, char[] buffer, int maxlength, bool truncate=true)
{
	int writeIndex = 0;
	int len = strlen(source);
	if (!len || count < 1)
	{
		buffer[0] = '\0';
		return 0;
	}
	   
	for (int i = 0; i < count && writeIndex < maxlength; ++i)
	{      
		if (!truncate)
		{
			int charsRemaining = maxlength - writeIndex;
			if (charsRemaining < len)
			{
				break;
			}
		}
		
		for (int j = 0; j < len && writeIndex < maxlength; ++j)
		{
			buffer[writeIndex++] = source[j];
		}
	}
	
	buffer[writeIndex] = '\0';
	return writeIndex;
}

stock char CharAt(char c)                                               // <<<<<<<<<<<<<<<<<<<<<<<<<<< Is this function worth adding
{
	return c;
}

stock int strncopy(char[] dest, int destLen, const char[] source, int n)  // <<<<<<<<<<<<<<<<<<<<<<<<<<< idk if this works at all and i even tested it
{
	if (destLen <= 0 || n <= 0)
	{
		return 0;
	}
	
	if (n >= destLen)
	{
		n = destLen - 1;
	}
	
	// Source string might be shorter than n, so use i to set terminator
	int i = 0
	for ( ; source[i] != '\0' && i < n; ++i)
	{
		dest[i] = source[i];
	}
	
	if (i > destLen)
	{
		i = destLen;
	}
	
	dest[i] = '\0';
	return i;
}

stock bool StringToIntStrict(const char[] str, int &result, int nBase=10)
{
	int len = strlen(str);
	if (len == 0)
	{
		return false;
	}
	
	int temp;
	if (StringToIntEx(str, temp, nBase) != len)
	{
		return false;
	}
	result = temp;
	return true;
}

stock bool StringToFloatStrict(const char[] str, float &result)
{
	int len = strlen(str);
	if (len == 0)
	{
		return false;
	}

	float temp;
	if (StringToFloatEx(str, temp) != len)
	{
		return false;
	}
	result = temp;
	return true;
}

stock ArrayList ExplodeStringToList(const char[] text, const char[] split, int maxStringLength, bool copyRemainder=false, int blockSize=0)
{
	ArrayList list;
	if (blockSize <= 0)
	{
		list = new ArrayList(ByteCountToCells(maxStringLength));
	}
	else
	{
		list = new ArrayList(blockSize);
	}
	
	char[] buffer = new char[maxStringLength];
	int index;
	
	if (split[0])
	{
		while ((index += SplitString(text[index], split, buffer, maxStringLength)) != -1)
		{
			list.PushString(buffer);
		}
	}
	
	if (copyRemainder) 
	{
		list.PushString(text[index]);                               // <<<<<<<<<<<<<<<<<<<<<<<<<<< Does this work as expected? Does any of this function?
	}

	return list;
}

stock int SubstringToInt(const char[] str, int length, int nBase=10) // <<<<<<<<<<<<<<<<<<<<<<<<<<< StringToInt with a length param, so it can specify any substring.
{
	int len = strlen(str);
	if (length <= 0 || len == 0)
	{
		return 0;
	}
	
	if (length > len)
	{
		length = len;
	}

	char[] substring = new char[length + 1];
	strcopy(substring, length + 1, str);
	
	return StringToInt(substring, nBase);
}

stock int SubstringToIntEx(const char[] str, int length, int &result, int nBase=10)
{
	int len = strlen(str);
	if (length <= 0 || len == 0)
	{
		return 0;
	}
	
	if (length > len)
	{
		length = len;
	}

	char[] substring = new char[length + 1];
	strcopy(substring, length + 1, str);
	
	return StringToIntEx(substring, result, nBase);
}

stock float SubstringToFloat(const char[] str, int length)
{
	int len = strlen(str);
	if (length <= 0 || len == 0)
	{
		return 0.0;
	}
	
	if (length > len)
	{
		length = len;
	}

	char[] substring = new char[length + 1];
	strcopy(substring, length + 1, str);
	
	return StringToFloat(substring);
}

stock int SubstringToFloatEx(const char[] str, int length, float &result)
{
	int len = strlen(str);
	if (length <= 0 || len == 0)
	{
		return 0;
	}
	
	if (length > len)
	{
		length = len;
	}

	char[] substring = new char[length + 1];
	strcopy(substring, length + 1, str);
	
	return StringToFloatEx(substring, result);
}

stock int SubstringCount(const char[] str, const char[] substring, bool caseSensitive=true)
{
	int count = 0;
	int index = 0;

	while ((index += StrContains(str[index], substring, caseSensitive)) != -1)
	{
		++count;
	}

	return count;
}
