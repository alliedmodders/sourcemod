/**
* vim: set ts=4 :
* =============================================================================
* SourceMod SDKTools Extension
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

#include "hooks.h"
#include "extension.h"

#include "basehandle.h"
#include "vector.h"
#include "utlvector.h"
#include <shareddefs.h>
#include "usercmd.h"
#include "filesystem.h"

#define FEATURECAP_PLAYERRUNCMD_11PARAMS	"SDKTools PlayerRunCmd 11Params"

CHookManager g_Hooks;
static bool PRCH_enabled = false;
static bool PRCH_used = false;
static bool FILE_used = false;

SH_DECL_MANUALHOOK2_void(PlayerRunCmdHook, 0, 0, 0, CUserCmd *, IMoveHelper *);
SH_DECL_HOOK2(IBaseFileSystem, FileExists, SH_NOATTRIB, 0, bool, const char*, const char *);
#if SOURCE_ENGINE >= SE_ALIENSWARM || SOURCE_ENGINE == SE_LEFT4DEAD || SOURCE_ENGINE == SE_LEFT4DEAD2
SH_DECL_HOOK3(INetChannel, SendFile, SH_NOATTRIB, 0, bool, const char *, unsigned int, bool);
#else
SH_DECL_HOOK2(INetChannel, SendFile, SH_NOATTRIB, 0, bool, const char *, unsigned int);
#endif
SH_DECL_HOOK2_void(INetChannel, ProcessPacket, SH_NOATTRIB, 0, struct netpacket_s *, bool);

SourceHook::CallClass<IBaseFileSystem> *basefilesystemPatch = NULL; 

CHookManager::CHookManager()
{
	m_usercmdsFwd = NULL;
	m_netFileSendFwd = NULL;
	m_netFileReceiveFwd = NULL;
	m_pActiveNetChannel = NULL;
}

void CHookManager::Initialize()
{
	int offset;
	if (g_pGameConf->GetOffset("PlayerRunCmd", &offset))
	{
		SH_MANUALHOOK_RECONFIGURE(PlayerRunCmdHook, offset, 0, 0);
		PRCH_enabled = true;
	}
	else
	{
		g_pSM->LogError(myself, "Failed to find PlayerRunCmd offset - OnPlayerRunCmd forward disabled.");
		PRCH_enabled = false;
	}

	basefilesystemPatch = SH_GET_CALLCLASS(basefilesystem);

	m_netFileSendFwd = forwards->CreateForward("OnFileSend", ET_Event, 2, NULL, Param_Cell, Param_String);
	m_netFileReceiveFwd = forwards->CreateForward("OnFileReceive", ET_Event, 2, NULL, Param_Cell, Param_String);

	plsys->AddPluginsListener(this);
	sharesys->AddCapabilityProvider(myself, this, FEATURECAP_PLAYERRUNCMD_11PARAMS);

	m_usercmdsFwd = forwards->CreateForward("OnPlayerRunCmd", ET_Event, 11, NULL,
		Param_Cell,			// client
		Param_CellByRef,	// buttons
		Param_CellByRef,	// impulse
		Param_Array,		// Float:vel[3]
		Param_Array,		// Float:angles[3]
		Param_CellByRef,	// weapon
		Param_CellByRef,	// subtype
		Param_CellByRef,	// cmdnum
		Param_CellByRef,	// tickcount
		Param_CellByRef,	// seed
		Param_Array);		// mouse[2]
}

void CHookManager::Shutdown()
{
	forwards->ReleaseForward(m_usercmdsFwd);
	forwards->ReleaseForward(m_netFileSendFwd);
	forwards->ReleaseForward(m_netFileReceiveFwd);

	plsys->RemovePluginsListener(this);
	sharesys->DropCapabilityProvider(myself, this, FEATURECAP_PLAYERRUNCMD_11PARAMS);
}

void CHookManager::OnMapStart()
{
	m_bFSTranHookWarned = false;
#if SOURCE_ENGINE == SE_TF2
	static ConVarRef replay_enable("replay_enable");
	m_bReplayEnabled = replay_enable.GetBool();
#endif
}

void CHookManager::OnClientConnect(int client)
{
	NetChannelHook(client);
}

void CHookManager::OnClientPutInServer(int client)
{
	PlayerRunCmdHook(client);
}

void CHookManager::PlayerRunCmdHook(int client)
{
	if (!PRCH_used)
		return;

	edict_t *pEdict = PEntityOfEntIndex(client);
	if (!pEdict)
	{
		return;
	}

	IServerUnknown *pUnknown = pEdict->GetUnknown();
	if (!pUnknown)
	{
		return;
	}

	CBaseEntity *pEntity = pUnknown->GetBaseEntity();
	if (!pEntity)
	{
		return;
	}

	CVTableHook hook(pEntity);
	for (size_t i = 0; i < m_runUserCmdHooks.length(); ++i)
	{
		if (hook == m_runUserCmdHooks[i])
		{
			return;
		}
	}

	int hookid = SH_ADD_MANUALVPHOOK(PlayerRunCmdHook, pEntity, SH_MEMBER(this, &CHookManager::PlayerRunCmd), false);
	hook.SetHookID(hookid);
	m_runUserCmdHooks.append(new CVTableHook(hook));
}

void CHookManager::PlayerRunCmd(CUserCmd *ucmd, IMoveHelper *moveHelper)
{
	if (m_usercmdsFwd->GetFunctionCount() == 0)
	{
		RETURN_META(MRES_IGNORED);
	}

	CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);

	if (!pEntity)
	{
		RETURN_META(MRES_IGNORED);
	}

	edict_t *pEdict = gameents->BaseEntityToEdict(pEntity);

	if (!pEdict)
	{
		RETURN_META(MRES_IGNORED);
	}

	int client = IndexOfEdict(pEdict);


	cell_t result = 0;
	/* Impulse is a byte so we copy it back manually */
	cell_t impulse = ucmd->impulse;
	cell_t vel[3] = {sp_ftoc(ucmd->forwardmove), sp_ftoc(ucmd->sidemove), sp_ftoc(ucmd->upmove)};
	cell_t angles[3] = {sp_ftoc(ucmd->viewangles.x), sp_ftoc(ucmd->viewangles.y), sp_ftoc(ucmd->viewangles.z)};
	cell_t mouse[2] = {ucmd->mousedx, ucmd->mousedy};

	m_usercmdsFwd->PushCell(client);
	m_usercmdsFwd->PushCellByRef(&ucmd->buttons);
	m_usercmdsFwd->PushCellByRef(&impulse);
	m_usercmdsFwd->PushArray(vel, 3, SM_PARAM_COPYBACK);
	m_usercmdsFwd->PushArray(angles, 3, SM_PARAM_COPYBACK);
	m_usercmdsFwd->PushCellByRef(&ucmd->weaponselect);
	m_usercmdsFwd->PushCellByRef(&ucmd->weaponsubtype);
	m_usercmdsFwd->PushCellByRef(&ucmd->command_number);
	m_usercmdsFwd->PushCellByRef(&ucmd->tick_count);
	m_usercmdsFwd->PushCellByRef(&ucmd->random_seed);
	m_usercmdsFwd->PushArray(mouse, 2, SM_PARAM_COPYBACK);
	m_usercmdsFwd->Execute(&result);

	ucmd->impulse = impulse;
	ucmd->forwardmove = sp_ctof(vel[0]);
	ucmd->sidemove = sp_ctof(vel[1]);
	ucmd->upmove = sp_ctof(vel[2]);
	ucmd->viewangles.x = sp_ctof(angles[0]);
	ucmd->viewangles.y = sp_ctof(angles[1]);
	ucmd->viewangles.z = sp_ctof(angles[2]);
	ucmd->mousedx = mouse[0];
	ucmd->mousedy = mouse[1];


	if (result == Pl_Handled)
	{
		RETURN_META(MRES_SUPERCEDE);
	}

	RETURN_META(MRES_IGNORED);
}

void CHookManager::NetChannelHook(int client)
{
	if (!FILE_used)
		return;

	INetChannel *pNetChannel = static_cast<INetChannel *>(engine->GetPlayerNetInfo(client));
	if (pNetChannel == NULL)
	{
		return;
	}

	/* Normal NetChannel Hooks. */
	{
		CVTableHook nethook(pNetChannel);
		size_t iter;

		/* Initial Hook */
#if SOURCE_ENGINE == SE_TF2
		if (m_bReplayEnabled)
		{
			if (!m_bFSTranHookWarned)
			{
				g_pSM->LogError(myself, "OnFileSend hooks are not currently working on TF2 servers with Replay enabled.");
				m_bFSTranHookWarned = true;
			}
		}
		else
#endif
		if (!m_netChannelHooks.length())
		{
			CVTableHook filehook(basefilesystem);

			int hookid = SH_ADD_VPHOOK(IBaseFileSystem, FileExists, basefilesystem, SH_MEMBER(this, &CHookManager::FileExists), false);
			filehook.SetHookID(hookid);
			m_netChannelHooks.append(new CVTableHook(filehook));
		}

		for (iter = 0; iter < m_netChannelHooks.length(); ++iter)
		{
			if (nethook == m_netChannelHooks[iter])
			{
				break;
			}
		}

		if (iter == m_netChannelHooks.length())
		{
			int hookid = SH_ADD_VPHOOK(INetChannel, SendFile, pNetChannel, SH_MEMBER(this, &CHookManager::SendFile), false);
			nethook.SetHookID(hookid);
			m_netChannelHooks.append(new CVTableHook(nethook));
			
			hookid = SH_ADD_VPHOOK(INetChannel, ProcessPacket, pNetChannel, SH_MEMBER(this, &CHookManager::ProcessPacket), false);
			nethook.SetHookID(hookid);
			m_netChannelHooks.append(new CVTableHook(nethook));
			
			hookid = SH_ADD_VPHOOK(INetChannel, ProcessPacket, pNetChannel, SH_MEMBER(this, &CHookManager::ProcessPacket_Post), true);
			nethook.SetHookID(hookid);
			m_netChannelHooks.append(new CVTableHook(nethook));
		}
	}
}

void CHookManager::ProcessPacket(struct netpacket_s *packet, bool bHasHeader)
{
	if (m_netFileReceiveFwd->GetFunctionCount() == 0)
	{
		RETURN_META(MRES_IGNORED);
	}

	m_pActiveNetChannel = META_IFACEPTR(INetChannel);
	RETURN_META(MRES_IGNORED);
}

bool CHookManager::FileExists(const char *filename, const char *pathID)
{
	if (m_pActiveNetChannel == NULL || m_netFileReceiveFwd->GetFunctionCount() == 0)
	{
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	bool ret = SH_CALL(basefilesystemPatch, &IBaseFileSystem::FileExists)(filename, pathID);
	if (ret == true) /* If the File Exists, the engine historically bails out. */
	{
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	int userid = 0;
	IClient *pClient = (IClient *)m_pActiveNetChannel->GetMsgHandler();
	if (pClient != NULL)
	{
		userid = pClient->GetUserID();
	}

	cell_t res = Pl_Continue;
	m_netFileReceiveFwd->PushCell(playerhelpers->GetClientOfUserId(userid));
	m_netFileReceiveFwd->PushString(filename);
	m_netFileReceiveFwd->Execute(&res);

	if (res != Pl_Continue)
	{
		RETURN_META_VALUE(MRES_SUPERCEDE, true);
	}

	RETURN_META_VALUE(MRES_IGNORED, false);
}

void CHookManager::ProcessPacket_Post(struct netpacket_s* packet, bool bHasHeader)
{
	m_pActiveNetChannel = NULL;
	RETURN_META(MRES_IGNORED);
}

#if SOURCE_ENGINE >= SE_ALIENSWARM || SOURCE_ENGINE == SE_LEFT4DEAD || SOURCE_ENGINE == SE_LEFT4DEAD2
bool CHookManager::SendFile(const char *filename, unsigned int transferID, bool isReplayDemo)
#else
bool CHookManager::SendFile(const char *filename, unsigned int transferID)
#endif
{
	if (m_netFileSendFwd->GetFunctionCount() == 0)
	{
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	INetChannel *pNetChannel = META_IFACEPTR(INetChannel);
	if (pNetChannel == NULL)
	{
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	int userid = 0;
	IClient *pClient = (IClient *)pNetChannel->GetMsgHandler();
	if (pClient != NULL)
	{
		userid = pClient->GetUserID();
	}

	cell_t res = Pl_Continue;
	m_netFileSendFwd->PushCell(playerhelpers->GetClientOfUserId(userid));
	m_netFileSendFwd->PushString(filename);
	m_netFileSendFwd->Execute(&res);

	if (res != Pl_Continue)
	{
		/* Mimic the Engine. */
#if SOURCE_ENGINE >= SE_ALIENSWARM || SOURCE_ENGINE == SE_LEFT4DEAD || SOURCE_ENGINE == SE_LEFT4DEAD2
		pNetChannel->DenyFile(filename, transferID, isReplayDemo);
#else
		pNetChannel->DenyFile(filename, transferID);
#endif
		RETURN_META_VALUE(MRES_SUPERCEDE, false);
	}

	RETURN_META_VALUE(MRES_IGNORED, false);
}

void CHookManager::OnPluginLoaded(IPlugin *plugin)
{
	if (PRCH_enabled && !PRCH_used && m_usercmdsFwd->GetFunctionCount())
	{
		PRCH_used = true;

		int MaxClients = playerhelpers->GetMaxClients();
		for (int i = 1; i <= MaxClients; i++)
		{
			if (playerhelpers->GetGamePlayer(i)->IsInGame())
			{
				OnClientPutInServer(i);
			}
		}
	}

	if (!FILE_used && (m_netFileSendFwd->GetFunctionCount() || m_netFileReceiveFwd->GetFunctionCount()))
	{
		FILE_used = true;

		int MaxClients = playerhelpers->GetMaxClients();
		for (int i = 1; i <= MaxClients; i++)
		{
			if (playerhelpers->GetGamePlayer(i)->IsConnected())
			{
				OnClientConnect(i);
			}
		}
	}
}

void CHookManager::OnPluginUnloaded(IPlugin *plugin)
{
	if (PRCH_used && !m_usercmdsFwd->GetFunctionCount())
	{
		for (size_t i = 0; i < m_runUserCmdHooks.length(); ++i)
		{
			delete m_runUserCmdHooks[i];
		}

		m_runUserCmdHooks.clear();
		PRCH_used = false;
	}

	if (FILE_used && !m_netFileSendFwd->GetFunctionCount() && !m_netFileReceiveFwd->GetFunctionCount())
	{
		for (size_t i = 0; i < m_netChannelHooks.length(); ++i)
		{
			delete m_netChannelHooks[i];
		}

		m_netChannelHooks.clear();
		FILE_used = false;
	}
}

FeatureStatus CHookManager::GetFeatureStatus(FeatureType type, const char *name)
{
	return FeatureStatus_Available;
}
