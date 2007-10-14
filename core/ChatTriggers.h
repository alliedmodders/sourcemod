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

#ifndef _INCLUDE_SOURCEMOD_CHAT_TRIGGERS_H_
#define _INCLUDE_SOURCEMOD_CHAT_TRIGGERS_H_

#include "sm_globals.h"
#include "sourcemm_api.h"
#include <IGameHelpers.h>
#include <compat_wrappers.h>

class ChatTriggers : public SMGlobalClass
{
public:
	ChatTriggers();
	~ChatTriggers();
public: //SMGlobalClass
	void OnSourceModGameInitialized();
	void OnSourceModShutdown();
	ConfigResult OnSourceModConfigChanged(const char *key, 
		const char *value, 
		ConfigSource source,
		char *error, 
		size_t maxlength);
private: //ConCommand
#if defined ORANGEBOX_BUILD
	void OnSayCommand_Pre(const CCommand &command);
	void OnSayCommand_Post(const CCommand &command);
#else
	void OnSayCommand_Pre();
	void OnSayCommand_Post();
#endif
public:
	unsigned int GetReplyTo();
	unsigned int SetReplyTo(unsigned int reply);
	bool IsChatTrigger();
private:
	bool PreProcessTrigger(edict_t *pEdict, const char *args, bool is_quoted);
private:
	ConCommand *m_pSayCmd;
	ConCommand *m_pSayTeamCmd;
	char *m_PubTrigger;
	size_t m_PubTriggerSize;
	char *m_PrivTrigger;
	size_t m_PrivTriggerSize;
	bool m_bWillProcessInPost;
	bool m_bTriggerWasSilent;
	bool m_bIsChatTrigger;
	unsigned int m_ReplyTo;
	char m_ToExecute[300];
};

extern ChatTriggers g_ChatTriggers;

#endif //_INCLUDE_SOURCEMOD_CHAT_TRIGGERS_H_
