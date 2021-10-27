/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
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
#include "sourcemod.h"
#include "HalfLife2.h"
#include "sourcemm_api.h"
#include "ConVarManager.h"
#include "ConCmdManager.h"
#include "sm_stringutil.h"
#include "PlayerManager.h"
#include "ChatTriggers.h"
#include <inetchannel.h>
#include <bitbuf.h>
#include <tier0/dbg.h>
#include "Logger.h"
#include "ConsoleDetours.h"
#include "ConCommandBaseIterator.h"
#include "logic_bridge.h"
#include <sm_namehashset.h>
#include "smn_keyvalues.h"
#include <bridge/include/IScriptManager.h>
#include <bridge/include/ILogger.h>
#include <ITranslator.h>

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
#include <netmessages.pb.h>
#endif

#if SOURCE_ENGINE >= SE_EYE
#define NETMSG_BITS 6
#else
#define NETMSG_BITS 5
#endif

#if SOURCE_ENGINE >= SE_LEFT4DEAD
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

class ConsoleHelpers : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public:
	virtual void OnSourceModAllInitialized()
	{
		HandleAccess access;

		handlesys->InitAccessDefaults(NULL, &access);

		htConCmdIter = handlesys->CreateType("ConCmdIter", this, 0, NULL, &access, g_pCoreIdent, NULL);

		access.access[HandleAccess_Clone] = HANDLE_RESTRICT_OWNER | HANDLE_RESTRICT_IDENTITY;

		hCmdIterType = handlesys->CreateType("CmdIter", this, 0, NULL, &access, g_pCoreIdent, NULL);
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
			ConCommandBaseIterator *iter = (ConCommandBaseIterator * )object;
			delete iter;
		}
	}
	virtual bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
	{
		if (type == htConCmdIter)
		{
			*pSize = sizeof(ConCommandBaseIterator);
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
	void OnUnlinkConCommandBase(ConCommandBase *pBase, const char *name) override
	{
		m_CmdFlags.remove(name);
	}
	bool GetFlags(const char *name, int *flags)
	{
		ConCommandBase *pCmd;
		if (m_CmdFlags.retrieve(name, &pCmd))
		{
			TrackConCommandBase(pCmd, this);
			*flags = pCmd->m_nFlags;
			return true;
		}
		else if ((pCmd=FindCommandBase(name)))
		{
			m_CmdFlags.insert(name, pCmd);
			TrackConCommandBase(pCmd, this);
			*flags = pCmd->m_nFlags;
			return true;
		}
		else
		{
			return false;
		}
	}
	bool SetFlags(const char *name, int flags)
	{
		ConCommandBase *pCmd;
		if (m_CmdFlags.retrieve(name, &pCmd))
		{
			pCmd->m_nFlags = flags;
			TrackConCommandBase(pCmd, this);
			return true;
		}
		else if ((pCmd=FindCommandBase(name)))
		{
			m_CmdFlags.insert(name, pCmd);
			pCmd->m_nFlags = flags;
			TrackConCommandBase(pCmd, this);
			return true;
		}
		else
		{
			return false;
		}
	}
private:
	struct ConCommandPolicy
	{
		static inline bool matches(const char *name, ConCommandBase *base)
		{
			const char *conCommandChars = base->GetName();
			
			std::string conCommandName = ke::Lowercase(conCommandChars);
			std::string input = ke::Lowercase(name);
			
			return conCommandName == input;
		}
		static inline uint32_t hash(const detail::CharsAndLength &key)
		{
			std::string lower = ke::Lowercase(key.c_str());
			return detail::CharsAndLength(lower.c_str()).hash();
		}
	};
	NameHashSet<ConCommandBase *, ConCommandPolicy> m_CmdFlags;
} s_CommandFlagsHelper;

#if SOURCE_ENGINE < SE_ORANGEBOX
static void ReplicateConVar(ConVar *pConVar)
{
	int maxClients = g_Players.GetMaxClients();

	char data[256];
	bf_write buffer(data, sizeof(data));

	buffer.WriteUBitLong(NET_SETCONVAR, NETMSG_BITS);
	buffer.WriteByte(1);
	buffer.WriteString(pConVar->GetName());
	buffer.WriteString(pConVar->GetString());

	for (int i = 1; i <= maxClients; i++)
	{
		CPlayer *pPlayer = g_Players.GetPlayerByIndex(i);

		if (pPlayer && pPlayer->IsInGame() && !pPlayer->IsFakeClient())
		{
			if (INetChannel *netchan = static_cast<INetChannel *>(engine->GetPlayerNetInfo(i)))
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

#if SOURCE_ENGINE < SE_ORANGEBOX
	/* Should we replicate it? */
	if (params[0] >= 3 && params[3] && IsFlagSet(pConVar, FCVAR_REPLICATED))
	{
		ReplicateConVar(pConVar);
	}

	/* Should we notify clients? */
	if (params[0] >= 4 && params[4] && IsFlagSet(pConVar, FCVAR_NOTIFY))
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

#if SOURCE_ENGINE < SE_ORANGEBOX
	/* Should we replicate it? */
	if (params[0] >= 3 && params[3] && IsFlagSet(pConVar, FCVAR_REPLICATED))
	{
		ReplicateConVar(pConVar);
	}

	/* Should we notify clients? */
	if (params[0] >= 4 && params[4] && IsFlagSet(pConVar, FCVAR_NOTIFY))
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

#if SOURCE_ENGINE < SE_ORANGEBOX
	/* Should we replicate it? */
	if (params[0] >= 3 && params[3] && IsFlagSet(pConVar, FCVAR_REPLICATED))
	{
		ReplicateConVar(pConVar);
	}

	/* Should we notify clients? */
	if (params[0] >= 4 && params[4] && IsFlagSet(pConVar, FCVAR_NOTIFY))
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
	
#if SOURCE_ENGINE < SE_ORANGEBOX
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

static cell_t GetConVarDefault(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	size_t bytes;
	pContext->StringToLocalUTF8(params[2], params[3], pConVar->GetDefault(), &bytes);

	return bytes;
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

	return pConVar->m_nFlags;
}

static cell_t sm_GetConVarPlugin(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	IPlugin *pPlugin;

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, nullptr, &pPlugin))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	return pPlugin ? pPlugin->GetMyHandle() : BAD_HANDLE;
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

	pConVar->m_nFlags = params[2];

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
		pConVar->m_fMaxVal = sp_ctof(params[4]);
		pConVar->m_bHasMax = params[3] ? true : false;
		break;
	case ConVarBound_Lower:
		pConVar->m_fMinVal = sp_ctof(params[4]);
		pConVar->m_bHasMin = params[3] ? true : false;
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

static cell_t sm_GetConVarDescription(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	ConVar *pConVar;

	if ((err=g_ConVarManager.ReadConVarHandle(hndl, &pConVar))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid convar handle %x (error %d)", hndl, err);
	}

	pContext->StringToLocalUTF8(params[2], params[3], pConVar->GetHelpText(), NULL);

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

	if (strcasecmp(name, "sm") == 0)
	{
		return pContext->ThrowNativeError("Cannot register \"sm\" command");
	}

	pContext->LocalToString(params[3], &help);
	pFunction = pContext->GetFunctionById(params[2]);

	if (!pFunction)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[2]);
	}

	IPlugin *pPlugin = scripts->FindPluginByContext(pContext->GetContext());
	if (!g_ConCmds.AddServerCommand(pFunction, name, help, params[4], pPlugin))
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

	if (strcasecmp(name, "sm") == 0)
	{
		return pContext->ThrowNativeError("Cannot register \"sm\" command");
	}

	pContext->LocalToString(params[3], &help);
	pFunction = pContext->GetFunctionById(params[2]);

	if (!pFunction)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[2]);
	}

	IPlugin *pPlugin = scripts->FindPluginByContext(pContext->GetContext());
	const char *group = pPlugin->GetFilename();
	if (!g_ConCmds.AddAdminCommand(pFunction, name, group, 0, help, params[4], pPlugin))
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

	if (strcasecmp(name, "sm") == 0)
	{
		return pContext->ThrowNativeError("Cannot register \"sm\" command");
	}

	pContext->LocalToString(params[4], &help);
	pContext->LocalToString(params[5], (char **)&group);
	pFunction = pContext->GetFunctionById(params[2]);

	IPlugin *pPlugin = scripts->FindPluginByContext(pContext->GetContext());
	if (group[0] == '\0')
	{
		group = pPlugin->GetFilename();
	}

	if (!pFunction)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[2]);
	}

	if (!g_ConCmds.AddAdminCommand(pFunction, name, group, flags, help, cmdflags, pPlugin))
	{
		return pContext->ThrowNativeError("Command \"%s\" could not be created. A convar with the same name already exists.", name);
	}

	return 1;
}

static cell_t sm_GetCmdArgs(IPluginContext *pContext, const cell_t *params)
{
	const ICommandArgs *pCmd = g_HL2.PeekCommandStack();

	if (!pCmd)
	{
		return pContext->ThrowNativeError("No command callback available");
	}

	return pCmd->ArgC() - 1;
}

static cell_t sm_GetCmdArg(IPluginContext *pContext, const cell_t *params)
{
	const ICommandArgs *pCmd = g_HL2.PeekCommandStack();

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
	const ICommandArgs *pCmd = g_HL2.PeekCommandStack();

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

char *g_ServerCommandBuffer = NULL;
cell_t g_ServerCommandBufferLength;

bool g_ShouldCatchSpew = false;

#if SOURCE_ENGINE < SE_NUCLEARDAWN
SpewOutputFunc_t g_OriginalSpewOutputFunc = NULL;

SpewRetval_t SourcemodSpewOutputFunc(SpewType_t spewType, tchar const *pMsg)
{
	if (g_ServerCommandBuffer)
	{
		V_strcat(g_ServerCommandBuffer, pMsg, g_ServerCommandBufferLength);
	}

	if (g_OriginalSpewOutputFunc)
	{
		return g_OriginalSpewOutputFunc(spewType, pMsg);
	} else {
		return SPEW_CONTINUE;
	}
}
#else
class CSourcemodLoggingListener: public ILoggingListener
{
public:
	void Log(const LoggingContext_t *pContext, const tchar *pMessage)
	{
		if (g_ServerCommandBuffer)
		{
			V_strcat(g_ServerCommandBuffer, pMessage, g_ServerCommandBufferLength);
		}
	}
} g_SourcemodLoggingListener;
#endif

CON_COMMAND(sm_conhook_start, "")
{
	if (!g_ShouldCatchSpew)
	{
		Warning("You shouldn't run this command!\n");
		return;
	}

#if SOURCE_ENGINE < SE_NUCLEARDAWN
	g_OriginalSpewOutputFunc = GetSpewOutputFunc();
	SpewOutputFunc(SourcemodSpewOutputFunc);
#else
#if SOURCE_ENGINE < SE_ALIENSWARM
	LoggingSystem_PushLoggingState(false);
#else
	LoggingSystem_PushLoggingState(false, false);
#endif
	LoggingSystem_RegisterLoggingListener(&g_SourcemodLoggingListener);
#endif
}

CON_COMMAND(sm_conhook_stop, "")
{
	if (!g_ShouldCatchSpew)
	{
		Warning("You shouldn't run this command!\n");
		return;
	}

#if SOURCE_ENGINE < SE_NUCLEARDAWN
	SpewOutputFunc(g_OriginalSpewOutputFunc);
#else
	LoggingSystem_PopLoggingState(false);
#endif

	g_ShouldCatchSpew = false;
}

static cell_t sm_ServerCommandEx(IPluginContext *pContext, const cell_t *params)
{
	g_SourceMod.SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);

	char buffer[1024];
	size_t len;
	{
		DetectExceptions eh(pContext);
		len = g_SourceMod.FormatString(buffer, sizeof(buffer)-2, pContext, params, 3);
		if (eh.HasException())
			return 0;
	}

	/* One byte for null terminator, one for newline */
	buffer[len++] = '\n';
	buffer[len] = '\0';

	pContext->LocalToString(params[1], &g_ServerCommandBuffer);
	g_ServerCommandBufferLength = params[2];
	
	if (g_ServerCommandBufferLength > 0)
	{
		g_ServerCommandBuffer[0] = '\0';
	}

	engine->ServerExecute();

	g_ShouldCatchSpew = true;
	engine->ServerCommand("sm_conhook_start\n");
	engine->ServerCommand(buffer);
	engine->ServerCommand("sm_conhook_stop\n");

	engine->ServerExecute();

	if (g_ServerCommandBufferLength > 0)
	{
		g_ServerCommandBuffer[g_ServerCommandBufferLength-1] = '\0';
	}

	g_ServerCommandBuffer = NULL;
	g_ServerCommandBufferLength = 0;

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

	g_SourceMod.SetGlobalTarget(params[1]);

	char buffer[256];
	{
		DetectExceptions eh(pContext);
		g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, 2);
		if (eh.HasException())
			return 0;
	}

	g_HL2.AddToFakeCliCmdQueue(params[1], engine->GetPlayerUserId(pPlayer->GetEdict()), buffer);

	return 1;
}

static cell_t GetCommandIterator(IPluginContext *pContext, const cell_t *params)
{
	GlobCmdIter *iter = new GlobCmdIter;
	iter->started = false;

	Handle_t hndl = handlesys->CreateHandle(hCmdIterType, iter, pContext->GetIdentity(), g_pCoreIdent, NULL);
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

	if ((err = handlesys->ReadHandle(params[1], hCmdIterType, &sec, (void **)&iter))
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
	*addr = pInfo->eflags;

	iter->iter++;

	return 1;
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
	ConCommandBaseIterator *pIter;
	cell_t *pIsCmd, *pFlags;
	const ConCommandBase *pConCmd;
	const char *desc;

	pContext->LocalToPhysAddr(params[3], &pIsCmd);
	pContext->LocalToPhysAddr(params[4], &pFlags);

	pIter = new ConCommandBaseIterator();

	if (!pIter->IsValid())
	{
		delete pIter;
		return BAD_HANDLE;
	}

	pConCmd = pIter->Get();

	pContext->StringToLocalUTF8(params[1], params[2], pConCmd->GetName(), NULL);
	*pIsCmd = pConCmd->IsCommand() ? 1 : 0;
	*pFlags = pConCmd->m_nFlags;

	if (params[6])
	{
		desc = pConCmd->GetHelpText();
		pContext->StringToLocalUTF8(params[5], params[6], (desc && desc[0]) ? desc : "", NULL);
	}

	if ((hndl = handlesys->CreateHandle(htConCmdIter, pIter, pContext->GetIdentity(), g_pCoreIdent, NULL))
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
	ConCommandBaseIterator *pIter;
	cell_t *pIsCmd, *pFlags;
	const char *desc;
	const ConCommandBase *pConCmd;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htConCmdIter, &sec, (void **)&pIter)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	if (!pIter->IsValid())
	{
		return 0;
	}

	pIter->Next();
	if (!pIter->IsValid())
	{
		return 0;
	}
	pConCmd = pIter->Get();

	pContext->LocalToPhysAddr(params[4], &pIsCmd);
	pContext->LocalToPhysAddr(params[5], &pFlags);

	pContext->StringToLocalUTF8(params[2], params[3], pConCmd->GetName(), NULL);
	*pIsCmd = pConCmd->IsCommand() ? 1 : 0;
	*pFlags = pConCmd->m_nFlags;

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

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	CNETMsg_SetConVar msg;
	CMsg_CVars_CVar *cvar = msg.mutable_convars()->add_cvars();

	cvar->set_name(pConVar->GetName());
	cvar->set_value(value);

	int msgsize = msg.ByteSize();
   
	buffer.WriteVarInt32(net_SetConVar);
	buffer.WriteVarInt32(msgsize);
	msg.SerializeWithCachedSizesToArray( (uint8 *)( buffer.GetBasePointer() + buffer.GetNumBytesWritten() ) );
	buffer.SeekToBit( ( buffer.GetNumBytesWritten() + msgsize ) * 8 );
#else
	buffer.WriteUBitLong(NET_SETCONVAR, NETMSG_BITS);
	buffer.WriteByte(1);
	buffer.WriteString(pConVar->GetName());
	buffer.WriteString(value);
#endif

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(params[1]);

	if (!pPlayer)
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);

	if (!pPlayer->IsConnected())
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);

	if (pPlayer->IsFakeClient())
		return pContext->ThrowNativeError("Client %d is fake and cannot be targeted", params[1]);

	INetChannel *netchan = static_cast<INetChannel *>(engine->GetPlayerNetInfo(params[1]));
	if (netchan == NULL)
		return 0;

	netchan->SendData(buffer);
	return 1;
}

static cell_t AddCommandListener(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	IPluginFunction *pFunction;

	pContext->LocalToString(params[2], &name);

	if (strcasecmp(name, "sm") == 0)
	{
		logger->LogError("Request to register \"sm\" command denied.");
		return 0;
	}

	pFunction = pContext->GetFunctionById(params[1]);

	if (!pFunction)
		return pContext->ThrowNativeError("Invalid function id (%X)", params[1]);

	if (!g_ConsoleDetours.AddListener(pFunction, name[0] == '\0' ? NULL : name))
		return pContext->ThrowNativeError("This game does not support command listeners");

	return 1;
}

static cell_t RemoveCommandListener(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	IPluginFunction *pFunction;

	pContext->LocalToString(params[2], &name);
	pFunction = pContext->GetFunctionById(params[1]);
	if (!pFunction)
		return pContext->ThrowNativeError("Invalid function id (%X)", params[1]);

	if (!g_ConsoleDetours.RemoveListener(pFunction, name[0] == '\0' ? NULL : name))
		return pContext->ThrowNativeError("No matching callback was registered");
	
	return 1;
}

static cell_t ConVar_ReplicateToClient(IPluginContext *pContext, const cell_t *params)
{
	// Old version is (client, handle, value).
	// New version is (handle, client, value).
	cell_t new_params[4] = {
		3,
		params[2],
		params[1],
		params[3],
	};

	return SendConVarValue(pContext, new_params);
}

static cell_t FakeClientCommandKeyValues(IPluginContext *pContext, const cell_t *params)
{
#if SOURCE_ENGINE >= SE_EYE
	int client = params[1];

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	Handle_t hndl = static_cast<Handle_t>(params[2]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr = handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **) &pStk))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	if (g_Players.InClientCommandKeyValuesHook())
	{
		SH_CALL(serverClients, &IServerGameClients::ClientCommandKeyValues)(pPlayer->GetEdict(), pStk->pBase);
	}
	else
	{
		serverClients->ClientCommandKeyValues(pPlayer->GetEdict(), pStk->pBase);
	}

	return 1;
#else
	return pContext->ThrowNativeError("FakeClientCommandKeyValues is not supported on this game.");
#endif
}

static cell_t sm_CommandIterator(IPluginContext *pContext, const cell_t *params)
{
	GlobCmdIter *iter = new GlobCmdIter;
	iter->started = false;

	Handle_t hndl = handlesys->CreateHandle(hCmdIterType, iter, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (hndl == BAD_HANDLE)
	{
		delete iter;
	}

	return hndl;
}

static cell_t sm_CommandIteratorNext(IPluginContext *pContext, const cell_t *params)
{
	GlobCmdIter *iter;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], hCmdIterType, &sec, (void **)&iter))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid CommandIterator Handle %x", params[1]);
	}

	const List<ConCmdInfo *> &cmds = g_ConCmds.GetCommandList();

	if (!iter->started)
	{
		iter->iter = cmds.begin();
		iter->started = true;
	}
	else
	{
		iter->iter++;
	}

	// iterate further, skip non-sourcemod cmds
	while (iter->iter != cmds.end() && !(*(iter->iter))->sourceMod)
	{
		iter->iter++;
	}
	
	return iter->iter != cmds.end();
}

static cell_t sm_CommandIteratorFlags(IPluginContext *pContext, const cell_t *params)
{
	GlobCmdIter *iter;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], hCmdIterType, &sec, (void **)&iter))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid CommandIterator Handle %x", params[1]);
	}
	const List<ConCmdInfo *> &cmds = g_ConCmds.GetCommandList();
	if (!iter->started || iter->iter == cmds.end())
	{
		return pContext->ThrowNativeError("Invalid CommandIterator position");
	}
	
	ConCmdInfo *pInfo = (*(iter->iter));
	return pInfo->eflags;
}

static cell_t sm_CommandIteratorGetDesc(IPluginContext *pContext, const cell_t *params)
{
	GlobCmdIter *iter;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], hCmdIterType, &sec, (void **)&iter))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid CommandIterator Handle %x", params[1]);
	}
	const List<ConCmdInfo *> &cmds = g_ConCmds.GetCommandList();
	if (!iter->started || iter->iter == cmds.end())
	{
		return pContext->ThrowNativeError("Invalid CommandIterator position");
	}

	ConCmdInfo *pInfo = (*(iter->iter));
	pContext->StringToLocalUTF8(params[2], params[3], pInfo->pCmd->GetHelpText(), NULL);

	return 1;
}

static cell_t sm_CommandIteratorGetName(IPluginContext *pContext, const cell_t *params)
{
	GlobCmdIter *iter;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], hCmdIterType, &sec, (void **)&iter))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid CommandIterator Handle %x", params[1]);
	}
	const List<ConCmdInfo *> &cmds = g_ConCmds.GetCommandList();
	if (!iter->started || iter->iter == cmds.end())
	{
		return pContext->ThrowNativeError("Invalid CommandIterator position");
	}

	ConCmdInfo *pInfo = (*(iter->iter));
	pContext->StringToLocalUTF8(params[2], params[3], pInfo->pCmd->GetName(), NULL);

	return 1;
}

static cell_t sm_CommandIteratorPlugin(IPluginContext *pContext, const cell_t *params)
{
	GlobCmdIter *iter;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], hCmdIterType, &sec, (void **)&iter))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid CommandIterator Handle %x", params[1]);
	}
	const List<ConCmdInfo *> &cmds = g_ConCmds.GetCommandList();
	if (!iter->started || iter->iter == cmds.end())
	{
		return pContext->ThrowNativeError("Invalid CommandIterator position");
	}

	ConCmdInfo *pInfo = (*(iter->iter));
	return pInfo->pPlugin->GetMyHandle();
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
	{"GetConVarDefault",	GetConVarDefault},
	{"RegServerCmd",		sm_RegServerCmd},
	{"RegConsoleCmd",		sm_RegConsoleCmd},
	{"GetCmdArgString",		sm_GetCmdArgString},
	{"GetCmdArgs",			sm_GetCmdArgs},
	{"GetCmdArg",			sm_GetCmdArg},
	{"RegAdminCmd",			sm_RegAdminCmd},
	{"ServerCommandEx",		sm_ServerCommandEx},
	{"GetCommandIterator",	GetCommandIterator},
	{"ReadCommandIterator",	ReadCommandIterator},
	{"FakeClientCommandEx",	FakeClientCommandEx},
	{"IsChatTrigger",		IsChatTrigger},
	{"SetCommandFlags",		SetCommandFlags},
	{"GetCommandFlags",		GetCommandFlags},
	{"FindFirstConCommand",	FindFirstConCommand},
	{"FindNextConCommand",	FindNextConCommand},
	{"SendConVarValue",		SendConVarValue},
	{"AddCommandListener",	AddCommandListener},
	{"RemoveCommandListener", RemoveCommandListener},
	{"FakeClientCommandKeyValues", FakeClientCommandKeyValues},

	// Transitional syntax support.
	{"ConVar.BoolValue.get",	sm_GetConVarBool},
	{"ConVar.BoolValue.set",	sm_SetConVarNum},
	{"ConVar.FloatValue.get",	sm_GetConVarFloat},
	{"ConVar.FloatValue.set",	sm_SetConVarFloat},
	{"ConVar.IntValue.get",		sm_GetConVarInt},
	{"ConVar.IntValue.set",		sm_SetConVarNum},
	{"ConVar.Flags.get",		sm_GetConVarFlags},
	{"ConVar.Flags.set",		sm_SetConVarFlags},
	{"ConVar.Plugin.get",		sm_GetConVarPlugin},
	{"ConVar.SetBool",			sm_SetConVarNum},
	{"ConVar.SetInt",			sm_SetConVarNum},
	{"ConVar.SetFloat",			sm_SetConVarFloat},
	{"ConVar.GetString",		sm_GetConVarString},
	{"ConVar.SetString",		sm_SetConVarString},
	{"ConVar.RestoreDefault",	sm_ResetConVar},
	{"ConVar.GetDefault",		GetConVarDefault},
	{"ConVar.GetBounds",		sm_GetConVarBounds},
	{"ConVar.SetBounds",		sm_SetConVarBounds},
	{"ConVar.GetName",			sm_GetConVarName},
	{"ConVar.GetDescription",	sm_GetConVarDescription},
	{"ConVar.ReplicateToClient",	ConVar_ReplicateToClient},
	{"ConVar.AddChangeHook",	sm_HookConVarChange},
	{"ConVar.RemoveChangeHook",	sm_UnhookConVarChange},

	{"CommandIterator.CommandIterator",	sm_CommandIterator},
	{"CommandIterator.Next",		sm_CommandIteratorNext},
	{"CommandIterator.GetDescription",	sm_CommandIteratorGetDesc},
	{"CommandIterator.GetName",		sm_CommandIteratorGetName},
	{"CommandIterator.Flags.get",		sm_CommandIteratorFlags},
	{"CommandIterator.Plugin.get",		sm_CommandIteratorPlugin},

	{NULL,					NULL}
};
