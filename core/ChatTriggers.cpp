/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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
#include <IPlayerHelpers.h>

/* :HACKHACK: We can't SH_DECL here because ConCmdManager.cpp does */
extern bool __SourceHook_FHRemoveConCommandDispatch(void *, bool, class fastdelegate::FastDelegate0<void>);

#if SH_IMPL_VERSION >= 4
extern int __SourceHook_FHAddConCommandDispatch(void *, bool, class fastdelegate::FastDelegate0<void>);
#else
extern bool __SourceHook_FHAddConCommandDispatch(void *, bool, class fastdelegate::FastDelegate0<void>);
#endif

ChatTriggers g_ChatTriggers;

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
	} else if (strcmp(key, "SilentChatTrigger") == 0) {
		delete [] m_PrivTrigger;
		m_PrivTrigger = sm_strdup(value);
		m_PrivTriggerSize = strlen(m_PrivTrigger);
		return ConfigResult_Accept;
	}

	return ConfigResult_Ignore;
}

void ChatTriggers::OnSourceModGameInitialized()
{
	unsigned int total = 2;
	ConCommandBase *pCmd = icvar->GetCommands();
	const char *name;
	while (pCmd)
	{
		if (pCmd->IsCommand())
		{
			name = pCmd->GetName();
			if (!m_pSayCmd && strcmp(name, "say") == 0)
			{
				m_pSayCmd = (ConCommand *)pCmd;
				if (--total == 0)
				{
					break;
				}
			} else if (!m_pSayTeamCmd && strcmp(name, "say_team") == 0) {
				m_pSayTeamCmd = (ConCommand *)pCmd;
				if (--total == 0)
				{
					break;
				}
			}
		}
		pCmd = const_cast<ConCommandBase *>(pCmd->GetNext());
	}

	if (m_pSayCmd)
	{
		SH_ADD_HOOK_MEMFUNC(ConCommand, Dispatch, m_pSayCmd, this, &ChatTriggers::OnSayCommand_Pre, false);
		SH_ADD_HOOK_MEMFUNC(ConCommand, Dispatch, m_pSayCmd, this, &ChatTriggers::OnSayCommand_Post, true);
	}
	if (m_pSayTeamCmd)
	{
		SH_ADD_HOOK_MEMFUNC(ConCommand, Dispatch, m_pSayTeamCmd, this, &ChatTriggers::OnSayCommand_Pre, false);
		SH_ADD_HOOK_MEMFUNC(ConCommand, Dispatch, m_pSayTeamCmd, this, &ChatTriggers::OnSayCommand_Post, true);
	}
}

void ChatTriggers::OnSourceModShutdown()
{
	if (m_pSayTeamCmd)
	{
		SH_REMOVE_HOOK_MEMFUNC(ConCommand, Dispatch, m_pSayTeamCmd, this, &ChatTriggers::OnSayCommand_Post, true);
		SH_REMOVE_HOOK_MEMFUNC(ConCommand, Dispatch, m_pSayTeamCmd, this, &ChatTriggers::OnSayCommand_Pre, false);
	}
	if (m_pSayCmd)
	{
		SH_REMOVE_HOOK_MEMFUNC(ConCommand, Dispatch, m_pSayCmd, this, &ChatTriggers::OnSayCommand_Post, true);
		SH_REMOVE_HOOK_MEMFUNC(ConCommand, Dispatch, m_pSayCmd, this, &ChatTriggers::OnSayCommand_Pre, false);
	}
}

void ChatTriggers::OnSayCommand_Pre()
{
	int client = g_ConCmds.GetCommandClient();
	m_bIsChatTrigger = false;

	/* The server console cannot do this */
	if (client == 0)
	{
		RETURN_META(MRES_IGNORED);
	}

	const char *args = engine->Cmd_Args();
	
	if (!args)
	{
		RETURN_META(MRES_IGNORED);
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
	} else if (m_PrivTriggerSize && strncmp(args, m_PrivTrigger, m_PrivTriggerSize) == 0) {
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
	if (!PreProcessTrigger(engine->PEntityOfEntIndex(client), args, is_quoted))
	{
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

void ChatTriggers::OnSayCommand_Post()
{
	m_bIsChatTrigger = false;
	if (m_bWillProcessInPost)
	{
		/* Reset this for re-entrancy */
		m_bWillProcessInPost = false;
		
		/* Execute the cached command */
		int client = g_ConCmds.GetCommandClient();
		unsigned int old = SetReplyTo(SM_REPLY_CHAT);
		serverpluginhelpers->ClientCommand(engine->PEntityOfEntIndex(client), m_ToExecute);
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
