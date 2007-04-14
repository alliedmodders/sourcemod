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

#include "sm_globals.h"
#include "HalfLife2.h"
#include "sourcemm_api.h"
#include "HandleSys.h"
#include "ConVarManager.h"
#include "ConCmdManager.h"
#include "PluginSys.h"
#include "sm_stringutil.h"
#include "PlayerManager.h"

static cell_t sm_CreateConVar(IPluginContext *pContext, const cell_t *params)
{
	char *name, *defaultVal, *helpText;

	pContext->LocalToString(params[1], &name);

	// While the engine seems to accept a blank convar name, it causes a crash upon server quit
	if (name == NULL || strcmp(name, "") == 0)
	{
		return pContext->ThrowNativeError("Convar with blank name is not permitted");
	}

	pContext->LocalToString(params[2], &defaultVal);
	pContext->LocalToString(params[3], &helpText);

	bool hasMin = params[5] ? true : false;
	bool hasMax = params[7] ? true : false;
	float min = sp_ctof(params[6]);
	float max = sp_ctof(params[8]);

	Handle_t hndl = g_ConVarManager.CreateConVar(pContext, name, defaultVal, helpText, params[4], hasMin, min, hasMax, max);

	if (hndl == BAD_HANDLE)
	{
		return pContext->ThrowNativeError("Convar \"%s\" was not created. A console command with the same name already exists.", name);
	}

	return hndl;
}

static cell_t sm_FindConVar(IPluginContext *pContext, const cell_t *params)
{
	char *name;

	pContext->LocalToString(params[1], &name);

	return g_ConVarManager.FindConVar(name);
}

static cell_t sm_HookConVarChange(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_HandleSys.ReadHandle(hndl, g_ConVarManager.GetHandleType(), NULL, (void **)&pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	IPluginFunction *pFunction = pContext->GetFunctionById(params[2]);

	if (!pFunction)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[2]);
	}

	g_ConVarManager.HookConVarChange(pConVar, pFunction);

	return 1;
}

static cell_t sm_UnhookConVarChange(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_HandleSys.ReadHandle(hndl, g_ConVarManager.GetHandleType(), NULL, (void **)&pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	IPluginFunction *pFunction = pContext->GetFunctionById(params[2]);

	if (!pFunction)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[2]);
	}

	g_ConVarManager.UnhookConVarChange(pConVar, pFunction);

	return 1;
}

static cell_t sm_GetConVarBool(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_HandleSys.ReadHandle(hndl, g_ConVarManager.GetHandleType(), NULL, (void **)&pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	return pConVar->GetBool();
}

static cell_t sm_GetConVarInt(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_HandleSys.ReadHandle(hndl, g_ConVarManager.GetHandleType(), NULL, (void **)&pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	return pConVar->GetInt();
}

/* This handles both SetConVarBool() and SetConVarInt() */
static cell_t sm_SetConVarNum(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_HandleSys.ReadHandle(hndl, g_ConVarManager.GetHandleType(), NULL, (void **)&pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	pConVar->SetValue(params[2]);

	return 1;
}

static cell_t sm_GetConVarFloat(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_HandleSys.ReadHandle(hndl, g_ConVarManager.GetHandleType(), NULL, (void **)&pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}
	
	float value = pConVar->GetFloat();

	return sp_ftoc(value);
}

static cell_t sm_SetConVarFloat(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_HandleSys.ReadHandle(hndl, g_ConVarManager.GetHandleType(), NULL, (void **)&pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	float value = sp_ctof(params[2]);
	pConVar->SetValue(value);

	return 1;
}

static cell_t sm_GetConVarString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_HandleSys.ReadHandle(hndl, g_ConVarManager.GetHandleType(), NULL, (void **)&pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	pContext->StringToLocalUTF8(params[2], params[3], pConVar->GetString(), NULL);

	return 1;
}

static cell_t sm_SetConVarString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_HandleSys.ReadHandle(hndl, g_ConVarManager.GetHandleType(), NULL, (void **)&pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	char *value;
	pContext->LocalToString(params[2], &value);

	pConVar->SetValue(value);

	return 1;
}

static cell_t sm_GetConVarFlags(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_HandleSys.ReadHandle(hndl, g_ConVarManager.GetHandleType(), NULL, (void **)&pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	return pConVar->GetFlags();
}

static cell_t sm_SetConVarFlags(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_HandleSys.ReadHandle(hndl, g_ConVarManager.GetHandleType(), NULL, (void **)&pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	pConVar->SetFlags(params[2]);

	return 1;
}

static cell_t sm_GetConVarMin(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_HandleSys.ReadHandle(hndl, g_ConVarManager.GetHandleType(), NULL, (void **)&pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	cell_t *addr;
	bool hasMin;
	float min;

	pContext->LocalToPhysAddr(params[2], &addr);

	hasMin = pConVar->GetMin(min);
	*addr = sp_ftoc(min);

	return hasMin;
}

static cell_t sm_GetConVarMax(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_HandleSys.ReadHandle(hndl, g_ConVarManager.GetHandleType(), NULL, (void **)&pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	cell_t *addr;
	bool hasMax;
	float max;

	pContext->LocalToPhysAddr(params[2], &addr);

	hasMax = pConVar->GetMax(max);
	*addr = sp_ftoc(max);

	return hasMax;
}

static cell_t sm_GetConVarName(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_HandleSys.ReadHandle(hndl, g_ConVarManager.GetHandleType(), NULL, (void **)&pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	pContext->StringToLocalUTF8(params[2], params[3], pConVar->GetName(), NULL);

	return 1;
}

static cell_t sm_ResetConVar(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_HandleSys.ReadHandle(hndl, g_ConVarManager.GetHandleType(), NULL, (void **)&pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	pConVar->Revert();

	return 1;
}

static bool s_QueryAlreadyWarned = false;

static cell_t sm_QueryClientConVar(IPluginContext *pContext, const cell_t *params)
{
	CPlayer *pPlayer;
	char *name;
	IPluginFunction *pCallback;

	if (g_IsOriginalEngine)
	{
		if (!s_QueryAlreadyWarned)
		{
			s_QueryAlreadyWarned = true;
			return pContext->ThrowNativeError("Game does not support client convar querying (one time warning)");
		}

		return 0;
	}

	pPlayer = g_Players.GetPlayerByIndex(params[1]);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Player %d is not a valid player", params[1]);
	}

	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Player %d is not connected", params[1]);
	}

	/* Trying a query on a bot results in callback not be fired, so don't bother */
	if (pPlayer->IsFakeClient())
	{
		return 0;
	}

	pContext->LocalToString(params[2], &name);
	pCallback = pContext->GetFunctionById(params[3]);

	if (!pCallback)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[3]);
	}

	return g_ConVarManager.QueryClientConVar(pPlayer->GetEdict(), name, pCallback, params[4]);
}

static cell_t sm_RegServerCmd(IPluginContext *pContext, const cell_t *params)
{
	char *name,*help;
	IPluginFunction *pFunction;

	pContext->LocalToString(params[1], &name);
	pContext->LocalToString(params[3], &help);
	pFunction = pContext->GetFunctionById(params[2]);

	if (!pFunction)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[2]);
	}

	if (!g_ConCmds.AddServerCommand(pFunction, name, help, params[4]))
	{
		return pContext->ThrowNativeError("Command \"%s\" could not be created. A convar with the same name already exists.", name);
	}

	return 1;
}

static cell_t sm_RegConsoleCmd(IPluginContext *pContext, const cell_t *params)
{
	char *name,*help;
	IPluginFunction *pFunction;

	pContext->LocalToString(params[1], &name);
	pContext->LocalToString(params[3], &help);
	pFunction = pContext->GetFunctionById(params[2]);

	if (!pFunction)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[2]);
	}

	if (!g_ConCmds.AddConsoleCommand(pFunction, name, help, params[4]))
	{
		return pContext->ThrowNativeError("Command \"%s\" could not be created. A convar with the same name already exists.", name);
	}

	return 1;
}

static cell_t sm_RegAdminCmd(IPluginContext *pContext, const cell_t *params)
{
	char *name,*help;
	const char *group;
	IPluginFunction *pFunction;
	FlagBits flags = params[3];
	int cmdflags = params[6];

	pContext->LocalToString(params[1], &name);
	pContext->LocalToString(params[4], &help);
	pContext->LocalToString(params[5], (char **)&group);
	pFunction = pContext->GetFunctionById(params[2]);

	if (group[0] == '\0')
	{
		CPlugin *pPlugin = g_PluginSys.GetPluginByCtx(pContext->GetContext());
		group = pPlugin->GetFilename();
	}

	if (!pFunction)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[2]);
	}

	if (!g_ConCmds.AddAdminCommand(pFunction, name, group, flags, help, cmdflags))
	{
		return pContext->ThrowNativeError("Command \"%s\" could not be created. A convar with the same name already exists.", name);
	}

	return 1;
}

static cell_t sm_GetCmdArgs(IPluginContext *pContext, const cell_t *params)
{
	return engine->Cmd_Argc() - 1;
}

static cell_t sm_GetCmdArg(IPluginContext *pContext, const cell_t *params)
{
	const char *arg = engine->Cmd_Argv(params[1]);
	
	pContext->StringToLocalUTF8(params[2], params[3], arg, NULL);

	return 1;
}

static cell_t sm_GetCmdArgString(IPluginContext *pContext, const cell_t *params)
{
	const char *args = engine->Cmd_Args();

	if (!args)
	{
		args = "";
	}

	pContext->StringToLocalUTF8(params[1], params[2], args, NULL);

	return 1;
}

static cell_t sm_PrintToServer(IPluginContext *pCtx, const cell_t *params)
{
	char buffer[1024];
	char *fmt;
	int arg = 2;

	pCtx->LocalToString(params[1], &fmt);
	size_t res = atcprintf(buffer, sizeof(buffer)-2, fmt, pCtx, params, &arg);

	buffer[res++] = '\n';
	buffer[res] = '\0';

	META_CONPRINT(buffer);

	return 1;
}

static cell_t sm_PrintToConsole(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 0) || (index > g_Players.GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Invalid client index %d", index);
	}

	CPlayer *pPlayer = NULL;
	if (index != 0)
	{
		pPlayer = g_Players.GetPlayerByIndex(index);
		if (!pPlayer->IsInGame())
		{
			return pCtx->ThrowNativeError("Client %d is not in game", index);
		}
		
		/* Silent fail on bots, engine will crash */
		if (pPlayer->IsFakeClient())
		{
			return 0;
		}
	}

	char buffer[1024];
	char *fmt;
	int arg = 3;

	pCtx->LocalToString(params[2], &fmt);
	size_t res = atcprintf(buffer, sizeof(buffer)-2, fmt, pCtx, params, &arg);

	buffer[res++] = '\n';
	buffer[res] = '\0';

	if (index != 0)
	{
		engine->ClientPrintf(pPlayer->GetEdict(), buffer);
	} else {
		META_CONPRINT(buffer);
	}

	return 1;
}

static cell_t sm_ServerCommand(IPluginContext *pContext, const cell_t *params)
{
	char buffer[1024];
	size_t len = g_SourceMod.FormatString(buffer, sizeof(buffer)-2, pContext, params, 1);

	/* One byte for null terminator, one for newline */
	buffer[len++] = '\n';
	buffer[len] = '\0';

	engine->ServerCommand(buffer);

	return 1;
}

static cell_t sm_InsertServerCommand(IPluginContext *pContext, const cell_t *params)
{
	char buffer[1024];
	size_t len = g_SourceMod.FormatString(buffer, sizeof(buffer)-2, pContext, params, 1);

	/* One byte for null terminator, one for newline */
	buffer[len++] = '\n';
	buffer[len] = '\0';

	engine->InsertServerCommand(buffer);

	return 1;
}

static cell_t sm_ServerExecute(IPluginContext *pContext, const cell_t *params)
{
	engine->ServerExecute();

	return 1;
}

static cell_t sm_ClientCommand(IPluginContext *pContext, const cell_t *params)
{
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(params[1]);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Player %d is not a valid player", params[1]);
	}

	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Player %d is not connected", params[1]);
	}

	char buffer[256];
	g_SourceMod.FormatString(buffer, sizeof(buffer)-1, pContext, params, 2);

	engine->ClientCommand(pPlayer->GetEdict(), "%s", buffer);

	return 1;
}

REGISTER_NATIVES(consoleNatives)
{
	{"CreateConVar",		sm_CreateConVar},
	{"FindConVar",			sm_FindConVar},
	{"HookConVarChange",	sm_HookConVarChange},
	{"UnhookConVarChange",	sm_UnhookConVarChange},
	{"GetConVarBool",		sm_GetConVarBool},
	{"SetConVarBool",		sm_SetConVarNum},
	{"GetConVarInt",		sm_GetConVarInt},
	{"SetConVarInt",		sm_SetConVarNum},
	{"GetConVarFloat",		sm_GetConVarFloat},
	{"SetConVarFloat",		sm_SetConVarFloat},
	{"GetConVarString",		sm_GetConVarString},
	{"SetConVarString",		sm_SetConVarString},
	{"GetConVarFlags",		sm_GetConVarFlags},
	{"SetConVarFlags",		sm_SetConVarFlags},
	{"GetConVarName",		sm_GetConVarName},
	{"GetConVarMin",		sm_GetConVarMin},
	{"GetConVarMax",		sm_GetConVarMax},
	{"ResetConVar",			sm_ResetConVar},
	{"QueryClientConVar",	sm_QueryClientConVar},
	{"RegServerCmd",		sm_RegServerCmd},
	{"RegConsoleCmd",		sm_RegConsoleCmd},
	{"GetCmdArgString",		sm_GetCmdArgString},
	{"GetCmdArgs",			sm_GetCmdArgs},
	{"GetCmdArg",			sm_GetCmdArg},
	{"PrintToServer",		sm_PrintToServer},
	{"PrintToConsole",		sm_PrintToConsole},
	{"RegAdminCmd",			sm_RegAdminCmd},
	{"ServerCommand",		sm_ServerCommand},
	{"InsertServerCommand",	sm_InsertServerCommand},
	{"ServerExecute",		sm_ServerExecute},
	{"ClientCommand",		sm_ClientCommand},
	{NULL,					NULL}
};
