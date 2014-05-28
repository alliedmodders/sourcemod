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
      char *symname;    /* name of another symbol */
      short level;      /* indirection level for that symbol */
    } size;             /* used for "sizeof" default value */
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
  struct s_symbol *parent;  /* hierarchical types (multi-dimensional arrays) */
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

#define uCOUNTOF  0x20  /* set in the "hasdefault" field of the arginfo struct */
#define uTAGOF    0x40  /* set in the "hasdefault" field of the arginfo struct */
#define uSIZEOF   0x80  /* set in the "hasdefault" field of the arginfo struct */

#define uMAINFUNC "main"
#define uENTRYFUNC "entry"

#define sGLOBAL   0     /* global variable/constant class (no states) */
#define sLOCAL    1     /* local variable/constant */
#define sSTATIC   2     /* global life, local scope */

#define sSTATEVAR  3    /* criterion to find variables (sSTATEVAR implies a global variable) */

typedef struct s_value {
  symbol *sym;          /* symbol in symbol table, NULL for (constant) expression */
  cell constval;        /* value of the constant expression (if ident==iCONSTEXPR)
                         * also used for the size of a literal array */
  int tag;              /* tag (of the expression) */
  int cmptag;           /* for searching symbols: choose the one with the matching tag */
  char ident;           /* iCONSTEXPR, iVARIABLE, iARRAY, iARRAYCELL,
                         * iEXPRESSION or iREFERENCE */
  char boolresult;      /* boolean result for relational operators */
  cell *arrayidx;       /* last used array indices, for checking self assignment */
} value;

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

/* macros for code generation */
#define opcodes(n)      ((n)*sizeof(cell))      /* opcode size */
#define opargs(n)       ((n)*sizeof(cell))      /* size of typical argument */

/*  Tokens recognized by lex()
 *  Some of these constants are assigned as well to the variable "lastst" (see SC1.C)
 */
#define tFIRST   256    /* value of first multi-character operator */
#define tMIDDLE  280    /* value of last multi-character operator */
#define tLAST    332    /* value of last multi-character match-able token */
/* multi-character operators */
#define taMULT   256    /* *= */
#define taDIV    257    /* /= */
#define taMOD    258    /* %= */
#define taADD    259    /* += */
#define taSUB    260    /* -= */
#define taSHL    261    /* <<= */
#define taSHRU   262    /* >>>= */
#define taSHR    263    /* >>= */
#define taAND    264    /* &= */
#define taXOR    265    /* ^= */
#define taOR     266    /* |= */
#define tlOR     267    /* || */
#define tlAND    268    /* && */
#define tlEQ     269    /* == */
#define tlNE     270    /* != */
#define tlLE     271    /* <= */
#define tlGE     272    /* >= */
#define tSHL     273    /* << */
#define tSHRU    274    /* >>> */
#define tSHR     275    /* >> */
#define tINC     276    /* ++ */
#define tDEC     277    /* -- */
#define tELLIPS  278    /* ... */
#define tDBLDOT  279    /* .. */
#define tDBLCOLON 280   /* :: */
/* reserved words (statements) */
#define tASSERT  281
#define tBEGIN   282
#define tBREAK   283
#define tCASE    284
#define tCELLSOF 285
#define tCHAR    286
#define tCONST   287
#define tCONTINUE 288
#define tDEFAULT 289
#define tDEFINED 290
#define tDO      291
#define tELSE    292
#define tEND     293
#define tENUM    294
#define tEXIT    295
#define tFOR     296
#define tFORWARD 297
#define tFUNCENUM 298
#define tFUNCTAG 299
#define tGOTO    300
#define tIF      301
#define tNATIVE  302
#define tNEW     303
#define tDECL    304
#define tOPERATOR 305
#define tPUBLIC  306
#define tRETURN  307
#define tSIZEOF  308
#define tSLEEP   309
#define tSTATIC  310
#define tSTOCK   311
#define tSTRUCT  312
#define tSWITCH  313
#define tTAGOF   314
#define tTHEN    315
#define tWHILE   316
/* compiler directives */
#define tpASSERT 317    /* #assert */
#define tpDEFINE 318
#define tpELSE   319    /* #else */
#define tpELSEIF 320    /* #elseif */
#define tpEMIT   321
#define tpENDIF  322
#define tpENDINPUT 323
#define tpENDSCRPT 324
#define tpERROR  325
#define tpFILE   326
#define tpIF     327    /* #if */
#define tINCLUDE 328
#define tpLINE   329
#define tpPRAGMA 330
#define tpTRYINCLUDE 331
#define tpUNDEF  332
/* semicolon is a special case, because it can be optional */
#define tTERM    333    /* semicolon or newline */
#define tENDEXPR 334    /* forced end of expression */
/* other recognized tokens */
#define tNUMBER  335    /* integer number */
#define tRATIONAL 336   /* rational number */
#define tSYMBOL  337
#define tLABEL   338
#define tSTRING  339
#define tEXPR    341 /* for assigment to "lastst" only (see SC1.C) */
#define tENDLESS 342 /* endless loop, for assigment to "lastst" only */
#define tEMPTYBLOCK 343 /* empty blocks for AM bug 4825 */

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

#if INT_MAX<0x8000u
  #define PUBLICTAG   0x8000u
  #define FIXEDTAG    0x4000u
  #define FUNCTAG     0x2000u
#else
  #define PUBLICTAG   0x80000000Lu
  #define FIXEDTAG    0x40000000Lu
  #define FUNCTAG     0x20000000Lu
#endif
#define TAGMASK       (~PUBLICTAG)
#define CELL_MAX      (((ucell)1 << (sizeof(cell)*8-1)) - 1)


/* interface functions */
#if defined __cplusplus
  extern "C" {
#endif

/*
 * Functions you call from the "driver" program
 */
int pc_compile(int argc, char **argv);
int pc_addconstant(char *name,cell value,int tag);
int pc_addtag(char *name);
int pc_addfunctag(char *name);
int pc_enablewarning(int number,int enable);

/*
 * Functions called from the compiler (to be implemented by you)
 */

/* general console output */
int pc_printf(const char *message,...);

/* error report function */
int pc_error(int number,char *message,char *filename,int firstline,int lastline,va_list argptr);

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
int  pc_writeasm(void *handle,char *str);
char *pc_readasm(void *handle,char *target,int maxchars);

/* output to binary (.AMX) file */
void *pc_openbin(char *filename);
void pc_closebin(void *handle,int deletefile);
void pc_resetbin(void *handle,long offset);
int  pc_writebin(void *handle,void *buffer,int size);
long pc_lengthbin(void *handle); /* return the length of the file */

#if defined __cplusplus
  }
#endif


/* by default, functions and variables used in throughout the compiler
 * files are "external"
 */
#if !defined SC_FUNC
  #define SC_FUNC
#endif
#if !defined SC_VDECL
  #define SC_VDECL  extern
#endif
#if !defined SC_VDEFINE
  #define SC_VDEFINE
#endif

void sp_fdbg_ntv_start(int num_natives);
void sp_fdbg_ntv_hook(int index, symbol *sym);

/* function prototypes in SC1.C */
SC_FUNC void set_extension(char *filename,char *extension,int force);
SC_FUNC symbol *fetchfunc(char *name,int tag);
SC_FUNC char *operator_symname(char *symname,char *opername,int tag1,int tag2,int numtags,int resulttag);
SC_FUNC char *funcdisplayname(char *dest,char *funcname);
SC_FUNC int constexpr(cell *val,int *tag,symbol **symptr);
SC_FUNC constvalue *append_constval(constvalue *table,const char *name,cell val,int index);
SC_FUNC constvalue *find_constval(constvalue *table,char *name,int index);
SC_FUNC void delete_consttable(constvalue *table);
SC_FUNC symbol *add_constant(char *name,cell val,int vclass,int tag);
SC_FUNC void exporttag(int tag);
SC_FUNC void sc_attachdocumentation(symbol *sym);
SC_FUNC constvalue *find_tag_byval(int tag);
SC_FUNC int get_actual_compound(symbol *sym);

/* function prototypes in SC2.C */
#define PUSHSTK_P(v)  { stkitem s_; s_.pv=(v); pushstk(s_); }
#define PUSHSTK_I(v)  { stkitem s_; s_.i=(v); pushstk(s_); }
#define POPSTK_P()    (popstk().pv)
#define POPSTK_I()    (popstk().i)
SC_FUNC void pushstk(stkitem val);
SC_FUNC stkitem popstk(void);
SC_FUNC void clearstk(void);
SC_FUNC int plungequalifiedfile(char *name);  /* explicit path included */
SC_FUNC int plungefile(char *name,int try_currentpath,int try_includepaths);   /* search through "include" paths */
SC_FUNC void preprocess(void);
SC_FUNC void lexinit(void);
SC_FUNC int lex(cell *lexvalue,char **lexsym);
SC_FUNC void lexpush(void);
SC_FUNC void lexclr(int clreol);
SC_FUNC int matchtoken(int token);
SC_FUNC int tokeninfo(cell *val,char **str);
SC_FUNC int needtoken(int token);
SC_FUNC void litadd(cell value);
SC_FUNC void litinsert(cell value,int pos);
SC_FUNC int alphanum(char c);
SC_FUNC int ishex(char c);
SC_FUNC void delete_symbol(symbol *root,symbol *sym);
SC_FUNC void delete_symbols(symbol *root,int level,int del_labels,int delete_functions);
SC_FUNC int refer_symbol(symbol *entry,symbol *bywhom);
SC_FUNC void markusage(symbol *sym,int usage);
SC_FUNC symbol *findglb(const char *name,int filter);
SC_FUNC symbol *findloc(const char *name);
SC_FUNC symbol *findconst(const char *name,int *matchtag);
SC_FUNC symbol *finddepend(const symbol *parent);
SC_FUNC symbol *addsym(const char *name,cell addr,int ident,int vclass,int tag,
                       int usage);
SC_FUNC symbol *addvariable(const char *name,cell addr,int ident,int vclass,int tag,
                            int dim[],int numdim,int idxtag[]);
SC_FUNC symbol *addvariable2(const char *name,cell addr,int ident,int vclass,int tag,
							 int dim[],int numdim,int idxtag[],int slength);
SC_FUNC int getlabel(void);
SC_FUNC char *itoh(ucell val);

/* function prototypes in SC3.C */
SC_FUNC int check_userop(void (*oper)(void),int tag1,int tag2,int numparam,
                         value *lval,int *resulttag);
SC_FUNC int matchtag(int formaltag,int actualtag,int allowcoerce);
SC_FUNC int checktag(int tags[],int numtags,int exprtag);
SC_FUNC int expression(cell *val,int *tag,symbol **symptr,int chkfuncresult,value *_lval);
SC_FUNC int sc_getstateid(constvalue **automaton,constvalue **state);
SC_FUNC cell array_totalsize(symbol *sym);
SC_FUNC int matchtag_string(int ident, int tag);
SC_FUNC int checktag_string(value *sym1, value *sym2);
SC_FUNC int checktags_string(int tags[], int numtags, value *sym1);

/* function prototypes in SC4.C */
SC_FUNC void writeleader(symbol *root);
SC_FUNC void writetrailer(void);
SC_FUNC void begcseg(void);
SC_FUNC void begdseg(void);
SC_FUNC void setline(int chkbounds);
SC_FUNC void setfiledirect(char *name);
SC_FUNC void setlinedirect(int line);
SC_FUNC void setlabel(int index);
SC_FUNC void markexpr(optmark type,const char *name,cell offset);
SC_FUNC void startfunc(char *fname);
SC_FUNC void endfunc(void);
SC_FUNC void alignframe(int numbytes);
SC_FUNC void rvalue(value *lval);
SC_FUNC void address(symbol *ptr,regid reg);
SC_FUNC void store(value *lval);
SC_FUNC void loadreg(cell address,regid reg);
SC_FUNC void storereg(cell address,regid reg);
SC_FUNC void memcopy(cell size);
SC_FUNC void copyarray(symbol *sym,cell size);
SC_FUNC void fillarray(symbol *sym,cell size,cell value);
SC_FUNC void ldconst(cell val,regid reg);
SC_FUNC void moveto1(void);
SC_FUNC void pushreg(regid reg);
SC_FUNC void pushval(cell val);
SC_FUNC void popreg(regid reg);
SC_FUNC void genarray(int dims, int _autozero);
SC_FUNC void swap1(void);
SC_FUNC void ffswitch(int label);
SC_FUNC void ffcase(cell value,char *labelname,int newtable);
SC_FUNC void ffcall(symbol *sym,const char *label,int numargs);
SC_FUNC void ffret(int remparams);
SC_FUNC void ffabort(int reason);
SC_FUNC void ffbounds(cell size);
SC_FUNC void jumplabel(int number);
SC_FUNC void defstorage(void);
SC_FUNC void modstk(int delta);
SC_FUNC void setstk(cell value);
SC_FUNC void modheap(int delta);
SC_FUNC void modheap_i();
SC_FUNC void setheap_pri(void);
SC_FUNC void setheap(cell value);
SC_FUNC void cell2addr(void);
SC_FUNC void cell2addr_alt(void);
SC_FUNC void addr2cell(void);
SC_FUNC void char2addr(void);
SC_FUNC void charalign(void);
SC_FUNC void addconst(cell value);
SC_FUNC void setheap_save(cell value);
SC_FUNC void stradjust(regid reg);

/*  Code generation functions for arithmetic operators.
 *
 *  Syntax: o[u|s|b]_name
 *          |   |   | +--- name of operator
 *          |   |   +----- underscore
 *          |   +--------- "u"nsigned operator, "s"igned operator or "b"oth
 *          +------------- "o"perator
 */
SC_FUNC void os_mult(void); /* multiplication (signed) */
SC_FUNC void os_div(void);  /* division (signed) */
SC_FUNC void os_mod(void);  /* modulus (signed) */
SC_FUNC void ob_add(void);  /* addition */
SC_FUNC void ob_sub(void);  /* subtraction */
SC_FUNC void ob_sal(void);  /* shift left (arithmetic) */
SC_FUNC void os_sar(void);  /* shift right (arithmetic, signed) */
SC_FUNC void ou_sar(void);  /* shift right (logical, unsigned) */
SC_FUNC void ob_or(void);   /* bitwise or */
SC_FUNC void ob_xor(void);  /* bitwise xor */
SC_FUNC void ob_and(void);  /* bitwise and */
SC_FUNC void ob_eq(void);   /* equality */
SC_FUNC void ob_ne(void);   /* inequality */
SC_FUNC void relop_prefix(void);
SC_FUNC void relop_suffix(void);
SC_FUNC void os_le(void);   /* less or equal (signed) */
SC_FUNC void os_ge(void);   /* greater or equal (signed) */
SC_FUNC void os_lt(void);   /* less (signed) */
SC_FUNC void os_gt(void);   /* greater (signed) */

SC_FUNC void lneg(void);
SC_FUNC void neg(void);
SC_FUNC void invert(void);
SC_FUNC void nooperation(void);
SC_FUNC void inc(value *lval);
SC_FUNC void dec(value *lval);
SC_FUNC void jmp_ne0(int number);
SC_FUNC void jmp_eq0(int number);
SC_FUNC void outval(cell val,int newline);

/* function prototypes in SC5.C */
SC_FUNC int error(int number,...);
SC_FUNC void errorset(int code,int line);

/* function prototypes in SC6.C */
SC_FUNC int assemble(FILE *fout,FILE *fin);

/* function prototypes in SC7.C */
SC_FUNC void stgbuffer_cleanup(void);
SC_FUNC void stgmark(char mark);
SC_FUNC void stgwrite(const char *st);
SC_FUNC void stgout(int index);
SC_FUNC void stgdel(int index,cell code_index);
SC_FUNC int stgget(int *index,cell *code_index);
SC_FUNC void stgset(int onoff);
SC_FUNC int phopt_init(void);
SC_FUNC int phopt_cleanup(void);

/* function prototypes in SCLIST.C */
SC_FUNC char* duplicatestring(const char* sourcestring);
SC_FUNC stringpair *insert_alias(char *name,char *alias);
SC_FUNC stringpair *find_alias(char *name);
SC_FUNC int lookup_alias(char *target,char *name);
SC_FUNC void delete_aliastable(void);
SC_FUNC stringlist *insert_path(char *path);
SC_FUNC char *get_path(int index);
SC_FUNC void delete_pathtable(void);
SC_FUNC stringpair *insert_subst(char *pattern,char *substitution,int prefixlen);
SC_FUNC int get_subst(int index,char **pattern,char **substitution);
SC_FUNC stringpair *find_subst(char *name,int length);
SC_FUNC int delete_subst(char *name,int length);
SC_FUNC void delete_substtable(void);
SC_FUNC stringlist *insert_sourcefile(char *string);
SC_FUNC char *get_sourcefile(int index);
SC_FUNC void delete_sourcefiletable(void);
SC_FUNC stringlist *insert_docstring(char *string);
SC_FUNC char *get_docstring(int index);
SC_FUNC void delete_docstring(int index);
SC_FUNC void delete_docstringtable(void);
SC_FUNC stringlist *insert_autolist(char *string);
SC_FUNC char *get_autolist(int index);
SC_FUNC void delete_autolisttable(void);
SC_FUNC stringlist *insert_dbgfile(const char *filename);
SC_FUNC stringlist *insert_dbgline(int linenr);
SC_FUNC stringlist *insert_dbgsymbol(symbol *sym);
SC_FUNC char *get_dbgstring(int index);
SC_FUNC void delete_dbgstringtable(void);
SC_FUNC stringlist *get_dbgstrings();

/* function prototypes in SCMEMFILE.C */
#if !defined tMEMFILE
  typedef unsigned char MEMFILE;
  #define tMEMFILE  1
#endif
MEMFILE *mfcreate(const char *filename);
void mfclose(MEMFILE *mf);
int mfdump(MEMFILE *mf);
long mflength(const MEMFILE *mf);
long mfseek(MEMFILE *mf,long offset,int whence);
unsigned int mfwrite(MEMFILE *mf,const unsigned char *buffer,unsigned int size);
unsigned int mfread(MEMFILE *mf,unsigned char *buffer,unsigned int size);
char *mfgets(MEMFILE *mf,char *string,unsigned int size);
int mfputs(MEMFILE *mf,const char *string);

/* function prototypes in SCI18N.C */
#define MAXCODEPAGE 12
SC_FUNC int cp_path(const char *root,const char *directory);
SC_FUNC int cp_set(const char *name);
SC_FUNC cell cp_translate(const unsigned char *string,const unsigned char **endptr);
SC_FUNC cell get_utf8_char(const unsigned char *string,const unsigned char **endptr);
SC_FUNC int scan_utf8(FILE *fp,const char *filename);

/* function prototypes in SCSTATE.C */
SC_FUNC constvalue *automaton_add(const char *name);
SC_FUNC constvalue *automaton_find(const char *name);
SC_FUNC constvalue *automaton_findid(int id);
SC_FUNC constvalue *state_add(const char *name,int fsa_id);
SC_FUNC constvalue *state_find(const char *name,int fsa_id);
SC_FUNC constvalue *state_findid(int id);
SC_FUNC void state_buildlist(int **list,int *listsize,int *count,int stateid);
SC_FUNC int state_addlist(int *list,int count,int fsa_id);
SC_FUNC void state_deletetable(void);
SC_FUNC int state_getfsa(int listid);
SC_FUNC int state_count(int listid);
SC_FUNC int state_inlist(int listid,int state);
SC_FUNC int state_listitem(int listid,int index);
SC_FUNC void state_conflict(symbol *root);
SC_FUNC int state_conflict_id(int listid1,int listid2);

/* external variables (defined in scvars.c) */
#if !defined SC_SKIP_VDECL
typedef struct HashTable HashTable;
SC_VDECL struct HashTable *sp_Globals;
SC_VDECL symbol loctab;       /* local symbol table */
SC_VDECL symbol glbtab;       /* global symbol table */
SC_VDECL cell *litq;          /* the literal queue */
SC_VDECL unsigned char pline[]; /* the line read from the input file */
SC_VDECL const unsigned char *lptr;/* points to the current position in "pline" */
SC_VDECL constvalue tagname_tab;/* tagname table */
SC_VDECL constvalue libname_tab;/* library table (#pragma library "..." syntax) */
SC_VDECL constvalue *curlibrary;/* current library */
SC_VDECL int pc_addlibtable;  /* is the library table added to the AMX file? */
SC_VDECL symbol *curfunc;     /* pointer to current function */
SC_VDECL char *inpfname;      /* name of the file currently read from */
SC_VDECL char outfname[];     /* intermediate (assembler) file name */
SC_VDECL char binfname[];     /* binary file name */
SC_VDECL char errfname[];     /* error file name */
SC_VDECL char sc_ctrlchar;    /* the control character (or escape character) */
SC_VDECL char sc_ctrlchar_org;/* the default control character */
SC_VDECL int litidx;          /* index to literal table */
SC_VDECL int litmax;          /* current size of the literal table */
SC_VDECL int stgidx;          /* index to the staging buffer */
SC_VDECL int sc_labnum;       /* number of (internal) labels */
SC_VDECL int staging;         /* true if staging output */
SC_VDECL cell declared;       /* number of local cells declared */
SC_VDECL cell glb_declared;   /* number of global cells declared */
SC_VDECL cell code_idx;       /* number of bytes with generated code */
SC_VDECL int ntv_funcid;      /* incremental number of native function */
SC_VDECL int errnum;          /* number of errors */
SC_VDECL int warnnum;         /* number of warnings */
SC_VDECL int sc_debug;        /* debug/optimization options (bit field) */
SC_VDECL int sc_packstr;      /* strings are packed by default? */
SC_VDECL int sc_asmfile;      /* create .ASM file? */
SC_VDECL int sc_listing;      /* create .LST file? */
SC_VDECL int sc_compress;     /* compress bytecode? */
SC_VDECL int sc_needsemicolon;/* semicolon required to terminate expressions? */
SC_VDECL int sc_dataalign;    /* data alignment value */
SC_VDECL int sc_alignnext;    /* must frame of the next function be aligned? */
SC_VDECL int pc_docexpr;      /* must expression be attached to documentation comment? */
SC_VDECL int sc_showincludes; /* show include files? */
SC_VDECL int curseg;          /* 1 if currently parsing CODE, 2 if parsing DATA */
SC_VDECL cell pc_stksize;     /* stack size */
SC_VDECL cell pc_amxlimit;    /* abstract machine size limit (code + data, or only code) */
SC_VDECL cell pc_amxram;      /* abstract machine data size limit */
SC_VDECL int freading;        /* is there an input file ready for reading? */
SC_VDECL int fline;           /* the line number in the current file */
SC_VDECL short fnumber;       /* number of files in the file table (debugging) */
SC_VDECL short fcurrent;      /* current file being processed (debugging) */
SC_VDECL short sc_intest;     /* true if inside a test */
SC_VDECL int sideeffect;      /* true if an expression causes a side-effect */
SC_VDECL int stmtindent;      /* current indent of the statement */
SC_VDECL int indent_nowarn;   /* skip warning "217 loose indentation" */
SC_VDECL int sc_tabsize;      /* number of spaces that a TAB represents */
SC_VDECL short sc_allowtags;  /* allow/detect tagnames in lex() */
SC_VDECL int sc_status;       /* read/write status */
SC_VDECL int sc_rationaltag;  /* tag for rational numbers */
SC_VDECL int rational_digits; /* number of fractional digits */
SC_VDECL int sc_allowproccall;/* allow/detect tagnames in lex() */
SC_VDECL short sc_is_utf8;    /* is this source file in UTF-8 encoding */
SC_VDECL char *pc_deprecate;  /* if non-NULL, mark next declaration as deprecated */
SC_VDECL int sc_curstates;    /* ID of the current state list */
SC_VDECL int pc_optimize;     /* (peephole) optimization level */
SC_VDECL int pc_memflags;     /* special flags for the stack/heap usage */
SC_VDECL int pc_functag;      /* global function tag */
SC_VDECL int pc_tag_string;   /* global string tag */
SC_VDECL int pc_anytag;       /* global any tag */
SC_VDECL int glbstringread;	  /* last global string read */

SC_VDECL constvalue sc_automaton_tab; /* automaton table */
SC_VDECL constvalue sc_state_tab;     /* state table */

SC_VDECL FILE *inpf;          /* file read from (source or include) */
SC_VDECL FILE *inpf_org;      /* main source file */
SC_VDECL FILE *outf;          /* file written to */

SC_VDECL jmp_buf errbuf;      /* target of longjmp() on a fatal error */

#if !defined SC_LIGHT
  SC_VDECL int sc_makereport; /* generate a cross-reference report */
#endif

#if defined WIN32
#if !defined snprintf
#define snprintf _snprintf
#endif
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

#endif /* SC_H_INCLUDED */
