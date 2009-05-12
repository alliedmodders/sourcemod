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

size_t g_VoiceFlags[65];
size_t g_VoiceHookCount = 0;
int g_VoiceMap[65][65];

SH_DECL_HOOK3(IVoiceServer, SetClientListening, SH_NOATTRIB, 0, bool, int, int, bool);

bool DecHookCount(int amount = 1);
bool DecHookCount(int amount)
{
	g_VoiceHookCount -= amount;
	if (g_VoiceHookCount == 0)
	{
		SH_REMOVE_HOOK_MEMFUNC(IVoiceServer, SetClientListening, voiceserver, &g_SdkTools, &SDKTools::OnSetClientListening, false);
		return true;
	}

	return false;
}

void IncHookCount()
{
	if (!g_VoiceHookCount++)
	{
		SH_ADD_HOOK_MEMFUNC(IVoiceServer, SetClientListening, voiceserver, &g_SdkTools, &SDKTools::OnSetClientListening, false);
	}
}

void SDKTools::VoiceInit()
{
	memset(g_VoiceMap, 0, sizeof(g_VoiceMap));
}

bool SDKTools::OnSetClientListening(int iReceiver, int iSender, bool bListen)
{
	if (g_VoiceMap[iReceiver][iSender] == LISTEN_NO)
	{
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, false));
	}
	else if (g_VoiceMap[iReceiver][iSender] == LISTEN_YES)
	{
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, true));
	}

	if (g_VoiceFlags[iSender] & SPEAK_MUTED)
	{
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, bListen, &IVoiceServer::SetClientListening, (iReceiver, iSender, false));
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
	g_Hooks.OnClientDisconnecting(client);

	int max_clients = playerhelpers->GetMaxClients();

	if (g_VoiceHookCount == 0)
	{
		return;
	}

	/* This can probably be optimized more, but I doubt it's that much 
	 * of an actual bottleneck.
	 */

	/* Reset clients who receive from us */
	for (int i = 1; i <= max_clients; i++)
	{
		if (i == client)
		{
			continue;
		}

		if (g_VoiceMap[i][client] != LISTEN_DEFAULT)
		{
			g_VoiceMap[i][client] = LISTEN_DEFAULT;
			if (DecHookCount())
			{
				return;
			}
		}
	}

	/* Reset clients who send to us.  I'm shoving a count in the 0 index! */
	if (g_VoiceMap[client][0] > 0)
	{
		DecHookCount(g_VoiceMap[client][0]);
		memset(&g_VoiceMap[client], 0, sizeof(int) * 65);
	}

	if (g_VoiceFlags[client])
	{
		g_VoiceFlags[client] = 0;
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
		return pContext->ThrowNativeError("(Receiver) client index %d is invalid", params[1]);
	}
	else if (!player->IsConnected())
	{
		return pContext->ThrowNativeError("(Receiver) client %d is not connected", params[1]);
	}

	player = playerhelpers->GetGamePlayer(params[2]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("(Sender) client index %d is invalid", params[2]);
	}
	else if (!player->IsConnected())
	{
		return pContext->ThrowNativeError("(Sender) client %d is not connected", params[2]);
	}

	r = params[1];
	s = params[2];
	
	if (g_VoiceMap[r][s] == LISTEN_DEFAULT && params[3] != LISTEN_DEFAULT)
	{
		g_VoiceMap[r][s] = params[3];
		g_VoiceMap[r][0]++;
		IncHookCount();
	}
	else if (g_VoiceMap[r][s] != LISTEN_DEFAULT && params[3] == LISTEN_DEFAULT)
	{
		g_VoiceMap[r][s] = params[3];
		g_VoiceMap[r][0]--;
		DecHookCount();
	}
	else
	{
		g_VoiceMap[r][s] = params[3];
	}

	return 1;
}

static cell_t GetClientListening(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *player;

	player = playerhelpers->GetGamePlayer(params[1]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("(Receiver) client index %d is invalid", params[1]);
	}
	else if (!player->IsConnected())
	{
		return pContext->ThrowNativeError("(Receiver) client %d is not connected", params[1]);
	}

	player = playerhelpers->GetGamePlayer(params[2]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("(Sender) client index %d is invalid", params[2]);
	}
	else if (!player->IsConnected())
	{
		return pContext->ThrowNativeError("(Sender) client %d is not connected", params[2]);
	}

	return g_VoiceMap[params[1]][params[2]];
}

sp_nativeinfo_t g_VoiceNatives[] =
{
	{"SetClientListeningFlags",		SetClientListeningFlags},
	{"GetClientListeningFlags",		GetClientListeningFlags},
	{"SetClientListening",			SetClientListening},
	{"GetClientListening",			GetClientListening},
	{NULL,							NULL},
};
