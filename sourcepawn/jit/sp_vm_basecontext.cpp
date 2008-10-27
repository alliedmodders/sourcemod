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

#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <limits.h>
#include "sp_vm_api.h"
#include "sp_vm_basecontext.h"
#include "sp_vm_engine.h"
#include "x86/jit_x86.h"

using namespace SourcePawn;

#define CELLBOUNDMAX	(INT_MAX/sizeof(cell_t))
#define STACKMARGIN		((cell_t)(16*sizeof(cell_t)))

BaseContext::BaseContext(BaseRuntime *pRuntime)
{
	m_pRuntime = pRuntime;
	m_pPlugin = m_pRuntime->m_pPlugin;

	m_InExec = false;
	m_CustomMsg = false;

	/* Initialize the null references */
	uint32_t index;
	if (FindPubvarByName("NULL_VECTOR", &index) == SP_ERROR_NONE)
	{
		sp_pubvar_t *pubvar;
		GetPubvarByIndex(index, &pubvar);
		m_pNullVec = pubvar->offs;
	}
	else
	{
		m_pNullVec = NULL;
	}

	if (FindPubvarByName("NULL_STRING", &index) == SP_ERROR_NONE)
	{
		sp_pubvar_t *pubvar;
		GetPubvarByIndex(index, &pubvar);
		m_pNullString = pubvar->offs;
	}
	else
	{
		m_pNullString = NULL;
	}

	m_ctx.hp = m_pPlugin->data_size;
	m_ctx.sp = m_pPlugin->mem_size - sizeof(cell_t);
	m_ctx.frm = m_ctx.sp;
	m_ctx.n_err = SP_ERROR_NONE;
	m_ctx.n_idx = SP_ERROR_NONE;
	m_ctx.rp = 0;

	g_Jit.SetupContextVars(m_pRuntime, this, &m_ctx);
}

BaseContext::~BaseContext()
{
	g_Jit.FreeContextVars(&m_ctx);
}

IVirtualMachine *BaseContext::GetVirtualMachine()
{
	return NULL;
}

sp_context_t *BaseContext::GetContext()
{
	return reinterpret_cast<sp_context_t *>((IPluginContext * )this);
}

sp_context_t *BaseContext::GetCtx()
{
	return &m_ctx;
}

bool BaseContext::IsDebugging()
{
	return true;
}

int BaseContext::SetDebugBreak(void *newpfn, void *oldpfn)
{
	return SP_ERROR_ABORTED;
}

IPluginDebugInfo *BaseContext::GetDebugInfo()
{
	return NULL;
}

int BaseContext::Execute(uint32_t code_addr, cell_t *result)
{
	return SP_ERROR_ABORTED;
}

void BaseContext::SetErrorMessage(const char *msg, va_list ap)
{
	m_CustomMsg = true;

	vsnprintf(m_MsgCache, sizeof(m_MsgCache), msg, ap);
}

void BaseContext::_SetErrorMessage(const char *msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	SetErrorMessage(msg, ap);
	va_end(ap);
}

cell_t BaseContext::ThrowNativeErrorEx(int error, const char *msg, ...)
{
	if (!m_InExec)
	{
		return 0;
	}

	m_ctx.n_err = error;
	
	if (msg)
	{
		va_list ap;
		va_start(ap, msg);
		SetErrorMessage(msg, ap);
		va_end(ap);
	}

	return 0;
}

cell_t BaseContext::ThrowNativeError(const char *msg, ...)
{
	if (!m_InExec)
	{
		return 0;
	}

	m_ctx.n_err = SP_ERROR_NATIVE;

	if (msg)
	{
		va_list ap;
		va_start(ap, msg);
		SetErrorMessage(msg, ap);
		va_end(ap);
	}

	return 0;
}

int BaseContext::HeapAlloc(unsigned int cells, cell_t *local_addr, cell_t **phys_addr)
{
	cell_t *addr;
	ucell_t realmem;

#if 0
	if (cells > CELLBOUNDMAX)
	{
		return SP_ERROR_ARAM;
	}
#else
	assert(cells < CELLBOUNDMAX);
#endif

	realmem = cells * sizeof(cell_t);

	/**
	 * Check if the space between the heap and stack is sufficient.
	 */
	if ((cell_t)(m_ctx.sp - m_ctx.hp - realmem) < STACKMARGIN)
	{
		return SP_ERROR_HEAPLOW;
	}

	addr = (cell_t *)(m_pPlugin->memory + m_ctx.hp);
	/* store size of allocation in cells */
	*addr = (cell_t)cells;
	addr++;
	m_ctx.hp += sizeof(cell_t);

	*local_addr = m_ctx.hp;

	if (phys_addr)
	{
		*phys_addr = addr;
	}

	m_ctx.hp += realmem;

	return SP_ERROR_NONE;
}

int BaseContext::HeapPop(cell_t local_addr)
{
	cell_t cellcount;
	cell_t *addr;

	/* check the bounds of this address */
	local_addr -= sizeof(cell_t);
	if (local_addr < (cell_t)m_pPlugin->data_size || local_addr >= m_ctx.sp)
	{
		return SP_ERROR_INVALID_ADDRESS;
	}

	addr = (cell_t *)(m_pPlugin->memory + local_addr);
	cellcount = (*addr) * sizeof(cell_t);
	/* check if this memory count looks valid */
	if ((signed)(m_ctx.hp - cellcount - sizeof(cell_t)) != local_addr)
	{
		return SP_ERROR_INVALID_ADDRESS;
	}

	m_ctx.hp = local_addr;

	return SP_ERROR_NONE;
}


int BaseContext::HeapRelease(cell_t local_addr)
{
	if (local_addr < (cell_t)m_pPlugin->data_size)
	{
		return SP_ERROR_INVALID_ADDRESS;
	}

	m_ctx.hp = local_addr - sizeof(cell_t);

	return SP_ERROR_NONE;
}

int BaseContext::FindNativeByName(const char *name, uint32_t *index)
{
	return m_pRuntime->FindNativeByName(name, index);
}

int BaseContext::GetNativeByIndex(uint32_t index, sp_native_t **native)
{
	return m_pRuntime->GetNativeByIndex(index, native);
}


uint32_t BaseContext::GetNativesNum()
{
	return m_pRuntime->GetNativesNum();
}

int BaseContext::FindPublicByName(const char *name, uint32_t *index)
{
	return m_pRuntime->FindPublicByName(name, index);
}

int BaseContext::GetPublicByIndex(uint32_t index, sp_public_t **pblic)
{
	return m_pRuntime->GetPublicByIndex(index, pblic);
}

uint32_t BaseContext::GetPublicsNum()
{
	return m_pRuntime->GetPublicsNum();
}

int BaseContext::GetPubvarByIndex(uint32_t index, sp_pubvar_t **pubvar)
{
	return m_pRuntime->GetPubvarByIndex(index, pubvar);
}

int BaseContext::FindPubvarByName(const char *name, uint32_t *index)
{
	return m_pRuntime->FindPubvarByName(name, index);
}

int BaseContext::GetPubvarAddrs(uint32_t index, cell_t *local_addr, cell_t **phys_addr)
{
	return m_pRuntime->GetPubvarAddrs(index, local_addr, phys_addr);
}

uint32_t BaseContext::GetPubVarsNum()
{
	return m_pRuntime->GetPubVarsNum();
}

int BaseContext::BindNatives(const sp_nativeinfo_t *natives, unsigned int num, int overwrite)
{
	return SP_ERROR_ABORTED;
}

int BaseContext::BindNative(const sp_nativeinfo_t *native)
{
	return SP_ERROR_ABORTED;
}

int BaseContext::BindNativeToIndex(uint32_t index, SPVM_NATIVE_FUNC func)
{
	return SP_ERROR_ABORTED;
}

int BaseContext::BindNativeToAny(SPVM_NATIVE_FUNC native)
{
	return SP_ERROR_ABORTED;
}

int BaseContext::LocalToPhysAddr(cell_t local_addr, cell_t **phys_addr)
{
	if (((local_addr >= m_ctx.hp) && (local_addr < m_ctx.sp)) 
		|| (local_addr < 0) || ((ucell_t)local_addr >= m_pPlugin->mem_size))
	{
		return SP_ERROR_INVALID_ADDRESS;
	}

	if (phys_addr)
	{
		*phys_addr = (cell_t *)(m_pPlugin->memory + local_addr);
	}

	return SP_ERROR_NONE;
}

int BaseContext::PushCell(cell_t value)
{
	return SP_ERROR_ABORTED;
}

int BaseContext::PushCellsFromArray(cell_t array[], unsigned int numcells)
{
	return SP_ERROR_ABORTED;
}

int BaseContext::PushCellArray(cell_t *local_addr, cell_t **phys_addr, cell_t array[], unsigned int numcells)
{
	return SP_ERROR_ABORTED;
}

int BaseContext::LocalToString(cell_t local_addr, char **addr)
{
	if (((local_addr >= m_ctx.hp) && (local_addr < m_ctx.sp))
		|| (local_addr < 0) || ((ucell_t)local_addr >= m_pPlugin->mem_size))
	{
		return SP_ERROR_INVALID_ADDRESS;
	}
	*addr = (char *)(m_pPlugin->memory + local_addr);

	return SP_ERROR_NONE;
}

int BaseContext::PushString(cell_t *local_addr, char **phys_addr, const char *string)
{
	return SP_ERROR_ABORTED;
}

int BaseContext::StringToLocal(cell_t local_addr, size_t bytes, const char *source)
{
	char *dest;
	size_t len;

	if (((local_addr >= m_ctx.hp) && (local_addr < m_ctx.sp))
		|| (local_addr < 0) || ((ucell_t)local_addr >= m_pPlugin->mem_size))
	{
		return SP_ERROR_INVALID_ADDRESS;
	}

	if (bytes == 0)
	{
		return SP_ERROR_NONE;
	}

	len = strlen(source);
	dest = (char *)(m_pPlugin->memory + local_addr);

	if (len >= bytes)
	{
		len = bytes - 1;
	}

	memmove(dest, source, len);
	dest[len] = '\0';

	return SP_ERROR_NONE;
}

inline int __CheckValidChar(char *c)
{
	int count;
	int bytecount = 0;

	for (count=1; (*c & 0xC0) == 0x80; count++)
	{
		c--;
	}

	switch (*c & 0xF0)
	{
	case 0xC0:
	case 0xD0:
		{
			bytecount = 2;
			break;
		}
	case 0xE0:
		{
			bytecount = 3;
			break;
		}
	case 0xF0:
		{
			bytecount = 4;
			break;
		}
	}

	if (bytecount != count)
	{
		return count;
	}

	return 0;
}

int BaseContext::StringToLocalUTF8(cell_t local_addr, size_t maxbytes, const char *source, size_t *wrtnbytes)
{
	char *dest;
	size_t len;
	bool needtocheck = false;

	if (((local_addr >= m_ctx.hp) && (local_addr < m_ctx.sp))
		|| (local_addr < 0)
		|| ((ucell_t)local_addr >= m_pPlugin->mem_size))
	{
		return SP_ERROR_INVALID_ADDRESS;
	}
	
	if (maxbytes == 0)
	{
		return SP_ERROR_NONE;
	}

	len = strlen(source);
	dest = (char *)(m_pPlugin->memory + local_addr);

	if ((size_t)len >= maxbytes)
	{
		len = maxbytes - 1;
		needtocheck = true;
	}

	memmove(dest, source, len);
	if ((dest[len-1] & 1<<7) && needtocheck)
	{
		len -= __CheckValidChar(dest+len-1);
	}
	dest[len] = '\0';

	if (wrtnbytes)
	{
		*wrtnbytes = len;
	}

	return SP_ERROR_NONE;
}

IPluginFunction *BaseContext::GetFunctionById(funcid_t func_id)
{
	return m_pRuntime->GetFunctionById(func_id);
}

IPluginFunction *BaseContext::GetFunctionByName(const char *public_name)
{
	return m_pRuntime->GetFunctionByName(public_name);
}

int BaseContext::LocalToStringNULL(cell_t local_addr, char **addr)
{
	int err;
	if ((err = LocalToString(local_addr, addr)) != SP_ERROR_NONE)
	{
		return err;
	}

	if ((cell_t *)*addr == m_pNullString)
	{
		*addr = NULL;
	}

	return SP_ERROR_NONE;
}

SourceMod::IdentityToken_t *BaseContext::GetIdentity()
{
	SourceMod::IdentityToken_t *tok;

	if (GetKey(1, (void **)&tok))
	{
		return tok;
	}

	return NULL;
}

cell_t *BaseContext::GetNullRef(SP_NULL_TYPE type)
{
	if (type == SP_NULL_VECTOR)
	{
		return m_pNullVec;
	}

	return NULL;
}

bool BaseContext::IsInExec()
{
	return m_InExec;
}

int BaseContext::Execute2(IPluginFunction *function, const cell_t *params, unsigned int num_params, cell_t *result)
{
	int ir;
	int serial;
	cell_t *sp;
	funcid_t fnid;
	JitFunction *fn;
	sp_public_t *pubfunc;
	cell_t _ignore_result;
	unsigned int public_id;

	fnid = function->GetFunctionID();

	if (fnid & 1)
	{	
		public_id = fnid >> 1;

		if (m_pRuntime->GetPublicByIndex(public_id, &pubfunc) != SP_ERROR_NONE)
		{
			return SP_ERROR_NOT_FOUND;
		}
	}
	else
	{
		return SP_ERROR_INVALID_ADDRESS;
	}

	if (m_pRuntime->IsPaused())
	{
		return SP_ERROR_NOT_RUNNABLE;
	}

	if ((cell_t)(m_ctx.hp + 16*sizeof(cell_t)) > (cell_t)(m_ctx.sp - (sizeof(cell_t) * (num_params + 1))))
	{
		return SP_ERROR_STACKLOW;
	}

	if (result == NULL)
	{
		result = &_ignore_result;
	}

	/* We got this far.  It's time to start profiling. */
	
	if ((m_pPlugin->prof_flags & SP_PROF_CALLBACKS) == SP_PROF_CALLBACKS)
	{
		serial = m_pPlugin->profiler->OnCallbackBegin(this, pubfunc);
	}

	/* See if we have to compile the callee. */
	if ((fn = m_pRuntime->m_PubJitFuncs[public_id]) == NULL)
	{
		uint32_t func_idx;

		/* We might not have to - check pcode offset. */
		if ((func_idx = FuncLookup((CompData *)m_pRuntime->m_pCo, pubfunc->code_offs)) != 0)
		{
			fn = m_pRuntime->GetJittedFunction(func_idx);
			assert(fn != NULL);
			m_pRuntime->m_PubJitFuncs[public_id] = fn;
		}
		else
		{
			if ((fn = g_Jit.CompileFunction(m_pRuntime, pubfunc->code_offs, &ir)) == NULL)
			{
				return ir;
			}
			m_pRuntime->m_PubJitFuncs[public_id] = fn;
		}
	}

	/* Save our previous state. */

	bool save_exec;
	uint32_t save_n_idx;
	cell_t save_sp, save_hp, save_rp, save_cip;

	save_sp = m_ctx.sp;
	save_hp = m_ctx.hp;
	save_exec = m_InExec;
	save_n_idx = m_ctx.n_idx;
	save_rp = m_ctx.rp;
	save_cip = m_ctx.err_cip;

	/* Push parameters */

	m_ctx.sp -= sizeof(cell_t) * (num_params + 1);
	sp = (cell_t *)(m_pPlugin->memory + m_ctx.sp);

	sp[0] = num_params;
	for (unsigned int i = 0; i < num_params; i++)
	{
		sp[i + 1] = params[i];
	}

	/* Clear internal state */
	m_ctx.n_err = SP_ERROR_NONE;
	m_ctx.n_idx = 0;
	m_MsgCache[0] = '\0';
	m_CustomMsg = false;
	m_InExec = true;

	/* Start the frame tracer */

	ir = g_Jit.InvokeFunction(m_pRuntime, fn, result);

	/* Restore some states, stop the frame tracer */

	m_InExec = save_exec;

	if (ir == SP_ERROR_NONE)
	{
		m_ctx.n_err = SP_ERROR_NONE;
		if (m_ctx.sp != save_sp)
		{
			ir = SP_ERROR_STACKLEAK;
			_SetErrorMessage("Stack leak detected: sp:%d should be %d!", 
				m_ctx.sp, 
				save_sp);
		}
		if (m_ctx.hp != save_hp)
		{
			ir = SP_ERROR_HEAPLEAK;
			_SetErrorMessage("Heap leak detected: hp:%d should be %d!", 
				m_ctx.hp, 
				save_hp);
		}
		if (m_ctx.rp != save_rp)
		{
			ir = SP_ERROR_STACKLEAK;
			_SetErrorMessage("Return stack leak detected: rp:%d should be %d!",
				m_ctx.rp,
				save_rp);
		}
	}

	if (ir != SP_ERROR_NONE)
	{
		g_engine1.ReportError(m_pRuntime, ir, m_MsgCache, save_rp);
	}

	m_ctx.sp = save_sp;
	m_ctx.hp = save_hp;
	m_ctx.rp = save_rp;
	
	if ((m_pPlugin->prof_flags & SP_PROF_CALLBACKS) == SP_PROF_CALLBACKS)
	{
		m_pPlugin->profiler->OnCallbackEnd(serial);
	}

	m_ctx.err_cip = save_cip;
	m_ctx.n_idx = save_n_idx;
	m_ctx.n_err = SP_ERROR_NONE;
	m_MsgCache[0] = '\0';
	m_CustomMsg = false;

	return ir;
}

IPluginRuntime *BaseContext::GetRuntime()
{
	return m_pRuntime;
}

DebugInfo::DebugInfo(sp_plugin_t *plugin) : m_pPlugin(plugin)
{
}

#define USHR(x) ((unsigned int)(x)>>1)

int DebugInfo::LookupFile(ucell_t addr, const char **filename)
{
	int high, low, mid;

	high = m_pPlugin->debug.files_num;
	low = -1;

	while (high - low > 1)
	{
		mid = USHR(low + high);
		if (m_pPlugin->debug.files[mid].addr <= addr)
		{
			low = mid;
		} else {
			high = mid;
		}
	}

	if (low == -1)
	{
		return SP_ERROR_NOT_FOUND;
	}

	*filename = m_pPlugin->debug.stringbase + m_pPlugin->debug.files[low].name;

	return SP_ERROR_NONE;
}

int DebugInfo::LookupFunction(ucell_t addr, const char **name)
{
	if (!m_pPlugin->debug.unpacked)
	{
		uint32_t max, iter;
		sp_fdbg_symbol_t *sym;
		sp_fdbg_arraydim_t *arr;
		uint8_t *cursor = (uint8_t *)(m_pPlugin->debug.symbols);

		max = m_pPlugin->debug.syms_num;
		for (iter = 0; iter < max; iter++)
		{
			sym = (sp_fdbg_symbol_t *)cursor;

			if (sym->ident == SP_SYM_FUNCTION
				&& sym->codestart <= addr 
				&& sym->codeend > addr)
			{
				*name = m_pPlugin->debug.stringbase + sym->name;
				return SP_ERROR_NONE;
			}

			if (sym->dimcount > 0)
			{
				cursor += sizeof(sp_fdbg_symbol_t);
				arr = (sp_fdbg_arraydim_t *)cursor;
				cursor += sizeof(sp_fdbg_arraydim_t) * sym->dimcount;
				continue;
			}

			cursor += sizeof(sp_fdbg_symbol_t);
		}

		return SP_ERROR_NOT_FOUND;
	}
	else
	{
		uint32_t max, iter;
		sp_u_fdbg_symbol_t *sym;
		sp_u_fdbg_arraydim_t *arr;
		uint8_t *cursor = (uint8_t *)(m_pPlugin->debug.symbols);

		max = m_pPlugin->debug.syms_num;
		for (iter = 0; iter < max; iter++)
		{
			sym = (sp_u_fdbg_symbol_t *)cursor;

			if (sym->ident == SP_SYM_FUNCTION
				&& sym->codestart <= addr 
				&& sym->codeend > addr)
			{
				*name = m_pPlugin->debug.stringbase + sym->name;
				return SP_ERROR_NONE;
			}

			if (sym->dimcount > 0)
			{
				cursor += sizeof(sp_u_fdbg_symbol_t);
				arr = (sp_u_fdbg_arraydim_t *)cursor;
				cursor += sizeof(sp_u_fdbg_arraydim_t) * sym->dimcount;
				continue;
			}

			cursor += sizeof(sp_u_fdbg_symbol_t);
		}

		return SP_ERROR_NOT_FOUND;
	}
}

int DebugInfo::LookupLine(ucell_t addr, uint32_t *line)
{
	int high, low, mid;

	high = m_pPlugin->debug.lines_num;
	low = -1;

	while (high - low > 1)
	{
		mid = USHR(low + high);
		if (m_pPlugin->debug.lines[mid].addr <= addr)
		{
			low = mid;
		} else {
			high = mid;
		}
	}

	if (low == -1)
	{
		return SP_ERROR_NOT_FOUND;
	}

	/* Since the CIP occurs BEFORE the line, we have to add one */
	*line = m_pPlugin->debug.lines[low].line + 1;

	return SP_ERROR_NONE;
}

#undef USHR

int BaseContext::GetLastNativeError()
{
	return m_ctx.n_err;
}

cell_t *BaseContext::GetLocalParams()
{
	return (cell_t *)(m_pPlugin->memory + m_ctx.frm + (2 * sizeof(cell_t)));
}

void BaseContext::SetKey(int k, void *value)
{
	if (k < 1 || k > 4)
	{
		return;
	}

	m_keys[k - 1] = value;
	m_keys_set[k - 1] = true;
}

bool BaseContext::GetKey(int k, void **value)
{
	if (k < 1 || k > 4 || m_keys_set[k - 1] == false)
	{
		return false;
	}

	*value = m_keys[k - 1];

	return true;
}
