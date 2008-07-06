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

#include "sp_vm_types.h"
#include <sh_memory.h>
/* HACK to avoid including sourcehook.h for just the SH_ASSERT definition */
#if !defined  SH_ASSERT
	#define SH_ASSERT(x, info)
	#include <sh_pagealloc.h>
	#undef SH_ASSERT
#else
	#include <sh_pagealloc.h>
#endif

#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "sp_file_headers.h"
#include "sp_vm_engine.h"
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

SourceHook::CPageAlloc g_ExeMemory(16);

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

const char *GetSourcePawnErrorMessage(int error)
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
#if 0
	m_pFreeFuncs = NULL;
#endif
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

#if 0
	CFunction *pNext;
	while (m_pFreeFuncs)
	{
		pNext = m_pFreeFuncs->m_pNext;
		delete m_pFreeFuncs;
		m_pFreeFuncs = pNext;
	}
#endif
}

void *SourcePawnEngine::ExecAlloc(size_t size)
{
#if defined WIN32
	return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#elif defined __GNUC__
	void *base = memalign(sysconf(_SC_PAGESIZE), size);
	if (mprotect(base, size, PROT_READ|PROT_WRITE|PROT_EXEC) != 0)
	{
		free(base);
		return NULL;
	}
	return base;
#endif
}

void *SourcePawnEngine::AllocatePageMemory(size_t size)
{
	return g_ExeMemory.Alloc(size);
}

void SourcePawnEngine::SetReadExecute(void *ptr)
{
	g_ExeMemory.SetRE(ptr);
}

void SourcePawnEngine::SetReadWrite(void *ptr)
{
	g_ExeMemory.SetRW(ptr);
}

void SourcePawnEngine::FreePageMemory(void *ptr)
{
	g_ExeMemory.Free(ptr);
}

void SourcePawnEngine::ExecFree(void *address)
{
#if defined WIN32
	VirtualFree(address, 0, MEM_RELEASE);
#elif defined __GNUC__
	free(address);
#endif
}

void *SourcePawnEngine::BaseAlloc(size_t size)
{
	return malloc(size);
}

void SourcePawnEngine::BaseFree(void *memory)
{
	free(memory);
}

IPluginContext *SourcePawnEngine::CreateBaseContext(sp_context_t *ctx)
{
	return new BaseContext(ctx);
}

void SourcePawnEngine::FreeBaseContext(IPluginContext *ctx)
{
	delete ctx;
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
		else if (!(plugin->data) && !strcmp(nameptr, ".data"))
		{
			sp_file_data_t *dat = (sp_file_data_t *)(base + secptr->dataoffs);
			plugin->data = base + secptr->dataoffs + dat->data;
			plugin->data_size = dat->datasize;
			plugin->memory = dat->memsize;
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

	if (!(plugin->pcode) || !(plugin->data) || !(plugin->info.stringbase))
	{
		goto return_error;
	}

	if ((plugin->flags & SP_FLAG_DEBUG) && (!(plugin->debug.files) || !(plugin->debug.lines) || !(plugin->debug.symbols)))
	{
		goto return_error;
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

sp_plugin_t *SourcePawnEngine::LoadFromFilePointer(FILE *fp, int *err)
{
	sp_file_hdr_t hdr;
	sp_plugin_t *plugin;
	uint8_t *base;
	int z_result;
	int error;

	if (!fp)
	{
		error = SP_ERROR_NOT_FOUND;
		goto return_error;
	}

	/* Rewind for safety */
	rewind(fp);
	fread(&hdr, sizeof(sp_file_hdr_t), 1, fp);

	if (hdr.magic != SPFILE_MAGIC)
	{
		error = SP_ERROR_FILE_FORMAT;
		goto return_error;
	}

	switch (hdr.compression)
	{
	case SPFILE_COMPRESSION_GZ:
		{
			uint32_t uncompsize = hdr.imagesize - hdr.dataoffs;
			uint32_t compsize = hdr.disksize - hdr.dataoffs;
			uint32_t sectsize = hdr.dataoffs - sizeof(sp_file_hdr_t);
			uLongf destlen = uncompsize;

			char *tempbuf = (char *)malloc(compsize);
			void *uncompdata = malloc(uncompsize);
			void *sectheader = malloc(sectsize);

			fread(sectheader, sectsize, 1, fp);
			fread(tempbuf, compsize, 1, fp);

			z_result = uncompress((Bytef *)uncompdata, &destlen, (Bytef *)tempbuf, compsize);
			free(tempbuf);
			if (z_result != Z_OK)
			{
				free(sectheader);
				free(uncompdata);
				error = SP_ERROR_DECOMPRESSOR;
				goto return_error;
			}

			base = (uint8_t *)malloc(hdr.imagesize);
			memcpy(base, &hdr, sizeof(sp_file_hdr_t));
			memcpy(base + sizeof(sp_file_hdr_t), sectheader, sectsize);
			free(sectheader);
			memcpy(base + hdr.dataoffs, uncompdata, uncompsize);
			free(uncompdata);
			break;
		}
	case SPFILE_COMPRESSION_NONE:
		{
			base = (uint8_t *)malloc(hdr.imagesize);
			rewind(fp);
			fread(base, hdr.imagesize, 1, fp);
			break;
		}
	default:
		{
			error = SP_ERROR_DECOMPRESSOR;
			goto return_error;
		}
	}

	plugin = (sp_plugin_t *)malloc(sizeof(sp_plugin_t));
	if (!_ReadPlugin(&hdr, base, plugin, err))
	{
		free(plugin);
		free(base);
		return NULL;
	}

	plugin->allocflags = 0;

	return plugin;

return_error:
	if (err)
	{
		*err = error;
	}
	return NULL;
}

sp_plugin_t *SourcePawnEngine::LoadFromMemory(void *base, sp_plugin_t *plugin, int *err)
{
	sp_file_hdr_t hdr;
	uint8_t noptr = 0;

	memcpy(&hdr, base, sizeof(sp_file_hdr_t));

	if (!plugin)
	{
		plugin = (sp_plugin_t *)malloc(sizeof(sp_plugin_t));
		noptr = 1;
	}

	if (!_ReadPlugin(&hdr, (uint8_t *)base, plugin, err))
	{
		if (noptr)
		{
			free(plugin);
		}
		return NULL;
	}

	if (!noptr)
	{
		plugin->allocflags |= SP_FA_SELF_EXTERNAL;
	}
	plugin->allocflags |= SP_FA_BASE_EXTERNAL;

	return plugin;
}

int SourcePawnEngine::FreeFromMemory(sp_plugin_t *plugin)
{
	if (!(plugin->allocflags & SP_FA_BASE_EXTERNAL))
	{
		free(plugin->base);
		plugin->base = NULL;
	}
	if (!(plugin->allocflags & SP_FA_SELF_EXTERNAL))
	{
		free(plugin);
	}

	return SP_ERROR_NONE;
}

#if 0
void SourcePawnEngine::ReleaseFunctionToPool(CFunction *func)
{
	if (!func)
	{
		return;
	}
	func->Cancel();
	func->m_pNext = m_pFreeFuncs;
	m_pFreeFuncs = func;
}

CFunction *SourcePawnEngine::GetFunctionFromPool(funcid_t f, IPluginContext *plugin)
{
	if (!m_pFreeFuncs)
	{
		return new CFunction(f, plugin);
	} else {
		CFunction *pFunc = m_pFreeFuncs;
		m_pFreeFuncs = m_pFreeFuncs->m_pNext;
		pFunc->Set(f, plugin);
		return pFunc;
	}
}
#endif

IDebugListener *SourcePawnEngine::SetDebugListener(IDebugListener *pListener)
{
	IDebugListener *old = m_pDebugHook;

	m_pDebugHook = pListener;

	return old;
}

unsigned int SourcePawnEngine::GetContextCallCount()
{
	if (!m_CallStack)
	{
		return 0;
	}

	return m_CallStack->chain;
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

void SourcePawnEngine::PushTracer(sp_context_t *ctx)
{
	TracedCall *pCall = MakeTracedCall(true);

	pCall->cip = INVALID_CIP;
	pCall->ctx = ctx;
	pCall->frm = INVALID_CIP;
}

void SourcePawnEngine::RunTracer(sp_context_t *ctx, uint32_t frame, uint32_t codeip)
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

		if (m_CallStack->ctx->n_err)
		{
			native = m_CallStack->ctx->n_idx;
		}

		CContextTrace trace(m_CallStack, error, msg, native);
		m_pDebugHook->OnContextExecuteError(m_CallStack->ctx->context, &trace);
	}

	/* Now pop the error chain  */
	while (m_CallStack && m_CallStack->chain == m_CurChain)
	{
		FreeTracedCall(m_CallStack);
	}

	m_CurChain--;
}

unsigned int SourcePawnEngine::GetEngineAPIVersion()
{
	return SOURCEPAWN_ENGINE_API_VERSION;
}

CContextTrace::CContextTrace(TracedCall *pStart, int error, const char *msg, uint32_t native) : 
 m_Error(error), m_pMsg(msg), m_pStart(pStart), m_pIterator(pStart), m_Native(native)
{
}

bool CContextTrace::DebugInfoAvailable()
{
	return ((m_pStart->ctx->flags & SP_FLAG_DEBUG) == SP_FLAG_DEBUG);
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

	IPluginContext *pContext = m_pIterator->ctx->context;
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
	if (m_pIterator->ctx->context->GetNativeByIndex(m_Native, &native) != SP_ERROR_NONE)
	{
		return NULL;
	}

	if (index)
	{
		*index = m_Native;
	}

	return native->name;
}
