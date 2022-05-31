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

#include "natives.h"
#include "util.h"
#include "dynhooks_sourcepawn.h"
#include "signatures.h"

// Must match same enum in sdktools.inc
enum SDKFuncConfSource
{
	SDKConf_Virtual,
	SDKConf_Signature,
	SDKConf_Address
};

bool GetHandleIfValidOrError(HandleType_t type, void **object, IPluginContext *pContext, cell_t param)
{
	if(param == BAD_HANDLE)
	{
		return pContext->ThrowNativeError("Invalid Handle %i", BAD_HANDLE) != 0;
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if((err = handlesys->ReadHandle(param, type, &sec, object)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", param, err) != 0;
	}
	return true;
}

bool GetCallbackArgHandleIfValidOrError(HandleType_t type, HandleType_t otherType, void **object, IPluginContext *pContext, cell_t param)
{
	if (param == BAD_HANDLE)
	{
		return pContext->ThrowNativeError("Invalid Handle %i", BAD_HANDLE) != 0;
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if ((err = handlesys->ReadHandle(param, type, &sec, object)) != HandleError_None)
	{
		// Check if the user mixed up the callback signature and tried to call 
		// e.g. DHookGetParam on a hReturn handle. Print a nicer error message in that case.
		void *dummy;
		if (handlesys->ReadHandle(param, otherType, &sec, &dummy) == HandleError_None)
			return pContext->ThrowNativeError("Invalid Handle %x (error %d). It looks like you've chosen the wrong hook callback signature for your setup and you're trying to access the wrong handle.", param, err) != 0;
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", param, err) != 0;
	}
	return true;
}

IPluginFunction *GetCallback(IPluginContext *pContext, HookSetup * setup, const cell_t *params, cell_t callback_index)
{
	IPluginFunction *ret = NULL;

	if (params[0] >= callback_index)
	{
		ret = pContext->GetFunctionById(params[callback_index]);
	}

	if (!ret && setup->callback)
	{
		ret = setup->callback;
	}

	return ret;
}

//native Handle:DHookCreate(offset, HookType:hooktype, ReturnType:returntype, ThisPointerType:thistype, DHookCallback:callback = INVALID_FUNCTION); // Callback is now optional here.
cell_t Native_CreateHook(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *callback = nullptr;
	// The methodmap constructor doesn't have the callback parameter anymore.
	if (params[0] >= 5)
		callback = pContext->GetFunctionById(params[5]);

	HookSetup *setup = new HookSetup((ReturnType)params[3], PASSFLAG_BYVAL, (HookType)params[2], (ThisPointerType)params[4], params[1], callback);

	Handle_t hndl = handlesys->CreateHandle(g_HookSetupHandle, setup, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if(!hndl)
	{
		delete setup;
		return pContext->ThrowNativeError("Failed to create hook");
	}

	return hndl;
}

//native Handle:DHookCreateDetour(Address:funcaddr, CallingConvention:callConv, ReturnType:returntype, ThisPointerType:thistype);
cell_t Native_CreateDetour(IPluginContext *pContext, const cell_t *params)
{
	HookSetup *setup = new HookSetup((ReturnType)params[3], PASSFLAG_BYVAL, (CallingConvention)params[2], (ThisPointerType)params[4], (void *)params[1]);

	Handle_t hndl = handlesys->CreateHandle(g_HookSetupHandle, setup, pContext->GetIdentity(), myself->GetIdentity(), NULL);

	if (!hndl)
	{
		delete setup;
		return pContext->ThrowNativeError("Failed to create hook");
	}

	return hndl;
}

// native Handle:DHookCreateFromConf(Handle:gameconf, const String:function[]);
cell_t Native_DHookCreateFromConf(IPluginContext *pContext, const cell_t *params)
{
	IGameConfig *conf;
	HandleError err;
	if ((conf = gameconfs->ReadHandle(params[1], pContext->GetIdentity(), &err)) == nullptr)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	char *function;
	pContext->LocalToString(params[2], &function);

	SignatureWrapper *sig = g_pSignatures->GetFunctionSignature(function);
	if (!sig)
	{
		return pContext->ThrowNativeError("Function signature \"%s\" was not found.", function);
	}

	HookSetup *setup = nullptr;
	// This is a virtual hook.
	if (sig->offset.length() > 0)
	{
		int offset;
		if (!conf->GetOffset(sig->offset.c_str(), &offset))
		{
			return BAD_HANDLE;
		}

		setup = new HookSetup(sig->retType, PASSFLAG_BYVAL, sig->hookType, sig->thisType, offset, nullptr);
	}
	// This is a detour.
	else
	{
		void *addr = nullptr;;
		if (sig->signature.length() > 0)
		{
			if (!conf->GetMemSig(sig->signature.c_str(), &addr) || !addr)
			{
				return BAD_HANDLE;
			}
		}
		else
		{
			if (!conf->GetAddress(sig->address.c_str(), &addr) || !addr)
			{
				return BAD_HANDLE;
			}
		}

		setup = new HookSetup(sig->retType, PASSFLAG_BYVAL, sig->callConv, sig->thisType, addr);
	}

	// Push all the arguments.
	for (ArgumentInfo &arg : sig->args)
	{
		ParamInfo info = arg.info;
		setup->params.push_back(info);
	}

	// Create the handle to hold this setup.
	Handle_t hndl = handlesys->CreateHandle(g_HookSetupHandle, setup, pContext->GetIdentity(), myself->GetIdentity(), NULL);
	if (!hndl)
	{
		delete setup;
		return pContext->ThrowNativeError("Failed to create hook");
	}

	return hndl;
}

//native bool:DHookSetFromConf(Handle:setup, Handle:gameconf, SDKFuncConfSource:source, const String:name[]);
cell_t Native_SetFromConf(IPluginContext *pContext, const cell_t *params)
{
	HookSetup *setup;
	if (!GetHandleIfValidOrError(g_HookSetupHandle, (void **)&setup, pContext, params[1]))
	{
		return 0;
	}

	IGameConfig *conf;
	HandleError err;
	if ((conf = gameconfs->ReadHandle(params[2], pContext->GetIdentity(), &err)) == nullptr)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[2], err);
	}

	char *key;
	pContext->LocalToString(params[4], &key);

	int offset = -1;
	void *addr = nullptr;;
	switch (params[3])
	{
	case SDKConf_Virtual:
		if (!conf->GetOffset(key, &offset))
		{
			return 0;
		}
		break;
	case SDKConf_Signature:
		if (!conf->GetMemSig(key, &addr) || !addr)
		{
			return 0;
		}
		break;
	case SDKConf_Address:
		if (!conf->GetAddress(key, &addr) || !addr)
		{
			return 0;
		}
		break;
	default:
		return pContext->ThrowNativeError("Unknown SDKFuncConfSource: %d", params[3]);
	}

	// Save the new info. This always invalidates the other option.
	// Detour or vhook.
	setup->funcAddr = addr;
	setup->offset = offset;

	if (addr == nullptr)
	{
		setup->hookMethod = Virtual;
	}
	else
	{
		setup->hookMethod = Detour;
	}

	return 1;
}

//native bool:DHookAddParam(Handle:setup, HookParamType:type, size=-1, DHookPassFlag:flag=DHookPass_ByVal, DHookRegister custom_register=DHookRegister_Default);
cell_t Native_AddParam(IPluginContext *pContext, const cell_t *params)
{
	HookSetup *setup;

	if(!GetHandleIfValidOrError(g_HookSetupHandle, (void **)&setup, pContext, params[1]))
	{
		return 0;
	}

	ParamInfo info;

	info.type = (HookParamType)params[2];

	if(params[0] >= 4)
	{
		info.flags = params[4];
	}
	else
	{
		info.flags = PASSFLAG_BYVAL;
	}

	// DynamicDetours doesn't expose the passflags concept like SourceHook.
	// See if we're trying to set some invalid flags on detour arguments.
	if(setup->hookMethod == Detour && (info.flags & ~PASSFLAG_BYVAL) > 0)
	{
		return pContext->ThrowNativeError("Pass flags are only supported for virtual hooks.");
	}

	if (params[0] >= 5)
	{
		PluginRegister custom_register = (PluginRegister)params[5];
		info.custom_register = DynamicHooks_ConvertRegisterFrom(custom_register);

		// Stay future proof.
		if (info.custom_register == None && custom_register != DHookRegister_Default)
			return pContext->ThrowNativeError("Unhandled DHookRegister %d", params[5]);

		if (info.custom_register != None && info.type == HookParamType_Object)
			return pContext->ThrowNativeError("Can't pass an object in a register.");
	}
	else
	{
		info.custom_register = None;
	}

	if(params[0] >= 3 && params[3] != -1)
	{
		info.size = params[3];
	}
	else if(info.type == HookParamType_Object)
	{
		return pContext->ThrowNativeError("Object param being set with no size");
	}
	else
	{
		info.size = GetParamTypeSize(info.type);
	}

	info.pass_type = GetParamTypePassType(info.type);
	setup->params.push_back(info);

	return 1;
}


// native bool:DHookEnableDetour(Handle:setup, bool:post, DHookCallback:callback);
cell_t Native_EnableDetour(IPluginContext *pContext, const cell_t *params)
{
	HookSetup *setup;

	if (!GetHandleIfValidOrError(g_HookSetupHandle, (void **)&setup, pContext, params[1]))
	{
		return 0;
	}

	if (setup->funcAddr == nullptr)
	{
		return pContext->ThrowNativeError("Hook not setup for a detour.");
	}

	IPluginFunction *callback = pContext->GetFunctionById(params[3]);
	if (!callback)
	{
		return pContext->ThrowNativeError("Failed to retrieve function by id");
	}

	bool post = params[2] != 0;
	HookType_t hookType = post ? HOOKTYPE_POST : HOOKTYPE_PRE;

	// Check if we already detoured that function.
	CHookManager *pDetourManager = GetHookManager();
	CHook* pDetour = pDetourManager->FindHook(setup->funcAddr);

	// If there is no detour on this function yet, create it.
	if (!pDetour)
	{
		ICallingConvention *pCallConv = ConstructCallingConvention(setup);
		pDetour = pDetourManager->HookFunction(setup->funcAddr, pCallConv);
		if (!UpdateRegisterArgumentSizes(pDetour, setup))
			return pContext->ThrowNativeError("A custom register for a parameter isn't supported.");
	}

	// Register our pre/post handler.
	pDetour->AddCallback(hookType, (HookHandlerFn *)&HandleDetour);

	// Add the plugin callback to the map.
	return AddDetourPluginHook(hookType, pDetour, setup, callback);
}

// native bool:DHookDisableDetour(Handle:setup, bool:post, DHookCallback:callback);
cell_t Native_DisableDetour(IPluginContext *pContext, const cell_t *params)
{
	HookSetup *setup;

	if (!GetHandleIfValidOrError(g_HookSetupHandle, (void **)&setup, pContext, params[1]))
	{
		return 0;
	}

	if (setup->funcAddr == nullptr)
	{
		return pContext->ThrowNativeError("Hook not setup for a detour.");
	}

	IPluginFunction *callback = pContext->GetFunctionById(params[3]);
	if (!callback)
	{
		return pContext->ThrowNativeError("Failed to retrieve function by id");
	}

	bool post = params[2] != 0;
	HookType_t hookType = post ? HOOKTYPE_POST : HOOKTYPE_PRE;

	// Check if we already detoured that function.
	CHookManager *pDetourManager = GetHookManager();
	CHook* pDetour = pDetourManager->FindHook(setup->funcAddr);

	if (!pDetour || !pDetour->IsCallbackRegistered(hookType, (HookHandlerFn *)&HandleDetour))
	{
		return pContext->ThrowNativeError("Function not detoured.");
	}

	// Remove the callback from the hook.
	return RemoveDetourPluginHook(hookType, pDetour, callback);
}

cell_t HookEntityImpl(IPluginContext *pContext, const cell_t *params, uint32_t callbackIndex, uint32_t removalcbIndex)
{
	HookSetup *setup;

	if(!GetHandleIfValidOrError(g_HookSetupHandle, (void **)&setup, pContext, params[1]))
	{
		return 0;
	}

	if (setup->offset == -1)
	{
		return pContext->ThrowNativeError("Hook not setup for a virtual hook.");
	}

	if(setup->hookType != HookType_Entity)
	{
		return pContext->ThrowNativeError("Hook is not an entity hook");
	}

	IPluginFunction *cb = GetCallback(pContext, setup, params, callbackIndex);
	if (!cb)
	{
		return pContext->ThrowNativeError("Failed to hook entity %i, no callback provided", params[3]);
	}

	bool post = params[2] != 0;
	IPluginFunction *removalcb = pContext->GetFunctionById(params[removalcbIndex]);

	for(int i = g_pHooks.size() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.at(i);
		if(manager->callback->hookType == HookType_Entity && manager->callback->entity == gamehelpers->ReferenceToBCompatRef(params[3]) && manager->callback->offset == setup->offset && manager->callback->post == post && manager->remove_callback == removalcb && manager->callback->plugin_callback == cb)
		{
			return manager->hookid;
		}
	}
	CBaseEntity *pEnt = gamehelpers->ReferenceToEntity(params[3]);

	if(!pEnt)
	{
		return pContext->ThrowNativeError("Invalid entity passed %i", params[3]);
	}

	DHooksManager *manager = new DHooksManager(setup, pEnt, removalcb, cb, post);

	if(!manager->hookid)
	{
		delete manager;
		return 0;
	}

	g_pHooks.push_back(manager);

	return manager->hookid;
}

// native DHookEntity(Handle:setup, bool:post, entity, DHookRemovalCB:removalcb, DHookCallback:callback = INVALID_FUNCTION); // Both callbacks are optional
cell_t Native_HookEntity(IPluginContext *pContext, const cell_t *params)
{
	return HookEntityImpl(pContext, params, 5, 4);
}
// public native int DynamicHook.HookEntity(HookMode mode, int entity, DHookCallback callback, DHookRemovalCB removalcb=INVALID_FUNCTION);
cell_t Native_HookEntity_Methodmap(IPluginContext *pContext, const cell_t *params)
{
	return HookEntityImpl(pContext, params, 4, 5);
}

cell_t HookGamerulesImpl(IPluginContext *pContext, const cell_t *params, uint32_t callbackIndex, uint32_t removalcbIndex)
{
	HookSetup *setup;

	if(!GetHandleIfValidOrError(g_HookSetupHandle, (void **)&setup, pContext, params[1]))
	{
		return 0;
	}

	if (setup->offset == -1)
	{
		return pContext->ThrowNativeError("Hook not setup for a virtual hook.");
	}

	if(setup->hookType != HookType_GameRules)
	{
		return pContext->ThrowNativeError("Hook is not a gamerules hook");
	}

	IPluginFunction *cb = GetCallback(pContext, setup, params, callbackIndex);
	if (!cb)
	{
		return pContext->ThrowNativeError("Failed to hook gamerules, no callback provided");
	}

	bool post = params[2] != 0;
	IPluginFunction *removalcb = pContext->GetFunctionById(params[removalcbIndex]);

	for(int i = g_pHooks.size() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.at(i);
		if(manager->callback->hookType == HookType_GameRules && manager->callback->offset == setup->offset && manager->callback->post == post && manager->remove_callback == removalcb && manager->callback->plugin_callback == cb)
		{
			return manager->hookid;
		}
	}

	void *rules = g_pSDKTools->GetGameRules();
	if(!rules)
	{
		return pContext->ThrowNativeError("Could not get gamerules pointer");
	}

	DHooksManager *manager = new DHooksManager(setup, rules, removalcb, cb, post);

	if(!manager->hookid)
	{
		delete manager;
		return 0;
	}

	g_pHooks.push_back(manager);

	return manager->hookid;
}

// native DHookGamerules(Handle:setup, bool:post, DHookRemovalCB:removalcb, DHookCallback:callback = INVALID_FUNCTION); // Both callbacks are optional
cell_t Native_HookGamerules(IPluginContext *pContext, const cell_t *params)
{
	return HookGamerulesImpl(pContext, params, 4, 3);
}

// public native int DynamicHook.HookGamerules(HookMode mode, DHookCallback callback, DHookRemovalCB removalcb=INVALID_FUNCTION);
cell_t Native_HookGamerules_Methodmap(IPluginContext *pContext, const cell_t *params)
{
	return HookGamerulesImpl(pContext, params, 3, 4);
}

cell_t HookRawImpl(IPluginContext *pContext, const cell_t *params, int callbackIndex, int removalcbIndex)
{
	HookSetup *setup;

	if(!GetHandleIfValidOrError(g_HookSetupHandle, (void **)&setup, pContext, params[1]))
	{
		return 0;
	}

	if (setup->offset == -1)
	{
		return pContext->ThrowNativeError("Hook not setup for a virtual hook.");
	}

	if(setup->hookType != HookType_Raw)
	{
		return pContext->ThrowNativeError("Hook is not a raw hook");
	}

	IPluginFunction *cb = GetCallback(pContext, setup, params, callbackIndex);
	if (!cb)
	{
		return pContext->ThrowNativeError("Failed to hook address, no callback provided");
	}

	bool post = params[2] != 0;
	IPluginFunction *removalcb = nullptr;
	if (removalcbIndex > 0)
		removalcb = pContext->GetFunctionById(params[removalcbIndex]);

	void *iface = (void *)(params[3]);

	for(int i = g_pHooks.size() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.at(i);
		if(manager->callback->hookType == HookType_Raw && manager->addr == (intptr_t)iface && manager->callback->offset == setup->offset && manager->callback->post == post && manager->remove_callback == removalcb && manager->callback->plugin_callback == cb)
		{
			return manager->hookid;
		}
	}

	if(!iface)
	{
		return pContext->ThrowNativeError("Invalid address passed");
	}

	DHooksManager *manager = new DHooksManager(setup, iface, removalcb, cb, post);

	if(!manager->hookid)
	{
		delete manager;
		return 0;
	}

	g_pHooks.push_back(manager);

	return manager->hookid;
}

// DHookRaw(Handle:setup, bool:post, Address:addr, DHookRemovalCB:removalcb, DHookCallback:callback = INVALID_FUNCTION); // Both callbacks are optional
cell_t Native_HookRaw(IPluginContext *pContext, const cell_t *params)
{
	return HookRawImpl(pContext, params, 5, 4);
}

// public native int DynamicHook.HookRaw(HookMode mode, Address addr, DHookCallback callback);
cell_t Native_HookRaw_Methodmap(IPluginContext *pContext, const cell_t *params)
{
	return HookRawImpl(pContext, params, 4, 0);
}

// native bool:DHookRemoveHookID(hookid);
cell_t Native_RemoveHookID(IPluginContext *pContext, const cell_t *params)
{
	for(int i = g_pHooks.size() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.at(i);
		if(manager->hookid == params[1] && manager->callback->plugin_callback->GetParentRuntime()->GetDefaultContext() == pContext)
		{
			delete manager;
			g_pHooks.erase(g_pHooks.begin() + i);
			return 1;
		}
	}
	return 0;
}
// native any:DHookGetParam(Handle:hParams, num);
cell_t Native_GetParam(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookParamsHandle, g_HookReturnHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] < 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}
	if(params[2] == 0)
	{
		return paramStruct->dg->params.size();
	}

	int index = params[2] - 1;

	size_t offset = GetParamOffset(paramStruct, index);

	void *addr = (void **)((intptr_t)paramStruct->orgParams + offset);

	if(*(void **)addr == NULL && (paramStruct->dg->params.at(index).type == HookParamType_CBaseEntity || paramStruct->dg->params.at(index).type == HookParamType_Edict))
	{
		return pContext->ThrowNativeError("Trying to get value for null pointer.");
	}

	switch(paramStruct->dg->params.at(index).type)
	{
		case HookParamType_Int:
			return *(int *)addr;
		case HookParamType_Bool:
			return *(cell_t *)addr != 0;
		case HookParamType_CBaseEntity:
			return gamehelpers->EntityToBCompatRef(*(CBaseEntity **)addr);
		case HookParamType_Edict:
			return gamehelpers->IndexOfEdict(*(edict_t **)addr);
		case HookParamType_Float:
			return sp_ftoc(*(float *)addr);
		default:
			return pContext->ThrowNativeError("Invalid param type (%i) to get", paramStruct->dg->params.at(index).type);
	}

	return 1;
}

// native DHookSetParam(Handle:hParams, param, any:value)
cell_t Native_SetParam(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookParamsHandle, g_HookReturnHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	size_t offset = GetParamOffset(paramStruct, index);
	void *addr = (void **)((intptr_t)paramStruct->newParams + offset);

	switch(paramStruct->dg->params.at(index).type)
	{
		case HookParamType_Int:
			*(int *)addr = params[3];
			break;
		case HookParamType_Bool:
			*(bool *)addr = (params[3] ? true : false);
			break;
		case HookParamType_CBaseEntity:
		{
			if(params[2] == -1)
			{
				*(CBaseEntity **)addr = nullptr;
			}
			else
			{
				CBaseEntity *pEnt = gamehelpers->ReferenceToEntity(params[2]);

				if(!pEnt)
				{
					return pContext->ThrowNativeError("Invalid entity index passed for param value");
				}

				*(CBaseEntity **)addr = pEnt;
			}
			break;
		}
		case HookParamType_Edict:
		{
			edict_t *pEdict = gamehelpers->EdictOfIndex(params[2]);

			if(!pEdict || pEdict->IsFree())
			{
				pContext->ThrowNativeError("Invalid entity index passed for param value");
			}

			*(edict_t **)addr = pEdict;
			break;
		}
		case HookParamType_Float:
			*(float *)addr = sp_ctof(params[3]);
			break;
		default:
			return pContext->ThrowNativeError("Invalid param type (%i) to set", paramStruct->dg->params.at(index).type);
	}

	paramStruct->isChanged[index] = true;
	return 1;
}

// native any:DHookGetReturn(Handle:hReturn);
cell_t Native_GetReturn(IPluginContext *pContext, const cell_t *params)
{
	HookReturnStruct *returnStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookReturnHandle, g_HookParamsHandle, (void **)&returnStruct, pContext, params[1]))
	{
		return 0;
	}

	switch(returnStruct->type)
	{
		case ReturnType_Int:
			return *(int *)returnStruct->orgResult;
		case ReturnType_Bool:
			return *(bool *)returnStruct->orgResult? 1 : 0;
		case ReturnType_CBaseEntity:
			return gamehelpers->EntityToBCompatRef((CBaseEntity *)returnStruct->orgResult);
		case ReturnType_Edict:
			return gamehelpers->IndexOfEdict((edict_t *)returnStruct->orgResult);
		case ReturnType_Float:
			return sp_ftoc(*(float *)returnStruct->orgResult);
		default:
			return pContext->ThrowNativeError("Invalid param type (%i) to get", returnStruct->type);
	}
	return 1;
}
// native DHookSetReturn(Handle:hReturn, any:value)
cell_t Native_SetReturn(IPluginContext *pContext, const cell_t *params)
{
	HookReturnStruct *returnStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookReturnHandle, g_HookParamsHandle, (void **)&returnStruct, pContext, params[1]))
	{
		return 0;
	}

	switch(returnStruct->type)
	{
		case ReturnType_Int:
			*(int *)returnStruct->newResult = params[2];
			break;
		case ReturnType_Bool:
			*(bool *)returnStruct->newResult = params[2] != 0;
			break;
		case ReturnType_CBaseEntity:
		{
			if(params[2] == -1) {
				returnStruct->newResult = nullptr;
			} else {
				CBaseEntity *pEnt = gamehelpers->ReferenceToEntity(params[2]);
				if(!pEnt)
				{
					return pContext->ThrowNativeError("Invalid entity index passed for return value");
				}
				returnStruct->newResult = pEnt;
			}
			break;
		}
		case ReturnType_Edict:
		{
			edict_t *pEdict = gamehelpers->EdictOfIndex(params[2]);
			if(!pEdict || pEdict->IsFree())
			{
				pContext->ThrowNativeError("Invalid entity index passed for return value");
			}
			returnStruct->newResult = pEdict;
			break;
		}
		case ReturnType_Float:
			*(float *)returnStruct->newResult = sp_ctof(params[2]);
			break;
		default:
			return pContext->ThrowNativeError("Invalid param type (%i) to get",returnStruct->type);
	}
	returnStruct->isChanged = true;
	return 1;
}
// native DHookGetParamVector(Handle:hParams, num, Float:vec[3])
cell_t Native_GetParamVector(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookParamsHandle, g_HookReturnHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	size_t offset = GetParamOffset(paramStruct, index);
	void *addr = (void **)((intptr_t)paramStruct->orgParams + offset);

	if (*(void **)addr == NULL)
	{
		return pContext->ThrowNativeError("Trying to get value for null pointer.");
	}

	switch(paramStruct->dg->params.at(index).type)
	{
		case HookParamType_VectorPtr:
		{
			cell_t *buffer;
			pContext->LocalToPhysAddr(params[3], &buffer);
			SDKVector *vec = *(SDKVector **)addr;

			buffer[0] = sp_ftoc(vec->x);
			buffer[1] = sp_ftoc(vec->y);
			buffer[2] = sp_ftoc(vec->z);
			return 1;
		}	
	}

	return pContext->ThrowNativeError("Invalid param type to get. Param is not a vector.");
}

static void FreeChangedVector(void *pData)
{
	delete (SDKVector *)pData;
}

// native DHookSetParamVector(Handle:hParams, num, Float:vec[3])
cell_t Native_SetParamVector(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookParamsHandle, g_HookReturnHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	size_t offset = GetParamOffset(paramStruct, index);
	void **origAddr = (void **)((intptr_t)paramStruct->orgParams + offset);
	void **newAddr = (void **)((intptr_t)paramStruct->newParams + offset);

	switch(paramStruct->dg->params.at(index).type)
	{
		case HookParamType_VectorPtr:
		{
			cell_t *buffer;
			pContext->LocalToPhysAddr(params[3], &buffer);
			SDKVector *origVec = *(SDKVector **)origAddr;
			SDKVector **newVec = (SDKVector **)newAddr;

			if(origVec == nullptr)
			{
				*newVec = new SDKVector(sp_ctof(buffer[0]), sp_ctof(buffer[1]), sp_ctof(buffer[2]));
				// Free it later (cheaply) after the function returned.
				smutils->AddFrameAction(FreeChangedVector, *newVec);
			}
			else
			{
				origVec->x = sp_ctof(buffer[0]);
				origVec->y = sp_ctof(buffer[1]);
				origVec->z = sp_ctof(buffer[2]);
				*newVec = origVec;
			}
			paramStruct->isChanged[index] = true;
			return 1;
		}
	}
	return pContext->ThrowNativeError("Invalid param type to set. Param is not a vector.");
}

// native DHookGetParamString(Handle:hParams, num, String:buffer[], size)
cell_t Native_GetParamString(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookParamsHandle, g_HookReturnHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	size_t offset = GetParamOffset(paramStruct, index);
	void *addr = (void **)((intptr_t)paramStruct->orgParams + offset);

	if(*(void **)addr == NULL)
	{
		return pContext->ThrowNativeError("Trying to get value for null pointer.");
	}

	if(paramStruct->dg->params.at(index).type == HookParamType_CharPtr)
	{
		pContext->StringToLocal(params[3], params[4], *(const char **)addr);
	}

	return 1;
}

// native DHookGetReturnString(Handle:hReturn, String:buffer[], size)
cell_t Native_GetReturnString(IPluginContext *pContext, const cell_t *params)
{
	HookReturnStruct *returnStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookReturnHandle, g_HookParamsHandle, (void **)&returnStruct, pContext, params[1]))
	{
		return 0;
	}

	switch(returnStruct->type)
	{
		case ReturnType_String:
			pContext->StringToLocal(params[2], params[3], (*(string_t *)returnStruct->orgResult == NULL_STRING) ? "" : STRING(*(string_t *)returnStruct->orgResult));
			return 1;
		case ReturnType_StringPtr:
			pContext->StringToLocal(params[2], params[3], ((string_t *)returnStruct->orgResult == NULL) ? "" : ((string_t *)returnStruct->orgResult)->ToCStr());
			return 1;
		case ReturnType_CharPtr:
			pContext->StringToLocal(params[2], params[3], ((char *)returnStruct->orgResult == NULL) ? "" : (const char *)returnStruct->orgResult);
			return 1;
		default:
			return pContext->ThrowNativeError("Invalid param type to get. Param is not a string.");
	}
}

static void FreeChangedCharPtr(void *pData)
{
	delete[](char *)pData;
}

//native DHookSetReturnString(Handle:hReturn, String:value[])
cell_t Native_SetReturnString(IPluginContext *pContext, const cell_t *params)
{
	HookReturnStruct *returnStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookReturnHandle, g_HookParamsHandle, (void **)&returnStruct, pContext, params[1]))
	{
		return 0;
	}

	char *value;
	pContext->LocalToString(params[2], &value);

	switch(returnStruct->type)
	{
		case ReturnType_CharPtr:
		{
			returnStruct->newResult = new char[strlen(value) + 1];
			strcpy((char *)returnStruct->newResult, value);
			returnStruct->isChanged = true;
			// Free it later (cheaply) after the function returned.
			smutils->AddFrameAction(FreeChangedCharPtr, returnStruct->newResult);
			return 1;
		}
		default:
			return pContext->ThrowNativeError("Invalid param type to get. Param is not a char pointer.");
	}
}

//native DHookSetParamString(Handle:hParams, num, String:value[])
cell_t Native_SetParamString(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookParamsHandle, g_HookReturnHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;
	size_t offset = GetParamOffset(paramStruct, index);
	void *addr = (void **)((intptr_t)paramStruct->newParams + offset);

	char *value;
	pContext->LocalToString(params[3], &value);

	if(paramStruct->dg->params.at(index).type == HookParamType_CharPtr)
	{
		*(char **)addr = new char[strlen(value)+1];
		strcpy(*(char **)addr, value);
		paramStruct->isChanged[index] = true;
		// Free it later (cheaply) after the function returned.
		smutils->AddFrameAction(FreeChangedCharPtr, *(char **)addr);
	}
	return 1;
}

//native DHookAddEntityListener(ListenType:type, ListenCB:callback);
cell_t Native_AddEntityListener(IPluginContext *pContext, const cell_t *params)
{
	if(g_pEntityListener)
	{
		return g_pEntityListener->AddPluginEntityListener((ListenType)params[1], pContext->GetFunctionById(params[2]));;
	}
	return pContext->ThrowNativeError("Failed to get g_pEntityListener");
}

//native bool:DHookRemoveEntityListener(ListenType:type, ListenCB:callback);
cell_t Native_RemoveEntityListener(IPluginContext *pContext, const cell_t *params)
{
	if(g_pEntityListener)
	{
		return g_pEntityListener->RemovePluginEntityListener((ListenType)params[1], pContext->GetFunctionById(params[2]));;
	}
	return pContext->ThrowNativeError("Failed to get g_pEntityListener");
}

//native any:DHookGetParamObjectPtrVar(Handle:hParams, num, offset, ObjectValueType:type);
cell_t Native_GetParamObjectPtrVar(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookParamsHandle, g_HookReturnHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	if(paramStruct->dg->params.at(index).type != HookParamType_ObjectPtr && paramStruct->dg->params.at(index).type != HookParamType_Object)
	{
		return pContext->ThrowNativeError("Invalid object value type %i", paramStruct->dg->params.at(index).type);
	}

	size_t offset = GetParamOffset(paramStruct, index);
	void *addr = GetObjectAddr(paramStruct->dg->params.at(index).type, paramStruct->dg->params.at(index).flags, paramStruct->orgParams, offset);

	switch((ObjectValueType)params[4])
	{
		case ObjectValueType_Int:
		{
			return *(int *)((intptr_t)addr + params[3]);
		}
		case ObjectValueType_Bool:
			return (*(bool *)((intptr_t)addr + params[3])) ? 1 : 0;
		case ObjectValueType_Ehandle:
		case ObjectValueType_EhandlePtr:
		{
			edict_t *pEdict = gamehelpers->GetHandleEntity(*(CBaseHandle *)((intptr_t)addr +params[3]));

			if(!pEdict)
			{
				return -1;
			}

			return gamehelpers->IndexOfEdict(pEdict);
		}
		case ObjectValueType_Float:
		{
			return sp_ftoc(*(float *)((intptr_t)addr + params[3]));
		}
		case ObjectValueType_CBaseEntityPtr:
			return gamehelpers->EntityToBCompatRef(*(CBaseEntity **)((intptr_t)addr + params[3]));
		case ObjectValueType_IntPtr:
		{
			int *ptr = *(int **)((intptr_t)addr + params[3]);
			return *ptr;
		}
		case ObjectValueType_BoolPtr:
		{
			bool *ptr = *(bool **)((intptr_t)addr + params[3]);
			return *ptr ? 1 : 0;
		}
		case ObjectValueType_FloatPtr:
		{
			float *ptr = *(float **)((intptr_t)addr + params[3]);
			return sp_ftoc(*ptr);
		}
		default:
			return pContext->ThrowNativeError("Invalid Object value type");
	}
}

//native DHookSetParamObjectPtrVar(Handle:hParams, num, offset, ObjectValueType:type, value)
cell_t Native_SetParamObjectPtrVar(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookParamsHandle, g_HookReturnHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	if(paramStruct->dg->params.at(index).type != HookParamType_ObjectPtr && paramStruct->dg->params.at(index).type != HookParamType_Object)
	{
		return pContext->ThrowNativeError("Invalid object value type %i", paramStruct->dg->params.at(index).type);
	}
	
	size_t offset = GetParamOffset(paramStruct, index);
	void *addr = GetObjectAddr(paramStruct->dg->params.at(index).type, paramStruct->dg->params.at(index).flags, paramStruct->orgParams, offset);

	switch((ObjectValueType)params[4])
	{
		case ObjectValueType_Int:
			*(int *)((intptr_t)addr + params[3]) = params[5];
			break;
		case ObjectValueType_Bool:
			*(bool *)((intptr_t)addr + params[3]) = params[5] != 0;
			break;
		case ObjectValueType_Ehandle:
		case ObjectValueType_EhandlePtr:
		{
			edict_t *pEdict = gamehelpers->EdictOfIndex(params[5]);

			if(!pEdict || pEdict->IsFree())
			{
				return pContext->ThrowNativeError("Invalid edict passed");
			}
			gamehelpers->SetHandleEntity(*(CBaseHandle *)((intptr_t)addr + params[3]), pEdict);

			break;
		}
		case ObjectValueType_Float:
			*(float *)((intptr_t)addr + params[3]) = sp_ctof(params[5]);
			break;
		case ObjectValueType_CBaseEntityPtr:
		{
			CBaseEntity *pEnt = gamehelpers->ReferenceToEntity(params[5]);

			if(!pEnt)
			{
				return pContext->ThrowNativeError("Invalid entity passed");
			}

			*(CBaseEntity **)((intptr_t)addr + params[3]) = pEnt;
			break;
		}
		case ObjectValueType_IntPtr:
		{
			int *ptr = *(int **)((intptr_t)addr + params[3]);
			*ptr = params[5];
			break;
		}
		case ObjectValueType_BoolPtr:
		{
			bool *ptr = *(bool **)((intptr_t)addr + params[3]);
			*ptr = params[5] != 0;
			break;
		}
		case ObjectValueType_FloatPtr:
		{
			float *ptr = *(float **)((intptr_t)addr + params[3]);
			*ptr = sp_ctof(params[5]);
			break;
		}
		default:
			return pContext->ThrowNativeError("Invalid Object value type");
	}
	return 1;
}

//native DHookGetParamObjectPtrVarVector(Handle:hParams, num, offset, ObjectValueType:type, Float:buffer[3]);
cell_t Native_GetParamObjectPtrVarVector(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookParamsHandle, g_HookReturnHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	if(paramStruct->dg->params.at(index).type != HookParamType_ObjectPtr && paramStruct->dg->params.at(index).type != HookParamType_Object)
	{
		return pContext->ThrowNativeError("Invalid object value type %i", paramStruct->dg->params.at(index).type);
	}

	size_t offset = GetParamOffset(paramStruct, index);
	void *addr = GetObjectAddr(paramStruct->dg->params.at(index).type, paramStruct->dg->params.at(index).flags, paramStruct->orgParams, offset);

	cell_t *buffer;
	pContext->LocalToPhysAddr(params[5], &buffer);

	if((ObjectValueType)params[4] == ObjectValueType_VectorPtr || (ObjectValueType)params[4] == ObjectValueType_Vector)
	{
		SDKVector *vec;

		if((ObjectValueType)params[4] == ObjectValueType_VectorPtr)
		{
			vec = *(SDKVector **)((intptr_t)addr + params[3]);
			if(vec == NULL)
			{
				return pContext->ThrowNativeError("Trying to get value for null pointer.");
			}
		}
		else
		{
			vec = (SDKVector *)((intptr_t)addr + params[3]);
		}

		buffer[0] = sp_ftoc(vec->x);
		buffer[1] = sp_ftoc(vec->y);
		buffer[2] = sp_ftoc(vec->z);
		return 1;
	}

	return pContext->ThrowNativeError("Invalid Object value type (not a type of vector)");
}

//native DHookSetParamObjectPtrVarVector(Handle:hParams, num, offset, ObjectValueType:type, Float:value[3]);
cell_t Native_SetParamObjectPtrVarVector(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookParamsHandle, g_HookReturnHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	if(paramStruct->dg->params.at(index).type != HookParamType_ObjectPtr && paramStruct->dg->params.at(index).type != HookParamType_Object)
	{
		return pContext->ThrowNativeError("Invalid object value type %i", paramStruct->dg->params.at(index).type);
	}

	size_t offset = GetParamOffset(paramStruct, index);
	void *addr = GetObjectAddr(paramStruct->dg->params.at(index).type, paramStruct->dg->params.at(index).flags, paramStruct->orgParams, offset);

	cell_t *buffer;
	pContext->LocalToPhysAddr(params[5], &buffer);

	if((ObjectValueType)params[4] == ObjectValueType_VectorPtr || (ObjectValueType)params[4] == ObjectValueType_Vector)
	{
		SDKVector *vec;

		if((ObjectValueType)params[4] == ObjectValueType_VectorPtr)
		{
			vec = *(SDKVector **)((intptr_t)addr + params[3]);
			if(vec == NULL)
			{
				return pContext->ThrowNativeError("Trying to set value for null pointer.");
			}
		}
		else
		{
			vec = (SDKVector *)((intptr_t)addr + params[3]);
		}

		vec->x = sp_ctof(buffer[0]);
		vec->y = sp_ctof(buffer[1]);
		vec->z = sp_ctof(buffer[2]);
		return 1;
	}
	return pContext->ThrowNativeError("Invalid Object value type (not a type of vector)");
}

//native DHookGetParamObjectPtrString(Handle:hParams, num, offset, ObjectValueType:type, String:buffer[], size)
cell_t Native_GetParamObjectPtrString(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookParamsHandle, g_HookReturnHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	if(paramStruct->dg->params.at(index).type != HookParamType_ObjectPtr && paramStruct->dg->params.at(index).type != HookParamType_Object)
	{
		return pContext->ThrowNativeError("Invalid object value type %i", paramStruct->dg->params.at(index).type);
	}

	size_t offset = GetParamOffset(paramStruct, index);
	void *addr = GetObjectAddr(paramStruct->dg->params.at(index).type, paramStruct->dg->params.at(index).flags, paramStruct->orgParams, offset);

	switch((ObjectValueType)params[4])
	{
		case ObjectValueType_CharPtr:
		{
			char *ptr = *(char **)((intptr_t)addr + params[3]);
			pContext->StringToLocal(params[5], params[6], ptr == NULL ? "" : (const char *)ptr);
			break;
		}
		case ObjectValueType_String:
		{
			string_t string = *(string_t *)((intptr_t)addr + params[3]);
			pContext->StringToLocal(params[5], params[6], string == NULL_STRING ? "" : STRING(string));
			break;
		}
		default:
			return pContext->ThrowNativeError("Invalid Object value type (not a type of string)");
	}
	return 1;
}

// DHookGetReturnVector(Handle:hReturn, Float:vec[3])
cell_t Native_GetReturnVector(IPluginContext *pContext, const cell_t *params)
{
	HookReturnStruct *returnStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookReturnHandle, g_HookParamsHandle, (void **)&returnStruct, pContext, params[1]))
	{
		return 0;
	}

	cell_t *buffer;
	pContext->LocalToPhysAddr(params[2], &buffer);

	if(returnStruct->type == ReturnType_Vector)
	{
		buffer[0] = sp_ftoc((*(SDKVector *)returnStruct->orgResult).x);
		buffer[1] = sp_ftoc((*(SDKVector *)returnStruct->orgResult).y);
		buffer[2] = sp_ftoc((*(SDKVector *)returnStruct->orgResult).z);

		return 1;
	}
	else if(returnStruct->type == ReturnType_VectorPtr)
	{
		buffer[0] = sp_ftoc(((SDKVector *)returnStruct->orgResult)->x);
		buffer[1] = sp_ftoc(((SDKVector *)returnStruct->orgResult)->y);
		buffer[2] = sp_ftoc(((SDKVector *)returnStruct->orgResult)->z);

		return 1;
	}
	return pContext->ThrowNativeError("Return type is not a vector type");
}

//DHookSetReturnVector(Handle:hReturn, Float:vec[3])
cell_t Native_SetReturnVector(IPluginContext *pContext, const cell_t *params)
{
	HookReturnStruct *returnStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookReturnHandle, g_HookParamsHandle, (void **)&returnStruct, pContext, params[1]))
	{
		return 0;
	}

	cell_t *buffer;
	pContext->LocalToPhysAddr(params[2], &buffer);

	if(returnStruct->type == ReturnType_Vector)
	{
		*(SDKVector *)returnStruct->newResult = SDKVector(sp_ctof(buffer[0]), sp_ctof(buffer[1]), sp_ctof(buffer[2]));
		returnStruct->isChanged = true;

		return 1;
	}
	else if(returnStruct->type == ReturnType_VectorPtr)
	{
		returnStruct->newResult = new SDKVector(sp_ctof(buffer[0]), sp_ctof(buffer[1]), sp_ctof(buffer[2]));
		returnStruct->isChanged = true;
		// Free it later (cheaply) after the function returned.
		smutils->AddFrameAction(FreeChangedVector, returnStruct->newResult);
		
		return 1;
	}
	return pContext->ThrowNativeError("Return type is not a vector type");
}

//native bool:DHookIsNullParam(Handle:hParams, num);
cell_t Native_IsNullParam(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookParamsHandle, g_HookReturnHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;
	size_t offset = GetParamOffset(paramStruct, index);
	void *addr = (void **)((intptr_t)paramStruct->orgParams + offset);

	HookParamType type = paramStruct->dg->params.at(index).type;

	//Check that the type is ptr
	if(type == HookParamType_StringPtr || type == HookParamType_CharPtr || type == HookParamType_VectorPtr || type == HookParamType_CBaseEntity || type == HookParamType_ObjectPtr || type == HookParamType_Edict || type == HookParamType_Unknown)
		return *(void **)addr == NULL;
	else
		return pContext->ThrowNativeError("Param is not a pointer!");
}

//native Address:DHookGetParamAddress(Handle:hParams, num);
cell_t Native_GetParamAddress(IPluginContext *pContext, const cell_t *params)
{
	HookParamsStruct *paramStruct;

	if(!GetCallbackArgHandleIfValidOrError(g_HookParamsHandle, g_HookReturnHandle, (void **)&paramStruct, pContext, params[1]))
	{
		return 0;
	}

	if(params[2] <= 0 || params[2] > (int)paramStruct->dg->params.size())
	{
		return pContext->ThrowNativeError("Invalid param number %i max params is %i", params[2], paramStruct->dg->params.size());
	}

	int index = params[2] - 1;

	HookParamType type = paramStruct->dg->params.at(index).type;
	if(type != HookParamType_StringPtr && type != HookParamType_CharPtr && type != HookParamType_VectorPtr && type != HookParamType_CBaseEntity && type != HookParamType_ObjectPtr && type != HookParamType_Edict && type != HookParamType_Unknown)
	{	
		return pContext->ThrowNativeError("Param is not a pointer!");
	}

	size_t offset = GetParamOffset(paramStruct, index);
	return *(cell_t *)((intptr_t)paramStruct->orgParams + offset);
}

sp_nativeinfo_t g_Natives[] = 
{
	{"DHookCreate",                         Native_CreateHook},
	{"DHookCreateDetour",                   Native_CreateDetour},
	{"DHookCreateFromConf",                 Native_DHookCreateFromConf},
	{"DHookSetFromConf",                    Native_SetFromConf},
	{"DHookAddParam",                       Native_AddParam},
	{"DHookEnableDetour",                   Native_EnableDetour},
	{"DHookDisableDetour",                  Native_DisableDetour},
	{"DHookEntity",                         Native_HookEntity},
	{"DHookGamerules",                      Native_HookGamerules},
	{"DHookRaw",                            Native_HookRaw},
	{"DHookRemoveHookID",                   Native_RemoveHookID},
	{"DHookGetParam",                       Native_GetParam},
	{"DHookGetReturn",                      Native_GetReturn},
	{"DHookSetReturn",                      Native_SetReturn},
	{"DHookSetParam",                       Native_SetParam},
	{"DHookGetParamVector",                 Native_GetParamVector},
	{"DHookGetReturnVector",                Native_GetReturnVector},
	{"DHookSetReturnVector",                Native_SetReturnVector},
	{"DHookSetParamVector",                 Native_SetParamVector},
	{"DHookGetParamString",                 Native_GetParamString},
	{"DHookGetReturnString",                Native_GetReturnString},
	{"DHookSetReturnString",                Native_SetReturnString},
	{"DHookSetParamString",                 Native_SetParamString},
	{"DHookAddEntityListener",              Native_AddEntityListener},
	{"DHookRemoveEntityListener",           Native_RemoveEntityListener},
	{"DHookGetParamObjectPtrVar",           Native_GetParamObjectPtrVar},
	{"DHookSetParamObjectPtrVar",           Native_SetParamObjectPtrVar},
	{"DHookGetParamObjectPtrVarVector",     Native_GetParamObjectPtrVarVector},
	{"DHookSetParamObjectPtrVarVector",     Native_SetParamObjectPtrVarVector},
	{"DHookGetParamObjectPtrString",        Native_GetParamObjectPtrString},
	{"DHookIsNullParam",                    Native_IsNullParam},
	{"DHookGetParamAddress",                Native_GetParamAddress},

	// Methodmap API
	{"DHookSetup.AddParam",                 Native_AddParam},
	{"DHookSetup.SetFromConf",              Native_SetFromConf},

	{"DynamicHook.DynamicHook",             Native_CreateHook},
	{"DynamicHook.FromConf",                Native_DHookCreateFromConf},
	{"DynamicHook.HookEntity",              Native_HookEntity_Methodmap},
	{"DynamicHook.HookGamerules",           Native_HookGamerules_Methodmap},
	{"DynamicHook.HookRaw",                 Native_HookRaw_Methodmap},
	{"DynamicHook.RemoveHook",              Native_RemoveHookID},

	{"DynamicDetour.DynamicDetour",         Native_CreateDetour},
	{"DynamicDetour.FromConf",              Native_DHookCreateFromConf},
	{"DynamicDetour.Enable",                Native_EnableDetour},
	{"DynamicDetour.Disable",               Native_DisableDetour},

	{"DHookParam.Get",                      Native_GetParam},
	{"DHookParam.GetVector",                Native_GetParamVector},
	{"DHookParam.GetString",                Native_GetParamString},
	{"DHookParam.Set",                      Native_SetParam},
	{"DHookParam.SetVector",                Native_SetParamVector},
	{"DHookParam.SetString",                Native_SetParamString},
	{"DHookParam.GetObjectVar",             Native_GetParamObjectPtrVar},
	{"DHookParam.GetObjectVarVector",       Native_GetParamObjectPtrVarVector},
	{"DHookParam.GetObjectVarString",       Native_GetParamObjectPtrString},
	{"DHookParam.SetObjectVar",             Native_SetParamObjectPtrVar},
	{"DHookParam.SetObjectVarVector",       Native_SetParamObjectPtrVarVector},
	{"DHookParam.IsNull",                   Native_IsNullParam},
	{"DHookParam.GetAddress",               Native_GetParamAddress},

	{"DHookReturn.Value.get",               Native_GetReturn},
	{"DHookReturn.Value.set",               Native_SetReturn},
	{"DHookReturn.GetVector",               Native_GetReturnVector},
	{"DHookReturn.SetVector",               Native_SetReturnVector},
	{"DHookReturn.GetString",               Native_GetReturnString},
	{"DHookReturn.SetString",               Native_SetReturnString},
	{NULL,                                  NULL}
};
