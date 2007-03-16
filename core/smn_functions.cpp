/**
* ===============================================================
* SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
* ===============================================================
*
* This file is not open source and may not be copied without explicit
* written permission of AlliedModders LLC.  This file may not be redistributed 
* in whole or significant part.
* For information, see LICENSE.txt or http://www.sourcemod.net/license.php
*
* Version: $Id$
*/

#include "sm_globals.h"
#include "PluginSys.h"
#include "ForwardSys.h"
#include "HandleSys.h"

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
		g_HandleSys.InitAccessDefaults(NULL, &sec);
		sec.access[HandleAccess_Read] = 0;
		sec.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;

		/* Create 'GlobalFwd' handle type */
		g_GlobalFwdType = g_HandleSys.CreateType("GlobalFwd", this, 0, NULL, &sec, g_pCoreIdent, NULL);

		/* Private forwards are cloneable */
		sec.access[HandleAccess_Clone] = 0;

		/* Create 'PrivateFwd' handle type */
		g_PrivateFwdType = g_HandleSys.CreateType("PrivateFwd", this, g_GlobalFwdType, NULL, &sec, g_pCoreIdent, NULL);
	}

	void OnSourceModShutdown()
	{
		g_HandleSys.RemoveType(g_PrivateFwdType, g_pCoreIdent);
		g_HandleSys.RemoveType(g_GlobalFwdType, g_pCoreIdent);
	}

	void OnHandleDestroy(HandleType_t type, void *object)
	{
		IForward *pForward = static_cast<IForward *>(object);

		g_Forwards.ReleaseForward(pForward);
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
		pPlugin = g_PluginSys.FindPluginByContext(pContext->GetContext());
	} else {
		pPlugin = g_PluginSys.PluginFromHandle(hndl, &err);

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

	IForward *pForward = g_Forwards.CreateForward(name, static_cast<ExecType>(params[2]), count - 2, forwardParams);

	return g_HandleSys.CreateHandle(g_GlobalFwdType, pForward, pContext->GetIdentity(), g_pCoreIdent, NULL);
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

	IChangeableForward *pForward = g_Forwards.CreateForwardEx(NULL, static_cast<ExecType>(params[1]), count - 1, forwardParams);

	return g_HandleSys.CreateHandle(g_PrivateFwdType, pForward, pContext->GetIdentity(), g_pCoreIdent, NULL);
}

static cell_t sm_GetForwardFunctionCount(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	IForward *pForward;

	if ((err=g_HandleSys.ReadHandle(hndl, g_GlobalFwdType, NULL, (void **)&pForward))
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

	if ((err=g_HandleSys.ReadHandle(fwdHandle, g_PrivateFwdType, NULL, (void **)&pForward))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid private forward handle %x (error %d)", fwdHandle, err);
	}

	if (plHandle == 0)
	{
		pPlugin = g_PluginSys.FindPluginByContext(pContext->GetContext());
	} else {
		pPlugin = g_PluginSys.PluginFromHandle(plHandle, &err);

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

	if ((err=g_HandleSys.ReadHandle(fwdHandle, g_PrivateFwdType, NULL, (void **)&pForward))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid private forward handle %x (error %d)", fwdHandle, err);
	}

	if (plHandle == 0)
	{
		pPlugin = g_PluginSys.FindPluginByContext(pContext->GetContext());
	} else {
		pPlugin = g_PluginSys.PluginFromHandle(plHandle, &err);

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

	if ((err=g_HandleSys.ReadHandle(fwdHandle, g_PrivateFwdType, NULL, (void **)&pForward))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid private forward handle %x (error %d)", fwdHandle, err);
	}

	if (plHandle == 0)
	{
		pPlugin = g_PluginSys.FindPluginByContext(pContext->GetContext());
	} else {
		pPlugin = g_PluginSys.PluginFromHandle(plHandle, &err);

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

	if (s_CallStarted)
	{
		return pContext->ThrowNativeError("Cannot start a call while one is already in progress");
	}

	hndl = static_cast<Handle_t>(params[1]);

	if (hndl == 0)
	{
		pPlugin = g_PluginSys.FindPluginByContext(pContext->GetContext());
	} else {
		pPlugin = g_PluginSys.PluginFromHandle(hndl, &err);

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

	if (s_CallStarted)
	{
		return pContext->ThrowNativeError("Cannot start a call while one is already in progress");
	}

	hndl = static_cast<Handle_t>(params[1]);

	if ((err=g_HandleSys.ReadHandle(hndl, g_GlobalFwdType, NULL, (void **)&pForward))
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

static cell_t sm_CallFinish(IPluginContext *pContext, const cell_t *params)
{
	int err = SP_ERROR_NOT_RUNNABLE;
	cell_t *result;

	if (!s_CallStarted)
	{
		return pContext->ThrowNativeError("Cannot finish call when there is no call in progress");
	}

	pContext->LocalToPhysAddr(params[1], &result);

	if (s_pFunction)
	{
		IPluginFunction *pFunction = s_pFunction;
		err = pFunction->Execute(result);
	} else if (s_pForward) {
		IForward *pForward = s_pForward;
		err = pForward->Execute(result, NULL);
	}

	ResetCall();

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

REGISTER_NATIVES(functionNatives)
{
	{"GetFunctionByName",		sm_GetFunctionByName},
	{"CreateGlobalForward",		sm_CreateGlobalForward},
	{"CreateForward",			sm_CreateForward},
	{"GetForwardFunctionCount",	sm_GetForwardFunctionCount},
	{"AddToForward",			sm_AddToForward},
	{"RemoveFromForward",		sm_RemoveFromForward},
	{"RemoveAllFromForward",	sm_RemoveAllFromForward},
	{"Call_StartFunction",		sm_CallStartFunction},
	{"Call_StartForward",		sm_CallStartForward},
	{"Call_PushCell",			sm_CallPushCell},
	{"Call_PushCellRef",		sm_CallPushCellRef},
	{"Call_PushFloat",			sm_CallPushFloat},
	{"Call_PushFloatRef",		sm_CallPushFloatRef},
	{"Call_PushArray",			sm_CallPushArray},
	{"Call_PushArrayEx",		sm_CallPushArrayEx},
	{"Call_PushString",			sm_CallPushString},
	{"Call_PushStringEx",		sm_CallPushStringEx},
	{"Call_Finish",				sm_CallFinish},
	{"Call_Cancel",				sm_CallCancel},
	{NULL,						NULL},
};
