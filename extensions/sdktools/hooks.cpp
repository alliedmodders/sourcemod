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
static bool PRCHPost_used = false;
static bool FILE_used = false;
#if !defined CLIENTVOICE_HOOK_SUPPORT
static bool PVD_used = false;
#endif

SH_DECL_MANUALHOOK2_void(PlayerRunCmdHook, 0, 0, 0, CUserCmd *, IMoveHelper *);
SH_DECL_HOOK2(IBaseFileSystem, FileExists, SH_NOATTRIB, 0, bool, const char*, const char *);
#if SOURCE_ENGINE >= SE_ALIENSWARM || SOURCE_ENGINE == SE_LEFT4DEAD || SOURCE_ENGINE == SE_LEFT4DEAD2
SH_DECL_HOOK3(INetChannel, SendFile, SH_NOATTRIB, 0, bool, const char *, unsigned int, bool);
#else
SH_DECL_HOOK2(INetChannel, SendFile, SH_NOATTRIB, 0, bool, const char *, unsigned int);
#endif
#if !defined CLIENTVOICE_HOOK_SUPPORT
SH_DECL_HOOK1(IClientMessageHandler, ProcessVoiceData, SH_NOATTRIB, 0, bool, CLC_VoiceData *);
#endif
SH_DECL_HOOK2_void(INetChannel, ProcessPacket, SH_NOATTRIB, 0, struct netpacket_s *, bool);

SourceHook::CallClass<IBaseFileSystem> *basefilesystemPatch = NULL; 

CHookManager::CHookManager()
#if SOURCE_ENGINE == SE_TF2
	: replay_enabled("replay_enabled", false)
#endif
{
	m_usercmdsFwd = NULL;
	m_usercmdsPostFwd = NULL;
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

	m_usercmdsPostFwd = forwards->CreateForward("OnPlayerRunCmdPost", ET_Ignore, 11, NULL,
		Param_Cell,			// client
		Param_Cell,			// buttons
		Param_Cell,			// impulse
		Param_Array,		// Float:vel[3]
		Param_Array,		// Float:angles[3]
		Param_Cell,			// weapon
		Param_Cell,			// subtype
		Param_Cell,			// cmdnum
		Param_Cell,			// tickcount
		Param_Cell,			// seed
		Param_Array);		// mouse[2]
}

void CHookManager::Shutdown()
{
	if (basefilesystemPatch)
	{
		SH_RELEASE_CALLCLASS(basefilesystemPatch);
		basefilesystemPatch = NULL;
	}

	if (PRCH_used)
	{
		for (size_t i = 0; i < m_runUserCmdHooks.size(); ++i)
		{
			delete m_runUserCmdHooks[i];
		}

		m_runUserCmdHooks.clear();
		PRCH_used = false;
	}

	if (PRCHPost_used)
	{
		for (size_t i = 0; i < m_runUserCmdPostHooks.size(); ++i)
		{
			delete m_runUserCmdPostHooks[i];
		}

		m_runUserCmdPostHooks.clear();
		PRCHPost_used = false;
	}

	if (FILE_used)
	{
		for (size_t i = 0; i < m_netChannelHooks.size(); ++i)
		{
			delete m_netChannelHooks[i];
		}

		m_netChannelHooks.clear();
		FILE_used = false;
	}
	
#if !defined CLIENTVOICE_HOOK_SUPPORT
	if (PVD_used)
	{
		for (size_t i = 0; i < m_netProcessVoiceData.size(); ++i)
		{
			delete m_netProcessVoiceData[i];
		}

		m_netProcessVoiceData.clear();
		PVD_used = false;
	}
#endif

	forwards->ReleaseForward(m_usercmdsFwd);
	forwards->ReleaseForward(m_usercmdsPostFwd);
	forwards->ReleaseForward(m_netFileSendFwd);
	forwards->ReleaseForward(m_netFileReceiveFwd);

	plsys->RemovePluginsListener(this);
	sharesys->DropCapabilityProvider(myself, this, FEATURECAP_PLAYERRUNCMD_11PARAMS);
}

void CHookManager::OnMapStart()
{
	m_bFSTranHookWarned = false;
}

void CHookManager::OnClientConnect(int client)
{
	NetChannelHook(client);
}

#if !defined CLIENTVOICE_HOOK_SUPPORT
void CHookManager::OnClientConnected(int client)
{
	if (!PVD_used)
	{
		return;	
	}

	IClient *pClient = iserver->GetClient(client-1);
	if (!pClient)
	{
		return;
	}
	
	std::vector<CVTableHook *> &netProcessVoiceData = m_netProcessVoiceData;
	CVTableHook hook(pClient);
	for (size_t i = 0; i < netProcessVoiceData.size(); ++i)
	{
		if (hook == netProcessVoiceData[i])
		{
			return;
		}
	}
	
	int hookid = SH_ADD_VPHOOK(IClientMessageHandler, ProcessVoiceData, (IClientMessageHandler *)((intptr_t)(pClient) + 4), SH_MEMBER(this, &CHookManager::ProcessVoiceData), true);
	hook.SetHookID(hookid);
	netProcessVoiceData.push_back(new CVTableHook(hook));
}
#endif

void CHookManager::OnClientPutInServer(int client)
{
	if (PRCH_used)
		PlayerRunCmdHook(client, false);
	if (PRCHPost_used)
		PlayerRunCmdHook(client, true);
}

void CHookManager::PlayerRunCmdHook(int client, bool post)
{
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

	std::vector<CVTableHook *> &runUserCmdHookVec = post ? m_runUserCmdPostHooks : m_runUserCmdHooks;
	CVTableHook hook(pEntity);
	for (size_t i = 0; i < runUserCmdHookVec.size(); ++i)
	{
		if (hook == runUserCmdHookVec[i])
		{
			return;
		}
	}

	int hookid;
	if (post)
		hookid = SH_ADD_MANUALVPHOOK(PlayerRunCmdHook, pEntity, SH_MEMBER(this, &CHookManager::PlayerRunCmdPost), true);
	else
		hookid = SH_ADD_MANUALVPHOOK(PlayerRunCmdHook, pEntity, SH_MEMBER(this, &CHookManager::PlayerRunCmd), false);

	hook.SetHookID(hookid);
	runUserCmdHookVec.push_back(new CVTableHook(hook));
}

void CHookManager::PlayerRunCmd(CUserCmd *ucmd, IMoveHelper *moveHelper)
{
	if (!ucmd)
	{
		RETURN_META(MRES_IGNORED);
	}

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

void CHookManager::PlayerRunCmdPost(CUserCmd *ucmd, IMoveHelper *moveHelper)
{
	if (!ucmd)
	{
		RETURN_META(MRES_IGNORED);
	}

	if (m_usercmdsPostFwd->GetFunctionCount() == 0)
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
	cell_t vel[3] = { sp_ftoc(ucmd->forwardmove), sp_ftoc(ucmd->sidemove), sp_ftoc(ucmd->upmove) };
	cell_t angles[3] = { sp_ftoc(ucmd->viewangles.x), sp_ftoc(ucmd->viewangles.y), sp_ftoc(ucmd->viewangles.z) };
	cell_t mouse[2] = { ucmd->mousedx, ucmd->mousedy };

	m_usercmdsPostFwd->PushCell(client);
	m_usercmdsPostFwd->PushCell(ucmd->buttons);
	m_usercmdsPostFwd->PushCell(ucmd->impulse);
	m_usercmdsPostFwd->PushArray(vel, 3);
	m_usercmdsPostFwd->PushArray(angles, 3);
	m_usercmdsPostFwd->PushCell(ucmd->weaponselect);
	m_usercmdsPostFwd->PushCell(ucmd->weaponsubtype);
	m_usercmdsPostFwd->PushCell(ucmd->command_number);
	m_usercmdsPostFwd->PushCell(ucmd->tick_count);
	m_usercmdsPostFwd->PushCell(ucmd->random_seed);
	m_usercmdsPostFwd->PushArray(mouse, 2);
	m_usercmdsPostFwd->Execute();

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
		if (replay_enabled.GetBool())
		{
			if (!m_bFSTranHookWarned)
			{
				g_pSM->LogError(myself, "OnFileSend hooks are not currently working on TF2 servers with Replay enabled.");
				m_bFSTranHookWarned = true;
			}
		}
		else
#endif
		if (!m_netChannelHooks.size())
		{
			CVTableHook filehook(basefilesystem);

			int hookid = SH_ADD_VPHOOK(IBaseFileSystem, FileExists, basefilesystem, SH_MEMBER(this, &CHookManager::FileExists), false);
			filehook.SetHookID(hookid);
			m_netChannelHooks.push_back(new CVTableHook(filehook));
		}

		for (iter = 0; iter < m_netChannelHooks.size(); ++iter)
		{
			if (nethook == m_netChannelHooks[iter])
			{
				break;
			}
		}

		if (iter == m_netChannelHooks.size())
		{
			int hookid = SH_ADD_VPHOOK(INetChannel, SendFile, pNetChannel, SH_MEMBER(this, &CHookManager::SendFile), false);
			nethook.SetHookID(hookid);
			m_netChannelHooks.push_back(new CVTableHook(nethook));
			
			hookid = SH_ADD_VPHOOK(INetChannel, ProcessPacket, pNetChannel, SH_MEMBER(this, &CHookManager::ProcessPacket), false);
			nethook.SetHookID(hookid);
			m_netChannelHooks.push_back(new CVTableHook(nethook));
			
			hookid = SH_ADD_VPHOOK(INetChannel, ProcessPacket, pNetChannel, SH_MEMBER(this, &CHookManager::ProcessPacket_Post), true);
			nethook.SetHookID(hookid);
			m_netChannelHooks.push_back(new CVTableHook(nethook));
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

#if !defined CLIENTVOICE_HOOK_SUPPORT
bool CHookManager::ProcessVoiceData(CLC_VoiceData *msg)
{
	IClient *pClient = (IClient *)((intptr_t)(META_IFACEPTR(IClient)) - 4);
	if (pClient == NULL)
	{
		return true;
	}

	int client = pClient->GetPlayerSlot() + 1;

	if (g_hTimerSpeaking[client])
	{
		timersys->KillTimer(g_hTimerSpeaking[client]);
	}

	g_hTimerSpeaking[client] = timersys->CreateTimer(&g_SdkTools, 0.3f, (void *)(intptr_t)client, 0);

	m_OnClientSpeaking->PushCell(client);
	m_OnClientSpeaking->Execute();

	return true;
}
#endif

void CHookManager::OnPluginLoaded(IPlugin *plugin)
{
	if (PRCH_enabled)
	{
		bool changed = false;
		if (!PRCH_used && (m_usercmdsFwd->GetFunctionCount() > 0))
		{
			PRCH_used = true;
			changed = true;
		}
		if (!PRCHPost_used && (m_usercmdsPostFwd->GetFunctionCount() > 0))
		{
			PRCHPost_used = true;
			changed = true;
		}

		// Only check the hooks on the players if a new hook is used by this plugin.
		if (changed)
		{
			int MaxClients = playerhelpers->GetMaxClients();
			for (int i = 1; i <= MaxClients; i++)
			{
				if (playerhelpers->GetGamePlayer(i)->IsInGame())
				{
					OnClientPutInServer(i);
				}
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
	
#if !defined CLIENTVOICE_HOOK_SUPPORT
	if (!PVD_used && (m_OnClientSpeaking->GetFunctionCount() || m_OnClientSpeakingEnd->GetFunctionCount()))
	{
		PVD_used = true;

		int MaxClients = playerhelpers->GetMaxClients();
		for (int i = 1; i <= MaxClients; i++)
		{
			if (playerhelpers->GetGamePlayer(i)->IsConnected())
			{
				OnClientConnected(i);
			}
		}
	}
#endif
}

void CHookManager::OnPluginUnloaded(IPlugin *plugin)
{
	if (PRCH_used && !m_usercmdsFwd->GetFunctionCount())
	{
		for (size_t i = 0; i < m_runUserCmdHooks.size(); ++i)
		{
			delete m_runUserCmdHooks[i];
		}

		m_runUserCmdHooks.clear();
		PRCH_used = false;
	}

	if (PRCHPost_used && !m_usercmdsPostFwd->GetFunctionCount())
	{
		for (size_t i = 0; i < m_runUserCmdPostHooks.size(); ++i)
		{
			delete m_runUserCmdPostHooks[i];
		}

		m_runUserCmdPostHooks.clear();
		PRCHPost_used = false;
	}

	if (FILE_used && !m_netFileSendFwd->GetFunctionCount() && !m_netFileReceiveFwd->GetFunctionCount())
	{
		for (size_t i = 0; i < m_netChannelHooks.size(); ++i)
		{
			delete m_netChannelHooks[i];
		}

		m_netChannelHooks.clear();
		FILE_used = false;
	}
	
#if !defined CLIENTVOICE_HOOK_SUPPORT
	if (PVD_used && !m_OnClientSpeaking->GetFunctionCount() && !m_OnClientSpeakingEnd->GetFunctionCount())
	{
		for (size_t i = 0; i < m_netProcessVoiceData.size(); ++i)
		{
			delete m_netProcessVoiceData[i];
		}

		m_netProcessVoiceData.clear();
		PVD_used = false;
	}
#endif
}

FeatureStatus CHookManager::GetFeatureStatus(FeatureType type, const char *name)
{
	return FeatureStatus_Available;
}
