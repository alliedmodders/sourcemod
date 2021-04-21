#pragma once

/*
	Maps pcre_compile2 error codes to posix error codes.
	From pcreposix.c and pcreposix.h
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
const int pcre_posix_compile_error_map[] = {
	0,           /* no error */
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
	REG_BADRPT,  /* operand of unlimited repeat could match the empty string */
	REG_ASSERT,  /* internal error: unexpected repeat */
	REG_BADPAT,  /* unrecognized character after (? */
	REG_BADPAT,  /* POSIX named classes are supported only within a class */
	REG_EPAREN,  /* missing ) */
	/* 15 */
	REG_ESUBREG, /* reference to non-existent subpattern */
	REG_INVARG,  /* erroffset passed as NULL */
	REG_INVARG,  /* unknown option bit(s) set */
	REG_EPAREN,  /* missing ) after comment */
	REG_ESIZE,   /* parentheses nested too deeply */
	/* 20 */
	REG_ESIZE,   /* regular expression too large */
	REG_ESPACE,  /* failed to get memory */
	REG_EPAREN,  /* unmatched parentheses */
	REG_ASSERT,  /* internal error: code overflow */
	REG_BADPAT,  /* unrecognized character after (?< */
	/* 25 */
	REG_BADPAT,  /* lookbehind assertion is not fixed length */
	REG_BADPAT,  /* malformed number or name after (?( */
	REG_BADPAT,  /* conditional group contains more than two branches */
	REG_BADPAT,  /* assertion expected after (?( */
	REG_BADPAT,  /* (?R or (?[+-]digits must be followed by ) */
	/* 30 */
	REG_ECTYPE,  /* unknown POSIX class name */
	REG_BADPAT,  /* POSIX collating elements are not supported */
	REG_INVARG,  /* this version of PCRE is not compiled with PCRE_UTF8 support */
	REG_BADPAT,  /* spare error */
	REG_BADPAT,  /* character value in \x{} or \o{} is too large */
	/* 35 */
	REG_BADPAT,  /* invalid condition (?(0) */
	REG_BADPAT,  /* \C not allowed in lookbehind assertion */
	REG_EESCAPE, /* PCRE does not support \L, \l, \N, \U, or \u */
	REG_BADPAT,  /* number after (?C is > 255 */
	REG_BADPAT,  /* closing ) for (?C expected */
	/* 40 */
	REG_BADPAT,  /* recursive call could loop indefinitely */
	REG_BADPAT,  /* unrecognized character after (?P */
	REG_BADPAT,  /* syntax error in subpattern name (missing terminator) */
	REG_BADPAT,  /* two named subpatterns have the same name */
	REG_BADPAT,  /* invalid UTF-8 string */
	/* 45 */
	REG_BADPAT,  /* support for \P, \p, and \X has not been compiled */
	REG_BADPAT,  /* malformed \P or \p sequence */
	REG_BADPAT,  /* unknown property name after \P or \p */
	REG_BADPAT,  /* subpattern name is too long (maximum 32 characters) */
	REG_BADPAT,  /* too many named subpatterns (maximum 10,000) */
	/* 50 */
	REG_BADPAT,  /* repeated subpattern is too long */
	REG_BADPAT,  /* octal value is greater than \377 (not in UTF-8 mode) */
	REG_BADPAT,  /* internal error: overran compiling workspace */
	REG_BADPAT,  /* internal error: previously-checked referenced subpattern not found */
	REG_BADPAT,  /* DEFINE group contains more than one branch */
	/* 55 */
	REG_BADPAT,  /* repeating a DEFINE group is not allowed */
	REG_INVARG,  /* inconsistent NEWLINE options */
	REG_BADPAT,  /* \g is not followed followed by an (optionally braced) non-zero number */
	REG_BADPAT,  /* a numbered reference must not be zero */
	REG_BADPAT,  /* an argument is not allowed for (*ACCEPT), (*FAIL), or (*COMMIT) */
	/* 60 */
	REG_BADPAT,  /* (*VERB) not recognized */
	REG_BADPAT,  /* number is too big */
	REG_BADPAT,  /* subpattern name expected */
	REG_BADPAT,  /* digit expected after (?+ */
	REG_BADPAT,  /* ] is an invalid data character in JavaScript compatibility mode */
	/* 65 */
	REG_BADPAT,  /* different names for subpatterns of the same number are not allowed */
	REG_BADPAT,  /* (*MARK) must have an argument */
	REG_INVARG,  /* this version of PCRE is not compiled with PCRE_UCP support */
	REG_BADPAT,  /* \c must be followed by an ASCII character */
	REG_BADPAT,  /* \k is not followed by a braced, angle-bracketed, or quoted name */
	/* 70 */
	REG_BADPAT,  /* internal error: unknown opcode in find_fixedlength() */
	REG_BADPAT,  /* \N is not supported in a class */
	REG_BADPAT,  /* too many forward references */
	REG_BADPAT,  /* disallowed UTF-8/16/32 code point (>= 0xd800 && <= 0xdfff) */
	REG_BADPAT,  /* invalid UTF-16 string (should not occur) */
	/* 75 */
	REG_BADPAT,  /* overlong MARK name */
	REG_BADPAT,  /* character value in \u.... sequence is too large */
	REG_BADPAT,  /* invalid UTF-32 string (should not occur) */
	REG_BADPAT,  /* setting UTF is disabled by the application */
	REG_BADPAT,  /* non-hex character in \\x{} (closing brace missing?) */
	/* 80 */
	REG_BADPAT,  /* non-octal character in \o{} (closing brace missing?) */
	REG_BADPAT,  /* missing opening brace after \o */
	REG_BADPAT,  /* parentheses too deeply nested */
	REG_BADPAT,  /* invalid range in character class */
	REG_BADPAT,  /* group name must start with a non-digit */
	/* 85 */
	REG_BADPAT   /* parentheses too deeply nested (stack check) */
};