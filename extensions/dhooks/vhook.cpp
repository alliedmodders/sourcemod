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

#include "vhook.h"
#include "vfunc_call.h"
#include "util.h"
#include <macro-assembler-x86.h>

SourceHook::IHookManagerAutoGen *g_pHookManager = NULL;

std::vector<DHooksManager *> g_pHooks;

using namespace SourceHook;
using namespace sp;

#ifdef  WIN32
#define OBJECT_OFFSET sizeof(void *)
#else
#define OBJECT_OFFSET (sizeof(void *)*2)
#endif

#ifndef  WIN32
void *GenerateThunk(ReturnType type)
{
	sp::MacroAssembler masm;
	static const size_t kStackNeeded = (2) * 4; // 2 args max
	static const size_t kReserve = ke::Align(kStackNeeded + 8, 16) - 8;

	masm.push(ebp);
	masm.movl(ebp, esp);
	masm.subl(esp, kReserve);
	if (type != ReturnType_String && type != ReturnType_Vector)
	{
		masm.lea(eax, Operand(ebp, 12)); // grab the incoming caller argument vector
		masm.movl(Operand(esp, 1 * 4), eax); // set that as the 2nd argument
		masm.movl(eax, Operand(ebp, 8)); // grab the |this|
		masm.movl(Operand(esp, 0 * 4), eax); // set |this| as the 1st argument*/
	}
	else
	{
		masm.lea(eax, Operand(ebp, 8)); // grab the incoming caller argument vector
		masm.movl(Operand(esp, 1 * 4), eax); // set that as the 2nd argument
		masm.movl(eax, Operand(ebp, 12)); // grab the |this|
		masm.movl(Operand(esp, 0 * 4), eax); // set |this| as the 1st argument*/
	}
	if (type == ReturnType_Float)
	{
		masm.call(ExternalAddress((void *)Callback_float));
	}
	else if (type == ReturnType_Vector)
	{
		masm.call(ExternalAddress((void *)Callback_vector));
	}
	else if (type == ReturnType_String)
	{
		masm.call(ExternalAddress((void *)Callback_stringt));
	}
	else
	{
		masm.call(ExternalAddress((void *)Callback));
	}
	masm.addl(esp, kReserve);
	masm.pop(ebp); // restore ebp
	masm.ret();

	void *base = g_pSM->GetScriptingEngine()->AllocatePageMemory(masm.length());
	masm.emitToExecutableMemory(base);
	return base;
}
#else
// HUGE THANKS TO BAILOPAN (dvander)!
void *GenerateThunk(ReturnType type)
{
	sp::MacroAssembler masm;
	static const size_t kStackNeeded = (3 + 1) * 4; // 3 args max, 1 locals max
	static const size_t kReserve = ke::Align(kStackNeeded + 8, 16) - 8;

	masm.push(ebp);
	masm.movl(ebp, esp);
	masm.subl(esp, kReserve);
	masm.lea(eax, Operand(esp, 3 * 4)); // ptr to 2nd var after argument space
	masm.movl(Operand(esp, 2 * 4), eax); // set the ptr as the third argument
	masm.lea(eax, Operand(ebp, 8)); // grab the incoming caller argument vector
	masm.movl(Operand(esp, 1 * 4), eax); // set that as the 2nd argument
	masm.movl(Operand(esp, 0 * 4), ecx); // set |this| as the 1st argument
	if (type == ReturnType_Float)
	{
		masm.call(ExternalAddress(Callback_float));
	}
	else if (type == ReturnType_Vector)
	{
		masm.call(ExternalAddress(Callback_vector));
	}
	else
	{
		masm.call(ExternalAddress(Callback));
	}
	masm.movl(ecx, Operand(esp, 3 * 4));
	masm.addl(esp, kReserve);
	masm.pop(ebp); // restore ebp
	masm.pop(edx); // grab return address in edx
	masm.addl(esp, ecx); // remove arguments
	masm.jmp(edx); // return to caller

	void *base = g_pSM->GetScriptingEngine()->AllocatePageMemory(masm.length());
	masm.emitToExecutableMemory(base);
	return base;
}
#endif

DHooksManager::DHooksManager(HookSetup *setup, void *iface, IPluginFunction *remove_callback, IPluginFunction *plugincb, bool post)
{
	this->callback = MakeHandler(setup->returnType);
	this->hookid = 0;
	this->remove_callback = remove_callback;
	this->callback->offset = setup->offset;
	this->callback->plugin_callback = plugincb;
	this->callback->returnFlag = setup->returnFlag;
	this->callback->thisType = setup->thisType;
	this->callback->post = post;
	this->callback->hookType = setup->hookType;
	this->callback->params = setup->params;

	this->addr = 0;

	if(this->callback->hookType == HookType_Entity)
	{
		this->callback->entity = gamehelpers->EntityToBCompatRef((CBaseEntity *)iface);
	}
	else
	{
		if(this->callback->hookType == HookType_Raw)
		{
			this->addr = (intptr_t)iface;
		}
		this->callback->entity = -1;
	}

	CProtoInfoBuilder protoInfo(ProtoInfo::CallConv_ThisCall);

	for(int i = this->callback->params.size() -1; i >= 0; i--)
	{
		protoInfo.AddParam(this->callback->params.at(i).size, this->callback->params.at(i).pass_type, PASSFLAG_BYVAL, NULL, NULL, NULL, NULL);//This seems like we need to do something about it at some point...
	}

	if(this->callback->returnType == ReturnType_Void)
	{
		protoInfo.SetReturnType(0, SourceHook::PassInfo::PassType_Unknown, 0, NULL, NULL, NULL, NULL);
	}
	else if(this->callback->returnType == ReturnType_Float)
	{
		protoInfo.SetReturnType(sizeof(float), SourceHook::PassInfo::PassType_Float, setup->returnFlag, NULL, NULL, NULL, NULL);
	}
	else if(this->callback->returnType == ReturnType_String)
	{
		protoInfo.SetReturnType(sizeof(string_t), SourceHook::PassInfo::PassType_Object, setup->returnFlag, NULL, NULL, NULL, NULL);//We have to be 4 really... or else RIP
	}
	else if(this->callback->returnType == ReturnType_Vector)
	{
		protoInfo.SetReturnType(sizeof(SDKVector), SourceHook::PassInfo::PassType_Object, setup->returnFlag, NULL, NULL, NULL, NULL);
	}
	else
	{
		protoInfo.SetReturnType(sizeof(void *), SourceHook::PassInfo::PassType_Basic, setup->returnFlag, NULL, NULL, NULL, NULL);
	}
	this->pManager = g_pHookManager->MakeHookMan(protoInfo, 0, this->callback->offset);

	this->hookid = g_SHPtr->AddHook(g_PLID,ISourceHook::Hook_Normal, iface, 0, this->pManager, this->callback, this->callback->post);
}

void CleanupHooks(IPluginContext *pContext)
{
	for(int i = g_pHooks.size() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.at(i);

		if(pContext == NULL || pContext == manager->callback->plugin_callback->GetParentRuntime()->GetDefaultContext())
		{
			delete manager;
			g_pHooks.erase(g_pHooks.begin() + i);
		}
	}
}

bool SetupHookManager(ISmmAPI *ismm)
{
	g_pHookManager = static_cast<SourceHook::IHookManagerAutoGen *>(ismm->MetaFactory(MMIFACE_SH_HOOKMANAUTOGEN, NULL, NULL));
	
	return g_pHookManager != NULL;
}

SourceHook::PassInfo::PassType GetParamTypePassType(HookParamType type)
{
	switch(type)
	{
		case HookParamType_Float:
			return SourceHook::PassInfo::PassType_Float;
		case HookParamType_Object:
			return SourceHook::PassInfo::PassType_Object;
	}
	return SourceHook::PassInfo::PassType_Basic;
}

size_t GetStackArgsSize(DHooksCallback *dg)
{
	size_t res = GetParamsSize(dg);
	#ifdef  WIN32
	if(dg->returnType == ReturnType_Vector)//Account for result vector ptr.
	#else
	if(dg->returnType == ReturnType_Vector || dg->returnType == ReturnType_String)
	#endif
	{
		res += OBJECT_OFFSET;
	}
	return res;
}

HookReturnStruct::~HookReturnStruct()
{
	if (this->type == ReturnType_String || this->type == ReturnType_Int || this->type == ReturnType_Bool || this->type == ReturnType_Float || this->type == ReturnType_Vector)
	{
		free(this->newResult);
		free(this->orgResult);
	}
}

HookParamsStruct::~HookParamsStruct()
{
	if (this->orgParams != NULL)
	{
		free(this->orgParams);
	}
	if (this->isChanged != NULL)
	{
		free(this->isChanged);
	}
	if (this->newParams != NULL)
	{
		free(this->newParams);
	}
}

HookParamsStruct *GetParamStruct(DHooksCallback *dg, void **argStack, size_t argStackSize)
{
	HookParamsStruct *params = new HookParamsStruct();
	params->dg = dg;
	#ifdef  WIN32
	if(dg->returnType != ReturnType_Vector)
	#else
	if(dg->returnType != ReturnType_Vector && dg->returnType != ReturnType_String)
	#endif
	{
		params->orgParams = (void **)malloc(argStackSize);
		memcpy(params->orgParams, argStack, argStackSize);
	}
	else //Offset result ptr
	{
		params->orgParams = (void **)malloc(argStackSize-OBJECT_OFFSET);
		memcpy(params->orgParams, argStack+OBJECT_OFFSET, argStackSize-OBJECT_OFFSET);
	}
	size_t paramsSize = GetParamsSize(dg);

	params->newParams = (void **)malloc(paramsSize);
	params->isChanged = (bool *)malloc(dg->params.size() * sizeof(bool));

	for (unsigned int i = 0; i < dg->params.size(); i++)
	{
		*(void **)((intptr_t)params->newParams + GetParamOffset(params, i)) = NULL;
		params->isChanged[i] = false;
	}

	return params;
}

HookReturnStruct *GetReturnStruct(DHooksCallback *dg)
{
	HookReturnStruct *res = new HookReturnStruct();
	res->isChanged = false;
	res->type = dg->returnType;
	res->orgResult = NULL;
	res->newResult = NULL;

	if(g_SHPtr->GetOrigRet() && dg->post)
	{
		switch(dg->returnType)
		{
			case ReturnType_String:
				res->orgResult = malloc(sizeof(string_t));
				res->newResult = malloc(sizeof(string_t));
				*(string_t *)res->orgResult = META_RESULT_ORIG_RET(string_t);
				break;
			case ReturnType_Int:
				res->orgResult = malloc(sizeof(int));
				res->newResult = malloc(sizeof(int));
				*(int *)res->orgResult = META_RESULT_ORIG_RET(int);
				break;
			case ReturnType_Bool:
				res->orgResult = malloc(sizeof(bool));
				res->newResult = malloc(sizeof(bool));
				*(bool *)res->orgResult = META_RESULT_ORIG_RET(bool);
				break;
			case ReturnType_Float:
				res->orgResult = malloc(sizeof(float));
				res->newResult = malloc(sizeof(float));
				*(float *)res->orgResult = META_RESULT_ORIG_RET(float);
				break;
			case ReturnType_Vector:
			{
				res->orgResult = malloc(sizeof(SDKVector));
				res->newResult = malloc(sizeof(SDKVector));
				SDKVector vec = META_RESULT_ORIG_RET(SDKVector);
				*(SDKVector *)res->orgResult = vec;
				break;
			}
			default:
				res->orgResult = META_RESULT_ORIG_RET(void *);
				break;
		}
	}
	else
	{
		switch(dg->returnType)
		{
			case ReturnType_String:
				res->orgResult = malloc(sizeof(string_t));
				res->newResult = malloc(sizeof(string_t));
				*(string_t *)res->orgResult = NULL_STRING;
				break;
			case ReturnType_Vector:
				res->orgResult = malloc(sizeof(SDKVector));
				res->newResult = malloc(sizeof(SDKVector));
				*(SDKVector *)res->orgResult = SDKVector();
				break;
			case ReturnType_Int:
				res->orgResult = malloc(sizeof(int));
				res->newResult = malloc(sizeof(int));
				*(int *)res->orgResult = 0;
				break;
			case ReturnType_Bool:
				res->orgResult = malloc(sizeof(bool));
				res->newResult = malloc(sizeof(bool));
				*(bool *)res->orgResult = false;
				break;
			case ReturnType_Float:
				res->orgResult = malloc(sizeof(float));
				res->newResult = malloc(sizeof(float));
				*(float *)res->orgResult = 0.0;
				break;
		}
	}

	return res;
}

cell_t GetThisPtr(void *iface, ThisPointerType type)
{
	if(type == ThisPointer_CBaseEntity)
	{
		if (!iface)
			return -1;
		return gamehelpers->EntityToBCompatRef((CBaseEntity *)iface);
	}

	return (cell_t)iface;
}

#ifdef  WIN32
void *Callback(DHooksCallback *dg, void **argStack, size_t *argsizep)
#else
void *Callback(DHooksCallback *dg, void **argStack)
#endif
{
	HookReturnStruct *returnStruct = NULL;
	HookParamsStruct *paramStruct = NULL;
	Handle_t rHndl;
	Handle_t pHndl;

	#ifdef  WIN32
	*argsizep = GetStackArgsSize(dg);
	#else
	size_t argsize = GetStackArgsSize(dg);
	#endif

	if(dg->thisType == ThisPointer_CBaseEntity || dg->thisType == ThisPointer_Address)
	{
		dg->plugin_callback->PushCell(GetThisPtr(g_SHPtr->GetIfacePtr(), dg->thisType));
	}
	if(dg->returnType != ReturnType_Void)
	{
		returnStruct = GetReturnStruct(dg);
		rHndl = handlesys->CreateHandle(g_HookReturnHandle, returnStruct, dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), NULL);
		if(!rHndl)
		{
			dg->plugin_callback->Cancel();
			if(returnStruct)
			{
				delete returnStruct;
			}
			g_SHPtr->SetRes(MRES_IGNORED);
			return NULL;
		}
		dg->plugin_callback->PushCell(rHndl);
	}

	#ifdef  WIN32
	if(*argsizep > 0)
	{
		paramStruct = GetParamStruct(dg, argStack, *argsizep);
	#else
	if(argsize > 0)
	{
		paramStruct = GetParamStruct(dg, argStack, argsize);
	#endif
		pHndl = handlesys->CreateHandle(g_HookParamsHandle, paramStruct, dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), NULL);
		if(!pHndl)
		{
			dg->plugin_callback->Cancel();
			if(returnStruct)
			{
				HandleSecurity sec(dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity());
				handlesys->FreeHandle(rHndl, &sec);
			}
			if(paramStruct)
			{
				delete paramStruct;
			}
			g_SHPtr->SetRes(MRES_IGNORED);
			return NULL;
		}
		dg->plugin_callback->PushCell(pHndl);
	}
	cell_t result = (cell_t)MRES_Ignored;
	META_RES mres = MRES_IGNORED;

	dg->plugin_callback->Execute(&result);

	void *ret = g_SHPtr->GetOverrideRetPtr();
	switch((MRESReturn)result)
	{
		case MRES_Handled:
		case MRES_ChangedHandled:
			g_SHPtr->DoRecall();
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			mres = MRES_SUPERCEDE;
			ret = CallVFunction<void *>(dg, paramStruct, g_SHPtr->GetIfacePtr());
			break;
		case MRES_ChangedOverride:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					if(dg->returnType == ReturnType_String || dg->returnType == ReturnType_Int || dg->returnType == ReturnType_Bool)
					{
						ret = *(void **)returnStruct->newResult;
					}
					else
					{
						ret = returnStruct->newResult;
					}
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(dg->plugin_callback, "Tried to override return value without return value being set");
					break;
				}
			}
			g_SHPtr->DoRecall();
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			mres = MRES_SUPERCEDE;
			CallVFunction<void *>(dg, paramStruct, g_SHPtr->GetIfacePtr());
			break;
		case MRES_Override:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					g_SHPtr->SetRes(MRES_OVERRIDE);
					mres = MRES_OVERRIDE;
					if(dg->returnType == ReturnType_String || dg->returnType == ReturnType_Int || dg->returnType == ReturnType_Bool)
					{
						ret = *(void **)returnStruct->newResult;
					}
					else
					{
						ret = returnStruct->newResult;
					}
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(dg->plugin_callback, "Tried to override return value without return value being set");
				}
			}
			break;
		case MRES_Supercede:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					g_SHPtr->SetRes(MRES_SUPERCEDE);
					mres = MRES_SUPERCEDE;
					if(dg->returnType == ReturnType_String || dg->returnType == ReturnType_Int || dg->returnType == ReturnType_Bool)
					{
						ret = *(void **)returnStruct->newResult;
					}
					else
					{
						ret = returnStruct->newResult;
					}
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(dg->plugin_callback, "Tried to override return value without return value being set");
				}
			}
			else
			{
				g_SHPtr->SetRes(MRES_SUPERCEDE);
				mres = MRES_SUPERCEDE;
			}
			break;
		default:
			g_SHPtr->SetRes(MRES_IGNORED);
			mres = MRES_IGNORED;
			break;
	}

	HandleSecurity sec(dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity());

	if(returnStruct)
	{
		handlesys->FreeHandle(rHndl, &sec);
	}
	if(paramStruct)
	{
		handlesys->FreeHandle(pHndl, &sec);
	}

	if(dg->returnType == ReturnType_Void || mres <= MRES_HANDLED)
	{
		return NULL;
	}
	return ret;
}
#ifdef  WIN32
float Callback_float(DHooksCallback *dg, void **argStack, size_t *argsizep)
#else
float Callback_float(DHooksCallback *dg, void **argStack)
#endif
{
	HookReturnStruct *returnStruct = NULL;
	HookParamsStruct *paramStruct = NULL;
	Handle_t rHndl;
	Handle_t pHndl;

	#ifdef  WIN32
	*argsizep = GetStackArgsSize(dg);
	#else
	size_t argsize = GetStackArgsSize(dg);
	#endif

	if(dg->thisType == ThisPointer_CBaseEntity || dg->thisType == ThisPointer_Address)
	{
		dg->plugin_callback->PushCell(GetThisPtr(g_SHPtr->GetIfacePtr(), dg->thisType));
	}

	returnStruct = GetReturnStruct(dg);
	rHndl = handlesys->CreateHandle(g_HookReturnHandle, returnStruct, dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), NULL);

	if(!rHndl)
	{
		dg->plugin_callback->Cancel();
		if(returnStruct)
		{
			delete returnStruct;
		}
		g_SHPtr->SetRes(MRES_IGNORED);
		return 0.0;
	}
	dg->plugin_callback->PushCell(rHndl);

	#ifdef  WIN32
	if(*argsizep > 0)
	{
		paramStruct = GetParamStruct(dg, argStack, *argsizep);
	#else
	if(argsize > 0)
	{
		paramStruct = GetParamStruct(dg, argStack, argsize);
	#endif
		pHndl = handlesys->CreateHandle(g_HookParamsHandle, paramStruct, dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), NULL);
		if(!pHndl)
		{
			dg->plugin_callback->Cancel();
			if(returnStruct)
			{
				delete returnStruct;
			}
			if(paramStruct)
			{
				delete paramStruct;
			}
			g_SHPtr->SetRes(MRES_IGNORED);
			return 0.0;
		}
		dg->plugin_callback->PushCell(pHndl);
	}
	cell_t result = (cell_t)MRES_Ignored;
	META_RES mres = MRES_IGNORED;
	dg->plugin_callback->Execute(&result);

	void *ret = g_SHPtr->GetOverrideRetPtr();
	switch((MRESReturn)result)
	{
		case MRES_Handled:
		case MRES_ChangedHandled:
			g_SHPtr->DoRecall();
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			mres = MRES_SUPERCEDE;
			*(float *)ret = CallVFunction<float>(dg, paramStruct, g_SHPtr->GetIfacePtr());
			break;
		case MRES_ChangedOverride:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					*(float *)ret = *(float *)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(dg->plugin_callback, "Tried to override return value without return value being set");
					break;
				}
			}
			g_SHPtr->DoRecall();
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			mres = MRES_SUPERCEDE;
			CallVFunction<float>(dg, paramStruct, g_SHPtr->GetIfacePtr());
			break;
		case MRES_Override:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					g_SHPtr->SetRes(MRES_OVERRIDE);
					mres = MRES_OVERRIDE;
					*(float *)ret = *(float *)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(dg->plugin_callback, "Tried to override return value without return value being set");
				}
			}
			break;
		case MRES_Supercede:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					g_SHPtr->SetRes(MRES_SUPERCEDE);
					mres = MRES_SUPERCEDE;
					*(float *)ret = *(float *)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(dg->plugin_callback, "Tried to override return value without return value being set");
				}
			}
			break;
		default:
			g_SHPtr->SetRes(MRES_IGNORED);
			mres = MRES_IGNORED;
			break;
	}

	HandleSecurity sec(dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity());

	if(returnStruct)
	{
		handlesys->FreeHandle(rHndl, &sec);
	}
	if(paramStruct)
	{
		handlesys->FreeHandle(pHndl, &sec);
	}

	if(dg->returnType == ReturnType_Void || mres <= MRES_HANDLED)
	{
		return 0.0;
	}
	return *(float *)ret;
}
#ifdef  WIN32
SDKVector *Callback_vector(DHooksCallback *dg, void **argStack, size_t *argsizep)
#else
SDKVector *Callback_vector(DHooksCallback *dg, void **argStack)
#endif
{
	SDKVector *vec_result = (SDKVector *)argStack[0]; // Save the result

	HookReturnStruct *returnStruct = NULL;
	HookParamsStruct *paramStruct = NULL;
	Handle_t rHndl;
	Handle_t pHndl;

	#ifdef  WIN32
	*argsizep = GetStackArgsSize(dg);
	#else
	size_t argsize = GetStackArgsSize(dg);
	#endif

	if(dg->thisType == ThisPointer_CBaseEntity || dg->thisType == ThisPointer_Address)
	{
		dg->plugin_callback->PushCell(GetThisPtr(g_SHPtr->GetIfacePtr(), dg->thisType));
	}

	returnStruct = GetReturnStruct(dg);
	rHndl = handlesys->CreateHandle(g_HookReturnHandle, returnStruct, dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), NULL);

	if(!rHndl)
	{
		dg->plugin_callback->Cancel();
		if(returnStruct)
		{
			delete returnStruct;
		}
		g_SHPtr->SetRes(MRES_IGNORED);
		return NULL;
	}
	dg->plugin_callback->PushCell(rHndl);

	#ifdef  WIN32
	if(*argsizep > 0)
	{
		paramStruct = GetParamStruct(dg, argStack, *argsizep);
	#else
	if(argsize > 0)
	{
		paramStruct = GetParamStruct(dg, argStack, argsize);
	#endif
		pHndl = handlesys->CreateHandle(g_HookParamsHandle, paramStruct, dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), NULL);
		if(!pHndl)
		{
			dg->plugin_callback->Cancel();
			if(returnStruct)
			{
				delete returnStruct;
			}
			if(paramStruct)
			{
				delete paramStruct;
			}
			g_SHPtr->SetRes(MRES_IGNORED);
			return NULL;
		}
		dg->plugin_callback->PushCell(pHndl);
	}
	cell_t result = (cell_t)MRES_Ignored;
	META_RES mres = MRES_IGNORED;
	dg->plugin_callback->Execute(&result);

	void *ret = g_SHPtr->GetOverrideRetPtr();
	ret = vec_result;
	switch((MRESReturn)result)
	{
		case MRES_Handled:
		case MRES_ChangedHandled:
			g_SHPtr->DoRecall();
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			mres = MRES_SUPERCEDE;
			*vec_result = CallVFunction<SDKVector>(dg, paramStruct, g_SHPtr->GetIfacePtr());
			break;
		case MRES_ChangedOverride:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					*vec_result = *(SDKVector *)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(dg->plugin_callback, "Tried to override return value without return value being set");
					break;
				}
			}
			g_SHPtr->DoRecall();
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			mres = MRES_SUPERCEDE;
			CallVFunction<SDKVector>(dg, paramStruct, g_SHPtr->GetIfacePtr());
			break;
		case MRES_Override:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					g_SHPtr->SetRes(MRES_OVERRIDE);
					mres = MRES_OVERRIDE;
					*vec_result = *(SDKVector *)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(dg->plugin_callback, "Tried to override return value without return value being set");
				}
			}
			break;
		case MRES_Supercede:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					g_SHPtr->SetRes(MRES_SUPERCEDE);
					mres = MRES_SUPERCEDE;
					*vec_result = *(SDKVector *)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(dg->plugin_callback, "Tried to override return value without return value being set");
				}
			}
			break;
		default:
			g_SHPtr->SetRes(MRES_IGNORED);
			mres = MRES_IGNORED;
			break;
	}

	HandleSecurity sec(dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity());

	if(returnStruct)
	{
		handlesys->FreeHandle(rHndl, &sec);
	}
	if(paramStruct)
	{
		handlesys->FreeHandle(pHndl, &sec);
	}

	if(dg->returnType == ReturnType_Void || mres <= MRES_HANDLED)
	{
		vec_result->x = 0;
		vec_result->y = 0;
		vec_result->z = 0;
		return vec_result;
	}
	return vec_result;
}

#ifndef WIN32
string_t *Callback_stringt(DHooksCallback *dg, void **argStack)
{
	string_t *string_result = (string_t *)argStack[0]; // Save the result

	HookReturnStruct *returnStruct = NULL;
	HookParamsStruct *paramStruct = NULL;
	Handle_t rHndl;
	Handle_t pHndl;

	size_t argsize = GetStackArgsSize(dg);

	if(dg->thisType == ThisPointer_CBaseEntity || dg->thisType == ThisPointer_Address)
	{
		dg->plugin_callback->PushCell(GetThisPtr(g_SHPtr->GetIfacePtr(), dg->thisType));
	}

	returnStruct = GetReturnStruct(dg);
	rHndl = handlesys->CreateHandle(g_HookReturnHandle, returnStruct, dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), NULL);

	if(!rHndl)
	{
		dg->plugin_callback->Cancel();
		if(returnStruct)
		{
			delete returnStruct;
		}
		g_SHPtr->SetRes(MRES_IGNORED);
		return NULL;
	}
	dg->plugin_callback->PushCell(rHndl);

	if(argsize > 0)
	{
		paramStruct = GetParamStruct(dg, argStack, argsize);
		pHndl = handlesys->CreateHandle(g_HookParamsHandle, paramStruct, dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), NULL);
		if(!pHndl)
		{
			dg->plugin_callback->Cancel();
			if(returnStruct)
			{
				delete returnStruct;
			}
			if(paramStruct)
			{
				delete paramStruct;
			}
			g_SHPtr->SetRes(MRES_IGNORED);
			return NULL;
		}
		dg->plugin_callback->PushCell(pHndl);
	}
	cell_t result = (cell_t)MRES_Ignored;
	META_RES mres = MRES_IGNORED;
	dg->plugin_callback->Execute(&result);

	void *ret = g_SHPtr->GetOverrideRetPtr();
	ret = string_result;
	switch((MRESReturn)result)
	{
		case MRES_Handled:
		case MRES_ChangedHandled:
			g_SHPtr->DoRecall();
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			mres = MRES_SUPERCEDE;
			*string_result = CallVFunction<string_t>(dg, paramStruct, g_SHPtr->GetIfacePtr());
			break;
		case MRES_ChangedOverride:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					*string_result = *(string_t *)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(dg->plugin_callback, "Tried to override return value without return value being set");
					break;
				}
			}
			g_SHPtr->DoRecall();
			g_SHPtr->SetRes(MRES_SUPERCEDE);
			mres = MRES_SUPERCEDE;
			CallVFunction<SDKVector>(dg, paramStruct, g_SHPtr->GetIfacePtr());
			break;
		case MRES_Override:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					g_SHPtr->SetRes(MRES_OVERRIDE);
					mres = MRES_OVERRIDE;
					*string_result = *(string_t *)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(dg->plugin_callback, "Tried to override return value without return value being set");
				}
			}
			break;
		case MRES_Supercede:
			if(dg->returnType != ReturnType_Void)
			{
				if(returnStruct->isChanged)
				{
					g_SHPtr->SetRes(MRES_SUPERCEDE);
					mres = MRES_SUPERCEDE;
					*string_result = *(string_t *)returnStruct->newResult;
				}
				else //Throw an error if no override was set
				{
					g_SHPtr->SetRes(MRES_IGNORED);
					mres = MRES_IGNORED;
					dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(dg->plugin_callback, "Tried to override return value without return value being set");
				}
			}
			break;
		default:
			g_SHPtr->SetRes(MRES_IGNORED);
			mres = MRES_IGNORED;
			break;
	}

	HandleSecurity sec(dg->plugin_callback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity());

	if(returnStruct)
	{
		handlesys->FreeHandle(rHndl, &sec);
	}
	if(paramStruct)
	{
		handlesys->FreeHandle(pHndl, &sec);
	}

	if(dg->returnType == ReturnType_Void || mres <= MRES_HANDLED)
	{
		*string_result = NULL_STRING;
		return string_result;
	}
	return string_result;
}
#endif
