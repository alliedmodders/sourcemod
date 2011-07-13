/*  Pawn Abstract Machine (for the Pawn language)
 *
 *  Copyright (c) ITB CompuPhase, 1997-2006
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

#ifndef AMX_H_INCLUDED
#define AMX_H_INCLUDED

#include <stdlib.h>   /* for size_t */
#include <limits.h>

#if defined FREEBSD && !defined __FreeBSD__
  #define __FreeBSD__
#endif
#if defined LINUX || defined __FreeBSD__ || defined __OpenBSD__
  #include <sclinux.h>
#endif

#if defined __GNUC__
  #include <stdint.h>
  #if !defined HAVE_STDINT_H
    #define HAVE_STDINT_H
  #endif
#elif !defined HAVE_STDINT_H
  #if defined __LCC__ || defined __DMC__ || defined LINUX || (defined __WATCOMC__ && __WATCOMC__ >= 1200)
    #if defined HAVE_INTTYPES_H
      #include <inttypes.h>
    #else
      #include <stdint.h>
    #endif
  #elif !defined __STDC_VERSION__ || __STDC_VERSION__ < 199901L
    /* The ISO C99 defines the int16_t and int_32t types. If the compiler got
     * here, these types are probably undefined.
     */
    #if defined __MACH__
      #include <ppc/types.h>
      typedef unsigned short int  uint16_t;
      typedef unsigned long int   uint32_t;
    #elif defined __FreeBSD__
      #include <inttypes.h>
    #else
      typedef short int           int16_t;
      typedef unsigned short int  uint16_t;
      #if defined SN_TARGET_PS2
        typedef int               int32_t;
        typedef unsigned int      uint32_t;
      #else
        typedef long int          int32_t;
        typedef unsigned long int uint32_t;
      #endif
      #if defined __WIN32__ || defined _WIN32 || defined WIN32
        typedef __int64	          int64_t;
        typedef unsigned __int64  uint64_t;
        #define HAVE_I64
      #elif defined __GNUC__
        typedef long long         int64_t;
        typedef unsigned long long uint64_t;
        #define HAVE_I64
      #endif
    #endif
  #endif
  #define HAVE_STDINT_H
#endif
#if defined _LP64 || defined WIN64 || defined _WIN64
  #if !defined __64BIT__
    #define __64BIT__
  #endif
#endif

#if HAVE_ALLOCA_H
  #include <alloca.h>
#endif
#if defined __WIN32__ || defined _WIN32 || defined WIN32 /* || defined __MSDOS__ */
  #if !defined alloca
    #define alloca(n)   _alloca(n)
  #endif
#endif

#if !defined arraysize
  #define arraysize(array)  (sizeof(array) / sizeof((array)[0]))
#endif
#if !defined assert_static
  /* see "Compile-Time Assertions" by Ralf Holly,
   * C/C++ Users Journal, November 2004
   */
  #define assert_static(e) \
    do { \
      enum { assert_static__ = 1/(e) }; \
    } while (0)
#endif

#ifdef  __cplusplus
extern  "C" {
#endif

#if defined PAWN_DLL
  #if !defined AMX_NATIVE_CALL
    #define AMX_NATIVE_CALL __stdcall
  #endif
  #if !defined AMXAPI
    #define AMXAPI          __stdcall
  #endif
#endif

/* calling convention for native functions */
#if !defined AMX_NATIVE_CALL
  #define AMX_NATIVE_CALL
#endif
/* calling convention for all interface functions and callback functions */
#if !defined AMXAPI
  #if defined STDECL
    #define AMXAPI      __stdcall
  #elif defined CDECL
    #define AMXAPI      __cdecl
  #elif defined GCC_HASCLASSVISIBILITY
    #define AMXAPI __attribute__ ((visibility("default")))
  #else
    #define AMXAPI
  #endif
#endif
#if !defined AMXEXPORT
  #define AMXEXPORT
#endif

/* File format version (in CUR_FILE_VERSION)
 *   0 (original version)
 *   1 (opcodes JUMP.pri, SWITCH and CASETBL)
 *   2 (compressed files)
 *   3 (public variables)
 *   4 (opcodes SWAP.pri/alt and PUSHADDR)
 *   5 (tagnames table)
 *   6 (reformatted header)
 *   7 (name table, opcodes SYMTAG & SYSREQ.D)
 *   8 (opcode STMT, renewed debug interface)
 *   9 (macro opcodes)
 * MIN_FILE_VERSION is the lowest file version number that the current AMX
 * implementation supports. If the AMX file header gets new fields, this number
 * often needs to be incremented. MAX_AMX_VERSION is the lowest AMX version that
 * is needed to support the current file version. When there are new opcodes,
 * this number needs to be incremented.
 * The file version supported by the JIT may run behind MIN_AMX_VERSION. So
 * there is an extra constant for it: MAX_FILE_VER_JIT.
 */
#define CUR_FILE_VERSION  9     /* current file version; also the current AMX version */
#define MIN_FILE_VERSION  6     /* lowest supported file format version for the current AMX version */
#define MIN_AMX_VERSION   9     /* minimum AMX version needed to support the current file format */
#define MAX_FILE_VER_JIT  8     /* file version supported by the JIT */
#define MIN_AMX_VER_JIT   8     /* AMX version supported by the JIT */

#if !defined PAWN_CELL_SIZE
  #define PAWN_CELL_SIZE 32     /* by default, use 32-bit cells */
#endif
#if PAWN_CELL_SIZE==16
  typedef uint16_t  ucell;
  typedef int16_t   cell;
#elif PAWN_CELL_SIZE==32
  typedef uint32_t  ucell;
  typedef int32_t   cell;
#elif PAWN_CELL_SIZE==64
  typedef uint64_t  ucell;
  typedef int64_t   cell;
#else
  #error Unsupported cell size (PAWN_CELL_SIZE)
#endif

#define UNPACKEDMAX   (((cell)1 << (sizeof(cell)-1)*8) - 1)
#define UNLIMITED     (~1u >> 1)

struct tagAMX;
typedef cell (AMX_NATIVE_CALL *AMX_NATIVE)(struct tagAMX *amx, cell *params);
typedef int (AMXAPI *AMX_CALLBACK)(struct tagAMX *amx, cell index,
                                   cell *result, cell *params);
typedef int (AMXAPI *AMX_DEBUG)(struct tagAMX *amx);
typedef int (AMXAPI *AMX_IDLE)(struct tagAMX *amx, int AMXAPI Exec(struct tagAMX *, cell *, int));
#if !defined _FAR
  #define _FAR
#endif

#if defined _MSC_VER
  #pragma warning(disable:4103)  /* disable warning message 4103 that complains
                                  * about pragma pack in a header file */
  #pragma warning(disable:4100)  /* "'%$S' : unreferenced formal parameter" */
#endif

/* Some compilers do not support the #pragma align, which should be fine. Some
 * compilers give a warning on unknown #pragmas, which is not so fine...
 */
#if (defined SN_TARGET_PS2 || defined __GNUC__) && !defined AMX_NO_ALIGN
  #define AMX_NO_ALIGN
#endif

#if !defined AMX_NO_ALIGN
  #if defined LINUX || defined __FreeBSD__
    #pragma pack(1)         /* structures must be packed (byte-aligned) */
  #elif defined MACOS && defined __MWERKS__
	#pragma options align=mac68k
  #else
    #pragma pack(push)
    #pragma pack(1)         /* structures must be packed (byte-aligned) */
    #if defined __TURBOC__
      #pragma option -a-    /* "pack" pragma for older Borland compilers */
    #endif
  #endif
#endif

typedef struct tagAMX_NATIVE_INFO {
  const char _FAR *name;
  AMX_NATIVE func;
} AMX_NATIVE_INFO;

#define AMX_USERNUM     4
#define sEXPMAX         19      /* maximum name length for file version <= 6 */
#define sNAMEMAX        63      /* maximum name length of symbol name */

typedef struct tagAMX_FUNCSTUB {
  ucell address;
  char name[sEXPMAX+1];
} AMX_FUNCSTUB;

typedef struct tagFUNCSTUBNT {
  ucell address;
  uint32_t nameofs;
} AMX_FUNCSTUBNT;

/* The AMX structure is the internal structure for many functions. Not all
 * fields are valid at all times; many fields are cached in local variables.
 */
typedef struct tagAMX {
  unsigned char _FAR *base; /* points to the AMX header plus the code, optionally also the data */
  unsigned char _FAR *data; /* points to separate data+stack+heap, may be NULL */
  AMX_CALLBACK callback;
  AMX_DEBUG debug       ; /* debug callback */
  /* for external functions a few registers must be accessible from the outside */
  cell cip              ; /* instruction pointer: relative to base + amxhdr->cod */
  cell frm              ; /* stack frame base: relative to base + amxhdr->dat */
  cell hea              ; /* top of the heap: relative to base + amxhdr->dat */
  cell hlw              ; /* bottom of the heap: relative to base + amxhdr->dat */
  cell stk              ; /* stack pointer: relative to base + amxhdr->dat */
  cell stp              ; /* top of the stack: relative to base + amxhdr->dat */
  int flags             ; /* current status, see amx_Flags() */
  /* user data */
  long usertags[AMX_USERNUM] ;
  void _FAR *userdata[AMX_USERNUM] ;
  /* native functions can raise an error */
  int error             ;
  /* passing parameters requires a "count" field */
  int paramcount;
  /* the sleep opcode needs to store the full AMX status */
  cell pri              ;
  cell alt              ;
  cell reset_stk        ;
  cell reset_hea        ;
  /* extra fields for increased performance */
  cell sysreq_d         ; /* relocated address/value for the SYSREQ.D opcode */
  #if defined JIT
    /* support variables for the JIT */
    int reloc_size      ; /* required temporary buffer for relocations */
    long code_size      ; /* estimated memory footprint of the native code */
  #endif
} AMX;

/* The AMX_HEADER structure is both the memory format as the file format. The
 * structure is used internaly.
 */
typedef struct tagAMX_HEADER {
  int32_t size          ; /* size of the "file" */
  uint16_t magic        ; /* signature */
  char    file_version  ; /* file format version */
  char    amx_version   ; /* required version of the AMX */
  int16_t flags         ;
  int16_t defsize       ; /* size of a definition record */
  int32_t cod           ; /* initial value of COD - code block */
  int32_t dat           ; /* initial value of DAT - data block */
  int32_t hea           ; /* initial value of HEA - start of the heap */
  int32_t stp           ; /* initial value of STP - stack top */
  int32_t cip           ; /* initial value of CIP - the instruction pointer */
  int32_t publics       ; /* offset to the "public functions" table */
  int32_t natives       ; /* offset to the "native functions" table */
  int32_t libraries     ; /* offset to the table of libraries */
  int32_t pubvars       ; /* the "public variables" table */
  int32_t tags          ; /* the "public tagnames" table */
  int32_t nametable     ; /* name table */
} AMX_HEADER;

#if PAWN_CELL_SIZE==16
  #define AMX_MAGIC     0xf1e2
#elif PAWN_CELL_SIZE==32
  #define AMX_MAGIC     0xf1e0
#elif PAWN_CELL_SIZE==64
  #define AMX_MAGIC     0xf1e1
#endif

enum {
  AMX_ERR_NONE,
  /* reserve the first 15 error codes for exit codes of the abstract machine */
  AMX_ERR_EXIT,         /* forced exit */
  AMX_ERR_ASSERT,       /* assertion failed */
  AMX_ERR_STACKERR,     /* stack/heap collision */
  AMX_ERR_BOUNDS,       /* index out of bounds */
  AMX_ERR_MEMACCESS,    /* invalid memory access */
  AMX_ERR_INVINSTR,     /* invalid instruction */
  AMX_ERR_STACKLOW,     /* stack underflow */
  AMX_ERR_HEAPLOW,      /* heap underflow */
  AMX_ERR_CALLBACK,     /* no callback, or invalid callback */
  AMX_ERR_NATIVE,       /* native function failed */
  AMX_ERR_DIVIDE,       /* divide by zero */
  AMX_ERR_SLEEP,        /* go into sleepmode - code can be restarted */
  AMX_ERR_INVSTATE,     /* invalid state for this access */

  AMX_ERR_MEMORY = 16,  /* out of memory */
  AMX_ERR_FORMAT,       /* invalid file format */
  AMX_ERR_VERSION,      /* file is for a newer version of the AMX */
  AMX_ERR_NOTFOUND,     /* function not found */
  AMX_ERR_INDEX,        /* invalid index parameter (bad entry point) */
  AMX_ERR_DEBUG,        /* debugger cannot run */
  AMX_ERR_INIT,         /* AMX not initialized (or doubly initialized) */
  AMX_ERR_USERDATA,     /* unable to set user data field (table full) */
  AMX_ERR_INIT_JIT,     /* cannot initialize the JIT */
  AMX_ERR_PARAMS,       /* parameter error */
  AMX_ERR_DOMAIN,       /* domain error, expression result does not fit in range */
  AMX_ERR_GENERAL,      /* general error (unknown or unspecific error) */
};

/*      AMX_FLAG_CHAR16   0x01     no longer used */
#define AMX_FLAG_DEBUG    0x02  /* symbolic info. available */
#define AMX_FLAG_COMPACT  0x04  /* compact encoding */
#define AMX_FLAG_SLEEP    0x08  /* script uses the sleep instruction (possible re-entry or power-down mode) */
#define AMX_FLAG_NOCHECKS 0x10  /* no array bounds checking; no BREAK opcodes */
#define AMX_FLAG_SYSREQN 0x800  /* script new (optimized) version of SYSREQ opcode */
#define AMX_FLAG_NTVREG 0x1000  /* all native functions are registered */
#define AMX_FLAG_JITC   0x2000  /* abstract machine is JIT compiled */
#define AMX_FLAG_BROWSE 0x4000  /* busy browsing */
#define AMX_FLAG_RELOC  0x8000  /* jump/call addresses relocated */

#define AMX_EXEC_MAIN   (-1)    /* start at program entry point */
#define AMX_EXEC_CONT   (-2)    /* continue from last address */

#define AMX_USERTAG(a,b,c,d)    ((a) | ((b)<<8) | ((long)(c)<<16) | ((long)(d)<<24))

#if !defined AMX_COMPACTMARGIN
  #define AMX_COMPACTMARGIN 64
#endif

/* for native functions that use floating point parameters, the following
 * two macros are convenient for casting a "cell" into a "float" type _without_
 * changing the bit pattern
 */
#if PAWN_CELL_SIZE==32
  #define amx_ftoc(f)   ( * ((cell*)&f) )   /* float to cell */
  #define amx_ctof(c)   ( * ((float*)&c) )  /* cell to float */
#elif PAWN_CELL_SIZE==64
  #define amx_ftoc(f)   ( * ((cell*)&f) )   /* float to cell */
  #define amx_ctof(c)   ( * ((double*)&c) ) /* cell to float */
#else
  #error Unsupported cell size
#endif

#define amx_StrParam(amx,param,result)                                      \
    do {                                                                    \
      cell *amx_cstr_; int amx_length_;                                     \
      amx_GetAddr((amx), (param), &amx_cstr_);                              \
      amx_StrLen(amx_cstr_, &amx_length_);                                  \
      if (amx_length_ > 0 &&                                                \
          ((result) = (void*)alloca((amx_length_ + 1) * sizeof(*(result)))) != NULL) \
        amx_GetString((char*)(result), amx_cstr_, sizeof(*(result))>1, amx_length_ + 1); \
      else (result) = NULL;                                                 \
    } while (0)

uint16_t * AMXAPI amx_Align16(uint16_t *v);
uint32_t * AMXAPI amx_Align32(uint32_t *v);
#if defined _I64_MAX || defined HAVE_I64
  uint64_t * AMXAPI amx_Align64(uint64_t *v);
#endif
int AMXAPI amx_Allot(AMX *amx, int cells, cell *amx_addr, cell **phys_addr);
int AMXAPI amx_Callback(AMX *amx, cell index, cell *result, cell *params);
int AMXAPI amx_Cleanup(AMX *amx);
int AMXAPI amx_Clone(AMX *amxClone, AMX *amxSource, void *data);
int AMXAPI amx_Exec(AMX *amx, cell *retval, int index);
int AMXAPI amx_FindNative(AMX *amx, const char *name, int *index);
int AMXAPI amx_FindPublic(AMX *amx, const char *funcname, int *index);
int AMXAPI amx_FindPubVar(AMX *amx, const char *varname, cell *amx_addr);
int AMXAPI amx_FindTagId(AMX *amx, cell tag_id, char *tagname);
int AMXAPI amx_Flags(AMX *amx,uint16_t *flags);
int AMXAPI amx_GetAddr(AMX *amx,cell amx_addr,cell **phys_addr);
int AMXAPI amx_GetNative(AMX *amx, int index, char *funcname);
int AMXAPI amx_GetPublic(AMX *amx, int index, char *funcname);
int AMXAPI amx_GetPubVar(AMX *amx, int index, char *varname, cell *amx_addr);
int AMXAPI amx_GetString(char *dest,const cell *source, int use_wchar, size_t size);
int AMXAPI amx_GetTag(AMX *amx, int index, char *tagname, cell *tag_id);
int AMXAPI amx_GetUserData(AMX *amx, long tag, void **ptr);
int AMXAPI amx_Init(AMX *amx, void *program);
int AMXAPI amx_InitJIT(AMX *amx, void *reloc_table, void *native_code);
int AMXAPI amx_MemInfo(AMX *amx, long *codesize, long *datasize, long *stackheap);
int AMXAPI amx_NameLength(AMX *amx, int *length);
AMX_NATIVE_INFO * AMXAPI amx_NativeInfo(const char *name, AMX_NATIVE func);
int AMXAPI amx_NumNatives(AMX *amx, int *number);
int AMXAPI amx_NumPublics(AMX *amx, int *number);
int AMXAPI amx_NumPubVars(AMX *amx, int *number);
int AMXAPI amx_NumTags(AMX *amx, int *number);
int AMXAPI amx_Push(AMX *amx, cell value);
int AMXAPI amx_PushArray(AMX *amx, cell *amx_addr, cell **phys_addr, const cell array[], int numcells);
int AMXAPI amx_PushString(AMX *amx, cell *amx_addr, cell **phys_addr, const char *string, int pack, int use_wchar);
int AMXAPI amx_RaiseError(AMX *amx, int error);
int AMXAPI amx_Register(AMX *amx, const AMX_NATIVE_INFO *nativelist, int number);
int AMXAPI amx_Release(AMX *amx, cell amx_addr);
int AMXAPI amx_SetCallback(AMX *amx, AMX_CALLBACK callback);
int AMXAPI amx_SetDebugHook(AMX *amx, AMX_DEBUG debug);
int AMXAPI amx_SetString(cell *dest, const char *source, int pack, int use_wchar, size_t size);
int AMXAPI amx_SetUserData(AMX *amx, long tag, void *ptr);
int AMXAPI amx_StrLen(const cell *cstring, int *length);
int AMXAPI amx_UTF8Check(const char *string, int *length);
int AMXAPI amx_UTF8Get(const char *string, const char **endptr, cell *value);
int AMXAPI amx_UTF8Len(const cell *cstr, int *length);
int AMXAPI amx_UTF8Put(char *string, char **endptr, int maxchars, cell value);

#if PAWN_CELL_SIZE==16
  #define amx_AlignCell(v) amx_Align16(v)
#elif PAWN_CELL_SIZE==32
  #define amx_AlignCell(v) amx_Align32(v)
#elif PAWN_CELL_SIZE==64 && (defined _I64_MAX || defined HAVE_I64)
  #define amx_AlignCell(v) amx_Align64(v)
#else
  #error Unsupported cell size
#endif

#define amx_RegisterFunc(amx, name, func) \
  amx_Register((amx), amx_NativeInfo((name),(func)), 1);

#if !defined AMX_NO_ALIGN
  #if defined LINUX || defined __FreeBSD__
    #pragma pack()    /* reset default packing */
  #elif defined MACOS && defined __MWERKS__
    #pragma options align=reset
  #else
    #pragma pack(pop) /* reset previous packing */
  #endif
#endif

#ifdef  __cplusplus
}
#endif

typedef enum {
  OP_NONE,              /* invalid opcode */
  OP_LOAD_PRI,
  OP_LOAD_ALT,
  OP_LOAD_S_PRI,
  OP_LOAD_S_ALT,
  OP_LREF_PRI,
  OP_LREF_ALT,
  OP_LREF_S_PRI,
  OP_LREF_S_ALT,
  OP_LOAD_I,
  OP_LODB_I,
  OP_CONST_PRI,
  OP_CONST_ALT,
  OP_ADDR_PRI,
  OP_ADDR_ALT,
  OP_STOR_PRI,
  OP_STOR_ALT,
  OP_STOR_S_PRI,
  OP_STOR_S_ALT,
  OP_SREF_PRI,
  OP_SREF_ALT,
  OP_SREF_S_PRI,
  OP_SREF_S_ALT,
  OP_STOR_I,
  OP_STRB_I,
  OP_LIDX,
  OP_LIDX_B,
  OP_IDXADDR,
  OP_IDXADDR_B,
  OP_ALIGN_PRI,
  OP_ALIGN_ALT,
  OP_LCTRL,
  OP_SCTRL,
  OP_MOVE_PRI,
  OP_MOVE_ALT,
  OP_XCHG,
  OP_PUSH_PRI,
  OP_PUSH_ALT,
  OP_PUSH_R,
  OP_PUSH_C,
  OP_PUSH,
  OP_PUSH_S,
  OP_POP_PRI,
  OP_POP_ALT,
  OP_STACK,
  OP_HEAP,
  OP_PROC,
  OP_RET,
  OP_RETN,
  OP_CALL,
  OP_CALL_PRI,
  OP_JUMP,
  OP_JREL,
  OP_JZER,
  OP_JNZ,
  OP_JEQ,
  OP_JNEQ,
  OP_JLESS,
  OP_JLEQ,
  OP_JGRTR,
  OP_JGEQ,
  OP_JSLESS,
  OP_JSLEQ,
  OP_JSGRTR,
  OP_JSGEQ,
  OP_SHL,
  OP_SHR,
  OP_SSHR,
  OP_SHL_C_PRI,
  OP_SHL_C_ALT,
  OP_SHR_C_PRI,
  OP_SHR_C_ALT,
  OP_SMUL,
  OP_SDIV,
  OP_SDIV_ALT,
  OP_UMUL,
  OP_UDIV,
  OP_UDIV_ALT,
  OP_ADD,
  OP_SUB,
  OP_SUB_ALT,
  OP_AND,
  OP_OR,
  OP_XOR,
  OP_NOT,
  OP_NEG,
  OP_INVERT,
  OP_ADD_C,
  OP_SMUL_C,
  OP_ZERO_PRI,
  OP_ZERO_ALT,
  OP_ZERO,
  OP_ZERO_S,
  OP_SIGN_PRI,
  OP_SIGN_ALT,
  OP_EQ,
  OP_NEQ,
  OP_LESS,
  OP_LEQ,
  OP_GRTR,
  OP_GEQ,
  OP_SLESS,
  OP_SLEQ,
  OP_SGRTR,
  OP_SGEQ,
  OP_EQ_C_PRI,
  OP_EQ_C_ALT,
  OP_INC_PRI,
  OP_INC_ALT,
  OP_INC,
  OP_INC_S,
  OP_INC_I,
  OP_DEC_PRI,
  OP_DEC_ALT,
  OP_DEC,
  OP_DEC_S,
  OP_DEC_I,
  OP_MOVS,
  OP_CMPS,
  OP_FILL,
  OP_HALT,
  OP_BOUNDS,
  OP_SYSREQ_PRI,
  OP_SYSREQ_C,
  OP_FILE,    /* obsolete */
  OP_LINE,    /* obsolete */
  OP_SYMBOL,  /* obsolete */
  OP_SRANGE,  /* obsolete */
  OP_JUMP_PRI,
  OP_SWITCH,
  OP_CASETBL,
  OP_SWAP_PRI,
  OP_SWAP_ALT,
  OP_PUSH_ADR,
  OP_NOP,
  OP_SYSREQ_N,
  OP_SYMTAG,  /* obsolete */
  OP_BREAK,
  OP_PUSH2_C,
  OP_PUSH2,
  OP_PUSH2_S,
  OP_PUSH2_ADR,
  OP_PUSH3_C,
  OP_PUSH3,
  OP_PUSH3_S,
  OP_PUSH3_ADR,
  OP_PUSH4_C,
  OP_PUSH4,
  OP_PUSH4_S,
  OP_PUSH4_ADR,
  OP_PUSH5_C,
  OP_PUSH5,
  OP_PUSH5_S,
  OP_PUSH5_ADR,
  OP_LOAD_BOTH,
  OP_LOAD_S_BOTH,
  OP_CONST,
  OP_CONST_S,
  /* ----- */
  OP_SYSREQ_D,
  OP_SYSREQ_ND,
  /* ----- */
  OP_HEAP_I,
  OP_PUSH_H_C,
  OP_GENARRAY,
  OP_NUM_OPCODES
} OPCODE;

#endif /* AMX_H_INCLUDED */
