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

#ifndef _INCLUDE_SOURCEPAWN_VM_ENGINE_H_
#define _INCLUDE_SOURCEPAWN_VM_ENGINE_H_

#include "sp_vm_api.h"
#include "sp_vm_function.h"

class BaseContext;

class CContextTrace : public IContextTrace
{
public:
	CContextTrace(BaseRuntime *pRuntime, int err, const char *errstr, cell_t start_rp);
public:
	int GetErrorCode();
	const char *GetErrorString();
	bool DebugInfoAvailable();
	const char *GetCustomErrorString();
	bool GetTraceInfo(CallStackInfo *trace);
	void ResetTrace();
	const char *GetLastNative(uint32_t *index);
private:
	BaseRuntime *m_pRuntime;
	sp_context_t *m_ctx;
	int m_Error;
	const char *m_pMsg;
	cell_t m_StartRp;
	cell_t m_Level;
	IPluginDebugInfo *m_pDebug;
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
	void SetReadWriteExecute(void *ptr);
	void FreePageMemory(void *ptr);
	const char *GetErrorString(int err);
	void ReportError(BaseRuntime *runtime, int err, const char *errstr, cell_t rp_start);
public: //Plugin function stuff
	CFunction *GetFunctionFromPool(funcid_t f, IPluginContext  *plugin);
	void ReleaseFunctionToPool(CFunction *func);
	IDebugListener *GetDebugHook();
private:
	IDebugListener *m_pDebugHook;
};

extern SourcePawnEngine g_engine1;

#endif //_INCLUDE_SOURCEPAWN_VM_ENGINE_H_
