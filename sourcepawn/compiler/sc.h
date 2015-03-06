// vim: set sts=2 ts=8 sw=2 tw=99 et:
/*  Pawn compiler
 *
 *  Drafted after the Small-C compiler Version 2.01, originally created
 *  by Ron Cain, july 1980, and enhanced by James E. Hendrix.
 *
 *  This version comes close to a complete rewrite.
 *
 *  Copyright R. Cain, 1980
 *  Copyright J.E. Hendrix, 1982, 1983
 *  Copyright ITB CompuPhase, 1997-2006
 *
 *  This software is provided "as-is", without any express or implied warranty.
 *  In no event will the authors be held liable for any damages arising from
 *  the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1.  The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software. If you use this software in
 *      a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 *  2.  Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 *  3.  This notice may not be removed or altered from any source distribution.
 *
 *  Version: $Id$
 */
#ifndef SC_H_INCLUDED
#define SC_H_INCLUDED
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#if defined __BORLANDC__ && defined _Windows && !(defined __32BIT__ || defined __WIN32__)
  /* setjmp() and longjmp() not well supported in 16-bit windows */
  #include <windows.h>
  typedef int jmp_buf[9];
  #define setjmp(b)     Catch(b)
  #define longjmp(b,e)  Throw(b,e)
#else
  #include <setjmp.h>
#endif
#include "osdefs.h"
#include "amx.h"

/* Note: the "cell" and "ucell" types are defined in AMX.H */

#define PUBLIC_CHAR '@'     /* character that defines a function "public" */
#define CTRL_CHAR   '\\'    /* default control character */
#define sCHARBITS   8       /* size of a packed character */

#define MAXTAGS 16
#define sDIMEN_MAX     4    /* maximum number of array dimensions */
#define sLINEMAX     4095   /* input line length (in characters) */
#define sCOMP_STACK   32    /* maximum nesting of #if .. #endif sections */
#define sDEF_LITMAX  500    /* initial size of the literal pool, in "cells" */
#define sDEF_AMXSTACK 4096  /* default stack size for AMX files */
#define PREPROC_TERM  '\x7f'/* termination character for preprocessor expressions (the "DEL" code) */
#define sDEF_PREFIX   "sourcemod.inc" /* default prefix filename */
#define sARGS_MAX		32	/* number of arguments a function can have, max */
#define sTAGS_MAX		16  /* maximum number of tags on an argument */

typedef union {
  void *pv;                 /* e.g. a name */
  int i;
} stkitem;                  /* type of items stored on the compiler stack */

typedef struct s_arginfo {  /* function argument info */
  char name[sNAMEMAX+1];
  char ident;           /* iVARIABLE, iREFERENCE, iREFARRAY or iVARARGS */
  char usage;           /* uCONST */
  int *tags;            /* argument tag id. list */
  int numtags;          /* number of tags in the tag list */
  int dim[sDIMEN_MAX];
  int idxtag[sDIMEN_MAX];
  int numdim;           /* number of dimensions */
  unsigned char hasdefault; /* bit0: is there a default value? bit6: "tagof"; bit7: "sizeof" */
  union {
    cell val;           /* default value */
    struct {
      cell *data;       /* values of default array */
      int size;         /* complete length of default array */
      int arraysize;    /* size to reserve on the heap */
      cell addr;        /* address of the default array in the data segment */
    } array;
  } defvalue;           /* default value, or pointer to default array */
  int defvalue_tag;     /* tag of the default value */
} arginfo;

/*  Equate table, tagname table, library table */
typedef struct s_constvalue {
  struct s_constvalue *next;
  char name[sNAMEMAX+1];
  cell value;
  int index;            /* index level, for constants referring to array sizes/tags
                         * automaton id. for states and automatons
                         * tag for enumeration lists */
} constvalue;

struct methodmap_t;

/*  Symbol table format
 *
 *  The symbol name read from the input file is stored in "name", the
 *  value of "addr" is written to the output file. The address in "addr"
 *  depends on the class of the symbol:
 *      global          offset into the data segment
 *      local           offset relative to the stack frame
 *      label           generated hexadecimal number
 *      function        offset into code segment
 */
typedef struct s_symbol {
  struct s_symbol *next;
  struct s_symbol *parent;  /* hierarchical types */
  char name[sNAMEMAX+1];
  uint32_t hash;        /* value derived from name, for quicker searching */
  cell addr;            /* address or offset (or value for constant, index for native function) */
  cell codeaddr;        /* address (in the code segment) where the symbol declaration starts */
  char vclass;          /* sLOCAL if "addr" refers to a local symbol */
  char ident;           /* see below for possible values */
  short usage;          /* see below for possible values */
  char flags;           /* see below for possible values */
  int compound;         /* compound level (braces nesting level) */
  int tag;              /* tagname id */
  union {
    int declared;       /* label: how many local variables are declared */
    struct {
      int index;        /* array & enum: tag of array indices or the enum item */
      int field;        /* enumeration fields, where a size is attached to the field */
    } tags;             /* extra tags */
    constvalue *lib;    /* native function: library it is part of */
    long stacksize;     /* normal/public function: stack requirements */
  } x;                  /* 'x' for 'extra' */
  union {
    arginfo *arglist;   /* types of all parameters for functions */
    constvalue *enumlist;/* list of names for the "root" of an enumeration */
    struct {
      cell length;      /* arrays: length (size) */
      cell slength;		/* if a string index, this will be set to the original size */
      short level;      /* number of dimensions below this level */
    } array;
  } dim;                /* for 'dimension', both functions and arrays */
  constvalue *states;   /* list of state function/state variable ids + addresses */
  int fnumber;          /* static global variables: file number in which the declaration is visible */
  int lnumber;          /* line number (in the current source file) for the declaration */
  struct s_symbol **refer;  /* referrer list, functions that "use" this symbol */
  int numrefers;        /* number of entries in the referrer list */
  char *documentation;  /* optional documentation string */
  methodmap_t *methodmap; /* if ident == iMETHODMAP */
  int funcid;           /* set for functions during codegen */
} symbol;

/*  Possible entries for "ident". These are used in the "symbol", "value"
 *  and arginfo structures. Not every constant is valid for every use.
 *  In an argument list, the list is terminated with a "zero" ident; labels
 *  cannot be passed as function arguments, so the value 0 is overloaded.
 */
#define iLABEL      0
#define iVARIABLE   1   /* cell that has an address and that can be fetched directly (lvalue) */
#define iREFERENCE  2   /* iVARIABLE, but must be dereferenced */
#define iARRAY      3
#define iREFARRAY   4   /* an array passed by reference (i.e. a pointer) */
#define iARRAYCELL  5   /* array element, cell that must be fetched indirectly */
#define iARRAYCHAR  6   /* array element, character from cell from array */
#define iEXPRESSION 7   /* expression result, has no address (rvalue) */
#define iCONSTEXPR  8   /* constant expression (or constant symbol) */
#define iFUNCTN     9
#define iREFFUNC    10
#define iVARARGS    11  /* function specified ... as argument(s) */
#define iACCESSOR   13  /* property accessor via a methodmap_method_t */
#define iMETHODMAP  14  /* symbol defining a methodmap */

/*  Possible entries for "usage"
 *
 *  This byte is used as a serie of bits, the syntax is different for
 *  functions and other symbols:
 *
 *  VARIABLE
 *  bits: 0     (uDEFINE) the variable is defined in the source file
 *        1     (uREAD) the variable is "read" (accessed) in the source file
 *        2     (uWRITTEN) the variable is altered (assigned a value)
 *        3     (uCONST) the variable is constant (may not be assigned to)
 *        4     (uPUBLIC) the variable is public
 *        6     (uSTOCK) the variable is discardable (without warning)
 *
 *  FUNCTION
 *  bits: 0     (uDEFINE) the function is defined ("implemented") in the source file
 *        1     (uREAD) the function is invoked in the source file
 *        2     (uRETVALUE) the function returns a value (or should return a value)
 *        3     (uPROTOTYPED) the function was prototyped (implicitly via a definition or explicitly)
 *        4     (uPUBLIC) the function is public
 *        5     (uNATIVE) the function is native
 *        6     (uSTOCK) the function is discardable (without warning)
 *        7     (uMISSING) the function is not implemented in this source file
 *        8     (uFORWARD) the function is explicitly forwardly declared
 *
 *  CONSTANT
 *  bits: 0     (uDEFINE) the symbol is defined in the source file
 *        1     (uREAD) the constant is "read" (accessed) in the source file
 *        2     (uWRITTEN) redundant, but may be set for constants passed by reference
 *        3     (uPREDEF) the constant is pre-defined and should be kept between passes
 *        5     (uENUMROOT) the constant is the "root" of an enumeration
 *        6     (uENUMFIELD) the constant is a field in a named enumeration
 */
#define uDEFINE   0x001
#define uREAD     0x002
#define uWRITTEN  0x004
#define uRETVALUE 0x004 /* function returns (or should return) a value */
#define uCONST    0x008
#define uPROTOTYPED 0x008
#define uPREDEF   0x008 /* constant is pre-defined */
#define uPUBLIC   0x010
#define uNATIVE   0x020
#define uENUMROOT 0x020
#define uSTOCK    0x040
#define uENUMFIELD 0x040
#define uMISSING  0x080
#define uFORWARD  0x100
#define uSTRUCT	  0x200 /* :TODO: make this an ident */
/* uRETNONE is not stored in the "usage" field of a symbol. It is
 * used during parsing a function, to detect a mix of "return;" and
 * "return value;" in a few special cases.
 */
#define uRETNONE  0x10

#define flgDEPRECATED 0x01  /* symbol is deprecated (avoid use) */

#define uMAINFUNC "main"
#define uENTRYFUNC "entry"

#define sGLOBAL   0     /* global variable/constant class (no states) */
#define sLOCAL    1     /* local variable/constant */
#define sSTATIC   2     /* global life, local scope */

#define sSTATEVAR  3    /* criterion to find variables (sSTATEVAR implies a global variable) */

struct methodmap_method_s;

typedef struct value_s {
  symbol *sym;          /* symbol in symbol table, NULL for (constant) expression */
  cell constval;        /* value of the constant expression (if ident==iCONSTEXPR)
                         * also used for the size of a literal array */
  int tag;              /* tag (of the expression) */
  int cmptag;           /* for searching symbols: choose the one with the matching tag */
  char ident;           /* iCONSTEXPR, iVARIABLE, iARRAY, iARRAYCELL,
                         * iEXPRESSION or iREFERENCE */
  char boolresult;      /* boolean result for relational operators */
  cell *arrayidx;       /* last used array indices, for checking self assignment */

  /* when ident == iACCESSOR */
  struct methodmap_method_s *accessor;
} value;

/* Wrapper around value + l/rvalue bit. */
typedef struct svalue_s {
  value val;
  int lvalue;
} svalue;

#define DECLFLAG_ARGUMENT        0x02 // The declaration is for an argument.
#define DECLFLAG_VARIABLE        0x04 // The declaration is for a variable.
#define DECLFLAG_ENUMROOT        0x08 // Multi-dimensional arrays should have an enumroot.
#define DECLFLAG_MAYBE_FUNCTION  0x10 // Might be a named function.
#define DECLFLAG_DYNAMIC_ARRAYS  0x20 // Dynamic arrays are allowed.
#define DECLFLAG_OLD             0x40 // Known old-style declaration.
#define DECLFLAG_FIELD           0x80 // Struct field.
#define DECLFLAG_NEW            0x100 // Known new-style declaration.
#define DECLMASK_NAMED_DECL      (DECLFLAG_ARGUMENT | DECLFLAG_VARIABLE | DECLFLAG_MAYBE_FUNCTION | DECLFLAG_FIELD)

typedef struct {
  // Array information.
  int numdim;
  int dim[sDIMEN_MAX];
  int idxtag[sDIMEN_MAX];
  cell size;
  constvalue *enumroot;

  // Type information.
  int tag;           // Same as tags[0].
  int tags[MAXTAGS]; // List of tags if multi-tagged.
  int numtags;       // Number of tags found.
  int ident;         // Either iREFERENCE, iARRAY, or iVARIABLE.
  char usage;        // Usage flags.
  bool is_new;       // New-style declaration.
  bool has_postdims; // Dimensions, if present, were in postfix position.

  bool isCharArray() const;
} typeinfo_t;

/* For parsing declarations. */
typedef struct {
  char name[sNAMEMAX + 1];
  typeinfo_t type;
  int opertok;       // Operator token, if applicable.
} declinfo_t;

/*  "while" statement queue (also used for "for" and "do - while" loops) */
enum {
  wqBRK,        /* used to restore stack for "break" */
  wqCONT,       /* used to restore stack for "continue" */
  wqLOOP,       /* loop start label number */
  wqEXIT,       /* loop exit label number (jump if false) */
  /* --- */
  wqSIZE        /* "while queue" size */
};
#define wqTABSZ (24*wqSIZE)    /* 24 nested loop statements */

enum {
  statIDLE,     /* not compiling yet */
  statFIRST,    /* first pass */
  statWRITE,    /* writing output */
  statSKIP,     /* skipping output */
};

typedef struct s_stringlist {
  struct s_stringlist *next;
  union {
    char *line;
    struct s_stringlist *tail;
  };
} stringlist;

typedef struct s_stringpair {
  struct s_stringpair *next;
  char *first;
  char *second;
  int matchlength;
  char flags;
  char *documentation;
} stringpair;

// Helper for token info.
typedef struct {
  int id;
  cell val;
  char *str;
} token_t;

// The method name buffer is larger since we can include our parent class's
// name, a "." to separate it, a "~" for constructors, or a ".get/.set" for
// accessors.
#define METHOD_NAMEMAX sNAMEMAX * 2 + 6

typedef struct {
  token_t tok;
  char name[METHOD_NAMEMAX + 1];
} token_ident_t;

/* macros for code generation */
#define opcodes(n)      ((n)*sizeof(cell))      /* opcode size */
#define opargs(n)       ((n)*sizeof(cell))      /* size of typical argument */

/*  Tokens recognized by lex()
 *  Some of these constants are assigned as well to the variable "lastst" (see SC1.C)
 */
enum TokenKind {
  /* value of first multi-character operator */
  tFIRST     = 256,
  /* multi-character operators */
  taMULT     = tFIRST, /* *= */
  taDIV,     /* /= */
  taMOD,     /* %= */
  taADD,     /* += */
  taSUB,     /* -= */
  taSHL,     /* <<= */
  taSHRU,    /* >>>= */
  taSHR,     /* >>= */
  taAND,     /* &= */
  taXOR,     /* ^= */
  taOR,      /* |= */
  tlOR,      /* || */
  tlAND,     /* && */
  tlEQ,      /* == */
  tlNE,      /* != */
  tlLE,      /* <= */
  tlGE,      /* >= */
  tSHL,      /* << */
  tSHRU,     /* >>> */
  tSHR,      /* >> */
  tINC,      /* ++ */
  tDEC,      /* -- */
  tELLIPS,   /* ... */
  tDBLDOT,   /* .. */
  tDBLCOLON, /* :: */
  /* value of last multi-character operator */
  tMIDDLE    = tDBLCOLON,
/* reserved words (statements) */
  tACQUIRE,
  tAS,
  tASSERT,
  tBEGIN,
  tBREAK,
  tBUILTIN,
  tCATCH,
  tCASE,
  tCAST_TO,
  tCELLSOF,
  tCHAR,
  tCONST,
  tCONTINUE,
  tDECL,
  tDEFAULT,
  tDEFINED,
  tDELETE,
  tDO,
  tDOUBLE,
  tELSE,
  tEND,
  tENUM,
  tEXIT,
  tEXPLICIT,
  tFINALLY,
  tFOR,
  tFOREACH,
  tFORWARD,
  tFUNCENUM,
  tFUNCTAG,
  tFUNCTION,
  tGOTO,
  tIF,
  tIMPLICIT,
  tIMPORT,
  tIN,
  tINT,
  tINT8,
  tINT16,
  tINT32,
  tINT64,
  tINTERFACE,
  tINTN,
  tLET,
  tMETHODMAP,
  tNAMESPACE,
  tNATIVE,
  tNEW,
  tNULL,
  tNULLABLE,
  tOBJECT,
  tOPERATOR,
  tPACKAGE,
  tPRIVATE,
  tPROTECTED,
  tPUBLIC,
  tREADONLY,
  tRETURN,
  tSEALED,
  tSIZEOF,
  tSLEEP,
  tSTATIC,
  tSTOCK,
  tSTRUCT,
  tSWITCH,
  tTAGOF,
  tTHEN,
  tTHIS,
  tTHROW,
  tTRY,
  tTYPEDEF,
  tTYPEOF,
  tTYPESET,
  tUINT8,
  tUINT16,
  tUINT32,
  tUINT64,
  tUINTN,
  tUNION,
  tUSING,
  tVAR,
  tVARIANT,
  tVIEW_AS,
  tVIRTUAL,
  tVOID,
  tVOLATILE,
  tWHILE,
  tWITH,
  /* compiler directives */
  tpASSERT,     /* #assert */
  tpDEFINE,
  tpELSE,       /* #else */
  tpELSEIF,     /* #elseif */
  tpENDIF,
  tpENDINPUT,
  tpENDSCRPT,
  tpERROR,
  tpFILE,
  tpIF,         /* #if */
  tINCLUDE,
  tpLINE,
  tpPRAGMA,
  tpTRYINCLUDE,
  tpUNDEF,
  tLAST = tpUNDEF,   /* value of last multi-character match-able token */
  /* semicolon is a special case, because it can be optional */
  tTERM,          /* semicolon or newline */
  tENDEXPR,       /* forced end of expression */
  /* other recognized tokens */
  tNUMBER,        /* integer number */
  tRATIONAL,      /* rational number */
  tSYMBOL,
  tLABEL,
  tSTRING,
  tPENDING_STRING, /* string, but not yet dequeued */
  tEXPR,          /* for assigment to "lastst" only (see SC1.C) */
  tENDLESS,       /* endless loop, for assigment to "lastst" only */
  tEMPTYBLOCK,    /* empty blocks for AM bug 4825 */
  tEOL,           /* newline, only returned by peek_new_line() */
  tNEWDECL,       /* for declloc() */
  tLAST_TOKEN_ID
};

/* (reversed) evaluation of staging buffer */
#define sSTARTREORDER 0x01
#define sENDREORDER   0x02
#define sEXPRSTART    0x80      /* top bit set, rest is free */
#define sMAXARGS      127       /* relates to the bit pattern of sEXPRSTART */

#define sDOCSEP       0x01      /* to separate documentation comments between functions */

/* codes for ffabort() */
#define xEXIT           1       /* exit code in PRI */
#define xASSERTION      2       /* abort caused by failing assertion */
#define xSTACKERROR     3       /* stack/heap overflow */
#define xBOUNDSERROR    4       /* array index out of bounds */
#define xMEMACCESS      5       /* data access error */
#define xINVINSTR       6       /* invalid instruction */
#define xSTACKUNDERFLOW 7       /* stack underflow */
#define xHEAPUNDERFLOW  8       /* heap underflow */
#define xCALLBACKERR    9       /* no, or invalid, callback */
#define xSLEEP         12       /* sleep, exit code in PRI, tag in ALT */

/* Miscellaneous  */
#if !defined TRUE
  #define FALSE         0
  #define TRUE          1
#endif
#define sIN_CSEG        1       /* if parsing CODE */
#define sIN_DSEG        2       /* if parsing DATA */
#define sCHKBOUNDS      1       /* bit position in "debug" variable: check bounds */
#define sSYMBOLIC       2       /* bit position in "debug" variable: symbolic info */
#define sRESET          0       /* reset error flag */
#define sFORCESET       1       /* force error flag on */
#define sEXPRMARK       2       /* mark start of expression */
#define sEXPRRELEASE    3       /* mark end of expression */
#define sSETPOS         4       /* set line number for the error */

enum {
  sOPTIMIZE_NONE,               /* no optimization */
  sOPTIMIZE_NOMACRO,            /* no macro instructions */
  sOPTIMIZE_DEFAULT,            /* full optimization */
  /* ----- */
  sOPTIMIZE_NUMBER
};

typedef enum s_regid {
  sPRI,                         /* indicates the primary register */
  sALT,                         /* indicates the secundary register */
} regid;

typedef enum s_optmark {
  sEXPR,                        /* end of expression (for expressions that form a statement) */
  sPARM,                        /* end of parameter (in a function parameter list) */
  sLDECL,                       /* start of local declaration (variable) */
} optmark;

#define suSLEEP_INSTR 0x01      /* the "sleep" instruction was used */

#define FIXEDTAG     0x40000000
#define FUNCTAG      0x20000000
#define OBJECTTAG    0x10000000
#define ENUMTAG      0x08000000
#define METHODMAPTAG 0x04000000
#define STRUCTTAG    0x02000000
#define TAGTYPEMASK   (FUNCTAG | OBJECTTAG | ENUMTAG | METHODMAPTAG | STRUCTTAG)
#define TAGFLAGMASK   (FIXEDTAG | TAGTYPEMASK)
#define TAGID(tag)    ((tag) & ~(TAGFLAGMASK))
#define CELL_MAX      (((ucell)1 << (sizeof(cell)*8-1)) - 1)


/*
 * Functions you call from the "driver" program
 */
int pc_compile(int argc, char **argv);
int pc_addconstant(const char *name,cell value,int tag);
int pc_addtag(const char *name);
int pc_addtag_flags(const char *name, int flags);
int pc_findtag(const char *name);
constvalue *pc_tagptr(const char *name);
int pc_enablewarning(int number,int enable);
const char *pc_tagname(int tag);
int parse_decl(declinfo_t *decl, int flags);
const char *type_to_name(int tag);
bool parse_new_typename(const token_t *tok, int *tagp);

/*
 * Functions called from the compiler (to be implemented by you)
 */

/* general console output */
int pc_printf(const char *message,...);

/* error report function */
int pc_error(int number,const char *message,const char *filename,int firstline,int lastline,va_list argptr);

/* input from source file */
void *pc_opensrc(char *filename); /* reading only */
void *pc_createsrc(char *filename);
void pc_closesrc(void *handle);   /* never delete */
char *pc_readsrc(void *handle,unsigned char *target,int maxchars);
int pc_writesrc(void *handle,unsigned char *source);
void *pc_getpossrc(void *handle,void *position); /* mark the current position */
void pc_resetsrc(void *handle,void *position);  /* reset to a position marked earlier */
int  pc_eofsrc(void *handle);

/* output to intermediate (.ASM) file */
void *pc_openasm(char *filename); /* read/write */
void pc_closeasm(void *handle,int deletefile);
void pc_resetasm(void *handle);
int  pc_writeasm(void *handle,const char *str);
char *pc_readasm(void *handle,char *target,int maxchars);

void sp_fdbg_ntv_start(int num_natives);
void sp_fdbg_ntv_hook(int index, symbol *sym);

/* function prototypes in SC1.C */
void set_extension(char *filename,const char *extension,int force);
symbol *fetchfunc(char *name);
char *operator_symname(char *symname,const char *opername,int tag1,int tag2,int numtags,int resulttag);
char *funcdisplayname(char *dest,char *funcname);
int exprconst(cell *val,int *tag,symbol **symptr);
constvalue *append_constval(constvalue *table,const char *name,cell val,int index);
constvalue *find_constval(constvalue *table,char *name,int index);
void delete_consttable(constvalue *table);
symbol *add_constant(const char *name,cell val,int vclass,int tag);
void sc_attachdocumentation(symbol *sym);
constvalue *find_tag_byval(int tag);
int get_actual_compound(symbol *sym);

/* function prototypes in SC2.C */
#define PUSHSTK_P(v)  { stkitem s_; s_.pv=(v); pushstk(s_); }
#define PUSHSTK_I(v)  { stkitem s_; s_.i=(v); pushstk(s_); }
#define POPSTK_P()    (popstk().pv)
#define POPSTK_I()    (popstk().i)
void pushstk(stkitem val);
stkitem popstk(void);
void clearstk(void);
int plungequalifiedfile(char *name);  /* explicit path included */
int plungefile(char *name,int try_currentpath,int try_includepaths);   /* search through "include" paths */
void preprocess(void);
void lexinit(void);
int lex(cell *lexvalue,char **lexsym);
int lextok(token_t *tok);
int lexpeek(int id);
void lexpush(void);
void lexclr(int clreol);
int matchtoken(int token);
int tokeninfo(cell *val,char **str);
int needtoken(int token);
int matchtoken2(int id, token_t *tok);
int expecttoken(int id, token_t *tok);
int matchsymbol(token_ident_t *ident);
int needsymbol(token_ident_t *ident);
int peek_same_line();
int require_newline(int allow_semi);
void litadd(cell value);
void litinsert(cell value,int pos);
int alphanum(char c);
int ishex(char c);
void delete_symbol(symbol *root,symbol *sym);
void delete_symbols(symbol *root,int level,int del_labels,int delete_functions);
int refer_symbol(symbol *entry,symbol *bywhom);
void markusage(symbol *sym,int usage);
symbol *findglb(const char *name,int filter);
symbol *findloc(const char *name);
symbol *findconst(const char *name,int *matchtag);
symbol *finddepend(const symbol *parent);
symbol *addsym(const char *name,cell addr,int ident,int vclass,int tag, int usage);
symbol *addvariable(const char *name,cell addr,int ident,int vclass,int tag,
                            int dim[],int numdim,int idxtag[]);
symbol *addvariable2(const char *name,cell addr,int ident,int vclass,int tag,
                             int dim[],int numdim,int idxtag[],int slength);
symbol *addvariable3(declinfo_t *decl,cell addr,int vclass,int slength);
int getlabel(void);
char *itoh(ucell val);

#define MATCHTAG_COERCE       0x1 // allow coercion
#define MATCHTAG_SILENT       0x2 // silence the error(213) warning
#define MATCHTAG_COMMUTATIVE  0x4 // order does not matter
#define MATCHTAG_DEDUCE       0x8 // correct coercion

/* function prototypes in SC3.C */
int check_userop(void (*oper)(void),int tag1,int tag2,int numparam,
                         value *lval,int *resulttag);
int matchtag(int formaltag,int actualtag,int allowcoerce);
int checktag(int tags[],int numtags,int exprtag);
int expression(cell *val,int *tag,symbol **symptr,int chkfuncresult,value *_lval);
int sc_getstateid(constvalue **automaton,constvalue **state);
cell array_totalsize(symbol *sym);
int matchtag_string(int ident, int tag);
int checktag_string(value *sym1, value *sym2);
int checktags_string(int tags[], int numtags, value *sym1);
int lvalexpr(svalue *sval);

/* function prototypes in SC4.C */
void writeleader(symbol *root);
void writetrailer(void);
void begcseg(void);
void begdseg(void);
void setline(int chkbounds);
void setfiledirect(char *name);
void setlinedirect(int line);
void setlabel(int index);
void markexpr(optmark type,const char *name,cell offset);
void startfunc(char *fname);
void endfunc(void);
void alignframe(int numbytes);
void rvalue(value *lval);
void address(symbol *ptr,regid reg);
void store(value *lval);
void loadreg(cell address,regid reg);
void storereg(cell address,regid reg);
void memcopy(cell size);
void copyarray(symbol *sym,cell size);
void fillarray(symbol *sym,cell size,cell value);
void ldconst(cell val,regid reg);
void moveto1(void);
void move_alt(void);
void pushreg(regid reg);
void pushval(cell val);
void popreg(regid reg);
void genarray(int dims, int _autozero);
void swap1(void);
void ffswitch(int label);
void ffcase(cell value,char *labelname,int newtable);
void ffcall(symbol *sym,const char *label,int numargs);
void ffret(int remparams);
void ffabort(int reason);
void ffbounds(cell size);
void jumplabel(int number);
void defstorage(void);
void modstk(int delta);
void setstk(cell value);
void modheap(int delta);
void modheap_i();
void setheap_pri(void);
void setheap(cell value);
void cell2addr(void);
void cell2addr_alt(void);
void addr2cell(void);
void char2addr(void);
void charalign(void);
void addconst(cell value);
void setheap_save(cell value);
void stradjust(regid reg);
void invoke_getter(struct methodmap_method_s *method);
void invoke_setter(struct methodmap_method_s *method, int save);
void inc_pri();
void dec_pri();
void load_hidden_arg();
void load_glbfn(symbol *sym);

/*  Code generation functions for arithmetic operators.
 *
 *  Syntax: o[u|s|b]_name
 *          |   |   | +--- name of operator
 *          |   |   +----- underscore
 *          |   +--------- "u"nsigned operator, "s"igned operator or "b"oth
 *          +------------- "o"perator
 */
void os_mult(void); /* multiplication (signed) */
void os_div(void);  /* division (signed) */
void os_mod(void);  /* modulus (signed) */
void ob_add(void);  /* addition */
void ob_sub(void);  /* subtraction */
void ob_sal(void);  /* shift left (arithmetic) */
void os_sar(void);  /* shift right (arithmetic, signed) */
void ou_sar(void);  /* shift right (logical, unsigned) */
void ob_or(void);   /* bitwise or */
void ob_xor(void);  /* bitwise xor */
void ob_and(void);  /* bitwise and */
void ob_eq(void);   /* equality */
void ob_ne(void);   /* inequality */
void relop_prefix(void);
void relop_suffix(void);
void os_le(void);   /* less or equal (signed) */
void os_ge(void);   /* greater or equal (signed) */
void os_lt(void);   /* less (signed) */
void os_gt(void);   /* greater (signed) */

void lneg(void);
void neg(void);
void invert(void);
void nooperation(void);
void inc(value *lval);
void dec(value *lval);
void jmp_ne0(int number);
void jmp_eq0(int number);
void outval(cell val,int newline);

/* function prototypes in SC5.C */
int error(int number,...);
void errorset(int code,int line);

/* function prototypes in SC6.C */
void assemble(const char *outname, void *fin);

/* function prototypes in SC7.C */
void stgbuffer_cleanup(void);
void stgmark(char mark);
void stgwrite(const char *st);
void stgout(int index);
void stgdel(int index,cell code_index);
int stgget(int *index,cell *code_index);
void stgset(int onoff);
int phopt_init(void);
int phopt_cleanup(void);

/* function prototypes in SCLIST.C */
char* duplicatestring(const char* sourcestring);
stringpair *insert_alias(char *name,char *alias);
stringpair *find_alias(char *name);
int lookup_alias(char *target,char *name);
void delete_aliastable(void);
stringlist *insert_path(char *path);
char *get_path(int index);
void delete_pathtable(void);
stringpair *insert_subst(const char *pattern,const char *substitution,int prefixlen);
int get_subst(int index,char **pattern,char **substitution);
stringpair *find_subst(char *name,int length);
int delete_subst(char *name,int length);
void delete_substtable(void);
stringlist *insert_sourcefile(char *string);
char *get_sourcefile(int index);
void delete_sourcefiletable(void);
stringlist *insert_docstring(char *string);
char *get_docstring(int index);
void delete_docstring(int index);
void delete_docstringtable(void);
stringlist *insert_autolist(const char *string);
char *get_autolist(int index);
void delete_autolisttable(void);
stringlist *insert_dbgfile(const char *filename);
stringlist *insert_dbgline(int linenr);
stringlist *insert_dbgsymbol(symbol *sym);
char *get_dbgstring(int index);
void delete_dbgstringtable(void);
stringlist *get_dbgstrings();

/* function prototypes in SCI18N.C */
#define MAXCODEPAGE 12
int cp_path(const char *root,const char *directory);
int cp_set(const char *name);
cell cp_translate(const unsigned char *string,const unsigned char **endptr);
cell get_utf8_char(const unsigned char *string,const unsigned char **endptr);
int scan_utf8(void *fp,const char *filename);

/* function prototypes in SCSTATE.C */
constvalue *automaton_add(const char *name);
constvalue *automaton_find(const char *name);
constvalue *automaton_findid(int id);
constvalue *state_add(const char *name,int fsa_id);
constvalue *state_find(const char *name,int fsa_id);
constvalue *state_findid(int id);
void state_buildlist(int **list,int *listsize,int *count,int stateid);
int state_addlist(int *list,int count,int fsa_id);
void state_deletetable(void);
int state_getfsa(int listid);
int state_count(int listid);
int state_inlist(int listid,int state);
int state_listitem(int listid,int index);
void state_conflict(symbol *root);
int state_conflict_id(int listid1,int listid2);

/* external variables (defined in scvars.c) */
#if !defined SC_SKIP_VDECL
typedef struct HashTable HashTable;
extern struct HashTable *sp_Globals;
extern symbol loctab;       /* local symbol table */
extern symbol glbtab;       /* global symbol table */
extern cell *litq;          /* the literal queue */
extern unsigned char pline[]; /* the line read from the input file */
extern const unsigned char *lptr;/* points to the current position in "pline" */
extern constvalue tagname_tab;/* tagname table */
extern constvalue libname_tab;/* library table (#pragma library "..." syntax) */
extern constvalue *curlibrary;/* current library */
extern int pc_addlibtable;  /* is the library table added to the AMX file? */
extern symbol *curfunc;     /* pointer to current function */
extern char *inpfname;      /* name of the file currently read from */
extern char outfname[];     /* intermediate (assembler) file name */
extern char binfname[];     /* binary file name */
extern char errfname[];     /* error file name */
extern char sc_ctrlchar;    /* the control character (or escape character) */
extern char sc_ctrlchar_org;/* the default control character */
extern int litidx;          /* index to literal table */
extern int litmax;          /* current size of the literal table */
extern int stgidx;          /* index to the staging buffer */
extern int sc_labnum;       /* number of (internal) labels */
extern int staging;         /* true if staging output */
extern cell declared;       /* number of local cells declared */
extern cell glb_declared;   /* number of global cells declared */
extern cell code_idx;       /* number of bytes with generated code */
extern int ntv_funcid;      /* incremental number of native function */
extern int errnum;          /* number of errors */
extern int warnnum;         /* number of warnings */
extern int sc_debug;        /* debug/optimization options (bit field) */
extern int sc_packstr;      /* strings are packed by default? */
extern int sc_asmfile;      /* create .ASM file? */
extern int sc_listing;      /* create .LST file? */
extern int sc_compress;     /* compress bytecode? */
extern int sc_needsemicolon;/* semicolon required to terminate expressions? */
extern int sc_dataalign;    /* data alignment value */
extern int sc_alignnext;    /* must frame of the next function be aligned? */
extern int pc_docexpr;      /* must expression be attached to documentation comment? */
extern int sc_showincludes; /* show include files? */
extern int curseg;          /* 1 if currently parsing CODE, 2 if parsing DATA */
extern cell pc_stksize;     /* stack size */
extern cell pc_amxlimit;    /* abstract machine size limit (code + data, or only code) */
extern cell pc_amxram;      /* abstract machine data size limit */
extern int freading;        /* is there an input file ready for reading? */
extern int fline;           /* the line number in the current file */
extern short fnumber;       /* number of files in the file table (debugging) */
extern short fcurrent;      /* current file being processed (debugging) */
extern short sc_intest;     /* true if inside a test */
extern int sideeffect;      /* true if an expression causes a side-effect */
extern int stmtindent;      /* current indent of the statement */
extern int indent_nowarn;   /* skip warning "217 loose indentation" */
extern int sc_tabsize;      /* number of spaces that a TAB represents */
extern short sc_allowtags;  /* allow/detect tagnames in lex() */
extern int sc_status;       /* read/write status */
extern int sc_err_status;   /* TRUE if errors should be generated even if sc_status = SKIP */
extern int sc_rationaltag;  /* tag for rational numbers */
extern int rational_digits; /* number of fractional digits */
extern int sc_allowproccall;/* allow/detect tagnames in lex() */
extern short sc_is_utf8;    /* is this source file in UTF-8 encoding */
extern char *pc_deprecate;  /* if non-NULL, mark next declaration as deprecated */
extern int sc_curstates;    /* ID of the current state list */
extern int pc_optimize;     /* (peephole) optimization level */
extern int pc_memflags;     /* special flags for the stack/heap usage */
extern int pc_functag;      /* global function tag */
extern int pc_tag_string;   /* global String tag */
extern int pc_tag_void;     /* global void tag */
extern int pc_tag_object;   /* root object tag */
extern int pc_tag_bool;     /* global bool tag */
extern int pc_tag_null_t;   /* the null type */
extern int pc_tag_nullfunc_t;   /* the null function type */
extern int pc_anytag;       /* global any tag */
extern int glbstringread;	  /* last global string read */
extern int sc_require_newdecls; /* only newdecls are allowed */
extern bool sc_warnings_are_errors;

extern constvalue sc_automaton_tab; /* automaton table */
extern constvalue sc_state_tab;     /* state table */

extern void *inpf;          /* file read from (source or include) */
extern void *inpf_org;      /* main source file */
extern void *outf;          /* file written to */

extern jmp_buf errbuf;      /* target of longjmp() on a fatal error */

#if !defined SC_LIGHT
  extern int sc_makereport; /* generate a cross-reference report */
#endif

#if defined WIN32
# if !defined snprintf
#  define snprintf _snprintf
#  define vsnprintf _vsnprintf
# endif
#endif

#endif /* SC_SKIP_VDECL */

typedef struct array_info_s
{
  const int *dim_list;				/* Dimension sizes */
  int dim_count;					/* Number of dimensions */
  const int *lastdim_list;			/* Sizes of last dimensions, if variable */
  const cell *dim_offs_precalc;		/* Cached calculations into the lastdim_list array */
  cell *data_offs;					/* Current offset AFTER the indirection vectors (data) */
  int *cur_dims;					/* Current dimensions the recursion is at */
  cell *base;						/* &litq[startlit] */
} array_info_t;

enum FatalError {
  FIRST_FATAL_ERROR = 183,

  FATAL_ERROR_READ  = FIRST_FATAL_ERROR,
  FATAL_ERROR_WRITE,
  FATAL_ERROR_ALLOC_OVERFLOW,
  FATAL_ERROR_OOM,
  FATAL_ERROR_INVALID_INSN,
  FATAL_ERROR_INT_OVERFLOW,
  FATAL_ERROR_SCRIPT_OVERFLOW,
  FATAL_ERROR_OVERWHELMED_BY_BAD,
  FATAL_ERROR_NO_CODEPAGE,
  FATAL_ERROR_INVALID_PATH,
  FATAL_ERROR_ASSERTION_FAILED,
  FATAL_ERROR_USER_ERROR,

  FATAL_ERRORS_TOTAL
};

struct AutoDisableLiteralQueue
{
 public:
  AutoDisableLiteralQueue();
  ~AutoDisableLiteralQueue();

 private:
  bool prev_value_;
};

#endif /* SC_H_INCLUDED */
