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

#include "common_logic.h"
#include <string.h>
#include <IGameHelpers.h>
#include <IPlayerHelpers.h>
#include <IForwardSys.h>
#include "stringutil.h"
#include <am-string.h>
#include <bridge/include/IVEngineServerBridge.h>
#include <bridge/include/CoreProvider.h>

#define BANFLAG_AUTO	(1<<0)	/**< Auto-detects whether to ban by steamid or IP */
#define BANFLAG_IP   	(1<<1)	/**< Always ban by IP address */
#define BANFLAG_AUTHID	(1<<2)	/**< Ban by SteamID */
#define BANFLAG_NOKICK	(1<<3)	/**< Does not kick the client */
#define BANFLAG_NOWRITE	(1<<4)	/**< Ban is not written to SourceDS's files if permanent */

IForward *g_pOnBanClient = NULL;
IForward *g_pOnBanIdentity = NULL;
IForward *g_pOnRemoveBan = NULL;

class BanNativeHelpers : public SMGlobalClass
{
public:
	void OnSourceModAllInitialized()
	{
		g_pOnBanClient = forwardsys->CreateForward(
			"OnBanClient",
			ET_Event,
			7,
			NULL,
			Param_Cell,
			Param_Cell,
			Param_Cell,
			Param_String,
			Param_String,
			Param_String,
			Param_Cell);
		g_pOnBanIdentity = forwardsys->CreateForward(
			"OnBanIdentity",
			ET_Event,
			6,
			NULL,
			Param_String,
			Param_Cell,
			Param_Cell,
			Param_String,
			Param_String,
			Param_Cell);
		g_pOnRemoveBan = forwardsys->CreateForward(
			"OnRemoveBan",
			ET_Event,
			4,
			NULL,
			Param_String,
			Param_Cell,
			Param_String,
			Param_Cell);
	}
	void OnSourceModShutdown()
	{
		forwardsys->ReleaseForward(g_pOnBanClient);
		forwardsys->ReleaseForward(g_pOnBanIdentity);
		forwardsys->ReleaseForward(g_pOnRemoveBan);

		g_pOnBanClient = NULL;
		g_pOnBanIdentity = NULL;
		g_pOnRemoveBan = NULL;
	}
} s_BanNativeHelpers;


static cell_t BanIdentity(IPluginContext *pContext, const cell_t *params)
{
	char *r_identity, *ban_reason, *ban_cmd;
	int ban_time, ban_flags, ban_source;

	pContext->LocalToString(params[1], &r_identity);
	pContext->LocalToString(params[4], &ban_reason);
	pContext->LocalToString(params[5], &ban_cmd);
	ban_time = params[2];
	ban_flags = params[3];
	ban_source = params[6];

	/* Make sure we can ban by one of the two methods! */
	bool ban_by_ip = ((ban_flags & BANFLAG_IP) == BANFLAG_IP);
	if (!ban_by_ip && ((ban_flags & BANFLAG_AUTHID) != BANFLAG_AUTHID))
	{
		return pContext->ThrowNativeError("No valid ban flags specified");
	}

	/* Sanitize the input */
	char identity[64];
	strncopy(identity, r_identity, sizeof(identity));
	UTIL_ReplaceAll(identity, sizeof(identity), ";", "", true);

	cell_t handled = 0;
	if (ban_cmd[0] != '\0' && g_pOnBanIdentity->GetFunctionCount() > 0)
	{
		g_pOnBanIdentity->PushString(identity);
		g_pOnBanIdentity->PushCell(ban_time);
		g_pOnBanIdentity->PushCell(ban_flags);
		g_pOnBanIdentity->PushString(ban_reason);
		g_pOnBanIdentity->PushString(ban_cmd);
		g_pOnBanIdentity->PushCell(ban_source);
		g_pOnBanIdentity->Execute(&handled);
	}

	if (!handled)
	{
		bool write_ban = ((ban_flags & BANFLAG_NOWRITE) != BANFLAG_NOWRITE);

		char command[256];
		if (ban_by_ip)
		{
			ke::SafeSprintf(
				command,
				sizeof(command),
				"addip %d %s\n",
				ban_time,
				identity);
			engine->ServerCommand(command);
	
			if (write_ban && ban_time == 0)
			{
				engine->ServerCommand("writeip\n");
			}
		}
		else if (!gamehelpers->IsLANServer())
		{
			ke::SafeSprintf(
				command,
				sizeof(command),
				"banid %d %s\n",
				ban_time,
				identity);
			engine->ServerCommand(command);
	
			if (write_ban && ban_time == 0)
			{
				engine->ServerCommand("writeid\n");
			}
		}
		else
		{
			return 0;
		}
	}

	return 1;
}

static cell_t RemoveBan(IPluginContext *pContext, const cell_t *params)
{
	char *r_identity, *ban_cmd;
	int ban_flags, ban_source;

	pContext->LocalToString(params[1], &r_identity);
	pContext->LocalToString(params[3], &ban_cmd);
	ban_flags = params[2];
	ban_source = params[4];

	/* Make sure we can ban by one of the two methods! */
	bool ban_by_ip = ((ban_flags & BANFLAG_IP) == BANFLAG_IP);
	if (!ban_by_ip && ((ban_flags & BANFLAG_AUTHID) != BANFLAG_AUTHID))
	{
		return pContext->ThrowNativeError("No valid ban flags specified");
	}

	char identity[64];
	strncopy(identity, r_identity, sizeof(identity));
	UTIL_ReplaceAll(identity, sizeof(identity), ";", "", true);

	cell_t handled = 0;
	if (ban_cmd[0] != '\0' && g_pOnRemoveBan->GetFunctionCount() > 0)
	{
		g_pOnRemoveBan->PushString(identity);
		g_pOnRemoveBan->PushCell(ban_flags);
		g_pOnRemoveBan->PushString(ban_cmd);
		g_pOnRemoveBan->PushCell(ban_source);
		g_pOnRemoveBan->Execute(&handled);
	}

	char command[256];
	if (ban_by_ip)
	{
		if (!handled)
		{
			ke::SafeSprintf(
				command,
				sizeof(command),
				"removeip %s\n",
				identity);
			engine->ServerCommand(command);	
			engine->ServerCommand("writeip\n");
		}
	}
	else if (!gamehelpers->IsLANServer())
	{
		if (!handled)
		{
			ke::SafeSprintf(
				command,
				sizeof(command),
				"removeid %s\n",
				identity);
			engine->ServerCommand(command);
			engine->ServerCommand("writeid\n");
		}
	}
	else
	{
		return 0;
	}

	return 1;
}

static cell_t BanClient(IPluginContext *pContext, const cell_t *params)
{
	const char *kick_message;
	char *ban_reason, *ban_cmd;
	int client, ban_flags, ban_source, ban_time;

	client = gamehelpers->ReferenceToIndex(params[1]);

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer || !pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}

	if (pPlayer->IsFakeClient())
	{
		return pContext->ThrowNativeError("Cannot ban fake client %d", client);
	}

	pContext->LocalToString(params[4], &ban_reason);
	pContext->LocalToString(params[5], (char **)&kick_message);
	pContext->LocalToString(params[6], &ban_cmd);
	ban_time = params[2];
	ban_flags = params[3];
	ban_source = params[7];

	/* Check how we should ban the player */
	if (!strcmp(bridge->GetSourceEngineName(), "darkmessiah"))
	{
		/* Dark Messiah doesn't have Steam IDs so there is only one ban method to choose */
		ban_flags |= BANFLAG_IP;
		ban_flags &= ~BANFLAG_AUTHID;
	}
	else if ((ban_flags & BANFLAG_AUTO) == BANFLAG_AUTO)
	{
		if (gamehelpers->IsLANServer() || !pPlayer->IsAuthorized())
		{
			ban_flags |= BANFLAG_IP;
			ban_flags &= ~BANFLAG_AUTHID;
		}
		else
		{
			ban_flags |= BANFLAG_AUTHID;
			ban_flags &= ~BANFLAG_IP;
		}
	}
	else if ((ban_flags & BANFLAG_IP) == BANFLAG_IP)
	{
		ban_flags |= BANFLAG_IP;
		ban_flags &= ~BANFLAG_AUTHID;
	}
	else if ((ban_flags & BANFLAG_AUTHID) == BANFLAG_AUTHID)
	{
		if (pPlayer->IsAuthorized())
		{
			ban_flags |= BANFLAG_AUTHID;
			ban_flags &= ~BANFLAG_IP;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return pContext->ThrowNativeError("No valid ban method flags specified");
	}

	cell_t handled = 0;
	if (ban_cmd[0] != '\0' && g_pOnBanClient->GetFunctionCount() > 0)
	{
		g_pOnBanClient->PushCell(client);
		g_pOnBanClient->PushCell(ban_time);
		g_pOnBanClient->PushCell(ban_flags);
		g_pOnBanClient->PushString(ban_reason);
		g_pOnBanClient->PushString(kick_message);
		g_pOnBanClient->PushString(ban_cmd);
		g_pOnBanClient->PushCell(ban_source);
		g_pOnBanClient->Execute(&handled);
	}

	/* Sanitize the kick message */
	if (kick_message[0] == '\0')
	{
		kick_message = "Kicked";
	}

	if (!handled)
	{
		if ((ban_flags & BANFLAG_IP) == BANFLAG_IP)
		{
			/* Get the IP address and strip the port */
			char ip[24], *ptr;
			strncopy(ip, pPlayer->GetIPAddress(), sizeof(ip));
			if ((ptr = strchr(ip, ':')) != NULL)
			{
				*ptr = '\0';
			}
		
			/* Tell the server to ban the ip */
			char command[256];
			ke::SafeSprintf(
				command,
				sizeof(command),
				"addip %d %s\n",
				ban_time,
				ip);
	
			/* Kick, then ban */
			if ((ban_flags & BANFLAG_NOKICK) != BANFLAG_NOKICK)
			{
   		 		pPlayer->Kick(kick_message);
			}
			engine->ServerCommand(command);
	
			/* Physically write the ban */
			if ((ban_time == 0) && ((ban_flags & BANFLAG_NOWRITE) != BANFLAG_NOWRITE))
			{
				engine->ServerCommand("writeip\n");
			}	
		} 
		else if ((ban_flags & BANFLAG_AUTHID) == BANFLAG_AUTHID)
		{
			/* Tell the server to ban the auth string */
			char command[256];
			ke::SafeSprintf(
				command, 
				sizeof(command), 
				"banid %d %s\n", 
				ban_time,
				pPlayer->GetAuthString());
	
			/* Kick, then ban */
			if ((ban_flags & BANFLAG_NOKICK) != BANFLAG_NOKICK)
			{
				gamehelpers->AddDelayedKick(client, pPlayer->GetUserId(), kick_message);
			}
			engine->ServerCommand(command);

			/* Physically write the ban if it's permanent */
			if ((ban_time == 0) && ((ban_flags & BANFLAG_NOWRITE) != BANFLAG_NOWRITE))
			{
				engine->ServerCommand("writeid\n");
			}
		}
	}
	else if ((ban_flags & BANFLAG_NOKICK) != BANFLAG_NOKICK)
	{
		gamehelpers->AddDelayedKick(client, pPlayer->GetUserId(), kick_message);
	}


	return 1;
}

REGISTER_NATIVES(banNatives)
{
	{"BanClient",				BanClient},
	{"BanIdentity",				BanIdentity},
	{"RemoveBan",				RemoveBan},
	{NULL,						NULL}
};

