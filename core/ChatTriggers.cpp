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
#include "sourcemod.h"

#if SOURCE_ENGINE == SE_DOTA
SH_DECL_EXTERN2_void(ConCommand, Dispatch, SH_NOATTRIB, false, const CCommandContext &, const CCommand &);
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
	m_ReplyTo(SM_REPLY_CONSOLE)
{
	m_PubTrigger = sm_strdup("!");
	m_PrivTrigger = sm_strdup("/");
	m_PubTriggerSize = 1;
	m_PrivTriggerSize = 1;
	m_bIsChatTrigger = false;
	m_bPluginIgnored = true;
#if SOURCE_ENGINE == SE_EPISODEONE
	m_bIsINS = false;
#endif
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
	m_pShouldFloodBlock = forwardsys->CreateForward("OnClientFloodCheck", ET_Event, 1, NULL, Param_Cell);
	m_pDidFloodBlock = forwardsys->CreateForward("OnClientFloodResult", ET_Event, 2, NULL, Param_Cell, Param_Cell);
	m_pOnClientSayCmd = forwardsys->CreateForward("OnClientSayCommand", ET_Event, 3, NULL, Param_Cell, Param_String, Param_String);
	m_pOnClientSayCmd_Post = forwardsys->CreateForward("OnClientSayCommand_Post", ET_Ignore, 3, NULL, Param_Cell, Param_String, Param_String);
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

#if SOURCE_ENGINE == SE_EPISODEONE
	m_bIsINS = (strcmp(g_SourceMod.GetGameFolderName(), "insurgency") == 0);

	if (m_bIsINS)
	{
		m_pSay2Cmd = FindCommand("say2");
		if (m_pSay2Cmd)
		{
			SH_ADD_HOOK(ConCommand, Dispatch, m_pSay2Cmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Pre), false);
			SH_ADD_HOOK(ConCommand, Dispatch, m_pSay2Cmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Post), true);
		}
	}
#elif SOURCE_ENGINE == SE_NUCLEARDAWN
	m_pSaySquadCmd = FindCommand("say_squad");

	if (m_pSaySquadCmd)
	{
		SH_ADD_HOOK(ConCommand, Dispatch, m_pSaySquadCmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Pre), false);
		SH_ADD_HOOK(ConCommand, Dispatch, m_pSaySquadCmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Post), true);
	}
#endif
}

void ChatTriggers::OnSourceModShutdown()
{
	if (m_pSayCmd)
	{
		SH_REMOVE_HOOK(ConCommand, Dispatch, m_pSayCmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Post), true);
		SH_REMOVE_HOOK(ConCommand, Dispatch, m_pSayCmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Pre), false);
	}

	if (m_pSayTeamCmd)
	{
		SH_REMOVE_HOOK(ConCommand, Dispatch, m_pSayTeamCmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Post), true);
		SH_REMOVE_HOOK(ConCommand, Dispatch, m_pSayTeamCmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Pre), false);
	}

#if SOURCE_ENGINE == SE_EPISODEONE
	if (m_bIsINS && m_pSay2Cmd)
	{
		SH_REMOVE_HOOK(ConCommand, Dispatch, m_pSay2Cmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Pre), false);
		SH_REMOVE_HOOK(ConCommand, Dispatch, m_pSay2Cmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Post), true);
	}
#elif SOURCE_ENGINE == SE_NUCLEARDAWN
	if (m_pSaySquadCmd)
	{
		SH_REMOVE_HOOK(ConCommand, Dispatch, m_pSaySquadCmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Pre), false);
		SH_REMOVE_HOOK(ConCommand, Dispatch, m_pSaySquadCmd, SH_MEMBER(this, &ChatTriggers::OnSayCommand_Post), true);
	}
#endif

	forwardsys->ReleaseForward(m_pShouldFloodBlock);
	forwardsys->ReleaseForward(m_pDidFloodBlock);
	forwardsys->ReleaseForward(m_pOnClientSayCmd);
	forwardsys->ReleaseForward(m_pOnClientSayCmd_Post);
}

#if SOURCE_ENGINE == SE_DOTA
void ChatTriggers::OnSayCommand_Pre(const CCommandContext &context, const CCommand &command)
{
#elif SOURCE_ENGINE >= SE_ORANGEBOX
void ChatTriggers::OnSayCommand_Pre(const CCommand &command)
{
#else
void ChatTriggers::OnSayCommand_Pre()
{
	CCommand command;
#endif
	int client = g_ConCmds.GetCommandClient();
	m_bIsChatTrigger = false;
	m_bWasFloodedMessage = false;
	m_bPluginIgnored = true;

	const char *args = command.ArgS();

	if (!args)
	{
		RETURN_META(MRES_IGNORED);
	}

	/* Save these off for post hook as the command data returned from the engine in older engine versions 
	 * can be NULL, despite the data still being there and valid. */
	m_Arg0Backup = command.Arg(0);
	m_ArgSBackup = command.ArgS();

	/* The server console cannot do this */
	if (client == 0)
	{
		if (CallOnClientSayCommand(client) >= Pl_Handled)
		{
			RETURN_META(MRES_SUPERCEDE);
		}

		RETURN_META(MRES_IGNORED);
	}

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);

	/* We guarantee the client is connected */
	if (!pPlayer || !pPlayer->IsConnected())
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

#if SOURCE_ENGINE == SE_EPISODEONE
	if (m_bIsINS && strcmp(m_Arg0Backup, "say2") == 0 && strlen(args) >= 4)
	{
		args += 4;
	}
#endif

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

	/**
	 * Test if this is actually a command!
	 */
	if (is_trigger && PreProcessTrigger(PEntityOfEntIndex(client), args, is_quoted))
	{
		m_bIsChatTrigger = true;

		/**
		 * We'll execute it in post.
		 */
		m_bWillProcessInPost = true;
	}

	if (is_silent && (m_bIsChatTrigger || (g_bSupressSilentFails && pPlayer->GetAdminId() != INVALID_ADMIN_ID)))
	{
		RETURN_META(MRES_SUPERCEDE);
	}

	if (CallOnClientSayCommand(client) >= Pl_Handled)
	{
		RETURN_META(MRES_SUPERCEDE);
	}

	/* Otherwise, let the command continue */
	RETURN_META(MRES_IGNORED);
}

#if SOURCE_ENGINE == SE_DOTA
void ChatTriggers::OnSayCommand_Post(const CCommandContext &context, const CCommand &command)
#elif SOURCE_ENGINE >= SE_ORANGEBOX
void ChatTriggers::OnSayCommand_Post(const CCommand &command)
#else
void ChatTriggers::OnSayCommand_Post()
#endif
{
	int client = g_ConCmds.GetCommandClient();

	if (m_bWillProcessInPost)
	{
		/* Reset this for re-entrancy */
		m_bWillProcessInPost = false;

		/* Execute the cached command */
		unsigned int old = SetReplyTo(SM_REPLY_CHAT);
#if SOURCE_ENGINE == SE_DOTA
		engine->ClientCommand(client, "%s", m_ToExecute);
#else
		serverpluginhelpers->ClientCommand(PEntityOfEntIndex(client), m_ToExecute);
#endif
		SetReplyTo(old);
	}

	if (!m_bPluginIgnored && m_pOnClientSayCmd_Post->GetFunctionCount() != 0)
	{
		m_pOnClientSayCmd_Post->PushCell(client);
		m_pOnClientSayCmd_Post->PushString(m_Arg0Backup);
		m_pOnClientSayCmd_Post->PushString(m_ArgSBackup);
		m_pOnClientSayCmd_Post->Execute(NULL);
	}

	m_bIsChatTrigger = false;
	m_bWasFloodedMessage = false;
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

cell_t ChatTriggers::CallOnClientSayCommand(int client)
{
	cell_t res = Pl_Continue;
	if (m_pOnClientSayCmd->GetFunctionCount() != 0)
	{
		m_pOnClientSayCmd->PushCell(client);
		m_pOnClientSayCmd->PushString(m_Arg0Backup);
		m_pOnClientSayCmd->PushString(m_ArgSBackup);
		m_pOnClientSayCmd->Execute(&res);
	}

	m_bPluginIgnored = (res >= Pl_Stop);

	return res;
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
