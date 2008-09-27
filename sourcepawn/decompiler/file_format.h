#ifndef _INCLUDE_SPDECOMP_FILE_FORMAT_H_
#define _INCLUDE_SPDECOMP_FILE_FORMAT_H_

#include <sp_vm_types.h>

struct sp_debug_t
{
	const char *stringbase;		/**< base of string table */
	uint32_t	files_num;		/**< number of files */
	sp_fdbg_file_t *files;		/**< files table */
	uint32_t	lines_num;		/**< number of lines */
	sp_fdbg_line_t *lines;		/**< lines table */
	uint32_t	syms_num;		/**< number of symbols */
	sp_fdbg_symbol_t *symbols;	/**< symbol table */
	sp_fdbg_ntvtab_s *ntv;		/**< native table */
};

struct sp_tag_t
{
	uint32_t id;
	const char *name;
};

struct sp_file_t
{
	uint8_t *base;
	size_t base_size;
	uint8_t *pcode;
	uint32_t pcode_size;
	uint32_t pcode_version;
	uint8_t *data;
	uint32_t data_size;
	uint32_t mem_size;
	sp_public_t *publics;
	uint32_t num_publics;
	sp_pubvar_t *pubvars;
	uint32_t num_pubvars;
	sp_native_t *natives;
	uint32_t num_natives;
	sp_tag_t *tags;
	uint32_t num_tags;
	const char *stringbase;
	uint32_t flags;
	sp_debug_t debug;
};

void Sp_FreePlugin(sp_file_t *plugin);
sp_file_t *Sp_ReadPlugin(const char *file, int *err);
sp_fdbg_symbol_t *Sp_FindFunctionSym(sp_file_t *plugin, ucell_t addr);
sp_tag_t *Sp_FindTag(sp_file_t *plugin, uint32_t tag_id);

#endif //_INCLUDE_SPDECOMP_FILE_FORMAT_H_
