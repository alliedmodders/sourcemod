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

#if defined _regex_included
  #endinput
#endif
#define _regex_included

/**
 * @section 	Flags for compiling regex expressions.  These come directly from the 
 * pcre library and can be used in MatchRegex and CompileRegex.
 */
#define PCRE_CASELESS           0x00000001 /* Ignore Case */
#define PCRE_MULTILINE          0x00000002 /* Multilines (affects ^ and $ so that they match the start/end of a line rather than matching the start/end of the string). */
#define PCRE_DOTALL             0x00000004 /* Single line (affects . so that it matches any character, even new line characters). */
#define PCRE_EXTENDED           0x00000008 /* Pattern extension (ignore whitespace and # comments). */
#define PCRE_UNGREEDY           0x00000200 /* Invert greediness of quantifiers */
#define PCRE_UTF8               0x00000800 /* Use UTF-8 Chars */
#define PCRE_NO_UTF8_CHECK      0x00002000 /* Do not check the pattern for UTF-8 validity (only relevant if PCRE_UTF8 is set) */


/**
 * Regex expression error codes.
 */
enum RegexError
{
	REGEX_ERROR_NONE = 0,				/* No error */
	REGEX_ERROR_NOMATCH = -1,			/* No match was found */
	REGEX_ERROR_NULL = -2,
	REGEX_ERROR_BADOPTION = -3,
	REGEX_ERROR_BADMAGIC = -4,
	REGEX_ERROR_UNKNOWN_OPCODE = -5,
	REGEX_ERROR_NOMEMORY = -6,
	REGEX_ERROR_NOSUBSTRING = -7,
	REGEX_ERROR_MATCHLIMIT = -8,
	REGEX_ERROR_CALLOUT = -9,  			/* Never used by PCRE itself */
	REGEX_ERROR_BADUTF8 = -10,
	REGEX_ERROR_BADUTF8_OFFSET = -11,
	REGEX_ERROR_PARTIAL = -12,
	REGEX_ERROR_BADPARTIAL = -13,
	REGEX_ERROR_INTERNAL = -14,
	REGEX_ERROR_BADCOUNT = -15,
	REGEX_ERROR_DFA_UITEM = -16,
	REGEX_ERROR_DFA_UCOND = -17,
	REGEX_ERROR_DFA_UMLIMIT = -18,
	REGEX_ERROR_DFA_WSSIZE = -19,
	REGEX_ERROR_DFA_RECURSE = -20,
	REGEX_ERROR_RECURSIONLIMIT = -21,
	REGEX_ERROR_NULLWSLIMIT = -22,  	/* No longer actually used */
	REGEX_ERROR_BADNEWLINE = -23
};

/**
 * Precompile a regular expression.  Use this if you intend on using the 
 * same expression multiple times.  Pass the regex handle returned here to
 * MatchRegex to check for matches.
 *
 * @param pattern		The regular expression pattern.
 * @param flags			General flags for the regular expression.
 * @param error			Error message encountered, if applicable.
 * @param maxLen		Maximum string length of the error buffer.
 * @param errcode		Regex type error code encountered, if applicable.
 * @return				Valid regex handle on success, INVALID_HANDLE on failure.
 */
native Handle:CompileRegex(const String:pattern[], flags = 0, String:error[]="", maxLen = 0, &RegexError:errcode = REGEX_ERROR_NONE);

/**
 * Matches a string against a pre-compiled regular expression pattern.
 *
 * @param str			The string to check.
 * @param regex			Regex Handle from CompileRegex()
 * @param ret			Error code, if applicable.
 * @return				Number of substrings found or -1 on failure.
 *
 * @note				Use the regex handle passed to this function to extract
 *						matches with GetRegexSubString().
 */
native MatchRegex(Handle:regex, const String:str[], &RegexError:ret = REGEX_ERROR_NONE);

/**
 * Returns a matched substring from a regex handle.
 * Substring ids start at 0 and end at substrings-1, where substrings is the number returned 
 * by MatchRegex
 *
 * @param regex			The regex handle to extract data from.
 * @param str_id		The index of the expression to get - starts at 0, and ends at substrings - 1.
 * @param buffer		The buffer to set to the matching substring.
 * @param maxLen		The maximum string length of the buffer.
 * @return				True if a substring was found, False on fail/error
 */
native bool:GetRegexSubString(Handle:regex, str_id, String:buffer[], maxlen);

/**
 * Matches a string against a regular expression pattern.
 *
 * @note				If you intend on using the same regular expression pattern
 *						multiple times, consider using CompileRegex and MatchRegex 
 *						instead of making this function reparse the expression each time.
 *
 * @param str			The string to check.
 * @param pattern		The regular expression pattern.
 * @param flags			General flags for the regular expression.
 * @param error			Error message, if applicable.
 * @param maxLen		Maximum length of the error buffer.
 * @return				Number of substrings found or -1 on failure.
 */
stock SimpleRegexMatch(const String:str[], const String:pattern[], flags = 0, String:error[]="", maxLen = 0)
{
	new Handle:regex = CompileRegex(pattern, flags, error, maxLen);
	
	if (regex == INVALID_HANDLE)
	{
		return -1;	
	}
	
	new substrings = MatchRegex(regex, str);
	
	CloseHandle(regex);
	
	return substrings;	
}

/**
 * @endsection
 */

/**
 * Do not edit below this line!
 */
public Extension:__ext_regex = 
{
	name = "Regex Extension",
	file = "regex.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};
