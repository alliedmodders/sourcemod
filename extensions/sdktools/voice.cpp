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

#include <extension.h>
#include <hooks.h>

#define SPEAK_NORMAL		0
#define SPEAK_MUTED			1
#define SPEAK_ALL			2
#define SPEAK_LISTENALL		4
#define SPEAK_TEAM			8
#define SPEAK_LISTENTEAM	16
#define LISTEN_DEFAULT		0
#define LISTEN_NO			1
#define LISTEN_YES			2

/* this enum needs to be in the order as the LISTEN_ defines above to keep
   the old broken SetClientListening functionality untouched */
enum ListenOverride
{
	Listen_Default,		/**< Leave it up to the game */
	Listen_No,			/**< Can't hear */
	Listen_Yes,			/**< Can hear */
};

size_t g_VoiceFlags[SM_MAXPLAYERS+1];
size_t g_VoiceHookCount = 0;
ListenOverride g_VoiceMap[SM_MAXPLAYERS+1][SM_MAXPLAYERS+1];
bool g_ClientMutes[SM_MAXPLAYERS+1][SM_MAXPLAYERS+1];

SH_DECL_HOOK3(IVoiceServer, SetClientListening, SH_NOATTRIB, 0, bool, int, int, bool);

#if SOURCE_ENGINE >= SE_ORANGEBOX
SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *, const CCommand &);
#else
SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);
#endif

bool DecHookCount()
{
	if (--g_VoiceHookCount == 0)
	{
		SH_REMOVE_HOOK(IVoiceServer, SetClientListening, voiceserver, SH_MEMBER(&g_SdkTools, &SDKTools::OnSetClientListening), false);
		return true;
	}

	return false;
}

void IncHookCount()
{
	if (!g_VoiceHookCount++)
	{
		SH_ADD_HOOK(IVoiceServer, SetClientListening, voiceserver, SH_MEMBER(&g_SdkTools, &SDKTools::OnSetClientListening), false);
	}
}

void SDKTools::VoiceInit()
{
	memset(g_VoiceMap, 0, sizeof(g_VoiceMap));
	memset(g_ClientMutes, 0, sizeof(g_ClientMutes));

	SH_ADD_HOOK(IServerGameClients, ClientCommand, serverClients, SH_MEMBER(this, &SDKTools::OnClientCommand), true);
}

void SDKTools::VoiceShutdown()
{
	if (g_VoiceHookCount > 0)
	{
		g_VoiceHookCount = 1;
		DecHookCount();
	}
	SH_REMOVE_HOOK(IServerGameClients, ClientCommand, serverClients, SH_MEMBER(this, &SDKTools::OnClientCommand), true);
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
void SDKTools::OnClientCommand(edict_t *pEntity, const CCommand &args)
{
	int client = IndexOfEdict(pEntity);
#else
void SDKTools::OnClientCommand(edict_t *pEntity)
{
	CCommand args;
	int client = IndexOfEdict(pEntity);
#endif

	if ((args.ArgC() > 1) && (stricmp(args.Arg(0), "vban") == 0))
	{
		for (int i = 1; (i < args.ArgC()) && (i < 3); i++)
		{
			unsigned long mask = 0;
			sscanf(args.Arg(i), "%p", (void**)&mask);
			
			for (int j = 0; j < 32; j++)
			{
				g_ClientMutes[client][1 + j + 32 * (i - 1)] = !!(mask & 1 << j);
			}
		}
	}

	RETURN_META(MRES_IGNORED);
}

bool SDKTools::OnSetClientListening(int iReceiver, int iSender, bool bListen)
{
	if (g_ClientMutes[iReceiver][iSender])
	{
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, false));
	}

	if (g_VoiceFlags[iSender] & SPEAK_MUTED)
	{
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, false));
	}

	if (g_VoiceMap[iReceiver][iSender] == Listen_No)
	{
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, false));
	}
	else if (g_VoiceMap[iReceiver][iSender] == Listen_Yes)
	{
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, true));
	}

	if ((g_VoiceFlags[iSender] & SPEAK_ALL) || (g_VoiceFlags[iReceiver] & SPEAK_LISTENALL))
	{
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, true));
	}

	if ((g_VoiceFlags[iSender] & SPEAK_TEAM) || (g_VoiceFlags[iReceiver] & SPEAK_LISTENTEAM))
	{
		IGamePlayer *pReceiver = playerhelpers->GetGamePlayer(iReceiver);
		IGamePlayer *pSender = playerhelpers->GetGamePlayer(iSender);

		if (pReceiver && pSender && pReceiver->IsInGame() && pSender->IsInGame())
		{
			IPlayerInfo *pRInfo = pReceiver->GetPlayerInfo();
			IPlayerInfo *pSInfo = pSender->GetPlayerInfo();

			if (pRInfo && pSInfo && pRInfo->GetTeamIndex() == pSInfo->GetTeamIndex())
			{
				RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, true));
			}
		}
	}

	RETURN_META_VALUE(MRES_IGNORED, bListen);
}

void SDKTools::OnClientDisconnecting(int client)
{
	if (g_hTimerSpeaking[client])
	{
		timersys->KillTimer(g_hTimerSpeaking[client]);
	}

	int max_clients = playerhelpers->GetMaxClients();

	if (g_VoiceHookCount == 0)
	{
		return;
	}

	/* This can probably be optimized more, but I doubt it's that much 
	 * of an actual bottleneck.
	 */

	/* Reset other clients who receive from or have muted this client */
	for (int i = 1; i <= max_clients; i++)
	{
		if (i == client)
		{
			continue;
		}

		g_ClientMutes[i][client] = false;
		g_ClientMutes[client][i] = false;
		
		if (g_VoiceMap[i][client] != Listen_Default)
		{
			g_VoiceMap[i][client] = Listen_Default;
			if (DecHookCount())
			{
				break;
			}
		}
		if (g_VoiceMap[client][i] != Listen_Default)
		{
			g_VoiceMap[client][i] = Listen_Default;
			if (DecHookCount())
			{
				break;
			}
		}
	}

	if (g_VoiceFlags[client])
	{
		g_VoiceFlags[client] = SPEAK_NORMAL;
		DecHookCount();
	}
}

static cell_t SetClientListeningFlags(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *player = playerhelpers->GetGamePlayer(params[1]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}
	else if (!player->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	if (!params[2] && g_VoiceFlags[params[1]])
	{
		DecHookCount();
	}
	else if (!g_VoiceFlags[params[1]] && params[2])
	{
		IncHookCount();
	}

	g_VoiceFlags[params[1]] = params[2];

	return 1;
}

static cell_t GetClientListeningFlags(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *player = playerhelpers->GetGamePlayer(params[1]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}
	else if (!player->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	return g_VoiceFlags[params[1]];
}

static cell_t SetClientListening(IPluginContext *pContext, const cell_t *params)
{
	int r, s;
	IGamePlayer *player;
	
	player = playerhelpers->GetGamePlayer(params[1]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Receiver client index %d is invalid", params[1]);
	}
	else if (!player->IsConnected())
	{
		return pContext->ThrowNativeError("Receiver client %d is not connected", params[1]);
	}

	player = playerhelpers->GetGamePlayer(params[2]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Sender client index %d is invalid", params[2]);
	}
	else if (!player->IsConnected())
	{
		return pContext->ThrowNativeError("Sender client %d is not connected", params[2]);
	}

	r = params[1];
	s = params[2];
	
	if (g_VoiceMap[r][s] == Listen_Default && params[3] != Listen_Default)
	{
		g_VoiceMap[r][s] = (ListenOverride) params[3];
		IncHookCount();
	}
	else if (g_VoiceMap[r][s] != Listen_Default && params[3] == Listen_Default)
	{
		g_VoiceMap[r][s] = (ListenOverride) params[3];
		DecHookCount();
	}
	else
	{
		g_VoiceMap[r][s] = (ListenOverride) params[3];
	}

	return 1;
}

static cell_t GetClientListening(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *player;

	player = playerhelpers->GetGamePlayer(params[1]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Receiver client index %d is invalid", params[1]);
	}
	else if (!player->IsConnected())
	{
		return pContext->ThrowNativeError("Receiver client %d is not connected", params[1]);
	}

	player = playerhelpers->GetGamePlayer(params[2]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Sender client index %d is invalid", params[2]);
	}
	else if (!player->IsConnected())
	{
		return pContext->ThrowNativeError("Sender client %d is not connected", params[2]);
	}

	return g_VoiceMap[params[1]][params[2]];
}

static cell_t IsClientMuted(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *player;

	player = playerhelpers->GetGamePlayer(params[1]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Muter client index %d is invalid", params[1]);
	}
	else if (!player->IsConnected())
	{
		return pContext->ThrowNativeError("Muter client %d is not connected", params[1]);
	}

	player = playerhelpers->GetGamePlayer(params[2]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Mutee client index %d is invalid", params[2]);
	}
	else if (!player->IsConnected())
	{
		return pContext->ThrowNativeError("Mutee client %d is not connected", params[2]);
	}

	return g_ClientMutes[params[1]][params[2]];
}

/* FIXME: Presently if there's no hook present these natives will result in an invalid state.
 * One suggestion could be to look at if the native is bound, and then invoke the hooks in the background.
 * However, at that point really we should be enforcing the forward usage to catch new-consumers immediately.
 * If you're looking to work on this, you're welcome to ping asherkin or KyleS, or even submit a patch to add this.
 * Additional comments can be found here (if GitHub still exists): https://github.com/alliedmodders/sourcemod/pull/1247
 */
static cell_t IsClientSpeaking(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *player;
	
	player = playerhelpers->GetGamePlayer(params[1]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}
	else if (!player->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}
	else if (!player->IsInGame())
	{
		return false;
	}
	
	return g_hTimerSpeaking[params[1]] != nullptr;
}

sp_nativeinfo_t g_VoiceNatives[] =
{
	{"SetClientListeningFlags",		SetClientListeningFlags},
	{"GetClientListeningFlags",		GetClientListeningFlags},
	{"SetClientListening",			SetClientListening},
	{"GetClientListening",			GetClientListening},
	{"SetListenOverride",			SetClientListening},
	{"GetListenOverride",			GetClientListening},
	{"IsClientMuted",				IsClientMuted},
	{NULL,							NULL},
};
