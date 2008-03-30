/**
 * vim: set ts=4 :
 * =============================================================================
 * SourcePawn
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
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

#ifndef _INCLUDE_SOURCEPAWN_VM_TYPES_H
#define _INCLUDE_SOURCEPAWN_VM_TYPES_H

/**
 * @file sp_vm_types.h
 * @brief Contains all run-time SourcePawn structures.
 */

#include "sp_file_headers.h"

typedef	uint32_t	ucell_t;			/**< Unsigned 32bit integer */
typedef int32_t		cell_t;				/**< Basic 32bit signed integer type for plugins */
typedef uint32_t	funcid_t;			/**< Function index code */

#include "sp_typeutil.h"

#define SP_MAX_EXEC_PARAMS				32	/**< Maximum number of parameters in a function */

#define SP_JITCONF_DEBUG		"debug"		/**< Configuration option for debugging. */
#define SP_JITCONF_PROFILE		"profile"	/**< Configuration option for profiling. */

#define SP_PROF_NATIVES			(1<<0)		/**< Profile natives. */
#define SP_PROF_CALLBACKS		(1<<1)		/**< Profile callbacks. */
#define SP_PROF_FUNCTIONS		(1<<2)		/**< Profile functions. */

/**
 * @brief Error codes for SourcePawn routines.
 */
#define SP_ERROR_NONE					0	/**< No error occurred */
#define SP_ERROR_FILE_FORMAT			1	/**< File format unrecognized */
#define SP_ERROR_DECOMPRESSOR			2	/**< A decompressor was not found */
#define SP_ERROR_HEAPLOW				3	/**< Not enough space left on the heap */
#define SP_ERROR_PARAM					4	/**< Invalid parameter or parameter type */
#define SP_ERROR_INVALID_ADDRESS		5	/**< A memory address was not valid */
#define SP_ERROR_NOT_FOUND				6	/**< The object in question was not found */
#define SP_ERROR_INDEX					7	/**< Invalid index parameter */
#define SP_ERROR_STACKLOW				8	/**< Not enough space left on the stack */
#define SP_ERROR_NOTDEBUGGING			9	/**< Debug mode was not on or debug section not found */
#define SP_ERROR_INVALID_INSTRUCTION	10	/**< Invalid instruction was encountered */
#define SP_ERROR_MEMACCESS				11	/**< Invalid memory access */
#define SP_ERROR_STACKMIN				12	/**< Stack went beyond its minimum value */
#define SP_ERROR_HEAPMIN				13  /**< Heap went beyond its minimum value */
#define SP_ERROR_DIVIDE_BY_ZERO			14	/**< Division by zero */
#define SP_ERROR_ARRAY_BOUNDS			15	/**< Array index is out of bounds */
#define SP_ERROR_INSTRUCTION_PARAM		16	/**< Instruction had an invalid parameter */
#define SP_ERROR_STACKLEAK				17  /**< A native leaked an item on the stack */
#define SP_ERROR_HEAPLEAK				18  /**< A native leaked an item on the heap */
#define SP_ERROR_ARRAY_TOO_BIG			19	/**< A dynamic array is too big */
#define SP_ERROR_TRACKER_BOUNDS			20	/**< Tracker stack is out of bounds */
#define SP_ERROR_INVALID_NATIVE			21	/**< Native was pending or invalid */
#define SP_ERROR_PARAMS_MAX				22	/**< Maximum number of parameters reached */
#define SP_ERROR_NATIVE					23	/**< Error originates from a native */
#define SP_ERROR_NOT_RUNNABLE			24	/**< Function or plugin is not runnable */
#define SP_ERROR_ABORTED				25	/**< Function call was aborted */
//Hey you! Update the string table if you add to the end of me! */

/**********************************************
 *** The following structures are reference structures.
 *** They are not essential to the API, but are used
 ***  to hold the back end database format of the plugin
 ***  binary.
 **********************************************/

/**
 * @brief Information about the core plugin tables.  These may or may not be present!
 */
typedef struct sp_plugin_infotab_s
{
	const char *stringbase;		/**< base of string table */
	uint32_t	publics_num;	/**< number of publics */
	sp_file_publics_t *publics;	/**< public table */
	uint32_t	natives_num;	/**< number of natives */
	sp_file_natives_t *natives; /**< native table */
	uint32_t	pubvars_num;	/**< number of pubvars */
	sp_file_pubvars_t *pubvars;	/**< pubvars table */
} sp_plugin_infotab_t;

/**
 * @brief Information about the plugin's debug tables.  These are all present if one is present.
 */
typedef struct sp_plugin_debug_s
{
	const char *stringbase;		/**< base of string table */
	uint32_t	files_num;		/**< number of files */
	sp_fdbg_file_t *files;		/**< files table */
	uint32_t	lines_num;		/**< number of lines */
	sp_fdbg_line_t *lines;		/**< lines table */
	uint32_t	syms_num;		/**< number of symbols */
	sp_fdbg_symbol_t *symbols;	/**< symbol table */
} sp_plugin_debug_t;

#define SP_FA_SELF_EXTERNAL		(1<<0)		/**< Allocation of structure is external */
#define SP_FA_BASE_EXTERNAL		(1<<1)		/**< Allocation of base is external */

/**
 * @brief The rebased memory format of a plugin.  This differs from the on-disk structure 
 * to ensure that the format is properly read.
 */
typedef struct sp_plugin_s
{
	uint8_t		*base;			/**< Base of memory for this plugin. */
	uint8_t		*pcode;			/**< P-Code of plugin */
	uint32_t	pcode_size;		/**< Size of p-code */
	uint8_t		*data;			/**< Data/memory layout */
	uint32_t	data_size;		/**< Size of data */
	uint32_t	memory;			/**< Required memory space */
	uint16_t	flags;			/**< Code flags */
	uint32_t	allocflags;		/**< Allocation flags */
	sp_plugin_infotab_t info;	/**< Base info table */
	sp_plugin_debug_t   debug;	/**< Debug info table */
} sp_plugin_t;


namespace SourcePawn
{
	class IPluginContext;
	class IVirtualMachine;
	class IProfiler;
};

struct sp_context_s;

/**
 * @brief Native callback prototype, passed a context and a parameter stack (0=count, 1+=args).  
 * A cell must be returned.
 */
typedef cell_t (*SPVM_NATIVE_FUNC)(SourcePawn::IPluginContext *, const cell_t *);

/**
 * @brief Fake native callback prototype, passed a context, parameter stack, and private data.
 * A cell must be returned.
 */
typedef cell_t (*SPVM_FAKENATIVE_FUNC)(SourcePawn::IPluginContext *, const cell_t *, void *);

/**********************************************
 *** The following structures are bound to the VM/JIT.
 *** Changing them will result in necessary recompilation.
 **********************************************/

/**
 * @brief Offsets and names to a public function.
 */
typedef struct sp_public_s
{
	funcid_t	funcid;		/**< Encoded function id */
	uint32_t	code_offs;	/**< Relocated code offset */
	const char *name;		/**< Name of function */
} sp_public_t;

/**
 * @brief Offsets and names to public variables.
 *
 * The offset is relocated and the name by default points back to the sp_plugin_infotab_t structure.
 */
typedef struct sp_pubvar_s
{
	cell_t	   *offs;		/**< Pointer to data */
	const char *name;		/**< Name */
} sp_pubvar_t;

#define SP_NATIVE_UNBOUND		(0)		/**< Native is undefined */
#define SP_NATIVE_BOUND			(1)		/**< Native is bound */

#define SP_NTVFLAG_OPTIONAL		(1<<0)	/**< Native is optional */

/** 
 * @brief Native lookup table, by default names point back to the sp_plugin_infotab_t structure.
 */
typedef struct sp_native_s
{
	SPVM_NATIVE_FUNC	pfn;	/**< Function pointer */
	const char *		name;	/**< Name of function */
	uint32_t			status;	/**< Status flags */
	uint32_t			flags;	/**< Native flags */
} sp_native_t;

/** 
 * @brief Used for setting natives from modules/host apps.
 */
typedef struct sp_nativeinfo_s
{
	const char		*name;	/**< Name of the native */
	SPVM_NATIVE_FUNC func;	/**< Address of native implementation */
} sp_nativeinfo_t;

/** 
 * @brief Run-time debug file table
 */
typedef struct sp_debug_file_s
{
	uint32_t		addr;	/**< Address into code */
	const char *	name;	/**< Name of file */
} sp_debug_file_t;

/**
 * @brief Contains run-time debug line table.
 */
typedef struct sp_debug_line_s
{
	uint32_t		addr;	/**< Address into code */
	uint32_t		line;	/**< Line number */
} sp_debug_line_t;

/**
 * @brief These structures are equivalent.
 */
typedef sp_fdbg_arraydim_t	sp_debug_arraydim_t;

/**
 * @brief The majority of this struct is already located in the parent 
 * block.  Thus, only the relocated portions are required.
 */
typedef struct sp_debug_symbol_s
{
	uint32_t		codestart;	/**< Relocated code address */
	uint32_t		codeend;	/**< Relocated code end address */
	const char *	name;		/**< Relocated name */
	sp_debug_arraydim_t *dims;	/**< Relocated dimension struct, if any */
	sp_fdbg_symbol_t	*sym;	/**< Pointer to original symbol */
} sp_debug_symbol_t;

/**
 * Breaks into a debugger
 * Params:
 *  [0] - plugin context
 *  [1] - frm
 *  [2] - cip
 */
typedef int (*SPVM_DEBUGBREAK)(struct sp_context_s *, uint32_t, uint32_t);

#define SPFLAG_PLUGIN_DEBUG		(1<<0)		/**< plugin is in debug mode */
#define SPFLAG_PLUGIN_PAUSED	(1<<1)		/**< plugin is "paused" (blocked from executing) */

/**
 * @brief This is the heart of the VM.  It contains all of the runtime 
 * information about a plugin context. Note that user[0..3] can be used for any user based pointers.  
 * However, vm[0..3] should not be touched, as it is reserved for the VM.
 */
typedef struct sp_context_s
{
	void			*codebase;	/**< Base of generated code and memory */
	sp_plugin_t		*plugin;	/**< Pointer back to parent information */
	SourcePawn::IPluginContext *context;	/**< Pointer to IPluginContext */
	SourcePawn::IVirtualMachine *vmbase;	/**< Pointer to IVirtualMachine */
	void			*user[4];	/**< User specific pointers */
	void			*vm[4];		/**< VM specific pointers */
	uint32_t		flags;		/**< Compilation flags */
	SPVM_DEBUGBREAK dbreak;		/**< Debug break function */
	uint8_t			*memory;	/**< Data chunk */
	ucell_t			mem_size;	/**< Total memory size; */
	cell_t			data_size;	/**< Data chunk size, always starts at 0 */
	cell_t			heap_base;	/**< Where the heap starts */
	cell_t			hp;			/**< Heap pointer */
	cell_t			sp;			/**< Stack pointer */
	cell_t			frm;		/**< Frame pointer */
	uint32_t		pushcount;	/**< Push count */
	int32_t			n_err;		/**< Error code set by a native */
	uint32_t		n_idx;		/**< Current native index being executed */
	sp_public_t		*publics;	/**< Public functions table */
	sp_pubvar_t		*pubvars;	/**< Public variables table */
	sp_native_t		*natives;	/**< Natives table */
	sp_debug_file_t	*files;		/**< Files */
	sp_debug_line_t	*lines;		/**< Lines */
	sp_debug_symbol_t *symbols;	/**< Symbols */
	SourcePawn::IProfiler *profiler;		/**< Pointer to IProfiler */
	uint32_t		prof_flags;	/**< Profiling flags */
} sp_context_t;

#endif //_INCLUDE_SOURCEPAWN_VM_TYPES_H
