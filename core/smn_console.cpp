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

#include "sm_globals.h"
#include "HalfLife2.h"
#include "sourcemm_api.h"
#include "HandleSys.h"
#include "ConVarManager.h"
#include "ConCmdManager.h"
#include "PluginSys.h"
#include "sm_stringutil.h"
#include "PlayerManager.h"
#include "ChatTriggers.h"
#include "AdminCache.h"
#include <inetchannel.h>
#include <bitbuf.h>
#include <sm_trie_tpl.h>

#if SOURCE_ENGINE == SE_LEFT4DEAD
#define NET_SETCONVAR	6
#else
#define NET_SETCONVAR	5
#endif

enum ConVarBounds
{
	ConVarBound_Upper = 0,
	ConVarBound_Lower
};

HandleType_t hCmdIterType = 0;
HandleType_t htConCmdIter = 0;

struct GlobCmdIter
{
	bool started;
	List<ConCmdInfo *>::iterator iter;
};

struct ConCmdIter
{
#if SOURCE_ENGINE == SE_LEFT4DEAD
	ICvarIteratorInternal *pLast;
#else
	const ConCommandBase *pLast;
#endif
};

class ConsoleHelpers : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public:
	virtual void OnSourceModAllInitialized()
	{
		HandleAccess access;

		g_HandleSys.InitAccessDefaults(NULL, &access);

		htConCmdIter = g_HandleSys.CreateType("ConCmdIter", this, 0, NULL, &access, g_pCoreIdent, NULL);

		access.access[HandleAccess_Clone] = HANDLE_RESTRICT_OWNER | HANDLE_RESTRICT_IDENTITY;

		hCmdIterType = g_HandleSys.CreateType("CmdIter", this, 0, NULL, &access, g_pCoreIdent, NULL);
	}
	virtual void OnHandleDestroy(HandleType_t type, void *object)
	{
		if (type == hCmdIterType)
		{
			GlobCmdIter *iter = (GlobCmdIter *)object;
			delete iter;
		}
		else if (type == htConCmdIter)
		{
			ConCmdIter *iter = (ConCmdIter * )object;
#if SOURCE_ENGINE == SE_LEFT4DEAD
			// ICvarIteratorInternal has no virtual destructor
			g_pMemAlloc->Free(iter->pLast);
#endif
			delete iter;
		}
	}
	virtual bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
	{
		if (type == htConCmdIter)
		{
			*pSize = sizeof(ConCmdIter);
			return true;
		}
		else if (type == hCmdIterType)
		{
			*pSize = sizeof(GlobCmdIter);
			return true;
		}
		return false;
	}
} s_ConsoleHelpers;

class CommandFlagsHelper : public IConCommandTracker
{
public:
	void OnUnlinkConCommandBase(ConCommandBase *pBase, const char *name, bool is_read_safe)
	{
		m_CmdFlags.remove(name);
	}
	bool GetFlags(const char *name, int *flags)
	{
		ConCommandBase **ppCmd;
		ConCommandBase *pCmd;
		if ((ppCmd=m_CmdFlags.retrieve(name)))
		{
			TrackConCommandBase((*ppCmd), this);
			*flags = (*ppCmd)->GetFlags();
			return true;
		}
		else if ((pCmd=FindCommandBase(name)))
		{
			m_CmdFlags.insert(name, pCmd);
			TrackConCommandBase(pCmd, this);
			*flags = pCmd->GetFlags();
			return true;
		}
		else
		{
			return false;
		}
	}
	bool SetFlags(const char *name, int flags)
	{
		ConCommandBase **ppCmd;
		ConCommandBase *pCmd;
		if ((ppCmd=m_CmdFlags.retrieve(name)))
		{
			(*ppCmd)->SetFlags(flags);
			TrackConCommandBase((*ppCmd), this);
			return true;
		}
		else if ((pCmd=FindCommandBase(name)))
		{
			m_CmdFlags.insert(name, pCmd);
			pCmd->SetFlags(flags);
			TrackConCommandBase(pCmd, this);
			return true;
		}
		else
		{
			return false;
		}
	}
private:
	KTrie<ConCommandBase *> m_CmdFlags;
} s_CommandFlagsHelper;

#if SOURCE_ENGINE >= SE_ORANGEBOX
static void ReplicateConVar(ConVar *pConVar)
{
	int maxClients = g_Players.GetMaxClients();

	char data[256];
	bf_write buffer(data, sizeof(data));

	buffer.WriteUBitLong(NET_SETCONVAR, 5);
	buffer.WriteByte(1);
	buffer.WriteString(pConVar->GetName());
	buffer.WriteString(pConVar->GetString());

	for (int i = 1; i <= maxClients; i++)
	{
		CPlayer *pPlayer = g_Players.GetPlayerByIndex(i);

		if (pPlayer && pPlayer->IsInGame() && !pPlayer->IsFakeClient())
		{
			INetChannel *netchan = static_cast<INetChannel *>(engine->GetPlayerNetInfo(i));
			netchan->SendData(buffer);
		}
	}
}

static void NotifyConVar(ConVar *pConVar)
{
	IGameEvent *pEvent = gameevents->CreateEvent("server_cvar");
	pEvent->SetString("cvarname", pConVar->GetName());
	if (IsFlagSet(pConVar, FCVAR_PROTECTED))
	{
		pEvent->SetString("cvarvalue", "***PROTECTED***");
	}
	else
	{
		pEvent->SetString("cvarvalue", pConVar->GetString());
	}

	gameevents->FireEvent(pEvent);
}
#endif

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
		return pContext->ThrowNativeError("Convar \"%s\" was not created. A console command with the same might already exist.", name);
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

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
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

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
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

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
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

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
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

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	pConVar->SetValue(params[2]);

#if SOURCE_ENGINE >= SE_ORANGEBOX
	/* Should we replicate it? */
	if (params[3] && IsFlagSet(pConVar, FCVAR_REPLICATED))
	{
		ReplicateConVar(pConVar);
	}

	/* Should we notify clients? */
	if (params[4] && IsFlagSet(pConVar, FCVAR_NOTIFY))
	{
		NotifyConVar(pConVar);
	}
#endif

	return 1;
}

static cell_t sm_GetConVarFloat(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
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

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	float value = sp_ctof(params[2]);
	pConVar->SetValue(value);

#if SOURCE_ENGINE >= SE_ORANGEBOX
	/* Should we replicate it? */
	if (params[3] && IsFlagSet(pConVar, FCVAR_REPLICATED))
	{
		ReplicateConVar(pConVar);
	}

	/* Should we notify clients? */
	if (params[4] && IsFlagSet(pConVar, FCVAR_NOTIFY))
	{
		NotifyConVar(pConVar);
	}
#endif

	return 1;
}

static cell_t sm_GetConVarString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
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

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	char *value;
	pContext->LocalToString(params[2], &value);

	pConVar->SetValue(value);

#if SOURCE_ENGINE >= SE_ORANGEBOX
	/* Should we replicate it? */
	if (params[3] && IsFlagSet(pConVar, FCVAR_REPLICATED))
	{
		ReplicateConVar(pConVar);
	}

	/* Should we notify clients? */
	if (params[4] && IsFlagSet(pConVar, FCVAR_NOTIFY))
	{
		NotifyConVar(pConVar);
	}
#endif

	return 1;
}

static cell_t sm_ResetConVar(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	pConVar->Revert();
	
#if SOURCE_ENGINE >= SE_ORANGEBOX
	/* Should we replicate it? */
	if (params[3] && IsFlagSet(pConVar, FCVAR_REPLICATED))
	{
		ReplicateConVar(pConVar);
	}

	/* Should we notify clients? */
	if (params[4] && IsFlagSet(pConVar, FCVAR_NOTIFY))
	{
		NotifyConVar(pConVar);
	}
#endif

	return 1;
}

static cell_t sm_GetConVarFlags(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
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

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	pConVar->SetFlags(params[2]);

	return 1;
}

static cell_t sm_GetConVarBounds(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	cell_t *addr;
	bool hasBound;
	float bound;

	switch (params[2])
	{
	case ConVarBound_Upper:
		hasBound = pConVar->GetMax(bound);
		break;
	case ConVarBound_Lower:
		hasBound = pConVar->GetMin(bound);
		break;
	default:
		return pContext->ThrowNativeError("Invalid ConVarBounds value %d");
	}
	
	pContext->LocalToPhysAddr(params[3], &addr);
	*addr = sp_ftoc(bound);

	return hasBound;
}

static cell_t sm_SetConVarBounds(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	switch (params[2])
	{
	case ConVarBound_Upper:
		pConVar->SetMax(params[3] ? true : false, sp_ctof(params[4]));
		break;
	case ConVarBound_Lower:
		pConVar->SetMin(params[3] ? true : false, sp_ctof(params[4]));
		break;
	default:
		return pContext->ThrowNativeError("Invalid ConVarBounds value %d");
	}

	return 1;
}

static cell_t sm_GetConVarName(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	pContext->StringToLocalUTF8(params[2], params[3], pConVar->GetName(), NULL);

	return 1;
}

static bool s_QueryAlreadyWarned = false;

static cell_t sm_QueryClientConVar(IPluginContext *pContext, const cell_t *params)
{
	CPlayer *pPlayer;
	char *name;
	IPluginFunction *pCallback;

	if (!g_ConVarManager.IsQueryingSupported())
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
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
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
	const CCommand *pCmd = g_HL2.PeekCommandStack();

	if (!pCmd)
	{
		return pContext->ThrowNativeError("No command callback available");
	}

	return pCmd->ArgC() - 1;
}

static cell_t sm_GetCmdArg(IPluginContext *pContext, const cell_t *params)
{
	const CCommand *pCmd = g_HL2.PeekCommandStack();

	if (!pCmd)
	{
		return pContext->ThrowNativeError("No command callback available");
	}

	size_t length;

	const char *arg = pCmd->Arg(params[1]);
	
	pContext->StringToLocalUTF8(params[2], params[3], arg ? arg : "", &length);

	return (cell_t)length;
}

static cell_t sm_GetCmdArgString(IPluginContext *pContext, const cell_t *params)
{
	const CCommand *pCmd = g_HL2.PeekCommandStack();

	if (!pCmd)
	{
		return pContext->ThrowNativeError("No command callback available");
	}

	const char *args = pCmd->ArgS();
	size_t length;

	if (!args)
	{
		args = "";
	}

	pContext->StringToLocalUTF8(params[1], params[2], args, &length);

	return (cell_t)length;
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
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
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
	g_SourceMod.SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);

	char buffer[1024];
	size_t len = g_SourceMod.FormatString(buffer, sizeof(buffer)-2, pContext, params, 1);

	if (pContext->GetLastNativeError() != SP_ERROR_NONE)
	{
		return 0;
	}

	/* One byte for null terminator, one for newline */
	buffer[len++] = '\n';
	buffer[len] = '\0';

	engine->ServerCommand(buffer);

	return 1;
}

static cell_t sm_InsertServerCommand(IPluginContext *pContext, const cell_t *params)
{
	g_SourceMod.SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);

	char buffer[1024];
	size_t len = g_SourceMod.FormatString(buffer, sizeof(buffer)-2, pContext, params, 1);

	if (pContext->GetLastNativeError() != SP_ERROR_NONE)
	{
		return 0;
	}

	/* One byte for null terminator, one for newline */
	buffer[len++] = '\n';
	buffer[len] = '\0';

	InsertServerCommand(buffer);

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
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	g_SourceMod.SetGlobalTarget(params[1]);

	char buffer[256];
	g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, 2);

	if (pContext->GetLastNativeError() != SP_ERROR_NONE)
	{
		return 0;
	}

	engine->ClientCommand(pPlayer->GetEdict(), "%s", buffer);

	return 1;
}

static cell_t FakeClientCommand(IPluginContext *pContext, const cell_t *params)
{
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(params[1]);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	char buffer[256];
	g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, 2);

	if (pContext->GetLastNativeError() != SP_ERROR_NONE)
	{
		return 0;
	}

	serverpluginhelpers->ClientCommand(pPlayer->GetEdict(), buffer);

	return 1;
}

static cell_t FakeClientCommandEx(IPluginContext *pContext, const cell_t *params)
{
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(params[1]);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	char buffer[256];
	g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, 2);

	if (pContext->GetLastNativeError() != SP_ERROR_NONE)
	{
		return 0;
	}

	g_HL2.AddToFakeCliCmdQueue(params[1], engine->GetPlayerUserId(pPlayer->GetEdict()), buffer);

	return 1;
}

static cell_t ReplyToCommand(IPluginContext *pContext, const cell_t *params)
{
	g_SourceMod.SetGlobalTarget(params[1]);

	/* Build the format string */
	char buffer[1024];
	size_t len = g_SourceMod.FormatString(buffer, sizeof(buffer)-2, pContext, params, 2);

	if (pContext->GetLastNativeError() != SP_ERROR_NONE)
	{
		return 0;
	}

	/* If we're printing to the server, shortcut out */
	if (params[1] == 0)
	{
		/* Print */
		buffer[len++] = '\n';
		buffer[len] = '\0';
		META_CONPRINT(buffer);
		return 1;
	}

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(params[1]);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	g_SourceMod.SetGlobalTarget(params[1]);

	unsigned int replyto = g_ChatTriggers.GetReplyTo();
	if (replyto == SM_REPLY_CONSOLE)
	{
		buffer[len++] = '\n';
		buffer[len] = '\0';
		engine->ClientPrintf(pPlayer->GetEdict(), buffer);
	} else if (replyto == SM_REPLY_CHAT) {
		if (len >= 191)
		{
			len = 191;
		}
		buffer[len] = '\0';
		g_HL2.TextMsg(params[1], HUD_PRINTTALK, buffer);
	}

	return 1;
}

static cell_t GetCmdReplyTarget(IPluginContext *pContext, const cell_t *params)
{
	return g_ChatTriggers.GetReplyTo();
}

static cell_t SetCmdReplyTarget(IPluginContext *pContext, const cell_t *params)
{
	return g_ChatTriggers.SetReplyTo(params[1]);
}

static cell_t GetCommandIterator(IPluginContext *pContext, const cell_t *params)
{
	GlobCmdIter *iter = new GlobCmdIter;
	iter->started = false;

	Handle_t hndl = g_HandleSys.CreateHandle(hCmdIterType, iter, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (hndl == BAD_HANDLE)
	{
		delete iter;
	}

	return hndl;
}

static cell_t ReadCommandIterator(IPluginContext *pContext, const cell_t *params)
{
	GlobCmdIter *iter;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = g_HandleSys.ReadHandle(params[1], hCmdIterType, &sec, (void **)&iter))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid GlobCmdIter Handle %x", params[1]);
	}

	const List<ConCmdInfo *> &cmds = g_ConCmds.GetCommandList();

	if (!iter->started)
	{
		iter->iter = cmds.begin();
		iter->started = true;
	}

	while (iter->iter != cmds.end()
			&& !(*(iter->iter))->sourceMod)
	{
		iter->iter++;
	}

	if (iter->iter == cmds.end())
	{
		return 0;
	}

	ConCmdInfo *pInfo = (*(iter->iter));

	pContext->StringToLocalUTF8(params[2], params[3], pInfo->pCmd->GetName(), NULL);
	pContext->StringToLocalUTF8(params[5], params[6], pInfo->pCmd->GetHelpText(), NULL);
	
	cell_t *addr;
	pContext->LocalToPhysAddr(params[4], &addr);
	*addr = pInfo->admin.eflags;

	iter->iter++;

	return 1;
}

static cell_t CheckCommandAccess(IPluginContext *pContext, const cell_t *params)
{
	if (params[1] == 0)
	{
		return 1;
	}

	char *cmd;
	pContext->LocalToString(params[2], &cmd);

	/* Auto-detect a command if we can */
	FlagBits bits = params[3];
	bool found_command = false;
	if (params[0] < 4 || !params[4])
	{
		found_command = g_ConCmds.LookForCommandAdminFlags(cmd, &bits);
	}
	
	if (!found_command)
	{
		g_Admins.GetCommandOverride(cmd, Override_Command, &bits);
	}

	return g_ConCmds.CheckCommandAccess(params[1], cmd, bits) ? 1 : 0;
}

static cell_t IsChatTrigger(IPluginContext *pContext, const cell_t *params)
{
	return g_ChatTriggers.IsChatTrigger() ? 1 : 0;
}

static cell_t SetCommandFlags(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	pContext->LocalToString(params[1], &name);

	return (s_CommandFlagsHelper.SetFlags(name, params[2])) ? 1 : 0;
}

static cell_t GetCommandFlags(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	int flags;
	pContext->LocalToString(params[1], &name);

	return (s_CommandFlagsHelper.GetFlags(name, &flags)) ? flags : -1;
}

static cell_t FindFirstConCommand(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	ConCmdIter *pIter;
	cell_t *pIsCmd, *pFlags;
	const ConCommandBase *pConCmd;
	const char *desc;

	pContext->LocalToPhysAddr(params[3], &pIsCmd);
	pContext->LocalToPhysAddr(params[4], &pFlags);

#if SOURCE_ENGINE == SE_LEFT4DEAD
	ICvarIteratorInternal *cvarIter = icvar->FactoryInternalIterator();
	cvarIter->SetFirst();
	if (!cvarIter->IsValid())
	{
		return BAD_HANDLE;
	}
	pConCmd = cvarIter->Get();
#else
	pConCmd = icvar->GetCommands();
	if (pConCmd == NULL)
	{
		return BAD_HANDLE;
	}
#endif

	pContext->StringToLocalUTF8(params[1], params[2], pConCmd->GetName(), NULL);
	*pIsCmd = pConCmd->IsCommand() ? 1 : 0;
	*pFlags = pConCmd->GetFlags();

	if (params[6])
	{
		desc = pConCmd->GetHelpText();
		pContext->StringToLocalUTF8(params[5], params[6], (desc && desc[0]) ? desc : "", NULL);
	}

	pIter = new ConCmdIter;
#if SOURCE_ENGINE == SE_LEFT4DEAD
	pIter->pLast = cvarIter;
#else
	pIter->pLast = pConCmd;
#endif

	if ((hndl = g_HandleSys.CreateHandle(htConCmdIter, pIter, pContext->GetIdentity(), g_pCoreIdent, NULL))
		== BAD_HANDLE)
	{
		delete pIter;
		return BAD_HANDLE;
	}

	return hndl;
}

static cell_t FindNextConCommand(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	ConCmdIter *pIter;
	cell_t *pIsCmd, *pFlags;
	const char *desc;
	const ConCommandBase *pConCmd;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = g_HandleSys.ReadHandle(params[1], htConCmdIter, &sec, (void **)&pIter)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	if (pIter->pLast == NULL)
	{
		return 0;
	}

#if SOURCE_ENGINE == SE_LEFT4DEAD
	ICvarIteratorInternal *cvarIter = pIter->pLast;
	cvarIter->Next();
	if (!cvarIter->IsValid())
	{
		return 0;
	}
	pConCmd = cvarIter->Get();
#else
	pConCmd = pIter->pLast->GetNext();
	if (pConCmd == NULL)
	{
		return 0;
	}
	pIter->pLast = pConCmd;
#endif

	pContext->LocalToPhysAddr(params[4], &pIsCmd);
	pContext->LocalToPhysAddr(params[5], &pFlags);

	pContext->StringToLocalUTF8(params[2], params[3], pConCmd->GetName(), NULL);
	*pIsCmd = pConCmd->IsCommand() ? 1 : 0;
	*pFlags = pConCmd->GetFlags();

	if (params[7])
	{
		desc = pConCmd->GetHelpText();
		pContext->StringToLocalUTF8(params[6], params[7], (desc && desc[0]) ? desc : "", NULL);
	}

	return 1;
}

static cell_t SendConVarValue(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[2]);
	HandleError err;
	ConVar *pConVar;

	char *value;
	pContext->LocalToString(params[3], &value);

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	char data[256];
	bf_write buffer(data, sizeof(data));

	buffer.WriteUBitLong(NET_SETCONVAR, 5);
	buffer.WriteByte(1);
	buffer.WriteString(pConVar->GetName());
	buffer.WriteString(value);

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(params[1]);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Client %d is fake and cannot be targeted", params[1]);
	}

	INetChannel *netchan = static_cast<INetChannel *>(engine->GetPlayerNetInfo(params[1]));
	netchan->SendData(buffer);

	return 1;
}

static cell_t AddServerTag(IPluginContext *pContext, const cell_t *params)
{
	return 0;
}

static cell_t RemoveServerTag(IPluginContext *pContext, const cell_t *params)
{
	return 0;
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
	{"ResetConVar",			sm_ResetConVar},
	{"GetConVarName",		sm_GetConVarName},
	{"GetConVarBounds",		sm_GetConVarBounds},
	{"SetConVarBounds",		sm_SetConVarBounds},
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
	{"FakeClientCommand",	FakeClientCommand},
	{"ReplyToCommand",		ReplyToCommand},
	{"GetCmdReplySource",	GetCmdReplyTarget},
	{"SetCmdReplySource",	SetCmdReplyTarget},
	{"GetCommandIterator",	GetCommandIterator},
	{"ReadCommandIterator",	ReadCommandIterator},
	{"CheckCommandAccess",	CheckCommandAccess},
	{"FakeClientCommandEx",	FakeClientCommandEx},
	{"IsChatTrigger",		IsChatTrigger},
	{"SetCommandFlags",		SetCommandFlags},
	{"GetCommandFlags",		GetCommandFlags},
	{"FindFirstConCommand",	FindFirstConCommand},
	{"FindNextConCommand",	FindNextConCommand},
	{"SendConVarValue",		SendConVarValue},
	{"AddServerTag",		AddServerTag},
	{"RemoveServerTag",		RemoveServerTag},
	{NULL,					NULL}
};
