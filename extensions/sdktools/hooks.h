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

#ifndef _INCLUDE_HOOKS_H_
#define _INCLUDE_HOOKS_H_

class CUserCmd;

#include "extension.h"
#include <am-vector.h>
#include "inetchannel.h"
#include "iclient.h"
#include "vtable_hook_helper.h"

class CHookManager : IPluginsListener, IFeatureProvider
{
public:
	CHookManager();
	void Initialize();
	void Shutdown();
	void OnClientConnect(int client);
#if !defined CLIENTVOICE_HOOK_SUPPORT
	void OnClientConnected(int client);
#endif
	void OnClientPutInServer(int client);
	void PlayerRunCmd(CUserCmd *ucmd, IMoveHelper *moveHelper);
	void PlayerRunCmdPost(CUserCmd *ucmd, IMoveHelper *moveHelper);
	void OnMapStart();
public: /* NetChannel/Related Hooks */
	bool FileExists(const char *filename, const char *pathID);
#if SOURCE_ENGINE >= SE_ALIENSWARM || SOURCE_ENGINE == SE_LEFT4DEAD || SOURCE_ENGINE == SE_LEFT4DEAD2
	bool SendFile(const char *filename, unsigned int transferID, bool isReplayDemo);
#else
	bool SendFile(const char *filename, unsigned int transferID);
#endif
#if !defined CLIENTVOICE_HOOK_SUPPORT
	bool ProcessVoiceData(CLC_VoiceData *msg);
#endif
	void ProcessPacket(struct netpacket_s *packet, bool bHasHeader);
	void ProcessPacket_Post(struct netpacket_s *packet, bool bHasHeader);
public: //IPluginsListener
	void OnPluginLoaded(IPlugin *plugin);
	void OnPluginUnloaded(IPlugin *plugin);
public: //IFeatureProvider
	virtual FeatureStatus GetFeatureStatus(FeatureType type, const char *name);

private:
	void PlayerRunCmdHook(int client, bool post);
	void NetChannelHook(int client);

private:
	IForward *m_usercmdsFwd;
	IForward *m_usercmdsPostFwd;
	IForward *m_netFileSendFwd;
	IForward *m_netFileReceiveFwd;
	std::vector<CVTableHook *> m_runUserCmdHooks;
	std::vector<CVTableHook *> m_runUserCmdPostHooks;
	std::vector<CVTableHook *> m_netChannelHooks;
#if !defined CLIENTVOICE_HOOK_SUPPORT
	std::vector<CVTableHook *> m_netProcessVoiceData;
#endif
	INetChannel *m_pActiveNetChannel;
	bool m_bFSTranHookWarned = false;
#if SOURCE_ENGINE == SE_TF2
	ConVarRef replay_enabled;
#endif
};

extern CHookManager g_Hooks;

#endif // _INCLUDE_HOOKS_H_
