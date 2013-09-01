// vim: set ts=4 sw=4 tw=99 noet: 
#ifndef _INCLUDE_SOURCEPAWN_JIT_SHARED_H_
#define _INCLUDE_SOURCEPAWN_JIT_SHARED_H_

#include <sp_vm_api.h>

using namespace SourcePawn;

#define SP_MAX_RETURN_STACK		1024

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
	bool unpacked;				/**< Whether debug structures are unpacked */
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
		sp_plugin_debug_t   debug;	/**< Debug info table */
		size_t		base_size;		/**< Size of the entire plugin base */
		uint8_t		*memory;		/**< Data chunk */
		const char *stringbase;		/**< base of string table */
		sp_public_t	*publics;		/**< Public functions table */
		uint32_t	num_publics;	/**< Number of publics. */
		sp_pubvar_t	*pubvars;		/**< Public variables table */
		uint32_t	num_pubvars;	/**< Number of public variables */
		sp_native_t	*natives;		/**< Natives table */
		uint32_t	num_natives;	/**< Number of natives */
		IProfiler *profiler;		/**< Pointer to IProfiler */
		uint32_t	prof_flags;		/**< Profiling flags */
		uint32_t	run_flags;		/**< Runtime flags */
		uint32_t	pcode_version;	/**< P-Code version number */
		char		*name;			/**< Plugin/script name */
	} sp_plugin_t;
}

struct tracker_t;
class BaseContext;

typedef struct sp_context_s
{
	cell_t			hp;				/**< Heap pointer */
	cell_t			sp;				/**< Stack pointer */
	cell_t			frm;			/**< Frame pointer */
	cell_t			rval;			/**< Return value from InvokeFunction() */
	int32_t			cip;			/**< Code pointer last error occurred in */
	int32_t			err;			/**< Error last set by interpreter */
	int32_t			n_err;			/**< Error code set by a native */
	uint32_t		n_idx;			/**< Current native index being executed */
	tracker_t 		*tracker;
	sp_plugin_t 	*plugin;
	BaseContext		*basecx;
	void *			vm[8];			/**< VM-specific pointers */
	cell_t			rp;				/**< Return stack pointer */
	cell_t			rstk_cips[SP_MAX_RETURN_STACK];
} sp_context_t;

//#define SPFLAG_PLUGIN_DEBUG			(1<<0)
#define SPFLAG_PLUGIN_PAUSED		(1<<1)

#define INVALID_CIP			0xFFFFFFFF

#endif //_INCLUDE_SOURCEPAWN_JIT_SHARED_H_
