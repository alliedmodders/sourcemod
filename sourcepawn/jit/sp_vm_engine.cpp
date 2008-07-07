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

SourcePawnEngine g_engine1;

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

sp_plugin_t *SourcePawnEngine::LoadFromFilePointer(FILE *fp, int *err)
{
	if (err != NULL)
	{
		*err = SP_ERROR_ABORTED;
	}

	return NULL;
}

sp_plugin_t *SourcePawnEngine::LoadFromMemory(void *base, sp_plugin_t *plugin, int *err)
{
	if (err != NULL)
	{
		*err = SP_ERROR_ABORTED;
	}

	return NULL;
}

int SourcePawnEngine::FreeFromMemory(sp_plugin_t *plugin)
{
	return SP_ERROR_ABORTED;
}

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

void SourcePawnEngine::PushTracer(BaseContext *ctx)
{
	TracedCall *pCall = MakeTracedCall(true);

	pCall->cip = INVALID_CIP;
	pCall->ctx = ctx;
	pCall->frm = INVALID_CIP;
}

void SourcePawnEngine::RunTracer(BaseContext *ctx, uint32_t frame, uint32_t codeip)
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
	return m_pStart->ctx->IsDebugging();
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
