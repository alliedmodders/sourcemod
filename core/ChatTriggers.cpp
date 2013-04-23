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

#include <ITextParsers.h>
#include "ChatTriggers.h"
#include "sm_stringutil.h"
#include "ConCmdManager.h"
#include "PlayerManager.h"
#include "HalfLife2.h"
#include "logic_bridge.h"

#if SOURCE_ENGINE == SE_DOTA
SH_DECL_EXTERN2_void(ConCommand, Dispatch, SH_NOATTRIB, false, void *, const CCommand &);
#elif SOURCE_ENGINE >= SE_ORANGEBOX
SH_DECL_EXTERN1_void(ConCommand, Dispatch, SH_NOATTRIB, false, const CCommand &);
#elif SOURCE_ENGINE == SE_DARKMESSIAH
SH_DECL_EXTERN0_void(ConCommand, Dispatch, SH_NOATTRIB, false);
#else
# if SH_IMPL_VERSION >= 4
 extern int __SourceHook_FHAddConCommandDispatch(void *,bool,class fastdelegate::FastDelegate0<void>);
# else
 extern bool __SourceHook_FHAddConCommandDispatch(void *,bool,class fastdelegate::FastDelegate0<void>);
# endif
extern bool __SourceHook_FHRemoveConCommandDispatch(void *,bool,class fastdelegate::FastDelegate0<void>);
#endif

ChatTriggers g_ChatTriggers;
bool g_bSupressSilentFails = false;

ChatTriggers::ChatTriggers() : m_pSayCmd(NULL), m_bWillProcessInPost(false), 
	m_bTriggerWasSilent(false), m_ReplyTo(SM_REPLY_CONSOLE)
{
	m_PubTrigger = sm_strdup("!");
	m_PrivTrigger = sm_strdup("/");
	m_PubTriggerSize = 1;
	m_PrivTriggerSize = 1;
	m_bIsChatTrigger = false;
}

ChatTriggers::~ChatTriggers()
{
	delete [] m_PubTrigger;
	m_PubTrigger = NULL;
	delete [] m_PrivTrigger;
	m_PrivTrigger = NULL;
}

ConfigResult ChatTriggers::OnSourceModConfigChanged(const char *key, 
													const char *value, 
													ConfigSource source,
													char *error, 
													size_t maxlength)
{
	if (strcmp(key, "PublicChatTrigger") == 0)
	{
		delete [] m_PubTrigger;
		m_PubTrigger = sm_strdup(value);
		m_PubTriggerSize = strlen(m_PubTrigger);
		return ConfigResult_Accept;
	}
	else if (strcmp(key, "SilentChatTrigger") == 0)
	{
		delete [] m_PrivTrigger;
		m_PrivTrigger = sm_strdup(value);
		m_PrivTriggerSize = strlen(m_PrivTrigger);
		return ConfigResult_Accept;
	}
	else if (strcmp(key, "SilentFailSuppress") == 0)
	{
		g_bSupressSilentFails = strcmp(value, "yes") == 0;
		return ConfigResult_Accept;
	}

	return ConfigResult_Ignore;
}

void ChatTriggers::OnSourceModAllInitialized()
{
	m_pShouldFloodBlock = g_Forwards.CreateForward("OnClientFloodCheck", ET_Event, 1, NULL, Param_Cell);
	m_pDidFloodBlock = g_Forwards.CreateForward("OnClientFloodResult", ET_Event, 2, NULL, Param_Cell, Param_Cell);
}

void ChatTriggers::OnSourceModAllInitialized_Post()
{
	logicore.AddCorePhraseFile("antiflood.phrases");
}

void ChatTriggers::OnSourceModGameInitialized()
{
	m_pSayCmd = FindCommand("say");
	m_pSayTeamCmd = FindCommand("say_team");

	if (m_pSayCmd)
	{
		SH_ADD_HOOK(ConCommand, Dispatch, m_pSayCmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Pre), false);
		SH_ADD_HOOK(ConCommand, Dispatch, m_pSayCmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Post), true);
	}
	if (m_pSayTeamCmd)
	{
		SH_ADD_HOOK(ConCommand, Dispatch, m_pSayTeamCmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Pre), false);
		SH_ADD_HOOK(ConCommand, Dispatch, m_pSayTeamCmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Post), true);
	}
}

void ChatTriggers::OnSourceModShutdown()
{
	if (m_pSayTeamCmd)
	{
		SH_REMOVE_HOOK(ConCommand, Dispatch, m_pSayTeamCmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Post), true);
		SH_REMOVE_HOOK(ConCommand, Dispatch, m_pSayTeamCmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Pre), false);
	}
	if (m_pSayCmd)
	{
		SH_REMOVE_HOOK(ConCommand, Dispatch, m_pSayCmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Post), true);
		SH_REMOVE_HOOK(ConCommand, Dispatch, m_pSayCmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Pre), false);
	}

	g_Forwards.ReleaseForward(m_pShouldFloodBlock);
	g_Forwards.ReleaseForward(m_pDidFloodBlock);
}

#if SOURCE_ENGINE == SE_DOTA
void ChatTriggers::OnSayCommand_Pre(void *pUnknown, const CCommand &command)
{
#elif SOURCE_ENGINE >= SE_ORANGEBOX
void ChatTriggers::OnSayCommand_Pre(const CCommand &command)
{
#else
void ChatTriggers::OnSayCommand_Pre()
{
	CCommand command;
#endif
	int client;
	CPlayer *pPlayer;
	
	client = g_ConCmds.GetCommandClient();
	m_bIsChatTrigger = false;
	m_bWasFloodedMessage = false;

	/* The server console cannot do this */
	if (client == 0 || (pPlayer = g_Players.GetPlayerByIndex(client)) == NULL)
	{
		RETURN_META(MRES_IGNORED);
	}

	/* We guarantee the client is connected */
	if (!pPlayer->IsConnected())
	{
		RETURN_META(MRES_IGNORED);
	}

	const char *args = command.ArgS();
	
	if (!args)
	{
		RETURN_META(MRES_IGNORED);
	}

	/* Check if we need to block this message from being sent */
	if (ClientIsFlooding(client))
	{
		char buffer[128];

		if (!logicore.CoreTranslate(buffer, sizeof(buffer), "%T", 2, NULL, "Flooding the server", &client))
			UTIL_Format(buffer, sizeof(buffer), "You are flooding the server!");

		/* :TODO: we should probably kick people who spam too much. */

		char fullbuffer[192];
		UTIL_Format(fullbuffer, sizeof(fullbuffer), "[SM] %s", buffer);
		g_HL2.TextMsg(client, HUD_PRINTTALK, fullbuffer);

		m_bWasFloodedMessage = true;

		RETURN_META(MRES_SUPERCEDE);
	}

	/* Handle quoted string sets */
	bool is_quoted = false;
	if (args[0] == '"')
	{
		args++;
		is_quoted = true;
	}

	bool is_trigger = false;
	bool is_silent = false;

	/* Check for either trigger */
	if (m_PubTriggerSize && strncmp(args, m_PubTrigger, m_PubTriggerSize) == 0)
	{
		is_trigger = true;
		args = &args[m_PubTriggerSize];
	} 
	else if (m_PrivTriggerSize && strncmp(args, m_PrivTrigger, m_PrivTriggerSize) == 0) 
	{
		is_trigger = true;
		is_silent = true;
		args = &args[m_PrivTriggerSize];
	}

	if (!is_trigger)
	{
		RETURN_META(MRES_IGNORED);
	}

	/**
	 * Test if this is actually a command!
	 */
	if (!PreProcessTrigger(PEntityOfEntIndex(client), args, is_quoted))
	{
		CPlayer *pPlayer;
		if (is_silent 
			&& g_bSupressSilentFails 
			&& client != 0
			&& (pPlayer = g_Players.GetPlayerByIndex(client)) != NULL
			&& pPlayer->GetAdminId() != INVALID_ADMIN_ID)
		{
			RETURN_META(MRES_SUPERCEDE);
		}
		RETURN_META(MRES_IGNORED);
	}

	m_bIsChatTrigger = true;

	/**
	 * We'll execute it in post.
	 */
	m_bWillProcessInPost = true;
	m_bTriggerWasSilent = is_silent;

	/* If we're silent, block */
	if (is_silent)
	{
		RETURN_META(MRES_SUPERCEDE);
	}

	/* Otherwise, let the command continue */
	RETURN_META(MRES_IGNORED);
}

#if SOURCE_ENGINE == SE_DOTA
void ChatTriggers::OnSayCommand_Post(void *pUnknown, const CCommand &command)
#elif SOURCE_ENGINE >= SE_ORANGEBOX
void ChatTriggers::OnSayCommand_Post(const CCommand &command)
#else
void ChatTriggers::OnSayCommand_Post()
#endif
{
	m_bIsChatTrigger = false;
	m_bWasFloodedMessage = false;
	if (m_bWillProcessInPost)
	{
		/* Reset this for re-entrancy */
		m_bWillProcessInPost = false;
		
		/* Execute the cached command */
		int client = g_ConCmds.GetCommandClient();
		unsigned int old = SetReplyTo(SM_REPLY_CHAT);
#if SOURCE_ENGINE == SE_DOTA
		engine->ClientCommand(PEntityOfEntIndex(client), "%s", m_ToExecute);
#else
		serverpluginhelpers->ClientCommand(PEntityOfEntIndex(client), m_ToExecute);
#endif
		SetReplyTo(old);
	}
}

bool ChatTriggers::PreProcessTrigger(edict_t *pEdict, const char *args, bool is_quoted)
{
	/* Extract a command.  This is kind of sloppy. */
	char cmd_buf[64];
	size_t cmd_len = 0;
	const char *inptr = args;
	while (*inptr != '\0' 
			&& !textparsers->IsWhitespace(inptr) 
			&& *inptr != '"'
			&& cmd_len < sizeof(cmd_buf) - 1)
	{
		cmd_buf[cmd_len++] = *inptr++;
	}
	cmd_buf[cmd_len] = '\0';

	if (cmd_len == 0)
	{
		return false;
	}

	/* See if we have this registered */
	bool prepended = false;
	if (!g_ConCmds.LookForSourceModCommand(cmd_buf))
	{
		/* Check if we had an "sm_" prefix */
		if (strncmp(cmd_buf, "sm_", 3) == 0)
		{
			return false;
		}

		/* Now, prepend.  Don't worry about the buffers.  This will 
		 * work because the sizes are limited from earlier.
		 */
		char new_buf[80];
		strcpy(new_buf, "sm_");
		strncopy(&new_buf[3], cmd_buf, sizeof(new_buf)-3);

		/* Recheck */
		if (!g_ConCmds.LookForSourceModCommand(new_buf))
		{
			return false;
		}
		
		prepended = true;
	}

	/* See if we need to do extra string manipulation */
	if (is_quoted || prepended)
	{
		size_t len;

		/* Check if we need to prepend sm_ */
		if (prepended)
		{
			len = UTIL_Format(m_ToExecute, sizeof(m_ToExecute), "sm_%s", args);
		} else {
			len = strncopy(m_ToExecute, args, sizeof(m_ToExecute));
		}
		
		/* Check if we need to strip a quote */
		if (is_quoted)
		{
			if (m_ToExecute[len-1] == '"')
			{
				m_ToExecute[--len] = '\0';
			}
		}
	} else {
		strncopy(m_ToExecute, args, sizeof(m_ToExecute));
	}

	return true;
}

unsigned int ChatTriggers::SetReplyTo(unsigned int reply)
{
	unsigned int old = m_ReplyTo;

	m_ReplyTo = reply;

	return old;
}

unsigned int ChatTriggers::GetReplyTo()
{
	return m_ReplyTo;
}

bool ChatTriggers::IsChatTrigger()
{
	return m_bIsChatTrigger;
}

bool ChatTriggers::ClientIsFlooding(int client)
{
	bool is_flooding = false;

	if (m_pShouldFloodBlock->GetFunctionCount() != 0)
	{
		cell_t res = 0;

		m_pShouldFloodBlock->PushCell(client);
		m_pShouldFloodBlock->Execute(&res);

		if (res != 0)
		{
			is_flooding = true;
		}
	}

	if (m_pDidFloodBlock->GetFunctionCount() != 0)
	{
		m_pDidFloodBlock->PushCell(client);
		m_pDidFloodBlock->PushCell(is_flooding ? 1 : 0);
		m_pDidFloodBlock->Execute(NULL);
	}

	return is_flooding;
}

bool ChatTriggers::WasFloodedMessage()
{
	return m_bWasFloodedMessage;
}
