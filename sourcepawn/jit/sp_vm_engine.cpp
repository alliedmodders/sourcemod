/**
 * vim: set ts=4 sw=4 tw=99 et:
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sp_vm_types.h"
#include <KeCodeAllocator.h>
#include "sp_file_headers.h"
#include "sp_vm_engine.h"
#include "zlib/zlib.h"
#include "sp_vm_basecontext.h"
#include "jit_x86.h"
#if defined __GNUC__
#include <unistd.h>
#endif

SourcePawnEngine g_engine1;

#if defined WIN32
 #define WIN32_LEAN_AND_MEAN
 #include <windows.h>
#elif defined __GNUC__
 #include <sys/mman.h>
#endif

#if defined __linux__
#include <malloc.h>
#endif

using namespace SourcePawn;

#define ERROR_MESSAGE_MAX		30
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
	"Plugin format is too old",
	"Plugin format is too new",
    "Out of memory",
    "Integer overflow",
    "Script execution timed out"
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
}

SourcePawnEngine::~SourcePawnEngine()
{
}

void *SourcePawnEngine::ExecAlloc(size_t size)
{
#if defined WIN32
	return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#elif defined __GNUC__
# if defined __APPLE__
	void *base = valloc(size);
# else
	void *base = memalign(sysconf(_SC_PAGESIZE), size);
# endif
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
	return g_Jit.AllocCode(size);
}

void SourcePawnEngine::SetReadExecute(void *ptr)
{
	/* already re */
}

void SourcePawnEngine::SetReadWrite(void *ptr)
{
	/* already rw */
}

void SourcePawnEngine::FreePageMemory(void *ptr)
{
	g_Jit.FreeCode(ptr);
}

void SourcePawnEngine::ExecFree(void *address)
{
#if defined WIN32
	VirtualFree(address, 0, MEM_RELEASE);
#elif defined __GNUC__
	free(address);
#endif
}

void SourcePawnEngine::SetReadWriteExecute(void *ptr)
{
//:TODO:	g_ExeMemory.SetRWE(ptr);
	SetReadExecute(ptr);
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

unsigned int SourcePawnEngine::GetEngineAPIVersion()
{
	return SOURCEPAWN_ENGINE_API_VERSION;
}

unsigned int SourcePawnEngine::GetContextCallCount()
{
	return 0;
}

void SourcePawnEngine::ReportError(BaseRuntime *runtime, int err, const char *errstr, cell_t rp_start)
{
	if (m_pDebugHook == NULL)
	{
		return;
	}

	CContextTrace trace(runtime, err, errstr, rp_start);

	m_pDebugHook->OnContextExecuteError(runtime->GetDefaultContext(), &trace);
}

CContextTrace::CContextTrace(BaseRuntime *pRuntime, int err, const char *errstr, cell_t start_rp) 
: m_pRuntime(pRuntime), m_Error(err), m_pMsg(errstr), m_StartRp(start_rp), m_Level(0)
{
	m_ctx = pRuntime->m_pCtx->GetCtx();
	m_pDebug = m_pRuntime->GetDebugInfo();
}

bool CContextTrace::DebugInfoAvailable()
{
	return (m_pDebug != NULL);
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
	if (m_Error > ERROR_MESSAGE_MAX || m_Error < 1)
	{
		return "Invalid error code";
	}
	else
	{
		return g_ErrorMsgTable[m_Error];
	}
}

void CContextTrace::ResetTrace()
{
	m_Level = 0;
}

bool CContextTrace::GetTraceInfo(CallStackInfo *trace)
{
	cell_t cip;

	if (m_Level == 0)
	{
		cip = m_ctx->cip;
	}
	else if (m_ctx->rp > 0)
	{
		/* Entries go from ctx.rp - 1 to m_StartRp */
		cell_t offs, start, end;

		offs = m_Level - 1;
		start = m_ctx->rp - 1;
		end = m_StartRp;

		if (start - offs < end)
		{
			return false;
		}

		cip = m_ctx->rstk_cips[start - offs];
	}
	else
	{
		return false;
	}

	if (trace == NULL)
	{
		m_Level++;
		return true;
	}

	if (m_pDebug->LookupFile(cip, &(trace->filename)) != SP_ERROR_NONE)
	{
		trace->filename = NULL;
	}

	if (m_pDebug->LookupFunction(cip, &(trace->function)) != SP_ERROR_NONE)
	{
		trace->function = NULL;
	}

	if (m_pDebug->LookupLine(cip, &(trace->line)) != SP_ERROR_NONE)
	{
		trace->line = 0;
	}

	m_Level++;

	return true;
}

const char *CContextTrace::GetLastNative(uint32_t *index)
{
	if (m_ctx->n_err == SP_ERROR_NONE)
	{
		return NULL;
	}

	sp_native_t *native;
	if (m_pRuntime->GetNativeByIndex(m_ctx->n_idx, &native) != SP_ERROR_NONE)
	{
		return NULL;
	}

	if (index)
	{
		*index = m_ctx->n_idx;
	}

	return native->name;
}

IDebugListener *SourcePawnEngine::GetDebugHook()
{
	return m_pDebugHook;
}

