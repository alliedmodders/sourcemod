/**
 * vim: set ts=4 sw=4 tw=99 noet :
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
 * @section     Flags for compiling regex expressions.  These come directly from the
 * pcre library and can be used in MatchRegex and CompileRegex.
 */
#define PCRE_CASELESS           0x00000001 /* Ignore Case */
#define PCRE_MULTILINE          0x00000002 /* Multilines (affects ^ and $ so that they match the start/end of a line rather than matching the start/end of the string). */
#define PCRE_DOTALL             0x00000004 /* Single line (affects . so that it matches any character, even new line characters). */
#define PCRE_EXTENDED           0x00000008 /* Pattern extension (ignore whitespace and # comments). */
#define PCRE_ANCHORED           0x00000010 /* Force pattern anchoring. */
#define PCRE_DOLLAR_ENDONLY     0x00000020 /* $ not to match newline at end. */
#define PCRE_UNGREEDY           0x00000200 /* Invert greediness of quantifiers */
#define PCRE_NOTEMPTY           0x00000400 /* An empty string is not a valid match. */
#define PCRE_UTF8               0x00000800 /* Use UTF-8 Chars */
#define PCRE_NO_UTF8_CHECK      0x00002000 /* Do not check the pattern for UTF-8 validity (only relevant if PCRE_UTF8 is set) */
#define PCRE_UCP                0x20000000 /* Use Unicode properties for \ed, \ew, etc. */


/**
 * Regex expression error codes.
 */
enum RegexError
{
	REGEX_ERROR_NONE = 0,               /* No error */

	REGEX_ERROR_ASSERT = 1,             /* internal error ? */
	REGEX_ERROR_BADBR,                  /* invalid repeat counts in {} */
	REGEX_ERROR_BADPAT,                 /* pattern error */
	REGEX_ERROR_BADRPT,                 /* ? * + invalid */
	REGEX_ERROR_EBRACE,                 /* unbalanced {} */
	REGEX_ERROR_EBRACK,                 /* unbalanced [] */
	REGEX_ERROR_ECOLLATE,               /* collation error - not relevant */
	REGEX_ERROR_ECTYPE,                 /* bad class */
	REGEX_ERROR_EESCAPE,                /* bad escape sequence */
	REGEX_ERROR_EMPTY,                  /* empty expression */
	REGEX_ERROR_EPAREN,                 /* unbalanced () */
	REGEX_ERROR_ERANGE,                 /* bad range inside [] */
	REGEX_ERROR_ESIZE,                  /* expression too big */
	REGEX_ERROR_ESPACE,                 /* failed to get memory */
	REGEX_ERROR_ESUBREG,                /* bad back reference */
	REGEX_ERROR_INVARG,                 /* bad argument */

	REGEX_ERROR_NOMATCH = -1,           /* No match was found */
	REGEX_ERROR_NULL = -2,
	REGEX_ERROR_BADOPTION = -3,
	REGEX_ERROR_BADMAGIC = -4,
	REGEX_ERROR_UNKNOWN_OPCODE = -5,
	REGEX_ERROR_NOMEMORY = -6,
	REGEX_ERROR_NOSUBSTRING = -7,
	REGEX_ERROR_MATCHLIMIT = -8,
	REGEX_ERROR_CALLOUT = -9,           /* Never used by PCRE itself */
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
	REGEX_ERROR_NULLWSLIMIT = -22,      /* No longer actually used */
	REGEX_ERROR_BADNEWLINE = -23,
	REGEX_ERROR_BADOFFSET = -24,
	REGEX_ERROR_SHORTUTF8 = -25,
	REGEX_ERROR_RECURSELOOP = -26,
	REGEX_ERROR_JIT_STACKLIMIT = -27,
	REGEX_ERROR_BADMODE = -28,
	REGEX_ERROR_BADENDIANNESS = -29,
	REGEX_ERROR_DFA_BADRESTART = -30,
	REGEX_ERROR_JIT_BADOPTION = -31,
	REGEX_ERROR_BADLENGTH = -32
};

// Regular expression objects are used to match or decompose strings based on
// patterns.
methodmap Regex < Handle
{
	// Compile a regular expression.
	//
	// @param pattern       The regular expression pattern.
	// @param flags         General flags for the regular expression.
	// @param error         Error message encountered, if applicable.
	// @param maxLen        Maximum string length of the error buffer.
	// @param errcode       Regex type error code encountered, if applicable.
	public native Regex(const char[] pattern, int flags = 0, char[] error="", int maxLen = 0, RegexError &errcode = REGEX_ERROR_NONE);

	// Matches a string against a pre-compiled regular expression pattern.
	//
	// @param str           The string to check.
	// @param ret           Error code, if applicable.
	// @param offset        Offset in the string to start searching from. MatchOffset returns the offset of the match.
	// @return              Number of captures found or -1 on failure.
	//
	// @note Use the regex handle passed to this function to extract
	//       matches with GetSubString().
	public native int Match(const char[] str, RegexError &ret = REGEX_ERROR_NONE, int offset = 0);

	// Gets all matches from a string against a pre-compiled regular expression pattern.
	//
	// @param str           The string to check.
	// @param ret           Error code, if applicable.
	// @return              Number of matches found or -1 on failure.
	//
	// @note Use GetSubString() and loop from 0 -> totalmatches - 1.
	public native int MatchAll(const char[] str, RegexError &ret = REGEX_ERROR_NONE);

	// Returns a matched substring from a regex handle.
	//
	// Substring ids start at 0 and end at captures-1, where captures is the
	// number returned by Regex.Match or Regex.CaptureCount.
	//
	// @param str_id        The index of the expression to get - starts at 0, and ends at captures - 1.
	// @param buffer        The buffer to set to the matching substring.
	// @param maxlen        The maximum string length of the buffer.
	// @param match         Match to get the captures for - starts at 0, and ends at MatchCount() -1
	// @return              True if a substring was found, False on fail/error
	//
	// @note str_id = 0 is the full captured string, anything else is the capture group index.
	//       if Regex.Match is used match can only be 0
	public native bool GetSubString(int str_id, char[] buffer, int maxlen, int match = 0);

	// Returns number of matches
	//
	// When using Match this is always 1 or 0 (unless an error occured)
	// @return              Total number of matches found.
	public native int MatchCount();

	// Returns number of captures for a match
	//
	// @param match         Match to get the number of captures for. Match starts at 0, and ends at MatchCount() -1
	// @return              Number of captures in the match.
	//
	// @note Use GetSubString() and loop from 1 -> captures -1 for str_id to get all captures
	public native int CaptureCount(int match = 0);

	// Returns the string offset of a match.
	//
	// @param match         Match to get the offset of. Match starts at 0, and ends at MatchCount() -1
	// @return              Offset of the match in the string.
	public native int MatchOffset(int match = 0);
};

/**
 * Precompile a regular expression.  Use this if you intend on using the
 * same expression multiple times.  Pass the regex handle returned here to
 * MatchRegex to check for matches.
 *
 * @param pattern       The regular expression pattern.
 * @param flags         General flags for the regular expression.
 * @param error         Error message encountered, if applicable.
 * @param maxLen        Maximum string length of the error buffer.
 * @param errcode       Regex type error code encountered, if applicable.
 * @return              Valid regex handle on success, INVALID_HANDLE on failure.
 */
native Regex CompileRegex(const char[] pattern, int flags = 0, char[] error="", int maxLen = 0, RegexError &errcode = REGEX_ERROR_NONE);

/**
 * Matches a string against a pre-compiled regular expression pattern.
 *
 * @param regex         Regex Handle from CompileRegex()
 * @param str           The string to check.
 * @param ret           Error code, if applicable.
 * @param offset        Offset in the string to start searching from.
 * @return              Number of captures found or -1 on failure.
 *
 * @note Use the regex handle passed to this function to extract
 *       matches with GetRegexSubString().
 */
native int MatchRegex(Handle regex, const char[] str, RegexError &ret = REGEX_ERROR_NONE, int offset = 0);

/**
 * Returns a matched substring from a regex handle.
 * Substring ids start at 0 and end at captures-1, where captures is the number returned
 * by MatchRegex.
 *
 * @param regex         The regex handle to extract data from.
 * @param str_id        The index of the expression to get - starts at 0, and ends at captures - 1.
 * @param buffer        The buffer to set to the matching substring.
 * @param maxlen        The maximum string length of the buffer.
 * @return              True if a substring was found, False on fail/error
 *
 * @note str_id = 0 is the full captured string, anything else is the capture group index.
 *
 */
native bool GetRegexSubString(Handle regex, int str_id, char[] buffer, int maxlen);

/**
 * Matches a string against a regular expression pattern.
 *
 * @note If you intend on using the same regular expression pattern
 *       multiple times, consider using CompileRegex and MatchRegex
 *       instead of making this function reparse the expression each time.
 *
 * @param str           The string to check.
 * @param pattern       The regular expression pattern.
 * @param flags         General flags for the regular expression.
 * @param error         Error message, if applicable.
 * @param maxLen        Maximum length of the error buffer.
 * @return              Number of substrings found or -1 on failure.
 */
stock int SimpleRegexMatch(const char[] str, const char[] pattern, int flags = 0, char[] error="", int maxLen = 0)
{
	Regex regex = new Regex(pattern, flags, error, maxLen);
	if (!regex)
	{
		return -1;
	}

	int substrings = regex.Match(str);
	delete regex;

	return substrings;
}

/**
 * @endsection
 */

/**
 * Do not edit below this line!
 */
public Extension __ext_regex =
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

#if !defined REQUIRE_EXTENSIONS
public void __ext_regex_SetNTVOptional()
{
	MarkNativeAsOptional("CompileRegex");
	MarkNativeAsOptional("MatchRegex");
	MarkNativeAsOptional("GetRegexSubString");
	MarkNativeAsOptional("Regex.Regex");
	MarkNativeAsOptional("Regex.Match");
	MarkNativeAsOptional("Regex.MatchAll");
	MarkNativeAsOptional("Regex.GetSubString");
	MarkNativeAsOptional("Regex.MatchCount");
	MarkNativeAsOptional("Regex.CaptureCount");
	MarkNativeAsOptional("Regex.MatchOffset");
}
#endif
