/*  Abstract Machine for the Pawn compiler, debugger support
 *
 *  This file contains extra definitions that are convenient for debugger
 *  support.
 *
 *  Copyright (c) ITB CompuPhase, 2005
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

#ifndef AMXDBG_H_INCLUDED
#define AMXDBG_H_INCLUDED

#ifndef AMX_H_INCLUDED
  #include "amx.h"
#endif

#ifdef  __cplusplus
extern  "C" {
#endif

/* Some compilers do not support the #pragma align, which should be fine. Some
 * compilers give a warning on unknown #pragmas, which is not so fine...
 */
#if defined SN_TARGET_PS2 || defined __GNUC__
  #define AMX_NO_ALIGN
#endif

#if defined __GNUC__
  #define PACKED        __attribute__((packed))
#else
  #define PACKED
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

typedef struct tagAMX_DBG_HDR {
  int32_t size          PACKED; /* size of the debug information chunk */
  uint16_t magic        PACKED; /* signature, must be 0xf1ef */
  char    file_version;         /* file format version */
  char    amx_version;          /* required version of the AMX */
  int16_t flags         PACKED; /* currently unused */
  int16_t files         PACKED; /* number of entries in the "file" table */
  int16_t lines         PACKED; /* number of entries in the "line" table */
  int16_t symbols       PACKED; /* number of entries in the "symbol" table */
  int16_t tags          PACKED; /* number of entries in the "tag" table */
  int16_t automatons    PACKED; /* number of entries in the "automaton" table */
  int16_t states        PACKED; /* number of entries in the "state" table */
} PACKED AMX_DBG_HDR;
#define AMX_DBG_MAGIC   0xf1ef

typedef struct tagAMX_DBG_FILE {
  ucell   address       PACKED; /* address in the code segment where generated code (for this file) starts */
  const char name[1];           /* ASCII string, zero-terminated */
} PACKED AMX_DBG_FILE;

typedef struct tagAMX_DBG_LINE {
  ucell   address       PACKED; /* address in the code segment where generated code (for this line) starts */
  int32_t line          PACKED; /* line number */
} PACKED AMX_DBG_LINE;

typedef struct tagAMX_DBG_SYMBOL {
  ucell   address       PACKED; /* address in the data segment or relative to the frame */
  int16_t tag           PACKED; /* tag for the symbol */
  ucell   codestart     PACKED; /* address in the code segment from which this symbol is valid (in scope) */
  ucell   codeend       PACKED; /* address in the code segment until which this symbol is valid (in scope) */
  char    ident;                /* kind of symbol (function/variable) */
  char    vclass;               /* class of symbol (global/local) */
  int16_t dim           PACKED; /* number of dimensions */
  const char name[1];           /* ASCII string, zero-terminated */
} PACKED AMX_DBG_SYMBOL;

typedef struct tagAMX_DBG_SYMDIM {
  int16_t tag           PACKED; /* tag for the array dimension */
  ucell   size          PACKED; /* size of the array dimension */
} PACKED AMX_DBG_SYMDIM;

typedef struct tagAMX_DBG_TAG {
  int16_t tag           PACKED; /* tag id */
  const char name[1];           /* ASCII string, zero-terminated */
} PACKED AMX_DBG_TAG;

typedef struct tagAMX_DBG_MACHINE {
  int16_t automaton     PACKED; /* automaton id */
  ucell address         PACKED; /* address of state variable */
  const char name[1];           /* ASCII string, zero-terminated */
} PACKED AMX_DBG_MACHINE;

typedef struct tagAMX_DBG_STATE {
  int16_t state         PACKED; /* state id */
  int16_t automaton     PACKED; /* automaton id */
  const char name[1];           /* ASCII string, zero-terminated */
} PACKED AMX_DBG_STATE;

typedef struct tagAMX_DBG {
  AMX_DBG_HDR     _FAR *hdr         PACKED; /* points to the AMX_DBG header */
  AMX_DBG_FILE    _FAR **filetbl    PACKED;
  AMX_DBG_LINE    _FAR *linetbl     PACKED;
  AMX_DBG_SYMBOL  _FAR **symboltbl  PACKED;
  AMX_DBG_TAG     _FAR **tagtbl     PACKED;
  AMX_DBG_MACHINE _FAR **automatontbl PACKED;
  AMX_DBG_STATE   _FAR **statetbl   PACKED;
} PACKED AMX_DBG;

#if !defined iVARIABLE
  #define iVARIABLE  1  /* cell that has an address and that can be fetched directly (lvalue) */
  #define iREFERENCE 2  /* iVARIABLE, but must be dereferenced */
  #define iARRAY     3
  #define iREFARRAY  4  /* an array passed by reference (i.e. a pointer) */
  #define iFUNCTN    9
#endif


int AMXAPI dbg_FreeInfo(AMX_DBG *amxdbg);
int AMXAPI dbg_LoadInfo(AMX_DBG *amxdbg, void *dbg_addr);

int AMXAPI dbg_LookupFile(AMX_DBG *amxdbg, ucell address, const char **filename);
int AMXAPI dbg_LookupFunction(AMX_DBG *amxdbg, ucell address, const char **funcname);
int AMXAPI dbg_LookupLine(AMX_DBG *amxdbg, ucell address, long *line);

int AMXAPI dbg_GetFunctionAddress(AMX_DBG *amxdbg, const char *funcname, const char *filename, ucell *address);
int AMXAPI dbg_GetLineAddress(AMX_DBG *amxdbg, long line, const char *filename, ucell *address);
int AMXAPI dbg_GetAutomatonName(AMX_DBG *amxdbg, int automaton, const char **name);
int AMXAPI dbg_GetStateName(AMX_DBG *amxdbg, int state, const char **name);
int AMXAPI dbg_GetTagName(AMX_DBG *amxdbg, int tag, const char **name);
int AMXAPI dbg_GetVariable(AMX_DBG *amxdbg, const char *symname, ucell scopeaddr, const AMX_DBG_SYMBOL **sym);
int AMXAPI dbg_GetArrayDim(AMX_DBG *amxdbg, const AMX_DBG_SYMBOL *sym, const AMX_DBG_SYMDIM **symdim);


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

#endif /* AMXDBG_H_INCLUDED */

