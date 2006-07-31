#ifndef _INCLUDE_SOURCEPAWN_VM_TYPES_H
#define _INCLUDE_SOURCEPAWN_VM_TYPES_H

#include "sp_file_headers.h"

typedef	uint32_t	ucell_t;
typedef int32_t		cell_t;

/**
 * Error codes
 */
#define SP_ERR_NONE					0
#define SP_ERR_FILE_FORMAT			1	/* File format unrecognized */
#define SP_ERR_DECOMPRESSOR			2	/* A decompressor was not found */
#define SP_ERR_HEAPLOW				3	/* Not enough space left on the heap */
#define SP_ERR_PARAM				4	/* Invalid parameter */
#define SP_ERR_INVALID_ADDRESS		5	/* A memory address was not valid */

/**********************************************
 *** The following structures are reference structures.
 *** They are not essential to the API, but are used
 ***  to hold the backend database format of the plugin
 ***  binary.
 **********************************************/

/**
 * Information about the core plugin tables.
 * These may or may not be present!
 */
typedef struct sp_plugin_infotab_s
{
	const char *stringbase;		/* base of string table */
	uint32_t	publics_num;	/* number of publics */
	sp_file_publics_t *publics;	/* public table */
	uint32_t	natives_num;	/* number of natives */
	sp_file_natives_t *natives; /* native table */
	uint32_t	pubvars_num;	/* number of pubvars */
	sp_file_pubvars_t *pubvars;	/* pubvars table */	
} sp_plugin_infotab_t;

/**
 * Information about the plugin's debug tables.
 * These are all present if one is present.
 */
typedef struct sp_plugin_debug_s
{
	const char *stringbase;		/* base of string table */
	uint32_t	files_num;		/* number of files */
	sp_fdbg_file_t *files;		/* files table */
	uint32_t	lines_num;		/* number of lines */
	sp_fdbg_line_t *lines;		/* lines table */
	uint32_t	syms_num;		/* number of symbols */
	sp_fdbg_symbol_t *symbols;	/* symbol table */
} sp_plugin_debug_t;

#define SP_FA_SELF_EXTERNAL		(1<<0)
#define SP_FA_BASE_EXTERNAL		(1<<1)

/**
 * The rebased, in-memory format of a plugin.
 * This differs from the on-disk structure to ensure
 *  that the format is properly read.
 */
typedef struct sp_plugin_s
{
	uint8_t		*base;			/* base of memory */
	uint8_t		*pcode;			/* p-code */
	uint32_t	pcode_size;		/* size of p-code */
	uint8_t		*data;			/* data size */
	uint32_t	data_size;		/* size of data */
	uint32_t	memory;			/* required memory */
	uint16_t	flags;			/* code flags */
	uint32_t	allocflags;		/* allocation flags */
	sp_plugin_infotab_t *info;	/* base info table */
	sp_plugin_debug_t *debug;	/* debug info table */
} sp_plugin_t;

struct sp_context_s;
typedef int (*SPVM_NATIVE_FUNC)(struct sp_context_s *, cell_t *);

/**********************************************
 *** The following structures are bound to the VM/JIT.
 *** Changing them will result in necessary recompilation.
 **********************************************/

/**
 * Offsets and names to a public function.
 * By default, these point back to the string table 
 *  in the sp_plugin_infotab_t structure.
 */
typedef struct sp_public_s
{
	uint32_t	offs;		/* code offset */
	const char *name;		/* name */
} sp_public_t;

/**
 * Offsets and names to public variables.
 * The offset is relocated and the name by default
 *  points back to the sp_plugin_infotab_t structure.
 */
typedef struct sp_pubvar_s
{
	cell_t	   *offs;		/* pointer to data */
	const char *name;		/* name */
} sp_pubvar_t;

#define SP_NATIVE_NONE		(0)		/* Native is not yet found */
#define SP_NATIVE_OKAY		(1)		/* Native has been added */
#define SP_NATIVE_PENDING	(2)		/* Native is marked as usable but replaceable */

/** 
 * Native lookup table, by default names
 *  point back to the sp_plugin_infotab_t structure.
 * A native is NULL if unit
 */
typedef struct sp_native_s
{
	SPVM_NATIVE_FUNC	pfn;	/* function pointer */
	const char *		name;	/* name of function */
	uint32_t			status;	/* status flags */
} sp_native_t;

/** 
 * Debug file table
 */
typedef struct sp_debug_file_s
{
	uint32_t		addr;	/* address into code */
	const char *	name;	/* name of file */
} sp_debug_file_t;

/**
 * Note that line is missing.  It is not necessary since
 *  this can be retrieved from the base plugin info.
 */
typedef struct sp_debug_line_s
{
	uint32_t		addr;	/* address into code */
	uint32_t		line;	/* line no. */
} sp_debug_line_t;

typedef sp_fdbg_arraydim_t	sp_debug_arraydim_t;

/**
 * The majority of this struct is already located in the parent 
 *  block.  Thus, only the relocated portions are required.
 */
typedef struct sp_debug_symbol_s
{
	uint32_t		codestart;	/* relocated code address */
	uint32_t		codeend;	/* relocated code end address */
	const char *	name;		/* relocated name */
	sp_debug_arraydim_t *dims;	/* relocated dimension struct, if any */
	sp_fdbg_symbol_t	*sym;	/* pointer to original symbol */
} sp_debug_symbol_t;

/**
 * Executes a Context.
 * @sp_context_s	- Execution Context
 * @uint32_t		- Offset from code pointer
 * @res				- return value of function
 * @return			- error code (0=none)
 */
typedef int (*SPVM_EXEC)(struct sp_context_s *,
							uint32_t,
							cell_t *res);

#define SP_CONTEXT_DEBUG			(1<<0)		/* in debug mode */
#define SP_CONTEXT_INHERIT_MEMORY	(1<<1)		/* inherits memory pointers */
#define SP_CONTEXT_INHERIT_CODE		(1<<2)		/* inherits code pointers */

/**
 * This is the heart of the VM.  It contains all of the runtime
 *  information about a plugin context.  
 * It is split into three sections.
 */
typedef struct sp_context_s
{
	/* parent information */
	void			*base;		/* base of generated code and memory */
	sp_plugin_t		*plugin;	/* pointer back to parent information */
	struct sp_context_s *parent;	/* pointer to parent context */
	uint32_t		flags;		/* context flags */
	/* execution specific data */
	SPVM_EXEC		exec;		/* execution base */
	cell_t			pri;		/* PRI register */
	cell_t			alt;		/* ALT register */
	uint8_t			*data;		/* data chunk */
	cell_t			heapbase;	/* heap base */
	cell_t			hp;			/* heap pointer */
	cell_t			sp;			/* stack pointer */
	ucell_t			memory;		/* total memory size; */
	int32_t			err;		/* error code */
	uint32_t		pushcount;	/* push count */
	/* context rebased database */
	sp_public_t		*publics;	/* public functions table */
	sp_pubvar_t		*pubvars;	/* public variables table */
	sp_native_t		*natives;	/* natives table */
	sp_debug_file_t	*files;		/* files */
	sp_debug_line_t	*lines;		/* lines */
	sp_debug_symbol_t *symbols;	/* symbols */
} sp_context_t;
								
#endif //_INCLUDE_SOURCEPAWN_VM_TYPES_H

