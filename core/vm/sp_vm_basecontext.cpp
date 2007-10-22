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

#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <limits.h>
#include "sp_vm_api.h"
#include "sp_vm_basecontext.h"
#include "sp_vm_engine.h"

#ifdef SOURCEMOD_BUILD
#include "Logger.h"
#include "DebugReporter.h"
#endif

using namespace SourcePawn;

extern SourcePawnEngine g_SourcePawn;

#define CELLBOUNDMAX	(INT_MAX/sizeof(cell_t))
#define STACKMARGIN		((cell_t)(16*sizeof(cell_t)))

int GlobalDebugBreak(sp_context_t *ctx, uint32_t frm, uint32_t cip)
{
	g_SourcePawn.RunTracer(ctx, frm, cip);

	return SP_ERROR_NONE;
}

BaseContext::BaseContext(sp_context_t *_ctx)
{
	ctx = _ctx;
	ctx->context = this;
	ctx->dbreak = GlobalDebugBreak;
	m_InExec = false;
	m_CustomMsg = false;
	m_funcsnum = ctx->vmbase->FunctionCount(ctx);
#if 0
	m_priv_funcs = NULL;
#endif
	m_pub_funcs = NULL;

#if 0
	/**
	 * Note: Since the m_plugin member will never change,
	 * it is safe to assume the function count will never change
	 */
	if (m_funcsnum && m_priv_funcs == NULL)
	{
		m_priv_funcs = new CFunction *[m_funcsnum];
		memset(m_priv_funcs, 0, sizeof(CFunction *) * m_funcsnum);
	} else {
		m_priv_funcs = NULL;
	}
#endif

	if (ctx->plugin->info.publics_num && m_pub_funcs == NULL)
	{
		m_pub_funcs = new CFunction *[ctx->plugin->info.publics_num];
		memset(m_pub_funcs, 0, sizeof(CFunction *) * ctx->plugin->info.publics_num);
	} else {
		m_pub_funcs = NULL;
	}

	/* Initialize the null references */
	uint32_t index;
	if (FindPubvarByName("NULL_VECTOR", &index) == SP_ERROR_NONE)
	{
		sp_pubvar_t *pubvar;
		GetPubvarByIndex(index, &pubvar);
		m_pNullVec = pubvar->offs;
	} else {
		m_pNullVec = NULL;
	}

	if (FindPubvarByName("NULL_STRING", &index) == SP_ERROR_NONE)
	{
		sp_pubvar_t *pubvar;
		GetPubvarByIndex(index, &pubvar);
		m_pNullString = pubvar->offs;
	} else {
		m_pNullString = NULL;
	}
}

void BaseContext::FlushFunctionCache()
{
	if (m_pub_funcs)
	{
		for (uint32_t i=0; i<ctx->plugin->info.publics_num; i++)
		{
			delete m_pub_funcs[i];
			m_pub_funcs[i] = NULL;
		}
	}

#if 0
	if (m_priv_funcs)
	{
		for (unsigned int i=0; i<m_funcsnum; i++)
		{
			delete m_priv_funcs[i];
			m_priv_funcs[i] = NULL;
		}
	}
#endif
}

void BaseContext::RefreshFunctionCache()
{
	if (m_pub_funcs)
	{
		sp_public_t *pub;
		for (uint32_t i=0; i<ctx->plugin->info.publics_num; i++)
		{
			if (!m_pub_funcs[i])
			{
				continue;
			}
			if (GetPublicByIndex(i, &pub) != SP_ERROR_NONE)
			{
				continue;
			}
			m_pub_funcs[i]->Set(pub->code_offs, this, pub->funcid);
		}
	}

#if 0
	if (m_priv_funcs)
	{
		for (unsigned int i=0; i<m_funcsnum; i++)
		{
			if (!m_priv_funcs[i])
			{
				continue;
			}
			g_pVM->
		}
	}
#endif
}

BaseContext::~BaseContext()
{
	FlushFunctionCache();
	delete [] m_pub_funcs;
	m_pub_funcs = NULL;
#if 0
	delete [] m_priv_funcs;
	m_priv_funcs = NULL;
#endif
}

void BaseContext::SetContext(sp_context_t *_ctx)
{
	if (!_ctx)
	{
		return;
	}
	ctx = _ctx;
	ctx->context = this;
	ctx->dbreak = GlobalDebugBreak;
	RefreshFunctionCache();
}

IVirtualMachine *BaseContext::GetVirtualMachine()
{
	return (IVirtualMachine *)ctx->vmbase;
}

sp_context_t *BaseContext::GetContext()
{
	return ctx;
}

bool BaseContext::IsDebugging()
{
	return (ctx->flags & SPFLAG_PLUGIN_DEBUG);
}

int BaseContext::SetDebugBreak(SPVM_DEBUGBREAK newpfn, SPVM_DEBUGBREAK *oldpfn)
{
	if (!IsDebugging())
	{
		return SP_ERROR_NOTDEBUGGING;
	}

	*oldpfn = ctx->dbreak;
	ctx->dbreak = newpfn;

	return SP_ERROR_NONE;
}

IPluginDebugInfo *BaseContext::GetDebugInfo()
{
	if (!IsDebugging())
	{
		return NULL;
	}
	return this;
}

int BaseContext::Execute(uint32_t code_addr, cell_t *result)
{
	if ((ctx->flags & SPFLAG_PLUGIN_PAUSED) == SPFLAG_PLUGIN_PAUSED)
	{
		return SP_ERROR_NOT_RUNNABLE;
	}

	/* tada, prevent a crash */
	cell_t _ignore_result;
	if (!result)
	{
		result = &_ignore_result;
	}

	IVirtualMachine *vm = (IVirtualMachine *)ctx->vmbase;

	uint32_t pushcount = ctx->pushcount;
	int err;

	if ((err = PushCell(pushcount++)) != SP_ERROR_NONE)
	{
#if defined SOURCEMOD_BUILD
		g_DbgReporter.GenerateCodeError(this, code_addr, err, "Stack error; cannot complete execution!");
#endif
		return SP_ERROR_NOT_RUNNABLE;
	}
	ctx->pushcount = 0;

	cell_t save_sp = ctx->sp + (pushcount * sizeof(cell_t));
	cell_t save_hp = ctx->hp;
	uint32_t n_idx = ctx->n_idx;

	bool wasExec = m_InExec;

	/* Clear the error state, if any */
	ctx->n_err = SP_ERROR_NONE;
	ctx->n_idx = 0;
	m_InExec = true;
	m_MsgCache[0] = '\0';
	m_CustomMsg = false;

	g_SourcePawn.PushTracer(ctx);

	err = vm->ContextExecute(ctx, code_addr, result);

	m_InExec = wasExec;

	/**
	 * :TODO: Calling from a plugin in here will erase the cached message...
	 * Should that be documented?
	 */
	g_SourcePawn.PopTracer(err, m_CustomMsg ? m_MsgCache : NULL);

#if defined SOURCEMOD_BUILD
	if (err == SP_ERROR_NONE)
	{
		if (ctx->sp != save_sp)
		{
			g_DbgReporter.GenerateCodeError(this,
				code_addr,
				SP_ERROR_STACKLEAK,
				"Stack leak detected: sp:%d should be %d!",
				ctx->sp,
				save_sp);
		}
		if (ctx->hp != save_hp)
		{
			g_DbgReporter.GenerateCodeError(this,
				code_addr,
				SP_ERROR_HEAPLEAK,
				"Heap leak detected: sp:%d should be %d!",
				ctx->hp,
				save_hp);
		}
	}
#endif

	if (err != SP_ERROR_NONE)
	{
		ctx->sp = save_sp;
		ctx->hp = save_hp;
	}

	ctx->n_idx = n_idx;
	ctx->n_err = SP_ERROR_NONE;
	m_MsgCache[0] = '\0';
	m_CustomMsg = false;

	return err;
}

void BaseContext::SetErrorMessage(const char *msg, va_list ap)
{
	m_CustomMsg = true;

	vsnprintf(m_MsgCache, sizeof(m_MsgCache), msg, ap);
}

cell_t BaseContext::ThrowNativeErrorEx(int error, const char *msg, ...)
{
	if (!m_InExec)
	{
		return 0;
	}

	ctx->n_err = error;
	
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

	ctx->n_err = SP_ERROR_NATIVE;

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
	if ((cell_t)(ctx->sp - ctx->hp - realmem) < STACKMARGIN)
	{
		return SP_ERROR_HEAPLOW;
	}

	addr = (cell_t *)(ctx->memory + ctx->hp);
	/* store size of allocation in cells */
	*addr = (cell_t)cells;
	addr++;
	ctx->hp += sizeof(cell_t);

	*local_addr = ctx->hp;

	if (phys_addr)
	{
		*phys_addr = addr;
	}

	ctx->hp += realmem;

	return SP_ERROR_NONE;
}

int BaseContext::HeapPop(cell_t local_addr)
{
	cell_t cellcount;
	cell_t *addr;

	/* check the bounds of this address */
	local_addr -= sizeof(cell_t);
	if (local_addr < ctx->heap_base || local_addr >= ctx->sp)
	{
		return SP_ERROR_INVALID_ADDRESS;
	}

	addr = (cell_t *)(ctx->memory + local_addr);
	cellcount = (*addr) * sizeof(cell_t);
	/* check if this memory count looks valid */
	if ((signed)(ctx->hp - cellcount - sizeof(cell_t)) != local_addr)
	{
		return SP_ERROR_INVALID_ADDRESS;
	}

	ctx->hp = local_addr;

	return SP_ERROR_NONE;
}


int BaseContext::HeapRelease(cell_t local_addr)
{
	if (local_addr < ctx->heap_base)
	{
		return SP_ERROR_INVALID_ADDRESS;
	}

	ctx->hp = local_addr - sizeof(cell_t);

	return SP_ERROR_NONE;
}

int BaseContext::FindNativeByName(const char *name, uint32_t *index)
{
	int high;

	high = ctx->plugin->info.natives_num - 1;

	for (uint32_t i=0; i<ctx->plugin->info.natives_num; i++)
	{
		if (strcmp(ctx->natives[i].name, name) == 0)
		{
			if (index)
			{
				*index = i;
			}
			return SP_ERROR_NONE;
		}
	}

	return SP_ERROR_NOT_FOUND;
}

int BaseContext::GetNativeByIndex(uint32_t index, sp_native_t **native)
{
	if (index >= ctx->plugin->info.natives_num)
	{
		return SP_ERROR_INDEX;
	}

	if (native)
	{
		*native = &(ctx->natives[index]);
	}

	return SP_ERROR_NONE;
}


uint32_t BaseContext::GetNativesNum()
{
	return ctx->plugin->info.natives_num;
}

int BaseContext::FindPublicByName(const char *name, uint32_t *index)
{
	int diff, high, low;
	uint32_t mid;

	high = ctx->plugin->info.publics_num - 1;
	low = 0;

	while (low <= high)
	{
		mid = (low + high) / 2;
		diff = strcmp(ctx->publics[mid].name, name);
		if (diff == 0)
		{
			if (index)
			{
				*index = mid;
			}
			return SP_ERROR_NONE;
		} else if (diff < 0) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}

	return SP_ERROR_NOT_FOUND;
}

int BaseContext::GetPublicByIndex(uint32_t index, sp_public_t **pblic)
{
	if (index >= ctx->plugin->info.publics_num)
	{
		return SP_ERROR_INDEX;
	}

	if (pblic)
	{
		*pblic = &(ctx->publics[index]);
	}

	return SP_ERROR_NONE;
}

uint32_t BaseContext::GetPublicsNum()
{
	return ctx->plugin->info.publics_num;
}

int BaseContext::GetPubvarByIndex(uint32_t index, sp_pubvar_t **pubvar)
{
	if (index >= ctx->plugin->info.pubvars_num)
	{
		return SP_ERROR_INDEX;
	}

	if (pubvar)
	{
		*pubvar = &(ctx->pubvars[index]);
	}

	return SP_ERROR_NONE;
}

int BaseContext::FindPubvarByName(const char *name, uint32_t *index)
{
	int diff, high, low;
	uint32_t mid;

	high = ctx->plugin->info.pubvars_num - 1;
	low = 0;

	while (low <= high)
	{
		mid = (low + high) / 2;
		diff = strcmp(ctx->pubvars[mid].name, name);
		if (diff == 0)
		{
			if (index)
			{
				*index = mid;
			}
			return SP_ERROR_NONE;
		} else if (diff < 0) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}

	return SP_ERROR_NOT_FOUND;
}

int BaseContext::GetPubvarAddrs(uint32_t index, cell_t *local_addr, cell_t **phys_addr)
{
	if (index >= ctx->plugin->info.pubvars_num)
	{
		return SP_ERROR_INDEX;
	}

	*local_addr = ctx->plugin->info.pubvars[index].address;
	*phys_addr = ctx->pubvars[index].offs;

	return SP_ERROR_NONE;
}

uint32_t BaseContext::GetPubVarsNum()
{
	return ctx->plugin->info.pubvars_num;
}

int BaseContext::BindNatives(const sp_nativeinfo_t *natives, unsigned int num, int overwrite)
{
	uint32_t i, j, max;

	max = ctx->plugin->info.natives_num;

	for (i=0; i<max; i++)
	{
		if ((ctx->natives[i].status == SP_NATIVE_BOUND) && !overwrite)
		{
			continue;
		}

		for (j=0; (natives[j].name) && (!num || j<num); j++)
		{
			if (!strcmp(ctx->natives[i].name, natives[j].name))
			{
				ctx->natives[i].pfn = natives[j].func;
				ctx->natives[i].status = SP_NATIVE_BOUND;
			}
		}
	}

	return SP_ERROR_NONE;
}

int BaseContext::BindNative(const sp_nativeinfo_t *native)
{
	uint32_t index;
	int err;

	if ((err = FindNativeByName(native->name, &index)) != SP_ERROR_NONE)
	{
		return err;
	}

	ctx->natives[index].pfn = native->func;
	ctx->natives[index].status = SP_NATIVE_BOUND;

	return SP_ERROR_NONE;
}

int BaseContext::BindNativeToIndex(uint32_t index, SPVM_NATIVE_FUNC func)
{
	int err;
	sp_native_t *native;

	if ((err = GetNativeByIndex(index, &native)) != SP_ERROR_NONE)
	{
		return err;
	}

	ctx->natives[index].pfn = func;
	ctx->natives[index].status = SP_NATIVE_BOUND;

	return SP_ERROR_NONE;
}

int BaseContext::BindNativeToAny(SPVM_NATIVE_FUNC native)
{
	uint32_t nativesnum, i;

	nativesnum = ctx->plugin->info.natives_num;

	for (i=0; i<nativesnum; i++)
	{
		if (ctx->natives[i].status == SP_NATIVE_UNBOUND)
		{
			ctx->natives[i].pfn = native;
			ctx->natives[i].status = SP_NATIVE_BOUND;
		}
	}

	return SP_ERROR_NONE;
}

int BaseContext::LocalToPhysAddr(cell_t local_addr, cell_t **phys_addr)
{
	if (((local_addr >= ctx->hp) && (local_addr < ctx->sp)) || (local_addr < 0) || ((ucell_t)local_addr >= ctx->mem_size))
	{
		return SP_ERROR_INVALID_ADDRESS;
	}

	if (phys_addr)
	{
		*phys_addr = (cell_t *)(ctx->memory + local_addr);
	}

	return SP_ERROR_NONE;
}

int BaseContext::PushCell(cell_t value)
{
	if ((ctx->hp + STACKMARGIN) > (cell_t)(ctx->sp - sizeof(cell_t)))
	{
		return SP_ERROR_STACKLOW;
	}

	ctx->sp -= sizeof(cell_t);
	*(cell_t *)(ctx->memory + ctx->sp) = value;
	ctx->pushcount++;

	return SP_ERROR_NONE;
}

int BaseContext::PushCellsFromArray(cell_t array[], unsigned int numcells)
{
	unsigned int i;
	int err;

	for (i=0; i<numcells; i++)
	{
		if ((err = PushCell(array[i])) != SP_ERROR_NONE)
		{
			ctx->sp += (cell_t)(i * sizeof(cell_t));
			ctx->pushcount -= i;
			return err;
		}
	}

	return SP_ERROR_NONE;
}

int BaseContext::PushCellArray(cell_t *local_addr, cell_t **phys_addr, cell_t array[], unsigned int numcells)
{
	cell_t *ph_addr;
	int err;

	if ((err = HeapAlloc(numcells, local_addr, &ph_addr)) != SP_ERROR_NONE)
	{
		return err;
	}

	memcpy(ph_addr, array, numcells * sizeof(cell_t));

	if ((err = PushCell(*local_addr)) != SP_ERROR_NONE)
	{
		HeapRelease(*local_addr);
		return err;
	}

	if (phys_addr)
	{
		*phys_addr = ph_addr;
	}

	return SP_ERROR_NONE;
}

int BaseContext::LocalToString(cell_t local_addr, char **addr)
{
	if (((local_addr >= ctx->hp) && (local_addr < ctx->sp)) || (local_addr < 0) || ((ucell_t)local_addr >= ctx->mem_size))
	{
		return SP_ERROR_INVALID_ADDRESS;
	}
	*addr = (char *)(ctx->memory + local_addr);

	return SP_ERROR_NONE;
}

int BaseContext::PushString(cell_t *local_addr, char **phys_addr, const char *string)
{
	char *ph_addr;
	int err;
	unsigned int len, numcells = ((len=strlen(string)) + sizeof(cell_t)) / sizeof(cell_t);

	if ((err = HeapAlloc(numcells, local_addr, (cell_t **)&ph_addr)) != SP_ERROR_NONE)
	{
		return err;
	}

	memcpy(ph_addr, string, len);
	ph_addr[len] = '\0';

	if ((err = PushCell(*local_addr)) != SP_ERROR_NONE)
	{
		HeapRelease(*local_addr);
		return err;
	}

	if (phys_addr)
	{
		*phys_addr = ph_addr;
	}

	return SP_ERROR_NONE;
}

int BaseContext::StringToLocal(cell_t local_addr, size_t bytes, const char *source)
{
	char *dest;
	size_t len;

	if (((local_addr >= ctx->hp) && (local_addr < ctx->sp)) || (local_addr < 0) || ((ucell_t)local_addr >= ctx->mem_size))
	{
		return SP_ERROR_INVALID_ADDRESS;
	}

	if (bytes == 0)
	{
		return SP_ERROR_NONE;
	}

	len = strlen(source);
	dest = (char *)(ctx->memory + local_addr);

	if (len >= bytes)
	{
		len = bytes - 1;
	}

	memcpy(dest, source, len);
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

	if (((local_addr >= ctx->hp) && (local_addr < ctx->sp)) || (local_addr < 0) || ((ucell_t)local_addr >= ctx->mem_size))
	{
		return SP_ERROR_INVALID_ADDRESS;
	}
	
	if (maxbytes == 0)
	{
		return SP_ERROR_NONE;
	}

	len = strlen(source);
	dest = (char *)(ctx->memory + local_addr);

	if ((size_t)len >= maxbytes)
	{
		len = maxbytes - 1;
		needtocheck = true;
	}

	memcpy(dest, source, len);
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

#define USHR(x) ((unsigned int)(x)>>1)

int BaseContext::LookupFile(ucell_t addr, const char **filename)
{
	int high, low, mid;

	high = ctx->plugin->debug.files_num;
	low = -1;

	while (high - low > 1)
	{
		mid = USHR(low + high);
		if (ctx->files[mid].addr <= addr)
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

	*filename = ctx->files[low].name;

	return SP_ERROR_NONE;
}

int BaseContext::LookupFunction(ucell_t addr, const char **name)
{
	uint32_t iter, max = ctx->plugin->debug.syms_num;

	for (iter=0; iter<max; iter++)
	{
		if ((ctx->symbols[iter].sym->ident == SP_SYM_FUNCTION) 
			&& (ctx->symbols[iter].codestart <= addr) 
			&& (ctx->symbols[iter].codeend > addr))
		{
			break;
		}
	}

	if (iter >= max)
	{
		return SP_ERROR_NOT_FOUND;
	}

	*name = ctx->symbols[iter].name;

	return SP_ERROR_NONE;
}

int BaseContext::LookupLine(ucell_t addr, uint32_t *line)
{
	int high, low, mid;

	high = ctx->plugin->debug.lines_num;
	low = -1;

	while (high - low > 1)
	{
		mid = USHR(low + high);
		if (ctx->lines[mid].addr <= addr)
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
	*line = ctx->lines[low].line + 1;

	return SP_ERROR_NONE;
}

IPluginFunction *BaseContext::GetFunctionById(funcid_t func_id)
{
	CFunction *pFunc = NULL;

	if (func_id & 1)
	{
		func_id >>= 1;
		if (func_id >= ctx->plugin->info.publics_num)
		{
			return NULL;
		}
		pFunc = m_pub_funcs[func_id];
		if (!pFunc)
		{
			m_pub_funcs[func_id] = new CFunction(ctx->publics[func_id].code_offs, this, ctx->publics[func_id].funcid);
			pFunc = m_pub_funcs[func_id];
		} else if (pFunc->IsInvalidated()) {
			pFunc->Set(ctx->publics[func_id].code_offs, this, ctx->publics[func_id].funcid);
		}
	} else {
		/* :TODO: currently not used */
#if 0
		func_id >>= 1;
		unsigned int index;
		if (!g_pVM->FunctionLookup(ctx, func_id, &index))
		{
			return NULL;
		}
		pFunc = m_priv_funcs[func_id];
		if (!pFunc)
		{
			m_priv_funcs[func_id] = new CFunction(save, this);
			pFunc = m_priv_funcs[func_id];
		}
#endif
	}

	return pFunc;
}

IPluginFunction *BaseContext::GetFunctionByName(const char *public_name)
{
	uint32_t index;

	if (FindPublicByName(public_name, &index) != SP_ERROR_NONE)
	{
		return NULL;
	}

	CFunction *pFunc = m_pub_funcs[index];
	if (!pFunc)
	{
		sp_public_t *pub = NULL;
		GetPublicByIndex(index, &pub);
		if (pub)
		{
			m_pub_funcs[index] = new CFunction(pub->code_offs, this, pub->funcid);
		}
		pFunc = m_pub_funcs[index];
	} else if (pFunc->IsInvalidated()) {
		sp_public_t *pub = NULL;
		GetPublicByIndex(index, &pub);
		if (pub)
		{
			pFunc->Set(pub->code_offs, this, pub->funcid);
		} else {
			pFunc = NULL;
		}
	}

	return pFunc;
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

#if defined SOURCEMOD_BUILD
SourceMod::IdentityToken_t *BaseContext::GetIdentity()
{
	return m_pToken;
}

void BaseContext::SetIdentity(SourceMod::IdentityToken_t *token)
{
	m_pToken = token;
}

cell_t *BaseContext::GetNullRef(SP_NULL_TYPE type)
{
	if (type == SP_NULL_VECTOR)
	{
		return m_pNullVec;
	}

	return NULL;
}
#endif

bool BaseContext::IsInExec()
{
	return m_InExec;
}
