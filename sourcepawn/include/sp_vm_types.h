#ifndef _INCLUDE_SOURCEPAWN_VM_TYPES_H
#define _INCLUDE_SOURCEPAWN_VM_TYPES_H

#include "sp_file_headers.h"

typedef	uint32_t	ucell_t;
typedef int32_t		cell_t;
typedef uint32_t	funcid_t;

#define SP_MAX_EXEC_PARAMS				32	/* Maximum number of parameters in a function */

/**
 * Error codes
 */
#define SP_ERROR_NONE					0
#define SP_ERROR_FILE_FORMAT			1	/* File format unrecognized */
#define SP_ERROR_DECOMPRESSOR			2	/* A decompressor was not found */
#define SP_ERROR_HEAPLOW				3	/* Not enough space left on the heap */
#define SP_ERROR_PARAM					4	/* Invalid parameter or parameter type */
#define SP_ERROR_INVALID_ADDRESS		5	/* A memory address was not valid */
#define SP_ERROR_NOT_FOUND				6	/* The object in question was not found */
#define SP_ERROR_INDEX					7	/* Invalid index parameter */
#define SP_ERROR_STACKLOW				8	/* Nnot enough space left on the stack */
#define SP_ERROR_NOTDEBUGGING			9	/* Debug mode was not on or debug section not found */
#define SP_ERROR_INVALID_INSTRUCTION	10	/* Invalid instruction was encountered */
#define SP_ERROR_MEMACCESS				11	/* Invalid memory access */
#define SP_ERROR_STACKMIN				12	/* Stack went beyond its minimum value */
#define SP_ERROR_HEAPMIN				13  /* Heap went beyond its minimum value */
#define SP_ERROR_DIVIDE_BY_ZERO			14	/* Division by zero */
#define SP_ERROR_ARRAY_BOUNDS			15	/* Array index is out of bounds */
#define SP_ERROR_INSTRUCTION_PARAM		16	/* Instruction had an invalid parameter */
#define SP_ERROR_STACKLEAK				17  /* A native leaked an item on the stack */
#define SP_ERROR_HEAPLEAK				18  /* A native leaked an item on the heap */
#define SP_ERROR_ARRAY_TOO_BIG			19	/* A dynamic array is too big */
#define SP_ERROR_TRACKER_BOUNDS			20	/* Tracker stack is out of bounds */
#define SP_ERROR_INVALID_NATIVE			21	/* Native was pending or invalid */
#define SP_ERROR_PARAMS_MAX				22	/* Maximum number of parameters reached */

/**********************************************
 *** The following structures are reference structures.
 *** They are not essential to the API, but are used
 ***  to hold the back end database format of the plugin
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
	uint32_t libraries_num;		/* number of libraries */
	sp_file_libraries_t *lib;	/* library table */
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
	sp_plugin_infotab_t info;	/* base info table */
	sp_plugin_debug_t   debug;	/* debug info table */
} sp_plugin_t;

/** Forward declarations */

namespace SourcePawn
{
	class IPluginContext;
	class IVirtualMachine;
};

struct sp_context_s;

typedef cell_t (*SPVM_NATIVE_FUNC)(struct sp_context_s *, const cell_t *);

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
	funcid_t	funcid;		/* encoded function id */
	uint32_t	code_offs;	/* code offset */
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

#define SP_NATIVE_UNBOUND		(0)		/* Native is undefined */
#define SP_NATIVE_BOUND			(1)		/* Native is bound */

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
 * Used for setting natives from modules/host apps.
 */
typedef struct sp_nativeinfo_s
{
	const char		*name;
	SPVM_NATIVE_FUNC func;
} sp_nativeinfo_t;

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
 * Breaks into a debugger
 * Params:
 *  [0] - plugin context
 *  [1] - frm
 *  [2] - cip
 */
typedef int (*SPVM_DEBUGBREAK)(SourcePawn::IPluginContext *, uint32_t, uint32_t);

#define SPFLAG_PLUGIN_DEBUG		(1<<0)		/* plugin is in debug mode */

/**
 * This is the heart of the VM.  It contains all of the runtime
 *  information about a plugin context.  
 * Note that user[0..3] can be used for any user based pointers.
 * vm[0..3] should not be touched, as it is reserved for the VM.
 */
typedef struct sp_context_s
{
	/* general/parent information */
	void			*codebase;	/* base of generated code and memory */
	sp_plugin_t		*plugin;	/* pointer back to parent information */
	SourcePawn::IPluginContext *context;	/* pointer to IPluginContext */
	SourcePawn::IVirtualMachine *vmbase;	/* pointer to IVirtualMachine */
	void			*user[4];	/* user specific pointers */
	void			*vm[4];		/* VM specific pointers */
	uint32_t		flags;		/* compilation flags */
	SPVM_DEBUGBREAK dbreak;		/* debug break function */
	/* context runtime information */
	uint8_t			*memory;	/* data chunk */
	ucell_t			mem_size;	/* total memory size; */
	cell_t			data_size;	/* data chunk size, always starts at 0 */
	cell_t			heap_base;	/* where the heap starts */
	/* execution specific data */
	cell_t			hp;			/* heap pointer */
	cell_t			sp;			/* stack pointer */
	cell_t			frm;		/* frame pointer */
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
