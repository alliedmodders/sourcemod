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

#ifndef _INCLUDE_SOURCEPAWN_VM_ENGINE_H_
#define _INCLUDE_SOURCEPAWN_VM_ENGINE_H_

#include <sh_memory.h>
/* HACK to avoid including sourcehook.h for just the SH_ASSERT definition */
#if !defined  SH_ASSERT
	#define SH_ASSERT(x, info)
	#include <sh_pagealloc.h>
	#undef SH_ASSERT
#else
	#include <sh_pagealloc.h>
#endif
#include "sp_vm_api.h"
#include "sp_vm_function.h"

struct TracedCall
{
	uint32_t cip;
	uint32_t frm;
	sp_context_t *ctx;
	TracedCall *next;
	unsigned int chain;
};

class CContextTrace : public IContextTrace
{
public:
	CContextTrace(TracedCall *pStart, int error, const char *msg, uint32_t native);
public:
	int GetErrorCode();
	const char *GetErrorString();
	bool DebugInfoAvailable();
	const char *GetCustomErrorString();
	bool GetTraceInfo(CallStackInfo *trace);
	void ResetTrace();
	const char *GetLastNative(uint32_t *index);
private:
	int m_Error;
	const char *m_pMsg;
	TracedCall *m_pStart;
	TracedCall *m_pIterator;
	uint32_t m_Native;
};

class SourcePawnEngine : public ISourcePawnEngine
{
public:
	SourcePawnEngine();
	~SourcePawnEngine();
public: //ISourcePawnEngine
	sp_plugin_t *LoadFromFilePointer(FILE *fp, int *err);
	sp_plugin_t *LoadFromMemory(void *base, sp_plugin_t *plugin, int *err);
	int FreeFromMemory(sp_plugin_t *plugin);
	IPluginContext *CreateBaseContext(sp_context_t *ctx);
	void FreeBaseContext(IPluginContext *ctx);
	void *BaseAlloc(size_t size);
	void BaseFree(void *memory);
	void *ExecAlloc(size_t size);
	void ExecFree(void *address);
	IDebugListener *SetDebugListener(IDebugListener *pListener);
	unsigned int GetContextCallCount();
	unsigned int GetEngineAPIVersion();
	void *AllocatePageMemory(size_t size);
	void SetReadWrite(void *ptr);
	void SetReadExecute(void *ptr);
	void FreePageMemory(void *ptr);
public: //Debugger Stuff
	/**
	 * @brief Pushes a context onto the top of the call tracer.
	 * 
	 * @param ctx		Plugin context.
	 */
	void PushTracer(sp_context_t *ctx);

	/**
	 * @brief Pops a plugin off the call tracer.
	 */
	void PopTracer(int error, const char *msg);

	/**
	 * @brief Runs tracer from a debug break.
	 */
	void RunTracer(sp_context_t *ctx, uint32_t frame, uint32_t codeip);
public: //Plugin function stuff
	CFunction *GetFunctionFromPool(funcid_t f, IPluginContext  *plugin);
	void ReleaseFunctionToPool(CFunction *func);
private:
	TracedCall *MakeTracedCall(bool new_chain);
	void FreeTracedCall(TracedCall *pCall);
private:
	IDebugListener *m_pDebugHook;
	TracedCall *m_FreedCalls;
	TracedCall *m_CallStack;
	unsigned int m_CurChain;
	SourceHook::CPageAlloc m_ExeMemory;
	//CFunction *m_pFreeFuncs;
};

#endif //_INCLUDE_SOURCEPAWN_VM_ENGINE_H_
