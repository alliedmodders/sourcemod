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

#ifndef _INCLUDE_SOURCEMOD_CHAT_TRIGGERS_H_
#define _INCLUDE_SOURCEMOD_CHAT_TRIGGERS_H_

#include "sm_globals.h"
#include "sourcemm_api.h"
#include "GameHooks.h"
#include <IGameHelpers.h>
#include <compat_wrappers.h>
#include <IForwardSys.h>
#include <amtl/am-string.h>

class ChatTriggers : public SMGlobalClass
{
public:
	ChatTriggers();
	~ChatTriggers();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModAllInitialized_Post();
	void OnSourceModGameInitialized();
	void OnSourceModShutdown();
	ConfigResult OnSourceModConfigChanged(const char *key,
		const char *value,
		ConfigSource source,
		char *error,
		size_t maxlength);
private: //ConCommand
	bool OnSayCommand_Pre(int client, const ICommandArgs *args);
	bool OnSayCommand_Post(int client, const ICommandArgs *args);
public:
	unsigned int GetReplyTo();
	unsigned int SetReplyTo(unsigned int reply);
	bool IsChatTrigger();
	bool WasFloodedMessage();
private:
	enum ChatTriggerType {
		ChatTrigger_Public,
		ChatTrigger_Private,
	};
	void SetChatTrigger(ChatTriggerType type, const char *value);
	bool PreProcessTrigger(edict_t *pEdict, const char *args);
	bool ClientIsFlooding(int client);
	cell_t CallOnClientSayCommand(int client);
private:
	std::vector<ke::RefPtr<CommandHook>> hooks_;
	std::string m_PubTrigger;
	std::string m_PrivTrigger;
	bool m_bWillProcessInPost;
	bool m_bIsChatTrigger;
	bool m_bWasFloodedMessage;
	bool m_bPluginIgnored;
	unsigned int m_ReplyTo;
	char m_ToExecute[300];
	const char *m_Arg0Backup;
	char *m_ArgSBackup;
	IForward *m_pShouldFloodBlock;
	IForward *m_pDidFloodBlock;
	IForward *m_pOnClientSayCmd;
	IForward *m_pOnClientSayCmd_Post;
#if SOURCE_ENGINE == SE_EPISODEONE
	bool m_bIsINS;
#endif
};

extern ChatTriggers g_ChatTriggers;

#endif //_INCLUDE_SOURCEMOD_CHAT_TRIGGERS_H_
