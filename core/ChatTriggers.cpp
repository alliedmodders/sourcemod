/**
 * vim: set ts=4 sw=4 tw=99 noet :
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

#include <memory>

#include <ITextParsers.h>
#include "ChatTriggers.h"
#include "sm_stringutil.h"
#include "ConCmdManager.h"
#include "PlayerManager.h"
#include "HalfLife2.h"
#include "logic_bridge.h"
#include "sourcemod.h"
#include "provider.h"
#include <bridge/include/ILogger.h>
#include <amtl/am-string.h>

ChatTriggers g_ChatTriggers;
bool g_bSupressSilentFails = false;

ChatTriggers::ChatTriggers() : m_bWillProcessInPost(false),
	m_ReplyTo(SM_REPLY_CONSOLE), m_ArgSBackup(NULL)
{
	m_PubTrigger = "!";
	m_PrivTrigger = "/";
	m_bIsChatTrigger = false;
	m_bPluginIgnored = true;
#if SOURCE_ENGINE == SE_EPISODEONE
	m_bIsINS = false;
#endif
}

ChatTriggers::~ChatTriggers()
{
	delete [] m_ArgSBackup;
	m_ArgSBackup = NULL;
}

void ChatTriggers::SetChatTrigger(ChatTriggerType type, const char *value)
{
	std::unique_ptr<char[]> filtered(new char[strlen(value) + 1]);

	const char *src = value;
	char *dest = filtered.get();
	char c;
	while ((c = *src++) != '\0') {
		if (c <= ' ' || c == '"' || c == '\'' || (c >= '0' && c <= '9') || c == ';' || (c >= 'A' && c <= 'Z') || c == '\\' || (c >= 'a' && c <= 'z') || c >= 0x7F) {
			logger->LogError("Ignoring %s chat trigger character '%c', not in valid set: %s", (type == ChatTrigger_Private ? "silent" : "public"), c, "!#$%&()*+,-./:<=>?@[]^_`{|}~");
			continue;
		}
		*dest++ = c;
	}
	*dest = '\0';

	if (type == ChatTrigger_Private) {
		m_PrivTrigger = filtered.get();
	} else {
		m_PubTrigger = filtered.get();
	}
}

ConfigResult ChatTriggers::OnSourceModConfigChanged(const char *key,
													const char *value,
													ConfigSource source,
													char *error,
													size_t maxlength)
{
	if (strcmp(key, "PublicChatTrigger") == 0)
	{
		SetChatTrigger(ChatTrigger_Public, value);
		return ConfigResult_Accept;
	}
	else if (strcmp(key, "SilentChatTrigger") == 0)
	{
		SetChatTrigger(ChatTrigger_Private, value);
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
	ConCommand *say_team = FindCommand("say_team");

	CommandHook::Callback pre_hook = [this] (int client, const ICommandArgs *args) -> bool {
		return this->OnSayCommand_Pre(client, args);
	};
	CommandHook::Callback post_hook = [this] (int client, const ICommandArgs *args) -> bool {
		return this->OnSayCommand_Post(client, args);
	};

	if (ConCommand *say = FindCommand("say")) {
		hooks_.push_back(sCoreProviderImpl.AddCommandHook(say, pre_hook));
		hooks_.push_back(sCoreProviderImpl.AddPostCommandHook(say, post_hook));
	}
	if (ConCommand *say_team = FindCommand("say_team")) {
		hooks_.push_back(sCoreProviderImpl.AddCommandHook(say_team, pre_hook));
		hooks_.push_back(sCoreProviderImpl.AddPostCommandHook(say_team, post_hook));
	}

#if SOURCE_ENGINE == SE_EPISODEONE
	m_bIsINS = (strcmp(g_SourceMod.GetGameFolderName(), "insurgency") == 0);
	if (m_bIsINS) {
		if (ConCommand *say2 = FindCommand("say2")) {
			hooks_.push_back(sCoreProviderImpl.AddCommandHook(say2, pre_hook));
			hooks_.push_back(sCoreProviderImpl.AddPostCommandHook(say2, post_hook));
		}
	}
#elif SOURCE_ENGINE == SE_NUCLEARDAWN
	if (ConCommand *say_squad = FindCommand("say_squad")) {
		hooks_.push_back(sCoreProviderImpl.AddCommandHook(say_squad, pre_hook));
		hooks_.push_back(sCoreProviderImpl.AddPostCommandHook(say_squad, post_hook));
	}
#endif
}

void ChatTriggers::OnSourceModShutdown()
{
	hooks_.clear();

	forwardsys->ReleaseForward(m_pShouldFloodBlock);
	forwardsys->ReleaseForward(m_pDidFloodBlock);
	forwardsys->ReleaseForward(m_pOnClientSayCmd);
	forwardsys->ReleaseForward(m_pOnClientSayCmd_Post);
}

bool ChatTriggers::OnSayCommand_Pre(int client, const ICommandArgs *command)
{
	m_bIsChatTrigger = false;
	m_bWasFloodedMessage = false;
	m_bPluginIgnored = true;

	const char *args = command->ArgS();

	if (!args)
		return false;

	/* Save these off for post hook as the command data returned from the engine in older engine versions
	 * can be NULL, despite the data still being there and valid. */
	m_Arg0Backup = command->Arg(0);
	size_t len = strlen(args);

#if SOURCE_ENGINE == SE_EPISODEONE
	if (m_bIsINS)
	{
		if (strcmp(m_Arg0Backup, "say2") == 0 && len >= 4)
		{
			args += 4;
			len -= 4;
		}

		if (len == 0)
			return true;
	}
#endif

	/* The first pair of quotes are stripped from client say commands, but not console ones.
	 * We do not want the forwards to differ from what is displayed.
	 * So only strip the first pair of quotes from client say commands. */
	bool is_quoted = false;

	if (
#if SOURCE_ENGINE == SE_EPISODEONE
		!m_bIsINS &&
#endif
		client != 0 && args[0] == '"' && args[len-1] == '"')
	{
		/* The server normally won't display empty say commands, but in this case it does.
		 * I don't think it's desired so let's block it. */
		if (len <= 2)
			return true;

		args++;
		len--;
		is_quoted = true;
	}

	/* Some? engines strip the last quote when printing the string to chat.
	 * This results in having a double-quoted message passed to the OnClientSayCommand ("message") forward,
	 * but losing the last quote in the OnClientSayCommand_Post ("message) forward.
	 * To compensate this, we copy the args into our own buffer where the engine won't mess with
	 * and strip the quotes. */
	delete [] m_ArgSBackup;
	m_ArgSBackup = new char[CCommand::MaxCommandLength()+1];
	memcpy(m_ArgSBackup, args, len+1);

	/* Strip the quotes from the argument */
	if (is_quoted)
	{
		if (m_ArgSBackup[len-1] == '"')
		{
			m_ArgSBackup[--len] = '\0';
		}
	}

	/* The server console cannot do this */
	if (client == 0)
	{
		if (CallOnClientSayCommand(client) >= Pl_Handled)
			return true;

		return false;
	}

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);

	/* We guarantee the client is connected */
	if (!pPlayer || !pPlayer->IsConnected())
		return false;

	/* Check if we need to block this message from being sent */
	if (ClientIsFlooding(client))
	{
		char buffer[128];

		if (!logicore.CoreTranslate(buffer, sizeof(buffer), "%T", 2, NULL, "Flooding the server", &client))
			ke::SafeSprintf(buffer, sizeof(buffer), "You are flooding the server!");

		/* :TODO: we should probably kick people who spam too much. */

		char fullbuffer[192];
		ke::SafeSprintf(fullbuffer, sizeof(fullbuffer), "[SM] %s", buffer);
		g_HL2.TextMsg(client, HUD_PRINTTALK, fullbuffer);

		m_bWasFloodedMessage = true;
		return true;
	}

	bool is_trigger = false;
	bool is_silent = false;

	// Prefer the silent trigger in case of clashes.
	if (strchr(m_PrivTrigger.c_str(), m_ArgSBackup[0])) {
		is_trigger = true;
		is_silent = true;
	} else if (strchr(m_PubTrigger.c_str(), m_ArgSBackup[0])) {
		is_trigger = true;
	}

	if (is_trigger) {
		// Bump the args past the chat trigger - we only support single-character triggers now.
		args = &m_ArgSBackup[1];
	}

	/**
	 * Test if this is actually a command!
	 */
	if (is_trigger && PreProcessTrigger(PEntityOfEntIndex(client), args))
	{
		m_bIsChatTrigger = true;

		/**
		 * We'll execute it in post.
		 */
		m_bWillProcessInPost = true;
	}

	if (is_silent && (m_bIsChatTrigger || (g_bSupressSilentFails && pPlayer->GetAdminId() != INVALID_ADMIN_ID)))
		return true;

	if (CallOnClientSayCommand(client) >= Pl_Handled)
		return true;

	/* Otherwise, let the command continue */
	return false;
}

bool ChatTriggers::OnSayCommand_Post(int client, const ICommandArgs *command)
{
	if (m_bWillProcessInPost)
	{
		/* Reset this for re-entrancy */
		m_bWillProcessInPost = false;

		/* Execute the cached command */
		unsigned int old = SetReplyTo(SM_REPLY_CHAT);
		serverpluginhelpers->ClientCommand(PEntityOfEntIndex(client), m_ToExecute);
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
	return false;
}

bool ChatTriggers::PreProcessTrigger(edict_t *pEdict, const char *args)
{
	/* Extract a command. This is kind of sloppy. */
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
		if (strncasecmp(cmd_buf, "sm_", 3) == 0)
		{
			return false;
		}

		/* Now, prepend.  Don't worry about the buffers.  This will
		 * work because the sizes are limited from earlier.
		 */
		char new_buf[80];
		strcpy(new_buf, "sm_");
		ke::SafeStrcpy(&new_buf[3], sizeof(new_buf)-3, cmd_buf);

		/* Recheck */
		if (!g_ConCmds.LookForSourceModCommand(new_buf))
		{
			return false;
		}

		prepended = true;
	}

	/* See if we need to do extra string manipulation */
	if (prepended)
	{
		ke::SafeSprintf(m_ToExecute, sizeof(m_ToExecute), "sm_%s", args);
	} else {
		ke::SafeStrcpy(m_ToExecute, sizeof(m_ToExecute), args);
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
