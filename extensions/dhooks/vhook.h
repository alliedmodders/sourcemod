/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Dynamic Hooks Extension
 * Copyright (C) 2012-2021 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_VHOOK_H_
#define _INCLUDE_VHOOK_H_

#include "extension.h"
#include <sourcehook.h>
#include <sh_vector.h>
#include <sourcehook_pibuilder.h>
#include <registers.h>
#include <vector>

enum CallingConvention
{
	CallConv_CDECL,
	CallConv_THISCALL,
	CallConv_STDCALL,
	CallConv_FASTCALL,
};

enum MRESReturn
{
	MRES_ChangedHandled = -2,	// Use changed values and return MRES_Handled
	MRES_ChangedOverride,		// Use changed values and return MRES_Override
	MRES_Ignored,				// plugin didn't take any action
	MRES_Handled,				// plugin did something, but real function should still be called
	MRES_Override,				// call real function, but use my return value
	MRES_Supercede				// skip real function; use my return value
};

enum ObjectValueType
{
	ObjectValueType_Int = 0,
	ObjectValueType_Bool,
	ObjectValueType_Ehandle,
	ObjectValueType_Float,
	ObjectValueType_CBaseEntityPtr,
	ObjectValueType_IntPtr,
	ObjectValueType_BoolPtr,
	ObjectValueType_EhandlePtr,
	ObjectValueType_FloatPtr,
	ObjectValueType_Vector,
	ObjectValueType_VectorPtr,
	ObjectValueType_CharPtr,
	ObjectValueType_String
};

enum HookParamType
{
	HookParamType_Unknown,
	HookParamType_Int,
	HookParamType_Bool,
	HookParamType_Float,
	HookParamType_String,
	HookParamType_StringPtr,
	HookParamType_CharPtr,
	HookParamType_VectorPtr,
	HookParamType_CBaseEntity,
	HookParamType_ObjectPtr,
	HookParamType_Edict,
	HookParamType_Object
};

enum ReturnType
{
	ReturnType_Unknown,
	ReturnType_Void,
	ReturnType_Int,
	ReturnType_Bool,
	ReturnType_Float,
	ReturnType_String,
	ReturnType_StringPtr,
	ReturnType_CharPtr,
	ReturnType_Vector,
	ReturnType_VectorPtr,
	ReturnType_CBaseEntity,
	ReturnType_Edict
};

enum ThisPointerType
{
	ThisPointer_Ignore,
	ThisPointer_CBaseEntity,
	ThisPointer_Address
};

enum HookType
{
	HookType_Entity,
	HookType_GameRules,
	HookType_Raw
};

struct ParamInfo
{
	HookParamType type;
	size_t size;
	unsigned int flags;
	SourceHook::PassInfo::PassType pass_type;
	Register_t custom_register;
};

#ifdef  WIN32
#define OBJECT_OFFSET sizeof(void *)
#else
#define OBJECT_OFFSET (sizeof(void *)*2)
#endif

class HookReturnStruct
{
public:
	~HookReturnStruct();
public:
	ReturnType type;
	bool isChanged;
	void *orgResult;
	void *newResult;
};

class DHooksInfo
{
public:
	SourceHook::CVector<ParamInfo> params;
	int offset;
	unsigned int returnFlag;
	ReturnType returnType;
	bool post;
	IPluginFunction *plugin_callback;
	int entity;
	ThisPointerType thisType;
	HookType hookType;
};

class DHooksCallback : public SourceHook::ISHDelegate, public DHooksInfo
{
public:
    virtual bool IsEqual(ISHDelegate *pOtherDeleg){return false;};
    virtual void DeleteThis()
	{
		*(void ***)this = this->oldvtable;
		g_pSM->GetScriptingEngine()->FreePageMemory(this->newvtable[2]);
		delete this->newvtable;
		delete this;
	};
	virtual void Call() {};
public:
	void **newvtable;
	void **oldvtable;
};

#ifdef  WIN32
void *Callback(DHooksCallback *dg, void **stack, size_t *argsizep);
float Callback_float(DHooksCallback *dg, void **stack, size_t *argsizep);
SDKVector *Callback_vector(DHooksCallback *dg, void **stack, size_t *argsizep);
#else
void *Callback(DHooksCallback *dg, void **stack);
float Callback_float(DHooksCallback *dg, void **stack);
SDKVector *Callback_vector(DHooksCallback *dg, void **stack);
string_t *Callback_stringt(DHooksCallback *dg, void **stack);
#endif

bool SetupHookManager(ISmmAPI *ismm);
void CleanupHooks(IPluginContext *pContext = NULL);
size_t GetParamTypeSize(HookParamType type);
SourceHook::PassInfo::PassType GetParamTypePassType(HookParamType type);
void *GenerateThunk(ReturnType type);

static DHooksCallback *MakeHandler(ReturnType type)
{
	DHooksCallback *dg = new DHooksCallback();
	dg->returnType = type;
	dg->oldvtable = *(void ***)dg;
	dg->newvtable = new void *[3];
	dg->newvtable[0] = dg->oldvtable[0];
	dg->newvtable[1] = dg->oldvtable[1];
	dg->newvtable[2] = GenerateThunk(type);
	*(void ***)dg = dg->newvtable;
	return dg;
}

class HookParamsStruct
{
public:
	HookParamsStruct()
	{
		this->orgParams = NULL;
		this->newParams = NULL;
		this->dg = NULL;
		this->isChanged = NULL;
	}
	~HookParamsStruct();
public:
	void **orgParams;
	void **newParams;
	bool *isChanged;
	DHooksInfo *dg;
};

enum HookMethod {
	Virtual,
	Detour
};

class HookSetup
{
public:
	HookSetup(ReturnType returnType, unsigned int returnFlag, HookType hookType, ThisPointerType thisType, int offset, IPluginFunction *callback)
	{
		this->returnType = returnType;
		this->returnFlag = returnFlag;
		this->hookType = hookType;
		this->callConv = CallConv_THISCALL;
		this->thisType = thisType;
		this->offset = offset;
		this->funcAddr = nullptr;
		this->callback = callback;
		this->hookMethod = Virtual;
	};
	HookSetup(ReturnType returnType, unsigned int returnFlag, CallingConvention callConv, ThisPointerType thisType, void *funcAddr)
	{
		this->returnType = returnType;
		this->returnFlag = returnFlag;
		this->hookType = HookType_Raw;
		this->callConv = callConv;
		this->thisType = thisType;
		this->offset = -1;
		this->funcAddr = funcAddr;
		this->callback = nullptr;
		this->hookMethod = Detour;
	};
	~HookSetup(){};

	bool IsVirtual()
	{
		return this->offset != -1;
	}
public:
	unsigned int returnFlag;
	ReturnType returnType;
	HookType hookType;
	CallingConvention callConv;
	ThisPointerType thisType;
	SourceHook::CVector<ParamInfo> params;
	int offset;
	void *funcAddr;
	IPluginFunction *callback;
	HookMethod hookMethod;
};

class DHooksManager
{
public:
	DHooksManager(HookSetup *setup, void *iface, IPluginFunction *remove_callback, IPluginFunction *plugincb, bool post);
	~DHooksManager()
	{
		if(this->hookid)
		{
			g_SHPtr->RemoveHookByID(this->hookid);
			if(this->remove_callback)
			{
				this->remove_callback->PushCell(this->hookid);
				this->remove_callback->Execute(NULL);
			}
			if(this->pManager)
			{
				g_pHookManager->ReleaseHookMan(this->pManager);
			}
		}
	}
public:
	intptr_t addr;
	int hookid;
	DHooksCallback *callback;
	IPluginFunction *remove_callback;
	SourceHook::HookManagerPubFunc pManager;
};

size_t GetStackArgsSize(DHooksCallback *dg);
cell_t GetThisPtr(void *iface, ThisPointerType type);

extern IBinTools *g_pBinTools;
extern HandleType_t g_HookParamsHandle;
extern HandleType_t g_HookReturnHandle;
#endif
