/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod SDK Tools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC. All rights reserved.
 * ===============================================================
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#include "tempents.h"
#include "CellRecipientFilter.h"

CellRecipientFilter g_TERecFilter;
TempEntityInfo *g_CurrentTE = NULL;

static cell_t smn_TEStart(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}

	char *name;
	pContext->LocalToString(params[1], &name);

	g_CurrentTE = g_TEManager.GetTempEntityInfo(name);
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("Invalid TempEntity name: \"%s\"", name);
	}

	return 1;
}

static cell_t smn_TEWriteNum(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("No TempEntity call is in progress");
	}

	char *prop;
	pContext->LocalToString(params[1], &prop);

	if (!g_CurrentTE->TE_SetEntData(prop, params[2]))
	{
		return pContext->ThrowNativeError("Temp entity property \"%s\" not found", prop);
	}

	return 1;
}

static cell_t smn_TE_WriteFloat(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("No TempEntity call is in progress");
	}

	char *prop;
	pContext->LocalToString(params[1], &prop);

	if (!g_CurrentTE->TE_SetEntDataFloat(prop, sp_ctof(params[2])))
	{
		return pContext->ThrowNativeError("Temp entity property \"%s\" not found", prop);
	}

	return 1;
}

static cell_t smn_TEWriteVector(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("No TempEntity call is in progress");
	}

	char *prop;
	pContext->LocalToString(params[1], &prop);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);
	float vec[3] = {sp_ctof(addr[0]), sp_ctof(addr[1]), sp_ctof(addr[2])};

	if (!g_CurrentTE->TE_SetEntDataVector(prop, vec))
	{
		return pContext->ThrowNativeError("Temp entity property \"%s\" not found", prop);
	}

	return 1;
}

static cell_t smn_TEWriteFloatArray(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("No TempEntity call is in progress");
	}

	char *prop;
	pContext->LocalToString(params[1], &prop);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);

	if (!g_CurrentTE->TE_SetEntDataFloatArray(prop, addr, params[3]))
	{
		return pContext->ThrowNativeError("Temp entity property \"%s\" not found", prop);
	}

	return 1;
}

static cell_t smn_TESend(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("No TempEntity call is in progress");
	}

	cell_t *cl_array;
	pContext->LocalToPhysAddr(params[1], &cl_array);

	g_TERecFilter.Reset();
	g_TERecFilter.Initialize(cl_array, params[2]);

	g_CurrentTE->Send(g_TERecFilter, sp_ctof(params[3]));
	g_CurrentTE = NULL;

	return 1;
}

static cell_t smn_TEIsValidProp(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("No TempEntity call is in progress");
	}

	char *prop;
	pContext->LocalToString(params[1], &prop);

	return g_CurrentTE->IsValidProp(prop) ? 1 : 0;
}

sp_nativeinfo_t g_TENatives[] = 
{
	{"TE_Start",				smn_TEStart},
	{"TE_WriteNum",				smn_TEWriteNum},
	{"TE_WriteFloat",			smn_TE_WriteFloat},
	{"TE_WriteVector",			smn_TEWriteVector},
	{"TE_WriteAngles",			smn_TEWriteVector},
	{"TE_Send",					smn_TESend},
	{"TE_IsValidProp",			smn_TEIsValidProp},
	{"TE_WriteFloatArray",		smn_TEWriteFloatArray},
	{NULL,						NULL}
};
