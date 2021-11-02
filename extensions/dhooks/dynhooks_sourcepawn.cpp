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

#include "dynhooks_sourcepawn.h"
#include "util.h"
#include <memory>

#ifdef KE_WINDOWS
#include "conventions/x86MsCdecl.h"
#include "conventions/x86MsThiscall.h"
#include "conventions/x86MsStdcall.h"
#include "conventions/x86MsFastcall.h"
typedef x86MsCdecl x86DetourCdecl;
typedef x86MsThiscall x86DetourThisCall;
typedef x86MsStdcall x86DetourStdCall;
typedef x86MsFastcall x86DetourFastCall;
#elif defined KE_LINUX
#include "conventions/x86GccCdecl.h"
#include "conventions/x86GccThiscall.h"
#include "conventions/x86MsStdcall.h"
#include "conventions/x86MsFastcall.h"
typedef x86GccCdecl x86DetourCdecl;
typedef x86GccThiscall x86DetourThisCall;
// Uhm, stdcall on linux?
typedef x86MsStdcall x86DetourStdCall;
// Uhumm, fastcall on linux?
typedef x86MsFastcall x86DetourFastCall;
#else
#error "Unsupported platform."
#endif

// Keep a map of detours and their registered plugin callbacks.
DetourMap g_pPreDetours;
DetourMap g_pPostDetours;

void UnhookFunction(HookType_t hookType, CHook *pDetour)
{
	CHookManager *pDetourManager = GetHookManager();
	pDetour->RemoveCallback(hookType, (HookHandlerFn *)(void *)&HandleDetour);
	// Only disable the detour if there are no more listeners.
	if (!pDetour->AreCallbacksRegistered())
		pDetourManager->UnhookFunction(pDetour->m_pFunc);
}

bool AddDetourPluginHook(HookType_t hookType, CHook *pDetour, HookSetup *setup, IPluginFunction *pCallback)
{
	DetourMap *map;
	if (hookType == HOOKTYPE_PRE)
		map = &g_pPreDetours;
	else
		map = &g_pPostDetours;

	// See if we already have this detour in our list.
	PluginCallbackList *wrappers;
	DetourMap::Insert f = map->findForAdd(pDetour);
	if (f.found())
	{
		wrappers = f->value;
	}
	else
	{
		// Create a vector to store all the plugin callbacks in.
		wrappers = new PluginCallbackList;
		if (!map->add(f, pDetour, wrappers))
		{
			delete wrappers;
			UnhookFunction(hookType, pDetour);
			return false;
		}
	}

	// Add the plugin callback to the detour list.
	CDynamicHooksSourcePawn *pWrapper = new CDynamicHooksSourcePawn(setup, pDetour, pCallback, hookType == HOOKTYPE_POST);
	wrappers->push_back(pWrapper);

	return true;
}

bool RemoveDetourPluginHook(HookType_t hookType, CHook *pDetour, IPluginFunction *pCallback)
{
	DetourMap *map;
	if (hookType == HOOKTYPE_PRE)
		map = &g_pPreDetours;
	else
		map = &g_pPostDetours;

	DetourMap::Result res = map->find(pDetour);
	if (!res.found())
		return false;

	// Remove the plugin's callback
	bool bRemoved = false;
	PluginCallbackList *wrappers = res->value;
	for (int i = wrappers->size()-1; i >= 0 ; i--)
	{
		CDynamicHooksSourcePawn *pWrapper = wrappers->at(i);
		if (pWrapper->plugin_callback == pCallback)
		{
			bRemoved = true;
			delete pWrapper;
			wrappers->erase(wrappers->begin() + i);
		}
	}

	// No more plugin hooks on this callback. Free our structures.
	if (wrappers->empty())
	{
		delete wrappers;
		UnhookFunction(hookType, pDetour);
		map->remove(res);
	}

	return bRemoved;
}

void RemoveAllCallbacksForContext(HookType_t hookType, DetourMap *map, IPluginContext *pContext)
{
	PluginCallbackList *wrappers;
	CDynamicHooksSourcePawn *pWrapper;
	DetourMap::iterator it = map->iter();
	// Run through all active detours we added.
	for (; !it.empty(); it.next())
	{
		wrappers = it->value;
		// See if there are callbacks of this plugin context registered
		// and remove them.
		for (int i = wrappers->size() - 1; i >= 0; i--)
		{
			pWrapper = wrappers->at(i);
			if (pWrapper->plugin_callback->GetParentRuntime()->GetDefaultContext() != pContext)
				continue;

			delete pWrapper;
			wrappers->erase(wrappers->begin() + i);
		}

		// No plugin interested in this hook anymore. unhook.
		if (wrappers->empty())
		{
			delete wrappers;
			UnhookFunction(hookType, it->key);
			it.erase();
		}
	}
}

void RemoveAllCallbacksForContext(IPluginContext *pContext)
{
	RemoveAllCallbacksForContext(HOOKTYPE_PRE, &g_pPreDetours, pContext);
	RemoveAllCallbacksForContext(HOOKTYPE_POST, &g_pPostDetours, pContext);
}

void CleanupDetours(HookType_t hookType, DetourMap *map)
{
	PluginCallbackList *wrappers;
	CDynamicHooksSourcePawn *pWrapper;
	DetourMap::iterator it = map->iter();
	// Run through all active detours we added.
	for (; !it.empty(); it.next())
	{
		wrappers = it->value;
		// Remove all callbacks
		for (int i = wrappers->size() - 1; i >= 0; i--)
		{
			pWrapper = wrappers->at(i);
			delete pWrapper;
		}

		// Unhook the function
		delete wrappers;
		UnhookFunction(hookType, it->key);
	}
	map->clear();
}

void CleanupDetours()
{
	CleanupDetours(HOOKTYPE_PRE, &g_pPreDetours);
	CleanupDetours(HOOKTYPE_POST, &g_pPostDetours);
}

ICallingConvention *ConstructCallingConvention(HookSetup *setup)
{
	// Convert function parameter types into DynamicHooks structures.
	std::vector<DataTypeSized_t> vecArgTypes;
	for (size_t i = 0; i < setup->params.size(); i++)
	{
		ParamInfo &info = setup->params[i];
		DataTypeSized_t type;
		type.type = DynamicHooks_ConvertParamTypeFrom(info.type);
		type.size = info.size;
		type.custom_register = info.custom_register;
		vecArgTypes.push_back(type);
	}

	DataTypeSized_t returnType;
	returnType.type = DynamicHooks_ConvertReturnTypeFrom(setup->returnType);
	returnType.size = 0;
	// TODO: Add support for a custom return register.
	returnType.custom_register = None;

	ICallingConvention *pCallConv = nullptr;
	switch (setup->callConv)
	{
	case CallConv_CDECL:
		pCallConv = new x86DetourCdecl(vecArgTypes, returnType);
		break;
	case CallConv_THISCALL:
		pCallConv = new x86DetourThisCall(vecArgTypes, returnType);
		break;
	case CallConv_STDCALL:
		pCallConv = new x86DetourStdCall(vecArgTypes, returnType);
		break;
	case CallConv_FASTCALL:
		pCallConv = new x86DetourFastCall(vecArgTypes, returnType);
		break;
	default:
		smutils->LogError(myself, "Unknown calling convention %d.", setup->callConv);
		break;
	}

	return pCallConv;
}

// Some arguments might be optimized to be passed in registers instead of the stack.
bool UpdateRegisterArgumentSizes(CHook* pDetour, HookSetup *setup)
{
	// The registers the arguments are passed in might not be the same size as the actual parameter type.
	// Update the type info to the size of the register that's now holding that argument,
	// so we can copy the whole value.
	ICallingConvention* callingConvention = pDetour->m_pCallingConvention;
	std::vector<DataTypeSized_t> &argTypes = callingConvention->m_vecArgTypes;
	int numArgs = argTypes.size();

	for (int i = 0; i < numArgs; i++)
	{
		// Ignore regular arguments on the stack.
		if (argTypes[i].custom_register == None)
			continue;

		CRegister *reg = pDetour->m_pRegisters->GetRegister(argTypes[i].custom_register);
		// That register can't be handled yet.
		if (!reg)
			return false;

		argTypes[i].size = reg->m_iSize;
		setup->params[i].size = reg->m_iSize;
	}

	return true;
}

// Central handler for all detours. Heart of the detour support.
ReturnAction_t HandleDetour(HookType_t hookType, CHook* pDetour)
{
	// Can't call into SourcePawn offthread.
	if (g_MainThreadId != std::this_thread::get_id())
		return ReturnAction_Ignored;

	DetourMap *map;
	if (hookType == HOOKTYPE_PRE)
		map = &g_pPreDetours;
	else
		map = &g_pPostDetours;

	// Find the callback list for this detour.
	DetourMap::Result r = map->find(pDetour);
	if (!r.found())
		return ReturnAction_Ignored;

	// List of all callbacks.
	PluginCallbackList *wrappers = r->value;

	HookReturnStruct *returnStruct = NULL;
	Handle_t rHndl = BAD_HANDLE;

	HookParamsStruct *paramStruct = NULL;
	Handle_t pHndl = BAD_HANDLE;

	int argNum = pDetour->m_pCallingConvention->m_vecArgTypes.size();
	// Keep a copy of the last return value if some plugin wants to override or supercede the function.
	ReturnAction_t finalRet = ReturnAction_Ignored;
	std::unique_ptr<uint8_t[]> finalRetBuf = std::make_unique<uint8_t[]>(pDetour->m_pCallingConvention->m_returnType.size);

	// Call all the plugin functions..
	for (size_t i = 0; i < wrappers->size(); i++)
	{
		CDynamicHooksSourcePawn *pWrapper = wrappers->at(i);
		IPluginFunction *pCallback = pWrapper->plugin_callback;

		// Create a seperate buffer for changed return values for this plugin.
		// We update the finalRet above if the tempRet is higher than the previous ones in the callback list.
		ReturnAction_t tempRet = ReturnAction_Ignored;
		uint8_t *tempRetBuf = nullptr;

		// Find the this pointer for thiscalls.
		// Don't even try to load it if the plugin doesn't care and set it to be ignored.
		if (pWrapper->callConv == CallConv_THISCALL && pWrapper->thisType != ThisPointer_Ignore)
		{
			// The this pointer is implicitly always the first argument.
			void *thisPtr = pDetour->GetArgument<void *>(0);
			cell_t thisAddr = GetThisPtr(thisPtr, pWrapper->thisType);
			pCallback->PushCell(thisAddr);
		}

		// Create the structure for plugins to change/get the return value if the function returns something.
		if (pWrapper->returnType != ReturnType_Void)
		{
			// Create a handle for the return value to pass to the plugin callback.
			returnStruct = pWrapper->GetReturnStruct();
			HandleError err;
			rHndl = handlesys->CreateHandle(g_HookReturnHandle, returnStruct, pCallback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), &err);
			if (!rHndl)
			{
				pCallback->Cancel();
				pCallback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(pCallback, "Error creating ReturnHandle in preparation to call hook callback. (error %d)", err);

				if (returnStruct)
					delete returnStruct;

				// Don't call more callbacks. They will probably fail too.
				break;
			}
			pCallback->PushCell(rHndl);
		}

		// Create the structure for plugins to access the function arguments if it has some.
		if (argNum > 0)
		{
			paramStruct = pWrapper->GetParamStruct();
			HandleError err;
			pHndl = handlesys->CreateHandle(g_HookParamsHandle, paramStruct, pCallback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity(), &err);
			if (!pHndl)
			{
				pCallback->Cancel();
				pCallback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(pCallback, "Error creating ThisHandle in preparation to call hook callback. (error %d)", err);

				// Don't leak our own handles here! Free the return struct if we fail during the argument marshalling.
				if (rHndl)
				{
					HandleSecurity sec(pCallback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity());
					handlesys->FreeHandle(rHndl, &sec);
					rHndl = BAD_HANDLE;
				}

				if (paramStruct)
					delete paramStruct;

				// Don't call more callbacks. They will probably fail too.
				break;
			}
			pCallback->PushCell(pHndl);
		}

		// Run the plugin callback.
		cell_t result = (cell_t)MRES_Ignored;
		pCallback->Execute(&result);

		switch ((MRESReturn)result)
		{
		case MRES_Handled:
			tempRet = ReturnAction_Handled;
			break;
		case MRES_ChangedHandled:
			tempRet = ReturnAction_Handled;
			// Copy the changed parameter values from the plugin's parameter structure back into the actual detour arguments.
			pWrapper->UpdateParamsFromStruct(paramStruct);
			break;
		case MRES_ChangedOverride:
		case MRES_Override:
		case MRES_Supercede:
			// See if this function returns something we should override.
			if (pWrapper->returnType != ReturnType_Void)
			{
				// Make sure the plugin provided a new return value. Could be an oversight if MRES_ChangedOverride 
				// is called without the return value actually being changed.
				if (!returnStruct->isChanged)
				{
					//Throw an error if no override was set
					tempRet = ReturnAction_Ignored;
					pCallback->GetParentRuntime()->GetDefaultContext()->BlamePluginError(pCallback, "Tried to override return value without return value being set");
					break;
				}

				if (pWrapper->returnType == ReturnType_String || pWrapper->returnType == ReturnType_Int || pWrapper->returnType == ReturnType_Bool)
				{
					tempRetBuf = *(uint8_t **)returnStruct->newResult;
				}
				else if (pWrapper->returnType == ReturnType_Float)
				{
					*(float *)&tempRetBuf = *(float *)returnStruct->newResult;
				}
				else
				{
					tempRetBuf = (uint8_t *)returnStruct->newResult;
				}
			}

			// Store if the plugin wants the original function to be called.
			if (result == MRES_Supercede)
				tempRet = ReturnAction_Supercede;
			else
				tempRet = ReturnAction_Override;

			// Copy the changed parameter values from the plugin's parameter structure back into the actual detour arguments.
			if (result == MRES_ChangedOverride)
				pWrapper->UpdateParamsFromStruct(paramStruct);
			break;
		default:
			tempRet = ReturnAction_Ignored;
			break;
		}

		// Prioritize the actions. 
		if (finalRet <= tempRet)
		{
			// Copy the action and return value.
			finalRet = tempRet;
			memcpy(finalRetBuf.get(), &tempRetBuf, pDetour->m_pCallingConvention->m_returnType.size);
		}

		// Free the handles again.
		HandleSecurity sec(pCallback->GetParentRuntime()->GetDefaultContext()->GetIdentity(), myself->GetIdentity());
		if (returnStruct)
		{
			handlesys->FreeHandle(rHndl, &sec);
		}
		if (paramStruct)
		{
			handlesys->FreeHandle(pHndl, &sec);
		}
	}

	// If we want to use our own return value, write it back.
	if (finalRet >= ReturnAction_Override)
	{
		void* pPtr = pDetour->m_pCallingConvention->GetReturnPtr(pDetour->m_pRegisters);
		memcpy(pPtr, finalRetBuf.get(), pDetour->m_pCallingConvention->m_returnType.size);
		pDetour->m_pCallingConvention->ReturnPtrChanged(pDetour->m_pRegisters, pPtr);
	}

	return finalRet;
}

CDynamicHooksSourcePawn::CDynamicHooksSourcePawn(HookSetup *setup, CHook *pDetour, IPluginFunction *pCallback, bool post)
{
	this->params = setup->params;
	this->offset = -1;
	this->returnFlag = setup->returnFlag;
	this->returnType = setup->returnType;
	this->post = post;
	this->plugin_callback = pCallback;
	this->entity = -1;
	this->thisType = setup->thisType;
	this->hookType = setup->hookType;
	this->m_pDetour = pDetour;
	this->callConv = setup->callConv;
}

HookReturnStruct *CDynamicHooksSourcePawn::GetReturnStruct()
{
	// Create buffers to store the return value of the function.
	HookReturnStruct *res = new HookReturnStruct();
	res->isChanged = false;
	res->type = this->returnType;
	res->orgResult = NULL;
	res->newResult = NULL;

	// Copy the actual function's return value too for post hooks.
	if (this->post)
	{
		switch (this->returnType)
		{
		case ReturnType_String:
			res->orgResult = malloc(sizeof(string_t));
			res->newResult = malloc(sizeof(string_t));
			*(string_t *)res->orgResult = m_pDetour->GetReturnValue<string_t>();
			break;
		case ReturnType_Int:
			res->orgResult = malloc(sizeof(int));
			res->newResult = malloc(sizeof(int));
			*(int *)res->orgResult = m_pDetour->GetReturnValue<int>();
			break;
		case ReturnType_Bool:
			res->orgResult = malloc(sizeof(bool));
			res->newResult = malloc(sizeof(bool));
			*(bool *)res->orgResult = m_pDetour->GetReturnValue<bool>();
			break;
		case ReturnType_Float:
			res->orgResult = malloc(sizeof(float));
			res->newResult = malloc(sizeof(float));
			*(float *)res->orgResult = m_pDetour->GetReturnValue<float>();
			break;
		case ReturnType_Vector:
		{
			res->orgResult = malloc(sizeof(SDKVector));
			res->newResult = malloc(sizeof(SDKVector));
			SDKVector vec = m_pDetour->GetReturnValue<SDKVector>();
			*(SDKVector *)res->orgResult = vec;
			break;
		}
		default:
			res->orgResult = m_pDetour->GetReturnValue<void *>();
			break;
		}
	}
	// Pre hooks don't have access to the return value yet - duh.
	// Just create the buffers for overridden values.
	// TODO: Strip orgResult malloc.
	else
	{
		switch (this->returnType)
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

HookParamsStruct *CDynamicHooksSourcePawn::GetParamStruct()
{
	// Save argument values of detoured function.
	HookParamsStruct *params = new HookParamsStruct();
	params->dg = this;
	
	ICallingConvention* callingConvention = m_pDetour->m_pCallingConvention;
	size_t stackSize = callingConvention->GetArgStackSize();
	size_t paramsSize = stackSize + callingConvention->GetArgRegisterSize();
	std::vector<DataTypeSized_t> &argTypes = callingConvention->m_vecArgTypes;
	size_t numArgs = argTypes.size();

	// Create space for original parameters and changes plugins might do.
	params->orgParams = (void **)malloc(paramsSize);
	params->newParams = (void **)malloc(paramsSize);
	params->isChanged = (bool *)malloc(numArgs * sizeof(bool));

	// Save old stack parameters.
	if (stackSize > 0)
	{
		void *pArgPtr = m_pDetour->m_pCallingConvention->GetStackArgumentPtr(m_pDetour->m_pRegisters);
		memcpy(params->orgParams, pArgPtr, stackSize);
	}

	memset(params->newParams, 0, paramsSize);
	memset(params->isChanged, false, numArgs * sizeof(bool));

	size_t firstArg = 0;
	// TODO: Support custom register for this ptr.
	if (callConv == CallConv_THISCALL)
		firstArg = 1;

	// Save the old parameters passed in a register.
	size_t offset = stackSize;
	for (size_t i = 0; i < numArgs; i++)
	{
		// We already saved the stack arguments.
		if (argTypes[i].custom_register == None)
			continue;

		size_t size = argTypes[i].size;
		// Register argument values are saved after all stack arguments in this buffer.
		void *paramAddr = (void *)((intptr_t)params->orgParams + offset);
		void *regAddr = callingConvention->GetArgumentPtr(i + firstArg, m_pDetour->m_pRegisters);
		memcpy(paramAddr, regAddr, size);
		offset += size;
	}

	return params;
}

void CDynamicHooksSourcePawn::UpdateParamsFromStruct(HookParamsStruct *params)
{
	// Function had no params to update now.
	if (!params)
		return;

	ICallingConvention* callingConvention = m_pDetour->m_pCallingConvention;
	size_t stackSize = callingConvention->GetArgStackSize();
	std::vector<DataTypeSized_t> &argTypes = callingConvention->m_vecArgTypes;
	size_t numArgs = argTypes.size();

	size_t firstArg = 0;
	// TODO: Support custom register for this ptr.
	if (callConv == CallConv_THISCALL)
		firstArg = 1;

	size_t stackOffset = 0;
	// Values of arguments stored in registers are saved after the stack arguments.
	size_t registerOffset = stackSize;
	size_t offset;
	for (size_t i = 0; i < numArgs; i++)
	{
		size_t size = argTypes[i].size;
		// Only have to copy something if the plugin changed this parameter.
		if (params->isChanged[i])
		{
			// Get the offset of this argument in the linear buffer. Register argument values are placed after all stack arguments.
			offset = argTypes[i].custom_register == None ? stackOffset : registerOffset;

			void *paramAddr = (void *)((intptr_t)params->newParams + offset);
			void *stackAddr = callingConvention->GetArgumentPtr(i + firstArg, m_pDetour->m_pRegisters);
			memcpy(stackAddr, paramAddr, size);
		}

		// Keep track of the seperate stack and register arguments.
		if (argTypes[i].custom_register == None)
			stackOffset += size;
		else
			registerOffset += size;
	}
}
