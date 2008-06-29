/**
 * vim: set ts=4 :
 * =============================================================================
 * SourcePawn
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEPAWN_BASECONTEXT_H_
#define _INCLUDE_SOURCEPAWN_BASECONTEXT_H_

#include <sp_vm_api.h>
#include "sp_vm_function.h"
#include "sp_vm_engine.h"

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

/**
* @brief The rebased memory format of a plugin.  This differs from the on-disk structure 
* to ensure that the format is properly read.
*/
struct sp_plugin_t
{
	uint8_t		*base;			/**< Base of memory for this plugin. */
	uint8_t		*pcode;			/**< P-Code of plugin */
	uint32_t	pcode_size;		/**< Size of p-code */
	uint8_t		*data;			/**< Data/memory layout */
	uint32_t	data_size;		/**< Size of data */
	uint32_t	memory;			/**< Required memory space */
	uint16_t	flags;			/**< Code flags */
	sp_plugin_infotab_t info;	/**< Base info table */
	sp_plugin_debug_t   debug;	/**< Debug info table */
	sp_native_t *natives;		/**< Native mapping table */
	sp_public_t	*publics;		/**< Public functions table */
	sp_pubvar_t	*pubvars;		/**< Public variables table */
	SourcePawn::IProfiler *profiler;		/**< Pointer to IProfiler */
	uint32_t prof_flags;		/**< Profiling flags */
};

class BaseContext : 
	public IPluginContext,
	public IPluginDebugInfo
{
public:
	BaseContext(sp_plugin_t *pl);
	~BaseContext();
public: //IPluginContext
	bool IsDebugging();
	IPluginDebugInfo *GetDebugInfo();
	int HeapAlloc(unsigned int cells, cell_t *local_addr, cell_t **phys_addr);
	int HeapPop(cell_t local_addr);
	int HeapRelease(cell_t local_addr);
	int FindNativeByName(const char *name, uint32_t *index);
	int GetNativeByIndex(uint32_t index, sp_native_t **native);
	uint32_t GetNativesNum();
	int FindPublicByName(const char *name, uint32_t *index);
	int GetPublicByIndex(uint32_t index, sp_public_t **publicptr);
	uint32_t GetPublicsNum();
	int GetPubvarByIndex(uint32_t index, sp_pubvar_t **pubvar);
	int FindPubvarByName(const char *name, uint32_t *index);
	int GetPubvarAddrs(uint32_t index, cell_t *local_addr, cell_t **phys_addr);
	uint32_t GetPubVarsNum();
	int LocalToPhysAddr(cell_t local_addr, cell_t **phys_addr);
	int LocalToString(cell_t local_addr, char **addr);
	int StringToLocal(cell_t local_addr, size_t chars, const char *source);
	int StringToLocalUTF8(cell_t local_addr, size_t maxbytes, const char *source, size_t *wrtnbytes);
	int BindNative(const sp_nativeinfo_t *native);
	int BindNativeToAny(SPVM_NATIVE_FUNC native);
	cell_t ThrowNativeErrorEx(int error, const char *msg, ...);
	cell_t ThrowNativeError(const char *msg, ...);
	IPluginFunction *GetFunctionByName(const char *public_name);
	IPluginFunction *GetFunctionById(funcid_t func_id);
	int BindNativeToIndex(uint32_t index, SPVM_NATIVE_FUNC native);
	bool IsInExec();
	const sp_context_t *GetContext();
public: //IPluginDebugInfo
	int LookupFile(ucell_t addr, const char **filename);
	int LookupFunction(ucell_t addr, const char **name);
	int LookupLine(ucell_t addr, uint32_t *line);
public:
	unsigned int GetProfCallbackSerial();
private:
	void SetErrorMessage(const char *msg, va_list ap);
private:
	sp_plugin_t *m_pPlugin;
	char m_MsgCache[1024];
	bool m_CustomMsg;
	bool m_InExec;
	sp_context_t m_ctx;
	CFunction **m_PubFuncs;
public:
	SourcePawnEngine *m_pEngine;
};

#endif //_INCLUDE_SOURCEPAWN_BASECONTEXT_H_
