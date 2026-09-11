#ifndef _INCLUDE_PCRECOMPAT_H
#define _INCLUDE_PCRECOMPAT_H

#include "pcre2.h"

// The following are copied from scripting/include/regex.inc
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
	REGEX_ERROR_UNKNOWN_OPCODE = -5,	/* No equivalent exists in PCRE2 */
	REGEX_ERROR_NOMEMORY = -6,
	REGEX_ERROR_NOSUBSTRING = -7,
	REGEX_ERROR_MATCHLIMIT = -8,
	REGEX_ERROR_CALLOUT = -9,           /* Never used by PCRE itself */
	REGEX_ERROR_BADUTF8 = -10,
	REGEX_ERROR_BADUTF8_OFFSET = -11,
	REGEX_ERROR_PARTIAL = -12,
	REGEX_ERROR_BADPARTIAL = -13,		/* Unused since PCRE1 8 */
	REGEX_ERROR_INTERNAL = -14,
	REGEX_ERROR_BADCOUNT = -15,			/* Unused now that ovecsize is a uint32_t in PCRE2 */
	REGEX_ERROR_DFA_UITEM = -16,
	REGEX_ERROR_DFA_UCOND = -17,
	REGEX_ERROR_DFA_UMLIMIT = -18,		/* No equivalent exists in PCRE2 */
	REGEX_ERROR_DFA_WSSIZE = -19,
	REGEX_ERROR_DFA_RECURSE = -20,
	REGEX_ERROR_RECURSIONLIMIT = -21,
	REGEX_ERROR_NULLWSLIMIT = -22,      /* No longer actually used */
	REGEX_ERROR_BADNEWLINE = -23,		/* No equivalent exists in PCRE2 */
	REGEX_ERROR_BADOFFSET = -24,
	REGEX_ERROR_SHORTUTF8 = -25,		/* No equivalent exists in PCRE2 (?) */
	REGEX_ERROR_RECURSELOOP = -26,
	REGEX_ERROR_JIT_STACKLIMIT = -27,
	REGEX_ERROR_BADMODE = -28,
	REGEX_ERROR_BADENDIANNESS = -29,	/* No equivalent exists in PCRE2 */
	REGEX_ERROR_DFA_BADRESTART = -30,
	REGEX_ERROR_JIT_BADOPTION = -31,
	REGEX_ERROR_BADLENGTH = -32,			/* Unused now that length is a size_t in PCRE2 */

	/* New in PCRE2 */
	REGEX_ERROR_BADDATA = -33,
	REGEX_ERROR_MIXEDTABLES = -34,
	REGEX_ERROR_BADREPLACEMENT = -35,
	REGEX_ERROR_DFA_UFUNC = -36,
	REGEX_ERROR_NOUNIQUESUBSTRING = -37,
	REGEX_ERROR_UNAVAILABLE = -38,
	REGEX_ERROR_UNSET = -39,
	REGEX_ERROR_BADOFFSETLIMIT = -40,
	REGEX_ERROR_BADREPESCAPE = -41,
	REGEX_ERROR_REPMISSINGBRACE = -42,
	REGEX_ERROR_BADSUBSTITUTION = -43,
	REGEX_ERROR_BADSUBSPATTERN = -44,
	REGEX_ERROR_TOOMANYREPLACE = -45,
	REGEX_ERROR_BADSERIALIZEDDATA = -46,
	REGEX_ERROR_HEAPLIMIT = -47,
	REGEX_ERROR_CONVERT_SYNTAX = -48,
	REGEX_ERROR_DFA_UINVALID_UTF = -49,
	REGEX_ERROR_INVALIDOFFSET = -50,
	REGEX_ERROR_JIT_UNSUPPORTED = -51,
	REGEX_ERROR_REPLACECASE = -52,
	REGEX_ERROR_TOOLARGEREPLACE = -53,
	REGEX_ERROR_DIFFSUBSPATTERN = -54,
	REGEX_ERROR_DIFFSUBSSUBJECT = -55,
	REGEX_ERROR_DIFFSUBSOFFSET = -56,
	REGEX_ERROR_DIFFSUBSOPTIONS = -57,
	REGEX_ERROR_BAD_BACKSLASH_K = -58,
	REGEX_ERROR_PARTIALSUBS = -59
};

inline uint32_t PCREOptionsToPCRE2Options(int options)
{
	uint32_t result = 0;
	if (options & PCRE_CASELESS)
	{
		result |= PCRE2_CASELESS;
	}
	if (options & PCRE_MULTILINE)
	{
		result |= PCRE2_MULTILINE;
	}
	if (options & PCRE_DOTALL)
	{
		result |= PCRE2_DOTALL;
	}
	if (options & PCRE_EXTENDED)
	{
		result |= PCRE2_EXTENDED;
	}
	if (options & PCRE_ANCHORED)
	{
		result |= PCRE2_ANCHORED;
	}
	if (options & PCRE_DOLLAR_ENDONLY)
	{
		result |= PCRE2_DOLLAR_ENDONLY;
	}
	if (options & PCRE_UNGREEDY)
	{
		result |= PCRE2_UNGREEDY;
	}
	if (options & PCRE_NOTEMPTY)
	{
		result |= PCRE2_NOTEMPTY;
	}
	if (options & PCRE_UTF8)
	{
		result |= PCRE2_UTF;
	}
	if (options & PCRE_NO_UTF8_CHECK)
	{
		result |= PCRE2_NO_UTF_CHECK;
	}
	if (options & PCRE_UCP)
	{
		result |= PCRE2_UCP;
	}
	return result;
}

inline RegexError PCRE2ErrorToRegexError(int pcre2Error)
{
	switch (pcre2Error)
	{
	case PCRE2_ERROR_NOMATCH:
		return REGEX_ERROR_NOMATCH;
	case PCRE2_ERROR_NULL:
		return REGEX_ERROR_NULL;
	case PCRE2_ERROR_BADOPTION:
		return REGEX_ERROR_BADOPTION;
	case PCRE2_ERROR_BADMAGIC:
		return REGEX_ERROR_BADMAGIC;
	case PCRE2_ERROR_NOMEMORY:
		return REGEX_ERROR_NOMEMORY;
	case PCRE2_ERROR_NOSUBSTRING:
		return REGEX_ERROR_NOSUBSTRING;
	case PCRE2_ERROR_MATCHLIMIT:
		return REGEX_ERROR_MATCHLIMIT;
	case PCRE2_ERROR_CALLOUT:
		return REGEX_ERROR_CALLOUT;
	case PCRE2_ERROR_UTF8_ERR1:
	case PCRE2_ERROR_UTF8_ERR2:
	case PCRE2_ERROR_UTF8_ERR3:
	case PCRE2_ERROR_UTF8_ERR4:
	case PCRE2_ERROR_UTF8_ERR5:
	case PCRE2_ERROR_UTF8_ERR6:
	case PCRE2_ERROR_UTF8_ERR7:
	case PCRE2_ERROR_UTF8_ERR8:
	case PCRE2_ERROR_UTF8_ERR9:
	case PCRE2_ERROR_UTF8_ERR10:
	case PCRE2_ERROR_UTF8_ERR11:
	case PCRE2_ERROR_UTF8_ERR12:
	case PCRE2_ERROR_UTF8_ERR13:
	case PCRE2_ERROR_UTF8_ERR14:
	case PCRE2_ERROR_UTF8_ERR15:
	case PCRE2_ERROR_UTF8_ERR16:
	case PCRE2_ERROR_UTF8_ERR17:
	case PCRE2_ERROR_UTF8_ERR18:
	case PCRE2_ERROR_UTF8_ERR19:
	case PCRE2_ERROR_UTF8_ERR20:
	case PCRE2_ERROR_UTF8_ERR21:
	case PCRE2_ERROR_DFA_UINVALID_UTF:
		return REGEX_ERROR_BADUTF8;
	case PCRE2_ERROR_BADUTFOFFSET:
		return REGEX_ERROR_BADUTF8_OFFSET;
	case PCRE2_ERROR_PARTIAL:
		return REGEX_ERROR_PARTIAL;
	case PCRE2_ERROR_INTERNAL:
	case PCRE2_ERROR_INTERNAL_DUPMATCH:
		return REGEX_ERROR_INTERNAL;
	case PCRE2_ERROR_DFA_UITEM:
		return REGEX_ERROR_DFA_UITEM;
	case PCRE2_ERROR_DFA_UCOND:
		return REGEX_ERROR_DFA_UCOND;
	case PCRE2_ERROR_DFA_WSSIZE:
		return REGEX_ERROR_DFA_WSSIZE;
	case PCRE2_ERROR_DFA_RECURSE:
		return REGEX_ERROR_DFA_RECURSE;
	case PCRE2_ERROR_RECURSIONLIMIT:
		return REGEX_ERROR_RECURSIONLIMIT;
	case PCRE2_ERROR_BADOFFSET:
		return REGEX_ERROR_BADOFFSET;
	case PCRE2_ERROR_RECURSELOOP:
		return REGEX_ERROR_RECURSELOOP;
	case PCRE2_ERROR_JIT_STACKLIMIT:
		return REGEX_ERROR_JIT_STACKLIMIT;
	case PCRE2_ERROR_BADMODE:
		return REGEX_ERROR_BADMODE;
	case PCRE2_ERROR_DFA_BADRESTART:
		return REGEX_ERROR_DFA_BADRESTART;
	case PCRE2_ERROR_JIT_BADOPTION:
		return REGEX_ERROR_JIT_BADOPTION;
	case PCRE2_ERROR_BADDATA:
		return REGEX_ERROR_BADDATA;
	case PCRE2_ERROR_MIXEDTABLES:
		return REGEX_ERROR_MIXEDTABLES;
	case PCRE2_ERROR_BADREPLACEMENT:
		return REGEX_ERROR_BADREPLACEMENT;
	case PCRE2_ERROR_DFA_UFUNC:
		return REGEX_ERROR_DFA_UFUNC;
	case PCRE2_ERROR_NOUNIQUESUBSTRING:
		return REGEX_ERROR_NOUNIQUESUBSTRING;
	case PCRE2_ERROR_UNAVAILABLE:
		return REGEX_ERROR_UNAVAILABLE;
	case PCRE2_ERROR_UNSET:
		return REGEX_ERROR_UNSET;
	case PCRE2_ERROR_BADOFFSETLIMIT:
		return REGEX_ERROR_BADOFFSETLIMIT;
	case PCRE2_ERROR_BADREPESCAPE:
		return REGEX_ERROR_BADREPESCAPE;
	case PCRE2_ERROR_REPMISSINGBRACE:
		return REGEX_ERROR_REPMISSINGBRACE;
	case PCRE2_ERROR_BADSUBSTITUTION:
		return REGEX_ERROR_BADSUBSTITUTION;
	case PCRE2_ERROR_BADSUBSPATTERN:
		return REGEX_ERROR_BADSUBSPATTERN;
	case PCRE2_ERROR_TOOMANYREPLACE:
		return REGEX_ERROR_TOOMANYREPLACE;
	case PCRE2_ERROR_BADSERIALIZEDDATA:
		return REGEX_ERROR_BADSERIALIZEDDATA;
	case PCRE2_ERROR_HEAPLIMIT:
		return REGEX_ERROR_HEAPLIMIT;
	case PCRE2_ERROR_CONVERT_SYNTAX:
		return REGEX_ERROR_CONVERT_SYNTAX;
	case PCRE2_ERROR_DFA_UINVALID_UTF:
		return REGEX_ERROR_DFA_UINVALID_UTF;
	case PCRE2_ERROR_INVALIDOFFSET:
		return REGEX_ERROR_INVALIDOFFSET;
	case PCRE2_ERROR_JIT_UNSUPPORTED:
		return REGEX_ERROR_JIT_UNSUPPORTED;
	case PCRE2_ERROR_REPLACECASE:
		return REGEX_ERROR_REPLACECASE;
	case PCRE2_ERROR_TOOLARGEREPLACE:
		return REGEX_ERROR_TOOLARGEREPLACE;
	case PCRE2_ERROR_DIFFSUBSPATTERN:
		return REGEX_ERROR_DIFFSUBSPATTERN;
	case PCRE2_ERROR_DIFFSUBSSUBJECT:
		return REGEX_ERROR_DIFFSUBSSUBJECT;
	case PCRE2_ERROR_DIFFSUBSOFFSET:
		return REGEX_ERROR_DIFFSUBSOFFSET;
	case PCRE2_ERROR_DIFFSUBSOPTIONS:
		return REGEX_ERROR_DIFFSUBSOPTIONS;
	case PCRE2_ERROR_BAD_BACKSLASH_K:
		return REGEX_ERROR_BAD_BACKSLASH_K;
	case PCRE2_ERROR_PARTIALSUBS:
		return REGEX_ERROR_PARTIALSUBS;
	default:
		return REGEX_ERROR_INTERNAL;
	}
}

#endif //_INCLUDE_PCRECOMPAT_H
