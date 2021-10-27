/**
 * vim: set ts=4 sw=4 tw=99 noet:
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

#include <sh_list.h>
#include <sh_string.h>
#include <ISourceMod.h>
#include "common_logic.h"
#include "ShareSys.h"
#include "PluginSys.h"
#include "sprintf.h"

using namespace SourceHook;

static cell_t s_curparams[SP_MAX_EXEC_PARAMS+1];
static FakeNative *s_curnative = NULL;
static IPluginContext *s_curcaller = NULL;

cell_t FakeNativeRouter(IPluginContext *pContext, const cell_t *params, void *pData)
{
	FakeNative *native = (FakeNative *)pData;

	/* Check if too many parameters were passed */
	if (params[0] > SP_MAX_EXEC_PARAMS)
	{
		return pContext->ThrowNativeError("Called native with too many parameters (%d>%d)", params[0], SP_MAX_EXEC_PARAMS);
	}

	/* Check if the native is paused */
	if (native->ctx->GetRuntime()->IsPaused())
	{
		return pContext->ThrowNativeError("Plugin owning this native is currently paused.");
	}

	CPlugin *pCaller = g_PluginSys.GetPluginByCtx(pContext->GetContext());

	/* Save any old data on the stack */
	FakeNative *pSaveNative = s_curnative;
	IPluginContext *pSaveCaller = s_curcaller;
	cell_t save_params[SP_MAX_EXEC_PARAMS+1];
	if (pSaveNative != NULL)
	{
		/* Copy all old parameters */
		for (cell_t i=0; i<=s_curparams[0]; i++)
		{
			save_params[i] = s_curparams[i];
		}
	}

	/* Save the current parameters */
	s_curnative = native;
	s_curcaller = pContext;
	for (cell_t i=0; i<=params[0]; i++)
	{
		s_curparams[i] = params[i];
	}

	// Push info and execute. If Invoke() fails, the error will propagate up.
	// We still carry on below to clear our global state.
	cell_t result = 0;
	native->call->PushCell(pCaller->GetMyHandle());
	native->call->PushCell(params[0]);
	native->call->Invoke(&result);

	/* Restore everything from the stack if necessary */
	s_curnative = pSaveNative;
	s_curcaller = pSaveCaller;
	if (pSaveNative != NULL)
	{
		for (cell_t i=0; i<=save_params[0]; i++)
		{
			s_curparams[i] = save_params[i];
		}
	}

	return result;
}

static cell_t CreateNative(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	CPlugin *pPlugin;

	pContext->LocalToString(params[1], &name);

	IPluginFunction *pFunction = pContext->GetFunctionById(params[2]);
	if (!pFunction)
	{
		return pContext->ThrowNativeError("Failed to create native \"%s\", function %x is not a valid function", name, params[2]);
	}

	pPlugin = g_PluginSys.GetPluginByCtx(pContext->GetContext());

	if (!pPlugin->AddFakeNative(pFunction, name, FakeNativeRouter))
	{
		return pContext->ThrowNativeError("Failed to create native \"%s\", name is probably already in use", name);
	}

	return 1;
}

static cell_t ThrowNativeError(IPluginContext *pContext, const cell_t *params)
{
	if (!s_curnative || (s_curnative->ctx != pContext))
	{
		return pContext->ThrowNativeError("Not called from inside a native function");
	}

	g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);

	char buffer[512];

	{
		DetectExceptions eh(pContext);
		g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 2);
		if (eh.HasException())
			return 0;
	}

	pContext->ReportError("%s", buffer);
	return 0;
}

static cell_t GetNativeStringLength(IPluginContext *pContext, const cell_t *params)
{
	if (!s_curnative || (s_curnative->ctx != pContext))
	{
		return pContext->ThrowNativeError("Not called from inside a native function");
	}

	cell_t param = params[1];
	if (param < 1 || param > s_curparams[0])
	{
		return pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "Invalid parameter number: %d", param);
	}

	int err;
	char *str;
	if ((err=s_curcaller->LocalToString(s_curparams[param], &str)) != SP_ERROR_NONE)
	{
		return err;
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);
	*addr = (cell_t)strlen(str);

	return SP_ERROR_NONE;
}

static cell_t GetNativeString(IPluginContext *pContext, const cell_t *params)
{
	if (!s_curnative || (s_curnative->ctx != pContext))
	{
		return pContext->ThrowNativeError("Not called from inside a native function");
	}

	cell_t param = params[1];
	if (param < 1 || param > s_curparams[0])
	{
		return pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "Invalid parameter number: %d", param);
	}

	int err;
	char *str;
	if ((err=s_curcaller->LocalToString(s_curparams[param], &str)) != SP_ERROR_NONE)
	{
		return err;
	}

	size_t bytes = 0;
	pContext->StringToLocalUTF8(params[2], params[3], str, &bytes);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[4], &addr);
	*addr = (cell_t)bytes;

	return SP_ERROR_NONE;
}

static cell_t SetNativeString(IPluginContext *pContext, const cell_t *params)
{
	if (!s_curnative || (s_curnative->ctx != pContext))
	{
		return pContext->ThrowNativeError("Not called from inside a native function");
	}

	cell_t param = params[1];
	if (param < 1 || param > s_curparams[0])
	{
		return pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "Invalid parameter number: %d", param);
	}

	char *str;
	pContext->LocalToString(params[2], &str);

	int err;
	size_t bytes = 0;
	if (params[4])
	{
		err = s_curcaller->StringToLocalUTF8(s_curparams[param], params[3], str, &bytes);
	} else {
		err = s_curcaller->StringToLocal(s_curparams[param], params[3], str);
		/* Eww */
		bytes = strlen(str);
		if (bytes >= (size_t)params[3])
		{
			bytes = params[3] - 1;
		}
	}

	if (err != SP_ERROR_NONE)
	{
		return err;
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[5], &addr);
	*addr = (cell_t)bytes;

	return SP_ERROR_NONE;
}

static cell_t GetNativeCell(IPluginContext *pContext, const cell_t *params)
{
	if (!s_curnative || (s_curnative->ctx != pContext))
	{
		return pContext->ThrowNativeError("Not called from inside a native function");
	}

	cell_t param = params[1];
	if (param < 1 || param > s_curparams[0])
	{
		return pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "Invalid parameter number: %d", param);
	}

	return s_curparams[param];
}

static cell_t GetNativeCellRef(IPluginContext *pContext, const cell_t *params)
{
	if (!s_curnative || (s_curnative->ctx != pContext))
	{
		return pContext->ThrowNativeError("Not called from inside a native function");
	}

	cell_t param = params[1];
	if (param < 1 || param > s_curparams[0])
	{
		return pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "Invalid parameter number: %d", param);
	}

	cell_t *addr;
	if (s_curcaller->LocalToPhysAddr(s_curparams[param], &addr) != SP_ERROR_NONE)
	{
		return s_curcaller->ThrowNativeErrorEx(SP_ERROR_INVALID_ADDRESS, "Invalid address value");
	}

	return *addr;
}

static cell_t SetNativeCellRef(IPluginContext *pContext, const cell_t *params)
{
	if (!s_curnative || (s_curnative->ctx != pContext))
	{
		return pContext->ThrowNativeError("Not called from inside a native function");
	}

	cell_t param = params[1];
	if (param < 1 || param > s_curparams[0])
	{
		return pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "Invalid parameter number: %d", param);
	}

	cell_t *addr;
	if (s_curcaller->LocalToPhysAddr(s_curparams[param], &addr) != SP_ERROR_NONE)
	{
		return s_curcaller->ThrowNativeErrorEx(SP_ERROR_INVALID_ADDRESS, "Invalid address value");
	}

	*addr = params[2];

	return 1;
}

static cell_t GetNativeArray(IPluginContext *pContext, const cell_t *params)
{
	if (!s_curnative || (s_curnative->ctx != pContext))
	{
		return pContext->ThrowNativeError("Not called from inside a native function");
	}

	cell_t param = params[1];
	if (param < 1 || param > s_curparams[0])
	{
		return pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "Invalid parameter number: %d", param);
	}

	int err;
	cell_t *addr;
	if ((err=s_curcaller->LocalToPhysAddr(s_curparams[param], &addr)) != SP_ERROR_NONE)
	{
		return err;
	}

	cell_t *src;
	pContext->LocalToPhysAddr(params[2], &src);

	memcpy(src, addr, sizeof(cell_t) * params[3]);

	return SP_ERROR_NONE;
}

static cell_t SetNativeArray(IPluginContext *pContext, const cell_t *params)
{
	if (!s_curnative || (s_curnative->ctx != pContext))
	{
		return pContext->ThrowNativeError("Not called from inside a native function");
	}

	cell_t param = params[1];
	if (param < 1 || param > s_curparams[0])
	{
		return pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "Invalid parameter number: %d", param);
	}

	int err;
	cell_t *addr;
	if ((err=s_curcaller->LocalToPhysAddr(s_curparams[param], &addr)) != SP_ERROR_NONE)
	{
		return err;
	}

	cell_t *src;
	pContext->LocalToPhysAddr(params[2], &src);

	memcpy(addr, src, sizeof(cell_t) * params[3]);

	return SP_ERROR_NONE;
}

static cell_t FormatNativeString(IPluginContext *pContext, const cell_t *params)
{
	if (!s_curnative || (s_curnative->ctx != pContext))
	{
		return pContext->ThrowNativeError("Not called from inside a native function");
	}

	int out_param = params[1];
	int fmt_param = params[2];
	int var_param = params[3];

	/* Validate input */
	if (out_param && (out_param < 1 || out_param > s_curparams[0]))
	{
		return pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "Invalid parameter number: %d", out_param);
	}
	if (fmt_param && (fmt_param < 1 || fmt_param > s_curparams[0]))
	{
		return pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "Invalid parameter number: %d", fmt_param);
	}
	if (var_param && (var_param < 1 || var_param > s_curparams[0] + 1))
	{
		return pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "Invalid parameter number: %d", fmt_param);
	}

	/* Get buffer information */
	char *output_buffer;
	char *format_buffer;

	if (out_param)
		s_curcaller->LocalToString(s_curparams[out_param], &output_buffer);
	else
		pContext->LocalToString(params[6], &output_buffer);

	if (fmt_param)
		s_curcaller->LocalToString(s_curparams[fmt_param], &format_buffer);
	else
		pContext->LocalToString(params[7], &format_buffer);

	/* Get maximum length */
	size_t maxlen = (size_t)params[4];

	/* Do the format */
	size_t written;
	{
		DetectExceptions eh(pContext);
		written = atcprintf(output_buffer, maxlen, format_buffer, s_curcaller, s_curparams, &var_param);
		if (eh.HasException())
			return pContext->GetLastNativeError();
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[5], &addr);
	*addr = (cell_t)written;

	return SP_ERROR_NONE;
}

static cell_t IsNativeParamNullVector(IPluginContext *pContext, const cell_t *params)
{
	if (!s_curnative || (s_curnative->ctx != pContext))
	{
		return pContext->ThrowNativeError("Not called from inside a native function");
	}

	cell_t param = params[1];
	if (param < 1 || param > s_curparams[0])
	{
		return pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "Invalid parameter number: %d", param);
	}

	int err;
	cell_t *addr;
	if ((err = s_curcaller->LocalToPhysAddr(s_curparams[param], &addr)) != SP_ERROR_NONE)
	{
		return err;
	}

	cell_t *pNullVec = s_curcaller->GetNullRef(SP_NULL_VECTOR);
	if (!pNullVec)
	{
		return 0;
	}

	return addr == pNullVec ? 1 : 0;
}

static cell_t IsNativeParamNullString(IPluginContext *pContext, const cell_t *params)
{
	if (!s_curnative || (s_curnative->ctx != pContext))
	{
		return pContext->ThrowNativeError("Not called from inside a native function");
	}

	cell_t param = params[1];
	if (param < 1 || param > s_curparams[0])
	{
		return pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "Invalid parameter number: %d", param);
	}

	int err;
	char *str;
	if ((err = s_curcaller->LocalToStringNULL(s_curparams[param], &str)) != SP_ERROR_NONE)
	{
		return err;
	}

	return str == nullptr ? 1 : 0;
}

//tee hee
REGISTER_NATIVES(nativeNatives)
{
	{"CreateNative",			CreateNative},
	{"GetNativeArray",			GetNativeArray},
	{"GetNativeCell",			GetNativeCell},
	{"GetNativeCellRef",		GetNativeCellRef},
	{"GetNativeFunction",       GetNativeCell},
	{"GetNativeString",			GetNativeString},
	{"GetNativeStringLength",	GetNativeStringLength},
	{"FormatNativeString",		FormatNativeString},
	{"ThrowNativeError",		ThrowNativeError},
	{"SetNativeArray",			SetNativeArray},
	{"SetNativeCellRef",		SetNativeCellRef},
	{"SetNativeString",			SetNativeString},
	{"IsNativeParamNullVector",	IsNativeParamNullVector},
	{"IsNativeParamNullString",	IsNativeParamNullString},
	{NULL,						NULL},
};
