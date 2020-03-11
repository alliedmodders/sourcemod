/**
 * vim: set ts=4 sw=4 tw=99 noet:
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
#include <IGameConfigs.h>
#include "UserMessages.h"
#include "TimerSys.h"
#include "PlayerManager.h"
#include "logic_bridge.h"
#include "sourcemod.h"

#if SOURCE_ENGINE == SE_CSGO
#include <game/shared/csgo/protobuf/cstrike15_usermessages.pb.h>
#elif SOURCE_ENGINE == SE_BLADE
#include <game/shared/berimbau/protobuf/berimbau_usermessages.pb.h>
#endif

#define MAX_HUD_CHANNELS		6

int g_HudMsgNum = -1;

struct hud_syncobj_t
{
	int player_channels[256 + 1];
};

struct player_chaninfo_t
{
	double chan_times[MAX_HUD_CHANNELS];
	hud_syncobj_t *chan_syncobjs[MAX_HUD_CHANNELS];
};

struct hud_text_parms
{
	float       x;
	float       y;
	int         effect;
	byte        r1, g1, b1, a1;
	byte        r2, g2, b2, a2;
	float       fadeinTime;
	float       fadeoutTime;
	float       holdTime;
	float       fxTime;
	int         channel;
};

class HudMsgHelpers : 
	public SMGlobalClass,
	public IHandleTypeDispatch,
	public IClientListener
{
public:
	bool IsSupported()
	{
		return (g_HudMsgNum != -1);
	}

	virtual void OnSourceModAllInitialized_Post()
	{
		const char *key;

		key = g_pGameConf->GetKeyValue("HudTextMsg");
		if (key != NULL)
		{
			g_HudMsgNum = g_UserMsgs.GetMessageIndex(key);
		}

		if (!IsSupported())
		{
			m_hHudSyncObj = 0;
			m_PlayerHuds = NULL;
			return;
		}

		m_PlayerHuds = new player_chaninfo_t[256+1];
		m_hHudSyncObj = handlesys->CreateType("HudSyncObj", this, 0, NULL, NULL, g_pCoreIdent, NULL);

		g_Players.AddClientListener(this);
	}

	virtual void OnSourceModShutdown()
	{
		if (!IsSupported())
		{
			return;
		}

		delete [] m_PlayerHuds;
		handlesys->RemoveType(m_hHudSyncObj, g_pCoreIdent);

		g_Players.RemoveClientListener(this);
	}

	virtual void OnHandleDestroy(HandleType_t type, void *object)
	{
		hud_syncobj_t *obj = (hud_syncobj_t *)object;

		delete obj;
	}

	virtual bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
	{
		*pSize = sizeof(unsigned int) * g_Players.GetMaxClients();
		return true;
	}

	virtual void OnClientConnected(int client)
	{
		player_chaninfo_t *player;

		player = &m_PlayerHuds[client];
		memset(player->chan_syncobjs, 0, sizeof(player->chan_syncobjs));
		memset(player->chan_times, 0, sizeof(player->chan_times));
	}

	Handle_t CreateHudSyncObj(IdentityToken_t *pIdent)
	{
		Handle_t hndl;
		HandleError err;
		hud_syncobj_t *obj;
		HandleSecurity sec;

		obj = new hud_syncobj_t;

		memset(obj->player_channels, 0, sizeof(obj->player_channels));

		sec = HandleSecurity(pIdent, g_pCoreIdent);
		
		if ((hndl = handlesys->CreateHandleEx(m_hHudSyncObj, obj, &sec, NULL, &err))
			== BAD_HANDLE)
		{
			delete obj;
		}

		return hndl;
	}

	HandleError ReadHudSyncObj(Handle_t hndl, 
		IdentityToken_t *pOwner, 
		hud_syncobj_t **pObj)
	{
		HandleSecurity sec(pOwner, g_pCoreIdent);
		return handlesys->ReadHandle(hndl, m_hHudSyncObj, &sec, (void **)pObj);
	}

	unsigned int AutoSelectChannel(unsigned int client)
	{
		int last_channel;
		player_chaninfo_t *player;

		player = &m_PlayerHuds[client];

		last_channel = 0;
		for (unsigned int i = 1; i < MAX_HUD_CHANNELS; i++)
		{
			if (player->chan_times[i] < player->chan_times[last_channel])
			{
				last_channel = i;
			}
		}

		ManualSelectChannel(client, last_channel);

		return last_channel;
	}

	int TryReuseLastChannel(unsigned int client, hud_syncobj_t *obj)
	{
		int last_channel;
		player_chaninfo_t *player;

		player = &m_PlayerHuds[client];

		/* First, see if we can re-use the previous channel. */
		last_channel = obj->player_channels[client];

		if (player->chan_syncobjs[last_channel] == obj)
		{
			player->chan_times[last_channel] = *g_pUniversalTime;
			return last_channel;
		}

		return -1;
	}

	int AutoSelectChannel(unsigned int client, hud_syncobj_t *obj)
	{
		int last_channel;
		player_chaninfo_t *player;

		player = &m_PlayerHuds[client];
		
		/* First, see if we can re-use the previous channel. */
		last_channel = obj->player_channels[client];

		if (player->chan_syncobjs[last_channel] == obj)
		{
			player->chan_times[last_channel] = *g_pUniversalTime;
			return last_channel;
		}

		last_channel = 0;
		for (unsigned int i = 1; i < MAX_HUD_CHANNELS; i++)
		{
			if (player->chan_times[i] < player->chan_times[last_channel])
			{
				last_channel = i;
			}
		}

		obj->player_channels[client] = last_channel;
		player->chan_syncobjs[last_channel] = obj;
		player->chan_times[last_channel] = *g_pUniversalTime;

		return last_channel;
	}

	int ManualSelectChannel(unsigned int client, int channel)
	{
		player_chaninfo_t *player;

		player = &m_PlayerHuds[client];
		player->chan_times[channel] = *g_pUniversalTime;
		player->chan_syncobjs[channel] = NULL;

		return channel;
	}
private:
	HandleType_t m_hHudSyncObj;
	player_chaninfo_t *m_PlayerHuds;
} s_HudMsgHelpers;

hud_text_parms g_hud_params;

static cell_t CreateHudSynchronizer(IPluginContext *pContext, const cell_t *params)
{
	return s_HudMsgHelpers.CreateHudSyncObj(pContext->GetIdentity());
}

static cell_t SetHudTextParams(IPluginContext *pContext, const cell_t *params)
{
	g_hud_params.x = sp_ctof(params[1]);
	g_hud_params.y = sp_ctof(params[2]);
	g_hud_params.holdTime = sp_ctof(params[3]);
	g_hud_params.r1 = static_cast<byte>(params[4]);
	g_hud_params.g1 = static_cast<byte>(params[5]);
	g_hud_params.b1 = static_cast<byte>(params[6]);
	g_hud_params.a1 = static_cast<byte>(params[7]);
	g_hud_params.effect = params[8];
	g_hud_params.fxTime = sp_ctof(params[9]);
	g_hud_params.fadeinTime = sp_ctof(params[10]);
	g_hud_params.fadeoutTime = sp_ctof(params[11]);
	g_hud_params.r2 = 255;
	g_hud_params.g2 = 255;
	g_hud_params.b2 = 250;
	g_hud_params.a2 = 0;

	return 1;
}

static cell_t SetHudTextParamsEx(IPluginContext *pContext, const cell_t *params)
{
	cell_t *color1, *color2;

	pContext->LocalToPhysAddr(params[4], &color1);
	pContext->LocalToPhysAddr(params[5], &color2);

	g_hud_params.x = sp_ctof(params[1]);
	g_hud_params.y = sp_ctof(params[2]);
	g_hud_params.holdTime = sp_ctof(params[3]);
	g_hud_params.r1 = static_cast<byte>(color1[0]);
	g_hud_params.g1 = static_cast<byte>(color1[1]);
	g_hud_params.b1 = static_cast<byte>(color1[2]);
	g_hud_params.a1 = static_cast<byte>(color1[3]);
	g_hud_params.effect = params[6];
	g_hud_params.fxTime = sp_ctof(params[7]);
	g_hud_params.fadeinTime = sp_ctof(params[8]);
	g_hud_params.fadeoutTime = sp_ctof(params[9]);
	g_hud_params.r2 = static_cast<byte>(color2[0]);
	g_hud_params.g2 = static_cast<byte>(color2[1]);
	g_hud_params.b2 = static_cast<byte>(color2[2]);
	g_hud_params.a2 = static_cast<byte>(color2[3]);

	return 1;
}

void UTIL_SendHudText(int client, const hud_text_parms &textparms, const char *pMessage)
{
	cell_t players[1];

	players[0] = client;

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	CCSUsrMsg_HudMsg *msg = (CCSUsrMsg_HudMsg *)g_UserMsgs.StartProtobufMessage(g_HudMsgNum, players, 1, 0);
	msg->set_channel(textparms.channel & 0xFF);

	CMsgVector2D *pos = msg->mutable_pos();
	pos->set_x(textparms.x);
	pos->set_y(textparms.y);

	CMsgRGBA *color1 = msg->mutable_clr1();
	color1->set_r(textparms.r1);
	color1->set_g(textparms.g1);
	color1->set_b(textparms.b1);
	color1->set_a(textparms.a1);

	CMsgRGBA *color2 = msg->mutable_clr2();
	color2->set_r(textparms.r2);
	color2->set_g(textparms.g2);
	color2->set_b(textparms.b2);
	color2->set_a(textparms.a2);

	msg->set_effect(textparms.effect);
	msg->set_fade_in_time(textparms.fadeinTime);
	msg->set_fade_out_time(textparms.fadeoutTime);
	msg->set_hold_time(textparms.holdTime);
	msg->set_fx_time(textparms.fxTime);
	msg->set_text(pMessage);
#else
	bf_write *bf = g_UserMsgs.StartBitBufMessage(g_HudMsgNum, players, 1, 0);
	bf->WriteByte(textparms.channel & 0xFF );
	bf->WriteFloat(textparms.x);
	bf->WriteFloat(textparms.y);
	bf->WriteByte(textparms.r1);
	bf->WriteByte(textparms.g1);
	bf->WriteByte(textparms.b1);
	bf->WriteByte(textparms.a1);
	bf->WriteByte(textparms.r2);
	bf->WriteByte(textparms.g2);
	bf->WriteByte(textparms.b2);
	bf->WriteByte(textparms.a2);
	bf->WriteByte(textparms.effect);
	bf->WriteFloat(textparms.fadeinTime);
	bf->WriteFloat(textparms.fadeoutTime);
	bf->WriteFloat(textparms.holdTime);
	bf->WriteFloat(textparms.fxTime);
	bf->WriteString(pMessage);
#endif
	g_UserMsgs.EndMessage();
}

static cell_t ShowSyncHudText(IPluginContext *pContext, const cell_t *params)
{
	int client;
	Handle_t err;
	CPlayer *pPlayer;
	hud_syncobj_t *obj;
	char message_buffer[255-36];

	if (!s_HudMsgHelpers.IsSupported())
	{
		return -1;
	}

	if ((err = s_HudMsgHelpers.ReadHudSyncObj(params[2], pContext->GetIdentity(), &obj)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[2], err);
	}

	client = params[1];
	if ((pPlayer = g_Players.GetPlayerByIndex(client)) == NULL)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}
	else if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in-game", client);
	}

	g_SourceMod.SetGlobalTarget(client);

	{
		DetectExceptions eh(pContext);
		g_SourceMod.FormatString(message_buffer, sizeof(message_buffer), pContext, params, 3);
		if (eh.HasException())
			return 0;
	}

	g_hud_params.channel = s_HudMsgHelpers.AutoSelectChannel(client, obj);
	UTIL_SendHudText(client, g_hud_params, message_buffer);

	return 1;
}

static cell_t ClearSyncHud(IPluginContext *pContext, const cell_t *params)
{
	int client;
	int channel;
	Handle_t err;
	CPlayer *pPlayer;
	hud_syncobj_t *obj;

	if (!s_HudMsgHelpers.IsSupported())
	{
		return -1;
	}

	if ((err = s_HudMsgHelpers.ReadHudSyncObj(params[2], pContext->GetIdentity(), &obj)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[2], err);
	}

	client = params[1];
	if ((pPlayer = g_Players.GetPlayerByIndex(client)) == NULL)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}
	else if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in-game", client);
	}

	if ((channel = s_HudMsgHelpers.TryReuseLastChannel(client, obj)) == -1)
	{
		return -1;
	}

	g_hud_params.channel = channel;
	UTIL_SendHudText(client, g_hud_params, "");

	return g_hud_params.channel;
}

static cell_t ShowHudText(IPluginContext *pContext, const cell_t *params)
{
	int client;
	CPlayer *pPlayer;
	char message_buffer[255-36];

	if (!s_HudMsgHelpers.IsSupported())
	{
		return -1;
	}

	client = params[1];
	if ((pPlayer = g_Players.GetPlayerByIndex(client)) == NULL)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}
	else if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in-game", client);
	}

	g_SourceMod.SetGlobalTarget(client);

	{
		DetectExceptions eh(pContext);
		g_SourceMod.FormatString(message_buffer, sizeof(message_buffer), pContext, params, 3);
		if (eh.HasException())
			return 0;
	}

	if (params[2] == -1)
	{
		g_hud_params.channel = s_HudMsgHelpers.AutoSelectChannel(client);
	}
	else
	{
		g_hud_params.channel = params[2] % MAX_HUD_CHANNELS;
		s_HudMsgHelpers.ManualSelectChannel(client, g_hud_params.channel);
	}

	UTIL_SendHudText(client, g_hud_params, message_buffer);

	return g_hud_params.channel;
}

REGISTER_NATIVES(hudNatives)
{
	{"ClearSyncHud",				ClearSyncHud},
	{"CreateHudSynchronizer",		CreateHudSynchronizer},
	{"SetHudTextParams",			SetHudTextParams},
	{"SetHudTextParamsEx",			SetHudTextParamsEx},
	{"ShowHudText",					ShowHudText},
	{"ShowSyncHudText",				ShowSyncHudText},
	{NULL,							NULL},
};
