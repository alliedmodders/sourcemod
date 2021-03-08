/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
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

#include <memory>

#include "common_logic.h"
#include <IPluginSys.h>
#include <IHandleSys.h>
#include <IForwardSys.h>
#include <ISourceMod.h>

HandleType_t g_GlobalFwdType = 0;
HandleType_t g_PrivateFwdType = 0;

static bool s_CallStarted = false;
static ICallable *s_pCallable = NULL;
static IPluginFunction *s_pFunction = NULL;
static IForward *s_pForward = NULL;

class ForwardNativeHelpers : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public:
	void OnSourceModAllInitialized()
	{
		HandleAccess sec;

		/* Set GlobalFwd handle access security */
		handlesys->InitAccessDefaults(NULL, &sec);
		sec.access[HandleAccess_Read] = 0;
		sec.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;

		/* Create 'GlobalFwd' handle type */
		g_GlobalFwdType = handlesys->CreateType("GlobalFwd", this, 0, NULL, &sec, g_pCoreIdent, NULL);

		/* Private forwards are cloneable */
		sec.access[HandleAccess_Clone] = 0;

		/* Create 'PrivateFwd' handle type */
		g_PrivateFwdType = handlesys->CreateType("PrivateFwd", this, g_GlobalFwdType, NULL, &sec, g_pCoreIdent, NULL);
	}

	void OnSourceModShutdown()
	{
		handlesys->RemoveType(g_PrivateFwdType, g_pCoreIdent);
		handlesys->RemoveType(g_GlobalFwdType, g_pCoreIdent);
	}

	void OnHandleDestroy(HandleType_t type, void *object)
	{
		IForward *pForward = static_cast<IForward *>(object);

		forwardsys->ReleaseForward(pForward);
	}

	bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
	{
		*pSize = sizeof(IForward*) + (((IForward *)object)->GetFunctionCount() * 12);
		return true;
	}
} g_ForwardNativeHelpers;


/* Turn a public index into a function ID */
inline funcid_t PublicIndexToFuncId(uint32_t idx)
{
	return (idx << 1) | (1 << 0);
}

/* Reset global function/forward call variables */
inline void ResetCall()
{
	s_CallStarted = false;
	s_pFunction = NULL;
	s_pForward = NULL;
	s_pCallable = NULL;
}

static cell_t sm_GetFunctionByName(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	char *name;
	uint32_t idx;
	IPlugin *pPlugin;

	if (hndl == 0)
	{
		pPlugin = pluginsys->FindPluginByContext(pContext->GetContext());
	} else {
		pPlugin = pluginsys->PluginFromHandle(hndl, &err);

		if (!pPlugin)
		{
			return pContext->ThrowNativeError("Plugin handle %x is invalid (error %d)", hndl, err);
		}
	}

	pContext->LocalToString(params[2], &name);

	/* Get public function index */
	if (pPlugin->GetBaseContext()->FindPublicByName(name, &idx) == SP_ERROR_NOT_FOUND)
	{
		/* Return INVALID_FUNCTION if not found */
		return -1;
	}

	/* Return function ID */
	return PublicIndexToFuncId(idx);
}

static cell_t sm_CreateGlobalForward(IPluginContext *pContext, const cell_t *params)
{
	cell_t count = params[0];
	char *name;
	ParamType forwardParams[SP_MAX_EXEC_PARAMS];

	if (count - 2 > SP_MAX_EXEC_PARAMS)
	{
		return pContext->ThrowNativeErrorEx(SP_ERROR_PARAMS_MAX, NULL);
	}
	
	pContext->LocalToString(params[1], &name);

	cell_t *addr;
	for (int i = 3; i <= count; i++)
	{
		pContext->LocalToPhysAddr(params[i], &addr);
		forwardParams[i - 3] = static_cast<ParamType>(*addr);
	}

	IForward *pForward = forwardsys->CreateForward(name, static_cast<ExecType>(params[2]), count - 2, forwardParams);

	return handlesys->CreateHandle(g_GlobalFwdType, pForward, pContext->GetIdentity(), g_pCoreIdent, NULL);
}

static cell_t sm_CreateForward(IPluginContext *pContext, const cell_t *params)
{
	cell_t count = params[0];
	ParamType forwardParams[SP_MAX_EXEC_PARAMS];

	if (count - 1 > SP_MAX_EXEC_PARAMS)
	{
		return pContext->ThrowNativeErrorEx(SP_ERROR_PARAMS_MAX, NULL);
	}

	cell_t *addr;
	for (int i = 2; i <= count; i++)
	{
		pContext->LocalToPhysAddr(params[i], &addr);
		forwardParams[i - 2] = static_cast<ParamType>(*addr);
	}

	IChangeableForward *pForward = forwardsys->CreateForwardEx(NULL, static_cast<ExecType>(params[1]), count - 1, forwardParams);

	return handlesys->CreateHandle(g_PrivateFwdType, pForward, pContext->GetIdentity(), g_pCoreIdent, NULL);
}

static cell_t sm_GetForwardFunctionCount(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	IForward *pForward;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(hndl, g_GlobalFwdType, &sec, (void **)&pForward))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid forward handle %x (error %d)", hndl, err);
	}

	return pForward->GetFunctionCount();
}

static cell_t sm_AddToForward(IPluginContext *pContext, const cell_t *params)
{
	Handle_t fwdHandle = static_cast<Handle_t>(params[1]);
	Handle_t plHandle = static_cast<Handle_t>(params[2]);
	HandleError err;
	IChangeableForward *pForward;
	IPlugin *pPlugin;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(fwdHandle, g_PrivateFwdType, &sec, (void **)&pForward))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid private forward handle %x (error %d)", fwdHandle, err);
	}

	if (plHandle == 0)
	{
		pPlugin = pluginsys->FindPluginByContext(pContext->GetContext());
	} else {
		pPlugin = pluginsys->PluginFromHandle(plHandle, &err);

		if (!pPlugin)
		{
			return pContext->ThrowNativeError("Plugin handle %x is invalid (error %d)", plHandle, err);
		}
	}

	IPluginFunction *pFunction = pPlugin->GetBaseContext()->GetFunctionById(params[3]);

	if (!pFunction)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[3]);
	}

	return pForward->AddFunction(pFunction);
}

static cell_t sm_RemoveFromForward(IPluginContext *pContext, const cell_t *params)
{
	Handle_t fwdHandle = static_cast<Handle_t>(params[1]);
	Handle_t plHandle = static_cast<Handle_t>(params[2]);
	HandleError err;
	IChangeableForward *pForward;
	IPlugin *pPlugin;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(fwdHandle, g_PrivateFwdType, &sec, (void **)&pForward))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid private forward handle %x (error %d)", fwdHandle, err);
	}

	if (plHandle == 0)
	{
		pPlugin = pluginsys->FindPluginByContext(pContext->GetContext());
	} else {
		pPlugin = pluginsys->PluginFromHandle(plHandle, &err);

		if (!pPlugin)
		{
			return pContext->ThrowNativeError("Plugin handle %x is invalid (error %d)", plHandle, err);
		}
	}

	IPluginFunction *pFunction = pPlugin->GetBaseContext()->GetFunctionById(params[3]);

	if (!pFunction)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[3]);
	}

	return pForward->RemoveFunction(pFunction);
}

static cell_t sm_RemoveAllFromForward(IPluginContext *pContext, const cell_t *params)
{
	Handle_t fwdHandle = static_cast<Handle_t>(params[1]);
	Handle_t plHandle = static_cast<Handle_t>(params[2]);
	HandleError err;
	IChangeableForward *pForward;
	IPlugin *pPlugin;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err=handlesys->ReadHandle(fwdHandle, g_PrivateFwdType, &sec, (void **)&pForward))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid private forward handle %x (error %d)", fwdHandle, err);
	}

	if (plHandle == 0)
	{
		pPlugin = pluginsys->FindPluginByContext(pContext->GetContext());
	} else {
		pPlugin = pluginsys->PluginFromHandle(plHandle, &err);

		if (!pPlugin)
		{
			return pContext->ThrowNativeError("Plugin handle %x is invalid (error %d)", plHandle, err);
		}
	}

	return pForward->RemoveFunctionsOfPlugin(pPlugin);
}

static cell_t sm_CallStartFunction(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	HandleError err;
	IPlugin *pPlugin;

	ResetCall();

	hndl = static_cast<Handle_t>(params[1]);

	if (hndl == 0)
	{
		pPlugin = pluginsys->FindPluginByContext(pContext->GetContext());
	} else {
		pPlugin = pluginsys->PluginFromHandle(hndl, &err);

		if (!pPlugin)
		{
			return pContext->ThrowNativeError("Plugin handle %x is invalid (error %d)", hndl, err);
		}
	}

	s_pFunction = pPlugin->GetBaseContext()->GetFunctionById(params[2]);

	if (!s_pFunction)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[2]);
	}

	s_pCallable = static_cast<ICallable *>(s_pFunction);

	s_CallStarted = true;

	return 1;
}

static cell_t sm_CallStartForward(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	HandleError err;
	IForward *pForward;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	ResetCall();

	hndl = static_cast<Handle_t>(params[1]);

	if ((err=handlesys->ReadHandle(hndl, g_GlobalFwdType, &sec, (void **)&pForward))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid forward handle %x (error %d)", hndl, err);
	}

	s_pForward = pForward;

	s_pCallable = static_cast<ICallable *>(pForward);

	s_CallStarted = true;

	return 1;
}

static cell_t sm_CallPushCell(IPluginContext *pContext, const cell_t *params)
{
	int err;

	if (!s_CallStarted)
	{
		return pContext->ThrowNativeError("Cannot push parameters when there is no call in progress");
	}

	err = s_pCallable->PushCell(params[1]);

	if (err)
	{
		s_pCallable->Cancel();
		ResetCall();
		return pContext->ThrowNativeErrorEx(err, NULL);
	}

	return 1;
}

static cell_t sm_CallPushCellRef(IPluginContext *pContext, const cell_t *params)
{
	int err;
	cell_t *addr;

	if (!s_CallStarted)
	{
		return pContext->ThrowNativeError("Cannot push parameters when there is no call in progress");
	}

	pContext->LocalToPhysAddr(params[1], &addr);

	err = s_pCallable->PushCellByRef(addr);

	if (err)
	{
		s_pCallable->Cancel();
		ResetCall();
		return pContext->ThrowNativeErrorEx(err, NULL);
	}

	return 1;
}

static cell_t sm_CallPushFloat(IPluginContext *pContext, const cell_t *params)
{
	int err;

	if (!s_CallStarted)
	{
		return pContext->ThrowNativeError("Cannot push parameters when there is no call in progress");
	}

	err = s_pCallable->PushFloat(sp_ctof(params[1]));

	if (err)
	{
		s_pCallable->Cancel();
		ResetCall();
		return pContext->ThrowNativeErrorEx(err, NULL);
	}

	return 1;
}

static cell_t sm_CallPushFloatRef(IPluginContext *pContext, const cell_t *params)
{
	int err;
	cell_t *addr;

	if (!s_CallStarted)
	{
		return pContext->ThrowNativeError("Cannot push parameters when there is no call in progress");
	}

	pContext->LocalToPhysAddr(params[1], &addr);

	err = s_pCallable->PushFloatByRef(reinterpret_cast<float *>(addr));

	if (err)
	{
		s_pCallable->Cancel();
		ResetCall();
		return pContext->ThrowNativeErrorEx(err, NULL);
	}

	return 1;
}

static cell_t sm_CallPushArray(IPluginContext *pContext, const cell_t *params)
{
	int err;
	cell_t *addr;

	if (!s_CallStarted)
	{
		return pContext->ThrowNativeError("Cannot push parameters when there is no call in progress");
	}

	pContext->LocalToPhysAddr(params[1], &addr);

	err = s_pCallable->PushArray(addr, params[2]);

	if (err)
	{
		s_pCallable->Cancel();
		ResetCall();
		return pContext->ThrowNativeErrorEx(err, NULL);
	}

	return 1;
}

static cell_t sm_CallPushArrayEx(IPluginContext *pContext, const cell_t *params)
{
	int err;
	cell_t *addr;

	if (!s_CallStarted)
	{
		return pContext->ThrowNativeError("Cannot push parameters when there is no call in progress");
	}

	pContext->LocalToPhysAddr(params[1], &addr);

	err = s_pCallable->PushArray(addr, params[2], params[3]);

	if (err)
	{
		s_pCallable->Cancel();
		ResetCall();
		return pContext->ThrowNativeErrorEx(err, NULL);
	}

	return 1;
}

static cell_t sm_CallPushString(IPluginContext *pContext, const cell_t *params)
{
	int err;
	char *value;

	if (!s_CallStarted)
	{
		return pContext->ThrowNativeError("Cannot push parameters when there is no call in progress");
	}

	pContext->LocalToString(params[1], &value);

	err = s_pCallable->PushString(value);

	if (err)
	{
		s_pCallable->Cancel();
		ResetCall();
		return pContext->ThrowNativeErrorEx(err, NULL);
	}

	return 1;
}

static cell_t sm_CallPushStringEx(IPluginContext *pContext, const cell_t *params)
{
	int err;
	char *value;

	if (!s_CallStarted)
	{
		return pContext->ThrowNativeError("Cannot push parameters when there is no call in progress");
	}

	pContext->LocalToString(params[1], &value);

	err = s_pCallable->PushStringEx(value, params[2], params[3], params[4]);

	if (err)
	{
		s_pCallable->Cancel();
		ResetCall();
		return pContext->ThrowNativeErrorEx(err, NULL);
	}

	return 1;
}

static cell_t sm_CallPushNullVector(IPluginContext *pContext, const cell_t *params)
{
	int err = SP_ERROR_NOT_FOUND;

	if (!s_CallStarted)
	{
		return pContext->ThrowNativeError("Cannot push parameters when there is no call in progress");
	}

	if (s_pFunction)
	{
		// Find the NULL_VECTOR pubvar in the target plugin and push the local address.
		IPluginRuntime *runtime = s_pFunction->GetParentRuntime();
		uint32_t null_vector_idx;
		err = runtime->FindPubvarByName("NULL_VECTOR", &null_vector_idx);
		if (err)
		{
			return pContext->ThrowNativeErrorEx(err, "Target plugin has no NULL_VECTOR.");
		}

		cell_t null_vector;
		err = runtime->GetPubvarAddrs(null_vector_idx, &null_vector, nullptr);

		if (!err)
			err = s_pCallable->PushCell(null_vector);
	}
	else if (s_pForward)
	{
		err = s_pForward->PushArray(NULL, 3);
	}

	if (err)
	{
		s_pCallable->Cancel();
		ResetCall();
		return pContext->ThrowNativeErrorEx(err, NULL);
	}

	return 1;
}

static cell_t sm_CallPushNullString(IPluginContext *pContext, const cell_t *params)
{
	int err = SP_ERROR_NOT_FOUND;

	if (!s_CallStarted)
	{
		return pContext->ThrowNativeError("Cannot push parameters when there is no call in progress");
	}

	if (s_pFunction)
	{
		// Find the NULL_STRING pubvar in the target plugin and push the local address.
		IPluginRuntime *runtime = s_pFunction->GetParentRuntime();
		uint32_t null_string_idx;
		err = runtime->FindPubvarByName("NULL_STRING", &null_string_idx);
		if (err)
		{
			return pContext->ThrowNativeErrorEx(err, "Target plugin has no NULL_STRING.");
		}

		cell_t null_string;
		err = runtime->GetPubvarAddrs(null_string_idx, &null_string, nullptr);

		if (!err)
			err = s_pCallable->PushCell(null_string);
	}
	else if (s_pForward)
	{
		err = s_pForward->PushString(NULL);
	}

	if (err)
	{
		s_pCallable->Cancel();
		ResetCall();
		return pContext->ThrowNativeErrorEx(err, NULL);
	}

	return 1;
}

static cell_t sm_CallFinish(IPluginContext *pContext, const cell_t *params)
{
	int err = SP_ERROR_NOT_RUNNABLE;
	cell_t *result;

	if (!s_CallStarted)
	{
		return pContext->ThrowNativeError("Cannot finish call when there is no call in progress");
	}

	pContext->LocalToPhysAddr(params[1], &result);

	// Note: Execute() swallows exceptions, so this is okay.
	if (s_pFunction)
	{
		IPluginFunction *pFunction = s_pFunction;
		ResetCall();
		err = pFunction->Execute(result);
	} else if (s_pForward) {
		IForward *pForward = s_pForward;
		ResetCall();
		err = pForward->Execute(result, NULL);
	}

	return err;
}

static cell_t sm_CallCancel(IPluginContext *pContext, const cell_t *params)
{
	if (!s_CallStarted)
	{
		return pContext->ThrowNativeError("Cannot cancel call when there is no call in progress");
	}

	s_pCallable->Cancel();
	ResetCall();

	return 1;
}

struct SMFrameActionData
{
	SMFrameActionData(Handle_t handle, Handle_t ownerhandle, cell_t data) : handle(handle), ownerhandle(ownerhandle), data(data)
	{
	};
	Handle_t handle;
	Handle_t ownerhandle;
	cell_t data;
};

static void PawnFrameAction(void *pData)
{
	std::unique_ptr<SMFrameActionData> frame(reinterpret_cast<SMFrameActionData *>(pData));
	IPlugin *pPlugin = pluginsys->PluginFromHandle(frame->ownerhandle, NULL);
	if (!pPlugin)
	{
		return;
	}

	IChangeableForward *pForward;
	HandleSecurity sec(pPlugin->GetIdentity(), g_pCoreIdent);
	if (handlesys->ReadHandle(frame->handle, g_PrivateFwdType, &sec, (void **)&pForward) != HandleError_None)
	{
		return;
	}

	pForward->PushCell(frame->data);
	pForward->Execute(NULL);

	handlesys->FreeHandle(frame->handle, &sec);
}

static cell_t sm_AddFrameAction(IPluginContext *pContext, const cell_t *params)
{
	IPlugin *pPlugin = pluginsys->FindPluginByContext(pContext->GetContext());
	IPluginFunction *pFunction = pPlugin->GetBaseContext()->GetFunctionById(params[1]);
	if (!pFunction)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[1]);
	}

	IChangeableForward *pForward = forwardsys->CreateForwardEx(NULL, ET_Ignore, 1, NULL, Param_Cell);
	IdentityToken_t *pIdentity = pContext->GetIdentity();
	Handle_t Handle = handlesys->CreateHandle(g_PrivateFwdType, pForward, pIdentity, g_pCoreIdent, NULL);
	if (Handle == BAD_HANDLE)
	{
		forwardsys->ReleaseForward(pForward);
		return 0;
	}

	pForward->AddFunction(pFunction);

	SMFrameActionData *pData = new SMFrameActionData(Handle, pPlugin->GetMyHandle(), params[2]);
	g_pSM->AddFrameAction(PawnFrameAction, pData);
	return 1;
}

REGISTER_NATIVES(functionNatives)
{
	{"GetFunctionByName",                   sm_GetFunctionByName},
	{"CreateGlobalForward",     	        sm_CreateGlobalForward},
	{"CreateForward",                       sm_CreateForward},
	{"GetForwardFunctionCount",             sm_GetForwardFunctionCount},
	{"AddToForward",                        sm_AddToForward},
	{"RemoveFromForward",                   sm_RemoveFromForward},
	{"RemoveAllFromForward",                sm_RemoveAllFromForward},
	{"Call_StartFunction",                  sm_CallStartFunction},
	{"Call_StartForward",                   sm_CallStartForward},
	{"Call_PushCell",                       sm_CallPushCell},
	{"Call_PushCellRef",                    sm_CallPushCellRef},
	{"Call_PushFloat",                      sm_CallPushFloat},
	{"Call_PushFloatRef",                   sm_CallPushFloatRef},
	{"Call_PushArray",                      sm_CallPushArray},
	{"Call_PushArrayEx",                    sm_CallPushArrayEx},
	{"Call_PushString",                     sm_CallPushString},
	{"Call_PushStringEx",                   sm_CallPushStringEx},
	{"Call_PushNullVector",                 sm_CallPushNullVector},
	{"Call_PushNullString",                 sm_CallPushNullString},
	{"Call_Finish",                         sm_CallFinish},
	{"Call_Cancel",                         sm_CallCancel},
	{"RequestFrame",                        sm_AddFrameAction},

	{"GlobalForward.GlobalForward",         sm_CreateGlobalForward},
	{"GlobalForward.FunctionCount.get",     sm_GetForwardFunctionCount},

	{"PrivateForward.PrivateForward",       sm_CreateForward},
	{"PrivateForward.AddFunction",          sm_AddToForward},
	{"PrivateForward.RemoveFunction",       sm_RemoveFromForward},
	{"PrivateForward.RemoveAllFunctions",   sm_RemoveAllFromForward},

	{NULL,                                  NULL},
};
