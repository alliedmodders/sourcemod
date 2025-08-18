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
#if SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_HL2DM
class CBasePlayer;
#endif
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

unsigned int g_iPlayerRunCmdHook = 0;

CHookManager::CHookManager()
{
	m_usercmdsPreFwd = NULL;
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
		g_iPlayerRunCmdHook = offset;
		PRCH_enabled = true;
	}
	else
	{
		g_pSM->LogError(myself, "Failed to find PlayerRunCmd offset - OnPlayerRunCmd forward disabled.");
		PRCH_enabled = false;
	}

	m_netFileSendFwd = forwards->CreateForward("OnFileSend", ET_Event, 2, NULL, Param_Cell, Param_String);
	m_netFileReceiveFwd = forwards->CreateForward("OnFileReceive", ET_Event, 2, NULL, Param_Cell, Param_String);

	plsys->AddPluginsListener(this);
	sharesys->AddCapabilityProvider(myself, this, FEATURECAP_PLAYERRUNCMD_11PARAMS);
	
	m_usercmdsPreFwd = forwards->CreateForward("OnPlayerRunCmdPre", ET_Ignore, 11, NULL,
		Param_Cell,			// int client
		Param_Cell,			// int buttons
		Param_Cell,			// int impulse
		Param_Array,		// float vel[3]
		Param_Array,		// float angles[3]
		Param_Cell,			// int weapon
		Param_Cell,			// int subtype
		Param_Cell,			// int cmdnum
		Param_Cell,			// int tickcount
		Param_Cell,			// int seed
		Param_Array);		// int mouse[2]

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

	forwards->ReleaseForward(m_usercmdsPreFwd);
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
	void** vtable = *(void***)pClient;
	for (size_t i = 0; i < netProcessVoiceData.size(); ++i)
	{
		if (vtable == netProcessVoiceData[i]->GetVTablePtr())
		{
			return;
		}
	}
	
	auto msghndl = (IClientMessageHandler *)((intptr_t)(pClient) + sizeof(void *));
	auto func = KHook::GetVtableFunction(msghndl, &IClientMessageHandler::ProcessVoiceData);

	netProcessVoiceData.push_back(new CVTableHook(vtable,
	new KHook::Member(
		func,
		this, nullptr, &CHookManager::ProcessVoiceData
	)));
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
	void** vtable = *(void***)pEntity;
	for (size_t i = 0; i < runUserCmdHookVec.size(); ++i)
	{
		if (vtable == runUserCmdHookVec[i]->GetVTablePtr())
		{
			return;
		}
	}

	auto func = KHook::GetVtableFunction<CBaseEntity, void, CUserCmd*, IMoveHelper*>(pEntity, g_iPlayerRunCmdHook);

	runUserCmdHookVec.push_back(new CVTableHook(vtable,
	new KHook::Member(
		func,
		this, (post) ? nullptr : &CHookManager::PlayerRunCmd, (post) ? &CHookManager::PlayerRunCmdPost : nullptr
	)));
}

KHook::Return<void> CHookManager::PlayerRunCmd(CBaseEntity* this_ptr, CUserCmd *ucmd, IMoveHelper *moveHelper)
{
	if (!ucmd)
	{
		return { KHook::Action::Ignore };
	}

	bool hasUsercmdsPreFwds = (m_usercmdsPreFwd->GetFunctionCount() > 0);
	bool hasUsercmdsFwds = (m_usercmdsFwd->GetFunctionCount() > 0);

	if (!hasUsercmdsPreFwds && !hasUsercmdsFwds)
	{
		return { KHook::Action::Ignore };
	}

	CBaseEntity *pEntity = this_ptr;

	if (!pEntity)
	{
		return { KHook::Action::Ignore };
	}

	edict_t *pEdict = gameents->BaseEntityToEdict(pEntity);

	if (!pEdict)
	{
		return { KHook::Action::Ignore };
	}

	int client = IndexOfEdict(pEdict);


	cell_t result = 0;
	/* Impulse is a byte so we copy it back manually */
	cell_t impulse = ucmd->impulse;
	cell_t vel[3] = {sp_ftoc(ucmd->forwardmove), sp_ftoc(ucmd->sidemove), sp_ftoc(ucmd->upmove)};
	cell_t angles[3] = {sp_ftoc(ucmd->viewangles.x), sp_ftoc(ucmd->viewangles.y), sp_ftoc(ucmd->viewangles.z)};
	cell_t mouse[2] = {ucmd->mousedx, ucmd->mousedy};
	
	if (hasUsercmdsPreFwds)
	{
		m_usercmdsPreFwd->PushCell(client);
		m_usercmdsPreFwd->PushCell(ucmd->buttons);
		m_usercmdsPreFwd->PushCell(ucmd->impulse);
		m_usercmdsPreFwd->PushArray(vel, 3);
		m_usercmdsPreFwd->PushArray(angles, 3);
		m_usercmdsPreFwd->PushCell(ucmd->weaponselect);
		m_usercmdsPreFwd->PushCell(ucmd->weaponsubtype);
		m_usercmdsPreFwd->PushCell(ucmd->command_number);
		m_usercmdsPreFwd->PushCell(ucmd->tick_count);
		m_usercmdsPreFwd->PushCell(ucmd->random_seed);
		m_usercmdsPreFwd->PushArray(mouse, 2);
		m_usercmdsPreFwd->Execute();
	}

	if (hasUsercmdsFwds)
	{
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
			return { KHook::Action::Supersede };
		}
	}

	return { KHook::Action::Ignore };
}

KHook::Return<void> CHookManager::PlayerRunCmdPost(CBaseEntity* this_ptr, CUserCmd *ucmd, IMoveHelper *moveHelper)
{
	if (!ucmd)
	{
		return { KHook::Action::Ignore };
	}

	if (m_usercmdsPostFwd->GetFunctionCount() == 0)
	{
		return { KHook::Action::Ignore };
	}

	CBaseEntity *pEntity = this_ptr;

	if (!pEntity)
	{
		return { KHook::Action::Ignore };
	}

	edict_t *pEdict = gameents->BaseEntityToEdict(pEntity);

	if (!pEdict)
	{
		return { KHook::Action::Ignore };
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

	return { KHook::Action::Ignore };
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
		void** vtable = *(void***)pNetChannel;
		size_t iter;

		/* Initial Hook */
#if SOURCE_ENGINE == SE_TF2
		ConVarRef replay_enable("replay_enable", false);
		if (replay_enable.GetBool())
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
			m_netChannelHooks.push_back(new CVTableHook(*(void***)basefilesystem,
			new KHook::Member(
				KHook::GetVtableFunction(basefilesystem, &IBaseFileSystem::FileExists),
				this, &CHookManager::FileExists, nullptr
			)));
		}

		for (iter = 0; iter < m_netChannelHooks.size(); ++iter)
		{
			if (vtable == m_netChannelHooks[iter]->GetVTablePtr())
			{
				break;
			}
		}

		if (iter == m_netChannelHooks.size())
		{
			m_netChannelHooks.push_back(new CVTableHook(*(void***)pNetChannel,
			new KHook::Member(
				KHook::GetVtableFunction(pNetChannel, &INetChannel::SendFile),
				this, &CHookManager::SendFile, nullptr
			)));

			m_netChannelHooks.push_back(new CVTableHook(*(void***)pNetChannel,
			new KHook::Member(
				KHook::GetVtableFunction(pNetChannel, &INetChannel::ProcessPacket),
				this, &CHookManager::ProcessPacket, &CHookManager::ProcessPacket_Post
			)));
		}
	}
}

KHook::Return<void> CHookManager::ProcessPacket(INetChannel* this_ptr, struct netpacket_s *packet, bool bHasHeader)
{
	if (m_netFileReceiveFwd->GetFunctionCount() == 0)
	{
		return { KHook::Action::Ignore };
	}

	m_pActiveNetChannel = this_ptr;
	return { KHook::Action::Ignore };
}

KHook::Return<bool> CHookManager::FileExists(IBaseFileSystem*, const char *filename, const char *pathID)
{
	if (m_pActiveNetChannel == NULL || m_netFileReceiveFwd->GetFunctionCount() == 0)
	{
		return { KHook::Action::Ignore };
	}

	bool ret = KHook::CallOriginal(&IBaseFileSystem::FileExists, basefilesystem, filename, pathID);
	if (ret == true) /* If the File Exists, the engine historically bails out. */
	{
		return { KHook::Action::Ignore };
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
		return { KHook::Action::Supersede, true };
	}

	return { KHook::Action::Ignore };
}

KHook::Return<void> CHookManager::ProcessPacket_Post(INetChannel*, struct netpacket_s* packet, bool bHasHeader)
{
	m_pActiveNetChannel = NULL;
	return { KHook::Action::Ignore };
}

#if (SOURCE_ENGINE >= SE_ALIENSWARM || SOURCE_ENGINE == SE_LEFT4DEAD || SOURCE_ENGINE == SE_LEFT4DEAD2)
KHook::Return<bool> CHookManager::SendFile(INetChannel* this_ptr, const char *filename, unsigned int transferID, bool isReplayDemo)
#else
KHook::Return<bool> CHookManager::SendFile(INetChannel* this_ptr, const char *filename, unsigned int transferID)
#endif
{
	if (m_netFileSendFwd->GetFunctionCount() == 0)
	{
		return { KHook::Action::Ignore };
	}

	INetChannel *pNetChannel = this_ptr;
	if (pNetChannel == NULL)
	{
		return { KHook::Action::Ignore };
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
#if (SOURCE_ENGINE >= SE_ALIENSWARM || SOURCE_ENGINE == SE_LEFT4DEAD || SOURCE_ENGINE == SE_LEFT4DEAD2)
		pNetChannel->DenyFile(filename, transferID, isReplayDemo);
#else
		pNetChannel->DenyFile(filename, transferID);
#endif
		return { KHook::Action::Supersede, false };
	}

	return { KHook::Action::Ignore };
}

#if !defined CLIENTVOICE_HOOK_SUPPORT
KHook::Return<bool> CHookManager::ProcessVoiceData(IClientMessageHandler* this_ptr, CLC_VoiceData *msg)
{
	IClient *pClient = (IClient *)((intptr_t)(this_ptr) - sizeof(void *));
	if (pClient == NULL)
	{
		return { KHook::Action::Ignore, true };
	}

	int client = pClient->GetPlayerSlot() + 1;

	if (g_hTimerSpeaking[client])
	{
		timersys->KillTimer(g_hTimerSpeaking[client]);
	}

	g_hTimerSpeaking[client] = timersys->CreateTimer(&g_SdkTools, 0.3f, (void *)(intptr_t)client, 0);

	m_OnClientSpeaking->PushCell(client);
	m_OnClientSpeaking->Execute();

	return { KHook::Action::Ignore, true };
}
#endif

void CHookManager::OnPluginLoaded(IPlugin *plugin)
{
	if (PRCH_enabled)
	{
		bool changed = false;
		if (!PRCH_used && ((m_usercmdsFwd->GetFunctionCount() > 0) || (m_usercmdsPreFwd->GetFunctionCount() > 0)))
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
	if (PRCH_used && (!m_usercmdsFwd->GetFunctionCount() && !m_usercmdsPreFwd->GetFunctionCount()))
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
