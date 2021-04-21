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

#include "HalfLife2.h"

static cell_t sm_GetCommandLine(IPluginContext *pCtx, const cell_t *params)
{
	ICommandLine *pCmdLine = g_HL2.GetValveCommandLine();

	if (pCmdLine == NULL)
	{
		return pCtx->ThrowNativeError("Unable to get valve command line");
	}

	const char *commandLine = pCmdLine->GetCmdLine();

	if (commandLine == NULL)
	{
		return 0;
	}

	pCtx->StringToLocal(params[1], params[2], commandLine);

	return 1;
}

static cell_t sm_GetCommandLineParam(IPluginContext *pCtx, const cell_t *params)
{
	ICommandLine *pCmdLine = g_HL2.GetValveCommandLine();

	if (pCmdLine == NULL)
	{
		return pCtx->ThrowNativeError("Unable to get valve command line");
	}

	char *param = NULL;
	char *defValue = NULL;
	pCtx->LocalToString(params[1], &param);
	pCtx->LocalToString(params[4], &defValue);

	const char *value = pCmdLine->ParmValue(param, defValue);

	pCtx->StringToLocal(params[2], params[3], value);

	return 1;
}

static cell_t sm_GetCommandLineParamInt(IPluginContext *pCtx, const cell_t *params)
{
	ICommandLine *pCmdLine = g_HL2.GetValveCommandLine();

	if (pCmdLine == NULL)
	{
		return pCtx->ThrowNativeError("Unable to get valve command line");
	}

	char *param = NULL;
	pCtx->LocalToString(params[1], &param);

	return pCmdLine->ParmValue(param, params[2]);
}

static cell_t sm_GetCommandLineParamFloat(IPluginContext *pCtx, const cell_t *params)
{
	ICommandLine *pCmdLine = g_HL2.GetValveCommandLine();

	if (pCmdLine == NULL)
	{
		return pCtx->ThrowNativeError("Unable to get valve command line");
	}

	char *param = NULL;
	pCtx->LocalToString(params[1], &param);

	float value = pCmdLine->ParmValue(param, sp_ctof(params[2]));

	return sp_ftoc(value);
}

static cell_t sm_FindCommandLineParam(IPluginContext *pCtx, const cell_t *params)
{
	ICommandLine *pCmdLine = g_HL2.GetValveCommandLine();

	if (pCmdLine == NULL)
	{
		return pCtx->ThrowNativeError("Unable to get valve command line");
	}

	char *param = NULL;
	pCtx->LocalToString(params[1], &param);

	return pCmdLine->FindParm(param);
}

REGISTER_NATIVES(commandlinenatives)
{
	{"GetCommandLine",				sm_GetCommandLine},
	{"GetCommandLineParam",			sm_GetCommandLineParam},
	{"GetCommandLineParamInt",		sm_GetCommandLineParamInt},
	{"GetCommandLineParamFloat",	sm_GetCommandLineParamFloat},
	{"FindCommandLineParam",		sm_FindCommandLineParam},
	{NULL,							NULL},
};
