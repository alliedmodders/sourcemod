#ifndef _INCLUDE_SOURCEPAWN_JIT_SHARED_H_
#define _INCLUDE_SOURCEPAWN_JIT_SHARED_H_

#include <sp_vm_api.h>

using namespace SourcePawn;

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

class BaseContext;

/**
* Breaks into a debugger
* Params:
*  [0] - plugin context
*  [1] - frm
*  [2] - cip
*/
typedef int (*SPVM_DEBUGBREAK)(BaseContext *, uint32_t, uint32_t);

/**
 * @brief The rebased memory format of a plugin.  This differs from the on-disk structure 
 * to ensure that the format is properly read.
 */
namespace SourcePawn
{
	typedef struct sp_plugin_s
	{
		uint8_t		*base;			/**< Base of memory for this plugin. */
		uint8_t		*pcode;			/**< P-Code of plugin */
		uint32_t	pcode_size;		/**< Size of p-code */
		uint8_t		*data;			/**< Data/memory layout */
		uint32_t	data_size;		/**< Size of data */
		uint32_t	mem_size;		/**< Required memory space */
		uint16_t	flags;			/**< Code flags */
		sp_plugin_infotab_t info;	/**< Base info table */
		sp_plugin_debug_t   debug;	/**< Debug info table */
		void		*codebase;		/**< Base of generated code and memory */
		SPVM_DEBUGBREAK dbreak;		/**< Debug break function */
		uint8_t		*memory;		/**< Data chunk */
		sp_public_t	*publics;		/**< Public functions table */
		sp_pubvar_t	*pubvars;		/**< Public variables table */
		sp_native_t	*natives;		/**< Natives table */
		sp_debug_file_t	*files;		/**< Files */
		sp_debug_line_t	*lines;		/**< Lines */
		sp_debug_symbol_t *symbols;	/**< Symbols */
		IProfiler *profiler;		/**< Pointer to IProfiler */
		uint32_t	prof_flags;		/**< Profiling flags */
		uint32_t	run_flags;		/**< Runtime flags */
	} sp_plugin_t;
}

typedef struct sp_context_s
{
	cell_t			hp;			/**< Heap pointer */
	cell_t			sp;			/**< Stack pointer */
	cell_t			frm;		/**< Frame pointer */
	int32_t			n_err;		/**< Error code set by a native */
	uint32_t		n_idx;		/**< Current native index being executed */
	void *			vm[8];		/**< VM-specific pointers */
} sp_context_t;

#define SPFLAG_PLUGIN_DEBUG			(1<<0)
#define SPFLAG_PLUGIN_PAUSED		(1<<1)

#endif //_INCLUDE_SOURCEPAWN_JIT_SHARED_H_
