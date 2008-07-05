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

#include <sp_vm_types.h>
#include <sp_vm_api.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "sp_file_headers.h"
#include "zlib/zlib.h"
#include "sp_vm_basecontext.h"

#if defined WIN32
 #define WIN32_LEAN_AND_MEAN
 #include <windows.h>
#elif defined __GNUC__
 #include <sys/mman.h>
#endif

#define INVALID_CIP			0xFFFFFFFF

using namespace SourcePawn;

#define ERROR_MESSAGE_MAX		25
static const char *g_ErrorMsgTable[] = 
{
	NULL,
	"Unrecognizable file format",
	"Decompressor was not found",
	"Not enough space on the heap",
	"Invalid parameter or parameter type",
	"Invalid plugin address",
	"Object or index not found",
	"Invalid index or index not found",
	"Not enough space on the stack",
	"Debug section not found or debug not enabled",
	"Invalid instruction",
	"Invalid memory access",
	"Stack went below stack boundary",
	"Heap went below heap boundary",
	"Divide by zero",
	"Array index is out of bounds",
	"Instruction contained invalid parameter",
	"Stack memory leaked by native",
	"Heap memory leaked by native",
	"Dynamic array is too big",
	"Tracker stack is out of bounds",
	"Native is not bound",
	"Maximum number of parameters reached",
	"Native detected error",
	"Plugin not runnable",
	"Call was aborted",
};

const char *SourcePawnEngine::GetErrorString(int error)
{
	if (error < 1 || error > ERROR_MESSAGE_MAX)
	{
		return NULL;
	}

	return g_ErrorMsgTable[error];
}

SourcePawnEngine::SourcePawnEngine()
{
	m_pDebugHook = NULL;
	m_CallStack = NULL;
	m_FreedCalls = NULL;
	m_CurChain = 0;
}

SourcePawnEngine::~SourcePawnEngine()
{
	TracedCall *pTemp;
	while (m_FreedCalls)
	{
		pTemp = m_FreedCalls->next;
		delete m_FreedCalls;
		m_FreedCalls = pTemp;
	}
}

sp_plugin_t *_ReadPlugin(sp_file_hdr_t *hdr, uint8_t *base, sp_plugin_t *plugin, int *err)
{
	char *nameptr;
	uint8_t sectnum = 0;
	sp_file_section_t *secptr = (sp_file_section_t *)(base + sizeof(sp_file_hdr_t));

	memset(plugin, 0, sizeof(sp_plugin_t));

	plugin->base = base;

	while (sectnum < hdr->sections)
	{
		nameptr = (char *)(base + hdr->stringtab + secptr->nameoffs);

		if (!(plugin->pcode) && !strcmp(nameptr, ".code"))
		{
			sp_file_code_t *cod = (sp_file_code_t *)(base + secptr->dataoffs);
			plugin->pcode = base + secptr->dataoffs + cod->code;
			plugin->pcode_size = cod->codesize;
			plugin->flags = cod->flags;
		}
		else if (!(plugin->memory) && !strcmp(nameptr, ".data"))
		{
			sp_file_data_t *dat = (sp_file_data_t *)(base + secptr->dataoffs);
			plugin->memory = new uint8_t[dat->memsize];
			plugin->mem_size = dat->memsize;
			plugin->data_size = dat->datasize;
			memcpy(plugin->memory, base + secptr->dataoffs + dat->data, dat->datasize);
		}
		else if (!(plugin->info.publics) && !strcmp(nameptr, ".publics"))
		{
			plugin->info.publics_num = secptr->size / sizeof(sp_file_publics_t);
			plugin->info.publics = (sp_file_publics_t *)(base + secptr->dataoffs);
		}
		else if (!(plugin->info.pubvars) && !strcmp(nameptr, ".pubvars"))
		{
			plugin->info.pubvars_num = secptr->size / sizeof(sp_file_pubvars_t);
			plugin->info.pubvars = (sp_file_pubvars_t *)(base + secptr->dataoffs);
		}
		else if (!(plugin->info.natives) && !strcmp(nameptr, ".natives"))
		{
			plugin->info.natives_num = secptr->size / sizeof(sp_file_natives_t);
			plugin->info.natives = (sp_file_natives_t *)(base + secptr->dataoffs);
		}
		else if (!(plugin->info.stringbase) && !strcmp(nameptr, ".names"))
		{
			plugin->info.stringbase = (const char *)(base + secptr->dataoffs);
		}
		else if (!(plugin->debug.files) && !strcmp(nameptr, ".dbg.files"))
		{
			plugin->debug.files = (sp_fdbg_file_t *)(base + secptr->dataoffs);
		}
		else if (!(plugin->debug.lines) && !strcmp(nameptr, ".dbg.lines"))
		{
			plugin->debug.lines = (sp_fdbg_line_t *)(base + secptr->dataoffs);
		}
		else if (!(plugin->debug.symbols) && !strcmp(nameptr, ".dbg.symbols"))
		{
			plugin->debug.symbols = (sp_fdbg_symbol_t *)(base + secptr->dataoffs);
		}
		else if (!(plugin->debug.lines_num) && !strcmp(nameptr, ".dbg.info"))
		{
			sp_fdbg_info_t *inf = (sp_fdbg_info_t *)(base + secptr->dataoffs);
			plugin->debug.files_num = inf->num_files;
			plugin->debug.lines_num = inf->num_lines;
			plugin->debug.syms_num = inf->num_syms;
		}
		else if (!(plugin->debug.stringbase) && !strcmp(nameptr, ".dbg.strings"))
		{
			plugin->debug.stringbase = (const char *)(base + secptr->dataoffs);
		}

		secptr++;
		sectnum++;
	}

	if (!(plugin->pcode) || !(plugin->memory) || !(plugin->info.stringbase))
	{
		goto return_error;
	}

	if ((plugin->flags & SP_FLAG_DEBUG) && (!(plugin->debug.files) || !(plugin->debug.lines) || !(plugin->debug.symbols)))
	{
		goto return_error;
	}

	if (plugin->info.publics_num > 0)
	{
		plugin->publics = new sp_public_t[plugin->info.publics_num];
		for (uint32_t i = 0; i < plugin->info.publics_num; i++)
		{
			plugin->publics[i].funcid = (i << 1) | (1 << 0);
			plugin->publics[i].name = plugin->info.stringbase + plugin->info.publics[i].name;
		}
	}
	else
	{
		plugin->info.publics = NULL;
	}

	if (plugin->info.natives_num > 0)
	{
		plugin->natives = new sp_native_t[plugin->info.natives_num];
		for (uint32_t i = 0; i < plugin->info.natives_num; i++)
		{
			plugin->natives[i].flags = 0;
			plugin->natives[i].name = plugin->info.stringbase + plugin->info.natives[i].name;
			plugin->natives[i].pfn = NULL;
			plugin->natives[i].status = SP_NATIVE_UNBOUND;
		}
	}
	else
	{
		plugin->natives = NULL;
	}

	if (plugin->info.pubvars_num > 0)
	{
		plugin->pubvars = new sp_pubvar_t[plugin->info.pubvars_num];
		for (uint32_t i = 0; i < plugin->info.pubvars_num; i++)
		{
			plugin->pubvars[i].name = plugin->info.stringbase + plugin->info.pubvars[i].name;
			plugin->pubvars[i].offs = (cell_t *)(plugin->base + plugin->info.pubvars[i].address);
		}
	}
	else
	{
		plugin->pubvars = NULL;
	}

	if (err)
	{
		*err = SP_ERROR_NONE;
	}

	return plugin;

return_error:
	if (err)
	{
		*err = SP_ERROR_FILE_FORMAT;
	}

	return NULL;
}

IPluginContext *SourcePawnEngine::LoadPluginFromMemory(void *data, int *err)
{
	int error = SP_ERROR_NONE;
	sp_file_hdr_t hdr;
	sp_plugin_t *plugin;
	uint8_t *read_base = (uint8_t *)data;

	memcpy(&hdr, read_base, sizeof(sp_file_hdr_t));

	if (hdr.magic != SPFILE_MAGIC)
	{
		error = SP_ERROR_FILE_FORMAT;
		goto return_error;
	}

	switch (hdr.compression)
	{
	case SPFILE_COMPRESSION_GZ:
		{
			int z_result;
			uint32_t uncompsize = hdr.imagesize - hdr.dataoffs;
			uint32_t compsize = hdr.disksize - hdr.dataoffs;
			uint32_t sectsize = hdr.dataoffs - sizeof(sp_file_hdr_t);
			uLongf destlen = uncompsize;

			char *tempbuf = (char *)malloc(compsize);
			void *uncompdata = malloc(uncompsize);
			void *sectheader = malloc(sectsize);

			memcpy(sectheader, (uint8_t *)read_base + sizeof(sp_file_hdr_t), sectsize);
			memcpy(tempbuf, (uint8_t *)read_base + sizeof(sp_file_hdr_t) + sectsize, compsize);

			z_result = uncompress((Bytef *)uncompdata, &destlen, (Bytef *)tempbuf, compsize);
			free(tempbuf);
			if (z_result != Z_OK)
			{
				free(sectheader);
				free(uncompdata);
				error = SP_ERROR_DECOMPRESSOR;
				goto return_error;
			}

			read_base = new uint8_t[hdr.imagesize];
			memcpy(read_base, &hdr, sizeof(sp_file_hdr_t));
			memcpy(read_base + sizeof(sp_file_hdr_t), sectheader, sectsize);
			free(sectheader);
			memcpy(read_base + hdr.dataoffs, uncompdata, uncompsize);
			free(uncompdata);
			break;
		}
	case SPFILE_COMPRESSION_NONE:
		{
			/* :TODO: no support yet */
		}
	default:
		{
			error = SP_ERROR_DECOMPRESSOR;
			goto return_error;
		}
	}

	plugin = new sp_plugin_t;

	if (!_ReadPlugin(&hdr, (uint8_t *)read_base, plugin, err))
	{
		free(read_base);
		return NULL;
	}

	plugin->base = read_base;

	return new BaseContext(plugin);

return_error:
	if (err)
	{
		*err = error;
	}

	return NULL;
}

IDebugListener *SourcePawnEngine::SetDebugListener(IDebugListener *pListener)
{
	IDebugListener *old = m_pDebugHook;

	m_pDebugHook = pListener;

	return old;
}

TracedCall *SourcePawnEngine::MakeTracedCall(bool new_chain)
{
	TracedCall *pCall;

	if (!m_FreedCalls)
	{
		pCall = new TracedCall;
	} else {
		/* Unlink the head node from the free list */
		pCall = m_FreedCalls;
		m_FreedCalls = m_FreedCalls->next;
	}

	/* Link as the head node into the call stack */
	pCall->next = m_CallStack;

	if (new_chain)
	{
		pCall->chain = ++m_CurChain;
	} else {
		pCall->chain = m_CurChain;
	}

	m_CallStack = pCall;

	return pCall;
}

void SourcePawnEngine::FreeTracedCall(TracedCall *pCall)
{
	/* Check if this is the top of the call stack */
	if (pCall == m_CallStack)
	{
		m_CallStack = m_CallStack->next;
	}

	/* Add this to our linked list of freed calls */
	if (!m_FreedCalls)
	{
		m_FreedCalls = pCall;
		m_FreedCalls->next = NULL;
	} else {
		pCall->next = m_FreedCalls;
		m_FreedCalls = pCall;
	}
}

void SourcePawnEngine::PushTracer(IPluginContext *ctx)
{
	TracedCall *pCall = MakeTracedCall(true);

	pCall->cip = INVALID_CIP;
	pCall->ctx = ctx;
	pCall->frm = INVALID_CIP;
}

void SourcePawnEngine::RunTracer(IPluginContext *ctx, uint32_t frame, uint32_t codeip)
{
	assert(m_CallStack != NULL);
	assert(m_CallStack->ctx == ctx);
	assert(m_CallStack->chain == m_CurChain);

	if (m_CallStack->cip == INVALID_CIP)
	{
		/* We aren't logging anything yet, so begin the trace */
		m_CallStack->cip = codeip;
		m_CallStack->frm = frame;
	} else {
		if (m_CallStack->frm > frame)
		{
			/* The last frame has moved down the stack, 
			 * so we have to push a new call onto our list.
			 */
			TracedCall *pCall = MakeTracedCall(false);
			pCall->ctx = ctx;
			pCall->frm = frame;
		} else if (m_CallStack->frm < frame) {
			/* The last frame has moved up the stack,
			 * so we have to pop the call from our list.
			 */
			FreeTracedCall(m_CallStack);
		}
		/* no matter where we are, update the cip */
		m_CallStack->cip = codeip;
	}
}

void SourcePawnEngine::PopTracer(int error, const char *msg)
{
	assert(m_CallStack != NULL);

	if (error != SP_ERROR_NONE && m_pDebugHook)
	{
		uint32_t native = INVALID_CIP;

		if (m_CallStack->ctx->GetContext()->n_err)
		{
			native = m_CallStack->ctx->GetContext()->n_idx;
		}

		CContextTrace trace(m_CallStack, error, msg, native);
		m_pDebugHook->OnContextExecuteError(m_CallStack->ctx, &trace);
	}

	/* Now pop the error chain  */
	while (m_CallStack && m_CallStack->chain == m_CurChain)
	{
		FreeTracedCall(m_CallStack);
	}

	m_CurChain--;
}

CContextTrace::CContextTrace(TracedCall *pStart, int error, const char *msg, uint32_t native) : 
 m_Error(error), m_pMsg(msg), m_pStart(pStart), m_pIterator(pStart), m_Native(native)
{
}

bool CContextTrace::DebugInfoAvailable()
{
	return (m_pStart->ctx->GetDebugInfo() != NULL);
}

const char *CContextTrace::GetCustomErrorString()
{
	return m_pMsg;
}

int CContextTrace::GetErrorCode()
{
	return m_Error;
}

const char *CContextTrace::GetErrorString()
{
	if (m_Error > ERROR_MESSAGE_MAX || 
		m_Error < 1)
	{
		return "Invalid error code";
	} else {
		return g_ErrorMsgTable[m_Error];
	}
}

void CContextTrace::ResetTrace()
{
	m_pIterator = m_pStart;
}

bool CContextTrace::GetTraceInfo(CallStackInfo *trace)
{
	if (!m_pIterator || (m_pIterator->chain != m_pStart->chain))
	{
		return false;
	}

	if (m_pIterator->cip == INVALID_CIP)
	{
		return false;
	}

	IPluginContext *pContext = m_pIterator->ctx;
	IPluginDebugInfo *pInfo = pContext->GetDebugInfo();

	if (!pInfo)
	{
		return false;
	}

	if (!trace)
	{
		m_pIterator = m_pIterator->next;
		return true;
	}

	if (pInfo->LookupFile(m_pIterator->cip, &(trace->filename)) != SP_ERROR_NONE)
	{
		trace->filename = NULL;
	}

	if (pInfo->LookupFunction(m_pIterator->cip, &(trace->function)) != SP_ERROR_NONE)
	{
		trace->function = NULL;
	}

	if (pInfo->LookupLine(m_pIterator->cip, &(trace->line)) != SP_ERROR_NONE)
	{
		trace->line = 0;
	}

	m_pIterator = m_pIterator->next;

	return true;
}

const char *CContextTrace::GetLastNative(uint32_t *index)
{
	if (m_Native == INVALID_CIP)
	{
		return NULL;
	}

	sp_native_t *native;
	if (m_pIterator->ctx->GetNativeByIndex(m_Native, &native) != SP_ERROR_NONE)
	{
		return NULL;
	}

	if (index)
	{
		*index = m_Native;
	}

	return native->name;
}

void SourcePawnEngine::DestroyContext(IPluginContext *pContext)
{
	delete pContext;
}
