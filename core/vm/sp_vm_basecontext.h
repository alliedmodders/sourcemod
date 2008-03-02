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

#include "sp_vm_api.h"
#include "sp_vm_function.h"

/**
 * :TODO: Make functions allocate as a lump instead of individual allocations!
 */

extern IProfiler *sm_profiler;

namespace SourcePawn
{
	class BaseContext : 
		public IPluginContext,
		public IPluginDebugInfo
	{
	public:
		BaseContext(sp_context_t *ctx);
		~BaseContext();
	public: //IPluginContext
		IVirtualMachine *GetVirtualMachine();
		sp_context_t *GetContext();
		bool IsDebugging();
		int SetDebugBreak(SPVM_DEBUGBREAK newpfn, SPVM_DEBUGBREAK *oldpfn);
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
		int PushCell(cell_t value);
		int PushCellArray(cell_t *local_addr, cell_t **phys_addr, cell_t array[], unsigned int numcells);
		int PushString(cell_t *local_addr, char **phys_addr, const char *string);
		int PushCellsFromArray(cell_t array[], unsigned int numcells);
		int BindNatives(const sp_nativeinfo_t *natives, unsigned int num, int overwrite);
		int BindNative(const sp_nativeinfo_t *native);
		int BindNativeToAny(SPVM_NATIVE_FUNC native);
		int Execute(uint32_t code_addr, cell_t *result);
		cell_t ThrowNativeErrorEx(int error, const char *msg, ...);
		cell_t ThrowNativeError(const char *msg, ...);
		IPluginFunction *GetFunctionByName(const char *public_name);
		IPluginFunction *GetFunctionById(funcid_t func_id);
#if defined SOURCEMOD_BUILD
		SourceMod::IdentityToken_t *GetIdentity();
		void SetIdentity(SourceMod::IdentityToken_t *token);
		cell_t *GetNullRef(SP_NULL_TYPE type);
		int LocalToStringNULL(cell_t local_addr, char **addr);
#endif
		int BindNativeToIndex(uint32_t index, SPVM_NATIVE_FUNC native);
	public: //IPluginDebugInfo
		int LookupFile(ucell_t addr, const char **filename);
		int LookupFunction(ucell_t addr, const char **name);
		int LookupLine(ucell_t addr, uint32_t *line);
	public:
		void SetContext(sp_context_t *_ctx);
		bool IsInExec();
	private:
		void SetErrorMessage(const char *msg, va_list ap);
		void FlushFunctionCache();
		void RefreshFunctionCache();
	private:
		sp_context_t *ctx;
#if defined SOURCEMOD_BUILD
		SourceMod::IdentityToken_t *m_pToken;
		cell_t *m_pNullVec;
		cell_t *m_pNullString;
#endif
		char m_MsgCache[1024];
		bool m_CustomMsg;
		bool m_InExec;
		unsigned int m_funcsnum;
#if 0
		CFunction **m_priv_funcs;
#endif
		CFunction **m_pub_funcs;
	};
};

#endif //_INCLUDE_SOURCEPAWN_BASECONTEXT_H_
