#pragma once

/*
	Maps pcre_compile2 error codes to posix error codes.
	From pcre2posix.c and pcre2posix.h
*/

// posix error codes
enum {
	REG_ASSERT = 1,  /* internal error ? */
	REG_BADBR,       /* invalid repeat counts in {} */
	REG_BADPAT,      /* pattern error */
	REG_BADRPT,      /* ? * + invalid */
	REG_EBRACE,      /* unbalanced {} */
	REG_EBRACK,      /* unbalanced [] */
	REG_ECOLLATE,    /* collation error - not relevant */
	REG_ECTYPE,      /* bad class */
	REG_EESCAPE,     /* bad escape sequence */
	REG_EMPTY,       /* empty expression */
	REG_EPAREN,      /* unbalanced () */
	REG_ERANGE,      /* bad range inside [] */
	REG_ESIZE,       /* expression too big */
	REG_ESPACE,      /* failed to get memory */
	REG_ESUBREG,     /* bad back reference */
	REG_INVARG,      /* bad argument */

	// This isnt used below since it is not a compile error. So we remove it as to not conflict.
	//REG_NOMATCH      /* match failed */
};

// pcre compile error -> posix compile error
static const int eint1[] = {
	0,           /* No error */
	REG_EESCAPE, /* \ at end of pattern */
	REG_EESCAPE, /* \c at end of pattern */
	REG_EESCAPE, /* unrecognized character follows \ */
	REG_BADBR,   /* numbers out of order in {} quantifier */
	/* 5 */
	REG_BADBR,   /* number too big in {} quantifier */
	REG_EBRACK,  /* missing terminating ] for character class */
	REG_ECTYPE,  /* invalid escape sequence in character class */
	REG_ERANGE,  /* range out of order in character class */
	REG_BADRPT,  /* nothing to repeat */
	/* 10 */
	REG_ASSERT,  /* internal error: unexpected repeat */
	REG_BADPAT,  /* unrecognized character after (? or (?- */
	REG_BADPAT,  /* POSIX named classes are supported only within a class */
	REG_BADPAT,  /* POSIX collating elements are not supported */
	REG_EPAREN,  /* missing ) */
	/* 15 */
	REG_ESUBREG, /* reference to non-existent subpattern */
	REG_INVARG,  /* pattern passed as NULL */
	REG_INVARG,  /* unknown compile-time option bit(s) */
	REG_EPAREN,  /* missing ) after (?# comment */
	REG_ESIZE,   /* parentheses nested too deeply */
	/* 20 */
	REG_ESIZE,   /* regular expression too large */
	REG_ESPACE,  /* failed to get memory */
	REG_EPAREN,  /* unmatched closing parenthesis */
	REG_ASSERT   /* internal error: code overflow */
};

static const int eint2[] = {
	30, REG_ECTYPE,  /* unknown POSIX class name */
	32, REG_INVARG,  /* this version of PCRE2 does not have Unicode support */
	37, REG_EESCAPE, /* PCRE2 does not support \L, \l, \N{name}, \U, or \u */
	56, REG_INVARG,  /* internal error: unknown newline setting */
	92, REG_INVARG,  /* invalid option bits with PCRE2_LITERAL */
	98, REG_EESCAPE, /* missing digit after \0 in NO_BS0 mode */
	99, REG_EESCAPE, /* \K in lookaround */
	102, REG_EESCAPE  /* \ddd octal > \377 in PYTHON_OCTAL mode */
};

inline int PCRE2ErrorToPosixError(int errorcode)
{
	static constexpr int COMPILE_ERROR_BASE = 100;

	if (errorcode < COMPILE_ERROR_BASE)
	{
		return REG_BADPAT;
	}

	errorcode -= COMPILE_ERROR_BASE;

	if (errorcode < (int)(sizeof(eint1)/sizeof(const int)))
	{
		return eint1[errorcode];
	}
	for (int i = 0; i < sizeof(eint2)/sizeof(const int); i += 2)
	{
		if (errorcode == eint2[i])
		{
			return eint2[i+1];
		}
	}
	return REG_BADPAT;
}
