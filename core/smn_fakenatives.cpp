/**
 * vim: set ts=4 :
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

#include <sh_list.h>
#include <sh_string.h>
#include "sm_trie.h"
#include "sm_globals.h"
#include "PluginSys.h"
#include "sourcemod.h"
#include "sm_stringutil.h"

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
		return pContext->ThrowNativeError("Called native with too many parameters (%d>%d)", params[9], SP_MAX_EXEC_PARAMS);
	}

	/* Check if the native is paused */
	sp_context_t *pNativeCtx = native->ctx->GetContext();
	if ((pNativeCtx->flags & SPFLAG_PLUGIN_PAUSED) == SPFLAG_PLUGIN_PAUSED)
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

	/* Push info and execute. */
	cell_t result = 0;
	native->call->PushCell(pCaller->GetMyHandle());
	native->call->PushCell(params[0]);
	int error;
	if ((error=native->call->Execute(&result)) != SP_ERROR_NONE)
	{
		if (pContext->GetContext()->n_err == SP_ERROR_NONE)
		{
			pContext->ThrowNativeErrorEx(error, "Error encountered while processing a dynamic native");
		}
	}

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
	pContext->LocalToString(params[1], &name);

	IPluginFunction *pFunction = pContext->GetFunctionById(params[2]);
	if (!pFunction)
	{
		return pContext->ThrowNativeError("Function %x is not a valid function", params[2]);
	}

	if (!g_PluginSys.AddFakeNative(pFunction, name, FakeNativeRouter))
	{
		return pContext->ThrowNativeError("Fatal error creating dynamic native!");
	}

	return 1;
}

static cell_t ThrowNativeError(IPluginContext *pContext, const cell_t *params)
{
	if (!s_curnative || (s_curnative->ctx != pContext))
	{
		return pContext->ThrowNativeError("Not called from inside a native function");
	}

	g_SourceMod.SetGlobalTarget(LANG_SERVER);

	char buffer[512];

	g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, 2);

	if (pContext->GetContext()->n_err != SP_ERROR_NONE)
	{
		s_curcaller->ThrowNativeError("Error encountered while processing a dynamic native");
	} else {
		s_curcaller->ThrowNativeErrorEx(params[1], "%s", buffer);
	}

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
	int err;
	char *output_buffer;
	char *format_buffer;

	if (out_param)
	{
		if ((err=s_curcaller->LocalToString(s_curparams[out_param], &output_buffer)) != SP_ERROR_NONE)
		{
			return err;
		}
	} else {
		pContext->LocalToString(params[6], &output_buffer);
	}

	if (fmt_param)
	{
		if ((err=s_curcaller->LocalToString(s_curparams[fmt_param], &format_buffer)) != SP_ERROR_NONE)
		{
			return err;
		}
	} else {
		pContext->LocalToString(params[7], &format_buffer);
	}

	/* Get maximum length */
	size_t maxlen = (size_t)params[4];

	/* Do the format */
	size_t written = atcprintf(output_buffer, maxlen, format_buffer, s_curcaller, s_curparams, &var_param);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[5], &addr);
	*addr = (cell_t)written;

	return s_curcaller->GetContext()->n_err;
}

//tee hee
REGISTER_NATIVES(nativeNatives)
{
	{"CreateNative",			CreateNative},
	{"GetNativeArray",			GetNativeArray},
	{"GetNativeCell",			GetNativeCell},
	{"GetNativeCellRef",		GetNativeCellRef},
	{"GetNativeString",			GetNativeString},
	{"GetNativeStringLength",	GetNativeStringLength},
	{"FormatNativeString",		FormatNativeString},
	{"ThrowNativeError",		ThrowNativeError},
	{"SetNativeArray",			SetNativeArray},
	{"SetNativeCellRef",		SetNativeCellRef},
	{"SetNativeString",			SetNativeString},
	{NULL,						NULL},
};
