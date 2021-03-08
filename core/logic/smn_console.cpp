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

#include "common_logic.h"
#include "AdminCache.h"
#include <IGameHelpers.h>
#include <IPlayerHelpers.h>
#include <ISourceMod.h>
#include <ITranslator.h>
#include "sprintf.h"
#include <bridge/include/CoreProvider.h>
#include <bridge/include/IVEngineServerBridge.h>

static cell_t CheckCommandAccess(IPluginContext *pContext, const cell_t *params)
{
	if (params[1] == 0)
	{
		return 1;
	}

	char *cmd;
	pContext->LocalToString(params[2], &cmd);

	/* Match up with an admin command if possible */
	FlagBits bits = params[3];
	bool found_command = false;
	if (params[0] < 4 || !params[4])
	{
		found_command = bridge->LookForCommandAdminFlags(cmd, &bits);
	}

	if (!found_command)
	{
		adminsys->GetCommandOverride(cmd, Override_Command, &bits);
	}

	return adminsys->CheckClientCommandAccess(params[1], cmd, bits) ? 1 : 0;
}

static cell_t CheckAccess(IPluginContext *pContext, const cell_t *params)
{
	char *cmd;
	pContext->LocalToString(params[2], &cmd);

	/* Match up with an admin command if possible */
	FlagBits bits = params[3];
	bool found_command = false;
	if (params[0] < 4 || !params[4])
	{
		found_command = bridge->LookForCommandAdminFlags(cmd, &bits);
	}

	if (!found_command)
	{
		adminsys->GetCommandOverride(cmd, Override_Command, &bits);
	}

	return g_Admins.CheckAdminCommandAccess(params[1], cmd, bits) ? 1 : 0;
}

static cell_t sm_PrintToServer(IPluginContext *pCtx, const cell_t *params)
{
	char buffer[1024];
	char *fmt;
	int arg = 2;

	pCtx->LocalToString(params[1], &fmt);
	size_t res = atcprintf(buffer, sizeof(buffer) - 2, fmt, pCtx, params, &arg);

	buffer[res++] = '\n';
	buffer[res] = '\0';

	bridge->ConPrint(buffer);

	return 1;
}

static cell_t sm_PrintToConsole(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 0) || (index > playerhelpers->GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	IGamePlayer *pPlayer = NULL;
	if (index != 0)
	{
		pPlayer = playerhelpers->GetGamePlayer(index);
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
	size_t res = atcprintf(buffer, sizeof(buffer) - 2, fmt, pCtx, params, &arg);

	buffer[res++] = '\n';
	buffer[res] = '\0';

	if (index != 0)
	{
		pPlayer->PrintToConsole(buffer);
	}
	else {
		bridge->ConPrint(buffer);
	}

	return 1;
}

static cell_t sm_ServerCommand(IPluginContext *pContext, const cell_t *params)
{
	g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);

	char buffer[1024];
	size_t len;
	{
		DetectExceptions eh(pContext);
		len = g_pSM->FormatString(buffer, sizeof(buffer) - 2, pContext, params, 1);
		if (eh.HasException())
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
	g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);

	char buffer[1024];
	size_t len;
	{
		DetectExceptions eh(pContext);
		len = g_pSM->FormatString(buffer, sizeof(buffer) - 2, pContext, params, 1);
		if (eh.HasException())
			return 0;
	}

	/* One byte for null terminator, one for newline */
	buffer[len++] = '\n';
	buffer[len] = '\0';

	engine->InsertServerCommand(buffer);

	return 1;
}

static cell_t sm_ServerExecute(IPluginContext *pContext, const cell_t *params)
{
	engine->ServerExecute();

	return 1;
}

static cell_t sm_ClientCommand(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(params[1]);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	g_pSM->SetGlobalTarget(params[1]);

	char buffer[256];
	size_t len;
	{
		DetectExceptions eh(pContext);
		g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 2);
		if (eh.HasException())
			return 0;
	}

	engine->ClientCommand(pPlayer->GetEdict(), buffer);

	return 1;
}

static cell_t FakeClientCommand(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(params[1]);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	g_pSM->SetGlobalTarget(params[1]);

	char buffer[256];
	{
		DetectExceptions eh(pContext);
		g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 2);
		if (eh.HasException())
			return 0;
	}

	engine->FakeClientCommand(pPlayer->GetEdict(), buffer);

	return 1;
}

static cell_t ReplyToCommand(IPluginContext *pContext, const cell_t *params)
{
	g_pSM->SetGlobalTarget(params[1]);

	/* Build the format string */
	char buffer[1024];
	size_t len;
	{
		DetectExceptions eh(pContext);
		len = g_pSM->FormatString(buffer, sizeof(buffer) - 1, pContext, params, 2);
		if (eh.HasException())
			return 0;
	}

	/* If we're printing to the server, shortcut out */
	if (params[1] == 0)
	{
		/* Print */
		buffer[len++] = '\n';
		buffer[len] = '\0';
		bridge->ConPrint(buffer);
		return 1;
	}

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(params[1]);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	unsigned int replyto = playerhelpers->GetReplyTo();
	if (replyto == SM_REPLY_CONSOLE)
	{
		buffer[len++] = '\n';
		buffer[len] = '\0';
		pPlayer->PrintToConsole(buffer);
	}
	else if (replyto == SM_REPLY_CHAT) {
		if (len >= 191)
		{
			len = 191;
		}
		buffer[len] = '\0';
		gamehelpers->TextMsg(params[1], TEXTMSG_DEST_CHAT, buffer);
	}

	return 1;
}

static cell_t GetCmdReplyTarget(IPluginContext *pContext, const cell_t *params)
{
	return playerhelpers->GetReplyTo();
}

static cell_t SetCmdReplyTarget(IPluginContext *pContext, const cell_t *params)
{
	return playerhelpers->SetReplyTo(params[1]);
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
	{"CheckCommandAccess",	CheckCommandAccess},
	{"CheckAccess",			CheckAccess},
	{"PrintToServer",		sm_PrintToServer},
	{"PrintToConsole",		sm_PrintToConsole},
	{"ServerCommand",		sm_ServerCommand},
	{"InsertServerCommand",	sm_InsertServerCommand},
	{"ServerExecute",		sm_ServerExecute},
	{"ClientCommand",		sm_ClientCommand},
	{"FakeClientCommand",	FakeClientCommand},
	{"ReplyToCommand",		ReplyToCommand},
	{"GetCmdReplySource",	GetCmdReplyTarget},
	{"SetCmdReplySource",	SetCmdReplyTarget},
	{"AddServerTag",		AddServerTag},
	{"RemoveServerTag",		RemoveServerTag},
	{NULL,					NULL}
};
