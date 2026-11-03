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

// For natives created with NativeFlag_ReturnsInt64 or NativeFlag_ReturnsAddress: the caller's hidden int64 return
// slot, if it passed one, and the value set with SetNativeReturnInt64 or SetNativeReturnAddress.
static cell_t s_curreturnslot = 0;
static bool s_curreturnset = false;
static int64_t s_curreturn = 0;

// Whether a plugin was compiled with Address as an int64, which passes int64 arguments by reference and int64 return
// values through a hidden first argument.
static bool UsesInt64Address(IPluginContext *pContext)
{
	return pContext->GetRuntime()->FindPubvarByName("__Int64_Address__", nullptr) == SP_ERROR_NONE;
}

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

	CPlugin *pCaller = g_PluginSys.GetPluginByCtx(pContext);

	// A caller compiled with Address as an int64 passes a hidden slot for an int64 return value as its first argument.
	// Callers compiled before SourceMod 1.13 take a returned Address as one cell, but have no equivalent for any other
	// int64.
	bool returns_int64 = (native->flags & (NATIVEFLAG_RETURNS_INT64 | NATIVEFLAG_RETURNS_ADDRESS)) != 0;
	cell_t offset = 0;
	if (returns_int64)
	{
		if (UsesInt64Address(pContext))
		{
			offset = 1;
		}
		else if (!(native->flags & NATIVEFLAG_RETURNS_ADDRESS))
		{
			return pContext->ThrowNativeError("Plugin \"%s\" was compiled before SourceMod 1.13 and cannot call native \"%s\", which returns an int64",
				pCaller->GetFilename(), native->name.c_str());
		}
	}
	if (params[0] < offset)
	{
		return pContext->ThrowNativeError("Called native without its return value slot");
	}

	/* Save any old data on the stack */
	FakeNative *pSaveNative = s_curnative;
	IPluginContext *pSaveCaller = s_curcaller;
	cell_t save_returnslot = s_curreturnslot;
	bool save_returnset = s_curreturnset;
	int64_t save_return = s_curreturn;
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
	s_curparams[0] = params[0] - offset;
	for (cell_t i=1; i<=s_curparams[0]; i++)
	{
		s_curparams[i] = params[i + offset];
	}
	s_curreturnslot = offset ? params[1] : 0;
	s_curreturnset = false;
	s_curreturn = 0;

	// Push info and execute. If Invoke() fails, the error will propagate up.
	// We still carry on below to clear our global state.
	cell_t result = 0;
	native->call->PushCell(pCaller->GetMyHandle());
	native->call->PushCell(s_curparams[0]);
	native->call->Invoke(&result);

	if (returns_int64)
	{
		// Without SetNativeReturnInt64 or SetNativeReturnAddress, the returned cell is widened the way SourceMod widens
		// pointers.
		int64_t value = s_curreturnset ? s_curreturn : static_cast<int64_t>(static_cast<ucell_t>(result));
		cell_t *slot;
		if (s_curreturnslot && pContext->LocalToPhysAddr(s_curreturnslot, &slot) == SP_ERROR_NONE)
		{
			memcpy(slot, &value, sizeof(value));
		}
		result = static_cast<cell_t>(value);
	}

	/* Restore everything from the stack if necessary */
	s_curnative = pSaveNative;
	s_curcaller = pSaveCaller;
	s_curreturnslot = save_returnslot;
	s_curreturnset = save_returnset;
	s_curreturn = save_return;
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

	pPlugin = g_PluginSys.GetPluginByCtx(pContext);

	int flags = (params[0] >= 3) ? params[3] : 0;
	if (!pPlugin->AddFakeNative(pFunction, name, FakeNativeRouter, flags))
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

// Whether the plugin calling the current native takes int64 values as int64s. Plugins compiled before SourceMod 1.13
// take an Address as one cell, but have no equivalent for any other int64, so for those this throws an error.
static bool CallerTakesInt64(IPluginContext *pContext, bool address, bool *as_int64)
{
	*as_int64 = UsesInt64Address(s_curcaller);
	if (*as_int64 || address)
	{
		return true;
	}

	CPlugin *pCaller = g_PluginSys.GetPluginByCtx(s_curcaller);
	pContext->ThrowNativeError("Plugin \"%s\" was compiled before SourceMod 1.13 and cannot pass or receive int64 values of native \"%s\"",
		pCaller->GetFilename(), s_curnative->name.c_str());
	return false;
}

static cell_t GetNativeInt64Value(IPluginContext *pContext, const cell_t *params, bool address)
{
	if (!s_curnative || (s_curnative->ctx != pContext))
	{
		return pContext->ThrowNativeError("Not called from inside a native function");
	}

	// This native returns an int64, so a provider compiled with Address as an int64 passes a hidden return slot first.
	bool provider_int64 = UsesInt64Address(pContext);
	cell_t param = params[provider_int64 ? 2 : 1];
	if (param < 1 || param > s_curparams[0])
	{
		return pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "Invalid parameter number: %d", param);
	}

	bool as_int64;
	if (!CallerTakesInt64(pContext, address, &as_int64))
	{
		return 0;
	}

	int64_t value;
	if (as_int64)
	{
		// int64 arguments are passed by reference.
		cell_t *addr;
		if (s_curcaller->LocalToPhysAddr(s_curparams[param], &addr) != SP_ERROR_NONE)
		{
			return s_curcaller->ThrowNativeErrorEx(SP_ERROR_INVALID_ADDRESS, "Invalid address value");
		}
		memcpy(&value, addr, sizeof(value));
	}
	else
	{
		value = static_cast<int64_t>(static_cast<ucell_t>(s_curparams[param]));
	}

	if (provider_int64)
	{
		cell_t *slot;
		if (int err = pContext->LocalToPhysAddr(params[1], &slot); err != SP_ERROR_NONE)
		{
			return pContext->ThrowNativeErrorEx(err, nullptr);
		}
		memcpy(slot, &value, sizeof(value));
	}
	return static_cast<cell_t>(value);
}

static cell_t GetNativeInt64(IPluginContext *pContext, const cell_t *params)
{
	return GetNativeInt64Value(pContext, params, false);
}

static cell_t GetNativeAddress(IPluginContext *pContext, const cell_t *params)
{
	return GetNativeInt64Value(pContext, params, true);
}

// Reads an int64 argument of the native that is calling the current native.
static bool ReadInt64Argument(IPluginContext *pContext, cell_t arg, int64_t *value)
{
	if (UsesInt64Address(pContext))
	{
		cell_t *addr;
		if (int err = pContext->LocalToPhysAddr(arg, &addr); err != SP_ERROR_NONE)
		{
			pContext->ThrowNativeErrorEx(err, nullptr);
			return false;
		}
		memcpy(value, addr, sizeof(*value));
	}
	else
	{
		*value = static_cast<int64_t>(static_cast<ucell_t>(arg));
	}
	return true;
}

static cell_t SetNativeInt64RefValue(IPluginContext *pContext, const cell_t *params, bool address)
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

	int64_t value;
	if (!ReadInt64Argument(pContext, params[2], &value))
	{
		return 0;
	}

	bool as_int64;
	if (!CallerTakesInt64(pContext, address, &as_int64))
	{
		return 0;
	}

	cell_t *addr;
	if (s_curcaller->LocalToPhysAddr(s_curparams[param], &addr) != SP_ERROR_NONE)
	{
		return s_curcaller->ThrowNativeErrorEx(SP_ERROR_INVALID_ADDRESS, "Invalid address value");
	}

	if (as_int64)
	{
		memcpy(addr, &value, sizeof(value));
	}
	else
	{
		// The caller's reference holds a single cell.
		*addr = static_cast<cell_t>(value);
	}
	return 1;
}

static cell_t SetNativeInt64Ref(IPluginContext *pContext, const cell_t *params)
{
	return SetNativeInt64RefValue(pContext, params, false);
}

static cell_t SetNativeAddressRef(IPluginContext *pContext, const cell_t *params)
{
	return SetNativeInt64RefValue(pContext, params, true);
}

static cell_t GetNativeInt64ArrayValues(IPluginContext *pContext, const cell_t *params, bool address)
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

	bool as_int64;
	if (!CallerTakesInt64(pContext, address, &as_int64))
	{
		return 0;
	}

	cell_t *addr;
	if (int err = s_curcaller->LocalToPhysAddr(s_curparams[param], &addr); err != SP_ERROR_NONE)
	{
		return err;
	}

	int64_t *dest;
	if (int err = pContext->LocalToPhysAddr(params[2], reinterpret_cast<cell_t **>(&dest)); err != SP_ERROR_NONE)
	{
		return pContext->ThrowNativeErrorEx(err, nullptr);
	}

	cell_t size = params[3];
	if (as_int64)
	{
		memcpy(dest, addr, sizeof(int64_t) * size);
	}
	else
	{
		// The caller's array holds single cells.
		for (cell_t i = 0; i < size; i++)
			dest[i] = static_cast<int64_t>(static_cast<ucell_t>(addr[i]));
	}

	return SP_ERROR_NONE;
}

static cell_t GetNativeInt64Array(IPluginContext *pContext, const cell_t *params)
{
	return GetNativeInt64ArrayValues(pContext, params, false);
}

static cell_t GetNativeAddressArray(IPluginContext *pContext, const cell_t *params)
{
	return GetNativeInt64ArrayValues(pContext, params, true);
}

static cell_t SetNativeInt64ArrayValues(IPluginContext *pContext, const cell_t *params, bool address)
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

	bool as_int64;
	if (!CallerTakesInt64(pContext, address, &as_int64))
	{
		return 0;
	}

	cell_t *addr;
	if (int err = s_curcaller->LocalToPhysAddr(s_curparams[param], &addr); err != SP_ERROR_NONE)
	{
		return err;
	}

	int64_t *src;
	if (int err = pContext->LocalToPhysAddr(params[2], reinterpret_cast<cell_t **>(&src)); err != SP_ERROR_NONE)
	{
		return pContext->ThrowNativeErrorEx(err, nullptr);
	}

	cell_t size = params[3];
	if (as_int64)
	{
		memcpy(addr, src, sizeof(int64_t) * size);
	}
	else
	{
		// The caller's array holds single cells.
		for (cell_t i = 0; i < size; i++)
			addr[i] = static_cast<cell_t>(src[i]);
	}

	return SP_ERROR_NONE;
}

static cell_t SetNativeInt64Array(IPluginContext *pContext, const cell_t *params)
{
	return SetNativeInt64ArrayValues(pContext, params, false);
}

static cell_t SetNativeAddressArray(IPluginContext *pContext, const cell_t *params)
{
	return SetNativeInt64ArrayValues(pContext, params, true);
}

static cell_t SetNativeReturnInt64Value(IPluginContext *pContext, const cell_t *params, int flag, const char *flag_name)
{
	if (!s_curnative || (s_curnative->ctx != pContext))
	{
		return pContext->ThrowNativeError("Not called from inside a native function");
	}

	if (!(s_curnative->flags & flag))
	{
		return pContext->ThrowNativeError("Native \"%s\" was not created with %s", s_curnative->name.c_str(), flag_name);
	}

	if (!ReadInt64Argument(pContext, params[1], &s_curreturn))
	{
		return 0;
	}
	s_curreturnset = true;
	// Returns 0 so a native can return the result of this call.
	return 0;
}

static cell_t SetNativeReturnInt64(IPluginContext *pContext, const cell_t *params)
{
	return SetNativeReturnInt64Value(pContext, params, NATIVEFLAG_RETURNS_INT64, "NativeFlag_ReturnsInt64");
}

static cell_t SetNativeReturnAddress(IPluginContext *pContext, const cell_t *params)
{
	return SetNativeReturnInt64Value(pContext, params, NATIVEFLAG_RETURNS_ADDRESS, "NativeFlag_ReturnsAddress");
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

static cell_t GetNativeFunction(IPluginContext *pContext, const cell_t *params)
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
	
	cell_t funcid = s_curparams[param];
	if (s_curcaller->IsNullFunctionId(funcid))
	{
		// see alliedmodders/sourcepawn#912, alliedmodders/sourcemod#2068
		// convert null function to receiver's expected value so equality checks against INVALID_FUNCTION pass
		return pContext->GetNullFunctionValue();
	}
	else if (funcid <= 0)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", funcid);
	}
	return funcid;
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
	{"GetNativeInt64",			GetNativeInt64},
	{"GetNativeAddress",		GetNativeAddress},
	{"GetNativeInt64Array",		GetNativeInt64Array},
	{"GetNativeAddressArray",	GetNativeAddressArray},
	{"GetNativeFunction",       GetNativeFunction},
	{"GetNativeString",			GetNativeString},
	{"GetNativeStringLength",	GetNativeStringLength},
	{"FormatNativeString",		FormatNativeString},
	{"ThrowNativeError",		ThrowNativeError},
	{"SetNativeArray",			SetNativeArray},
	{"SetNativeCellRef",		SetNativeCellRef},
	{"SetNativeInt64Ref",		SetNativeInt64Ref},
	{"SetNativeAddressRef",		SetNativeAddressRef},
	{"SetNativeInt64Array",		SetNativeInt64Array},
	{"SetNativeAddressArray",	SetNativeAddressArray},
	{"SetNativeReturnInt64",	SetNativeReturnInt64},
	{"SetNativeReturnAddress",	SetNativeReturnAddress},
	{"SetNativeString",			SetNativeString},
	{"IsNativeParamNullVector",	IsNativeParamNullVector},
	{"IsNativeParamNullString",	IsNativeParamNullString},
	{NULL,						NULL},
};
