/**
 * vim: set ts=4 :
 * ================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
 * ================================================================
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License, 
 * version 3.0, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to 
 * link the code of this program (as well as its derivative works) to 
 * "Half-Life 2," the "Source Engine," the "SourcePawn JIT," and any 
 * Game MODs that run on software by the Valve Corporation.  You must 
 * obey the GNU General Public License in all respects for all other 
 * code used. Additionally, AlliedModders LLC grants this exception 
 * to all derivative works. AlliedModders LLC defines further 
 * exceptions, found in LICENSE.txt (as of this writing, version 
 * JULY-31-2007), or <http://www.sourcemod.net/license.php>.
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
