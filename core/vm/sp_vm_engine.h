/**
 * vim: set ts=4 :
 * ================================================================
 * SourcePawn (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ================================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEPAWN_VM_ENGINE_H_
#define _INCLUDE_SOURCEPAWN_VM_ENGINE_H_

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
	//CFunction *m_pFreeFuncs;
};

#endif //_INCLUDE_SOURCEPAWN_VM_ENGINE_H_
