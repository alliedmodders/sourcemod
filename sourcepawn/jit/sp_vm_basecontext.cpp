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

using namespace SourcePawn;

#define CELLBOUNDMAX	(INT_MAX/sizeof(cell_t))
#define STACKMARGIN		((cell_t)(16*sizeof(cell_t)))

BaseContext::BaseContext(sp_plugin_t *plugin)
{
	m_pPlugin = plugin;

	m_InExec = false;
	m_CustomMsg = false;

	if (plugin->info.publics_num > 0)
	{
		m_PubFuncs = new CFunction *[plugin->info.publics_num];
		memset(m_PubFuncs, 0, sizeof(CFunction *) * plugin->info.publics_num);
	}
	else
	{
		m_PubFuncs = NULL;
	}

	m_ctx.sp = m_pPlugin->mem_size - sizeof(cell_t);
	m_ctx.hp = m_pPlugin->data_size;
}

BaseContext::~BaseContext()
{
	for (uint32_t i = 0; i < m_pPlugin->info.publics_num; i++)
	{
		delete m_PubFuncs[i];
	}
	delete [] m_PubFuncs;

	delete [] m_pPlugin->memory;
	delete [] m_pPlugin->base;
	delete [] m_pPlugin->pubvars;
	delete [] m_pPlugin->natives;
	delete [] m_pPlugin->publics;
	delete m_pPlugin;
}

bool BaseContext::IsDebugging()
{
	/* :XXX: TODO */
	return false;
}

const sp_plugin_t *BaseContext::GetPlugin()
{
	return m_pPlugin;
}

IPluginDebugInfo *BaseContext::GetDebugInfo()
{
	if (!IsDebugging())
	{
		return NULL;
	}

	return this;
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

	//:TODO:
//	ctx->n_err = error;
	
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

	//:TODO:
//	ctx->n_err = SP_ERROR_NATIVE;

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

	assert(cells < CELLBOUNDMAX);

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
	if ((ucell_t)local_addr < m_pPlugin->data_size || local_addr >= m_ctx.sp)
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
	int high;

	high = m_pPlugin->info.natives_num - 1;

	for (uint32_t i=0; i<m_pPlugin->info.natives_num; i++)
	{
		if (strcmp(m_pPlugin->natives[i].name, name) == 0)
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
	if (index >= m_pPlugin->info.natives_num)
	{
		return SP_ERROR_INDEX;
	}

	if (native)
	{
		*native = &(m_pPlugin->natives[index]);
	}

	return SP_ERROR_NONE;
}


uint32_t BaseContext::GetNativesNum()
{
	return m_pPlugin->info.natives_num;
}

int BaseContext::FindPublicByName(const char *name, uint32_t *index)
{
	int diff, high, low;
	uint32_t mid;

	high = m_pPlugin->info.publics_num - 1;
	low = 0;

	while (low <= high)
	{
		mid = (low + high) / 2;
		diff = strcmp(m_pPlugin->publics[mid].name, name);
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
	if (index >= m_pPlugin->info.publics_num)
	{
		return SP_ERROR_INDEX;
	}

	if (pblic)
	{
		*pblic = &(m_pPlugin->publics[index]);
	}

	return SP_ERROR_NONE;
}

uint32_t BaseContext::GetPublicsNum()
{
	return m_pPlugin->info.publics_num;
}

int BaseContext::GetPubvarByIndex(uint32_t index, sp_pubvar_t **pubvar)
{
	if (index >= m_pPlugin->info.pubvars_num)
	{
		return SP_ERROR_INDEX;
	}

	if (pubvar)
	{
		*pubvar = &(m_pPlugin->pubvars[index]);
	}

	return SP_ERROR_NONE;
}

int BaseContext::FindPubvarByName(const char *name, uint32_t *index)
{
	int diff, high, low;
	uint32_t mid;

	high = m_pPlugin->info.pubvars_num - 1;
	low = 0;

	while (low <= high)
	{
		mid = (low + high) / 2;
		diff = strcmp(m_pPlugin->pubvars[mid].name, name);
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
	if (index >= m_pPlugin->info.pubvars_num)
	{
		return SP_ERROR_INDEX;
	}

	*local_addr = m_pPlugin->info.pubvars[index].address;
	*phys_addr = m_pPlugin->pubvars[index].offs;

	return SP_ERROR_NONE;
}

uint32_t BaseContext::GetPubVarsNum()
{
	return m_pPlugin->info.pubvars_num;
}

int BaseContext::BindNative(const sp_nativeinfo_t *native)
{
	int err;
	uint32_t idx;

	if ((err = FindNativeByName(native->name, &idx)) != SP_ERROR_NONE)
	{
		return err;
	}

	return BindNativeToIndex(idx, native->func);
}

int BaseContext::BindNativeToIndex(uint32_t index, SPVM_NATIVE_FUNC func)
{
	int err;
	sp_native_t *native;

	if ((err = GetNativeByIndex(index, &native)) != SP_ERROR_NONE)
	{
		return err;
	}

	m_pPlugin->natives[index].pfn = func;
	m_pPlugin->natives[index].status = SP_NATIVE_BOUND;

	return SP_ERROR_NONE;
}

int BaseContext::BindNativeToAny(SPVM_NATIVE_FUNC native)
{
	uint32_t nativesnum, i;

	nativesnum = m_pPlugin->info.natives_num;

	for (i=0; i<nativesnum; i++)
	{
		if (m_pPlugin->natives[i].status == SP_NATIVE_UNBOUND)
		{
			m_pPlugin->natives[i].pfn = native;
			m_pPlugin->natives[i].status = SP_NATIVE_BOUND;
		}
	}

	return SP_ERROR_NONE;
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
	dest = (char *)(m_pPlugin->data_size + local_addr);

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
		|| (local_addr < 0) || ((ucell_t)local_addr >= m_pPlugin->mem_size))
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

#define USHR(x) ((unsigned int)(x)>>1)

int BaseContext::LookupFile(ucell_t addr, const char **filename)
{
#if 0
	int high, low, mid;

	high = m_pPlugin->debug.files_num;
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
#endif
	return SP_ERROR_NOT_FOUND;
}

int BaseContext::LookupFunction(ucell_t addr, const char **name)
{
#if 0
	uint32_t iter, max = m_pPlugin->debug.syms_num;

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
#endif
	return SP_ERROR_NOT_FOUND;
}

int BaseContext::LookupLine(ucell_t addr, uint32_t *line)
{
#if 0
	int high, low, mid;

	high = m_pPlugin->debug.lines_num;
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
#endif
	return SP_ERROR_NOT_FOUND;
}

IPluginFunction *BaseContext::GetFunctionById(funcid_t func_id)
{
	CFunction *pFunc = NULL;

	if (func_id & 1)
	{
		func_id >>= 1;
		if (func_id >= m_pPlugin->info.publics_num)
		{
			return NULL;
		}
		pFunc = m_PubFuncs[func_id];
		if (!pFunc)
		{
			m_PubFuncs[func_id] = new CFunction(this, func_id, m_pPlugin->info.publics[func_id].address);
			pFunc = m_PubFuncs[func_id];
		}
	}
	else
	{
		/* not supported yet. */
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

	CFunction *pFunc = m_PubFuncs[index];
	if (!pFunc)
	{
		m_PubFuncs[index] = new CFunction(this, index, m_pPlugin->info.publics[index].address);
		pFunc = m_PubFuncs[index];
	}

	return pFunc;
}

bool BaseContext::IsInExec()
{
	return m_InExec;
}

const sp_context_t *BaseContext::GetContext()
{
	return &m_ctx;
}

unsigned int BaseContext::GetProfCallbackSerial()
{
	//:TODO:
	return 0;
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
		*phys_addr = (cell_t *)(m_pPlugin->mem_size + local_addr);
	}

	return SP_ERROR_NONE;
}

int BaseContext::PushCell(cell_t value)
{
	if ((m_ctx.hp + STACKMARGIN) > (cell_t)(m_ctx.sp - sizeof(cell_t)))
	{
		return SP_ERROR_STACKLOW;
	}

	m_ctx.sp -= sizeof(cell_t);
	*(cell_t *)(m_pPlugin->base + m_ctx.sp) = value;

	return SP_ERROR_NONE;
}

void BaseContext::MarkFrame()
{
	m_ctx.frm = m_ctx.sp;
}
