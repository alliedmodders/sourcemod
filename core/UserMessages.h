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

#ifndef _INCLUDE_SOURCEMOD_CUSERMESSAGES_H_
#define _INCLUDE_SOURCEMOD_CUSERMESSAGES_H_

#include "ShareSys.h"
#include <IUserMessages.h>
#include "sourcemm_api.h"
#include "sm_trie.h"
#include "CellRecipientFilter.h"

using namespace SourceHook;
using namespace SourceMod;

#define INVALID_MESSAGE_ID -1

struct ListenerInfo
{
	IUserMessageListener *Callback;
	bool IsHooked;
	bool KillMe;
};

typedef List<ListenerInfo *> MsgList;
typedef List<ListenerInfo *>::iterator MsgIter;

class UserMessages : 
	public IUserMessages,
	public SMGlobalClass
{
public:
	UserMessages();
	~UserMessages();
public: //SMGlobalClass
	void OnSourceModStartup(bool late);
	void OnSourceModAllInitialized();
	void OnSourceModAllShutdown();
public: //IUserMessages
	int GetMessageIndex(const char *msg);
	bool GetMessageName(int msgid, char *buffer, size_t maxlength) const;
	bool HookUserMessage(int msg_id, IUserMessageListener *pListener, bool intercept=false);
	bool UnhookUserMessage(int msg_id, IUserMessageListener *pListener, bool intercept=false);
	bf_write *StartMessage(int msg_id, const cell_t players[], unsigned int playersNum, int flags);
	bool EndMessage();
public:
#if SOURCE_ENGINE == SE_LEFT4DEAD
	bf_write *OnStartMessage_Pre(IRecipientFilter *filter, int msg_type, const char *msg_name);
	bf_write *OnStartMessage_Post(IRecipientFilter *filter, int msg_type, const char *msg_name);
#else
	bf_write *OnStartMessage_Pre(IRecipientFilter *filter, int msg_type);
	bf_write *OnStartMessage_Post(IRecipientFilter *filter, int msg_type);
#endif
	void OnMessageEnd_Pre();
	void OnMessageEnd_Post();
private:
	void _DecRefCounter();
private:
	List<ListenerInfo *> m_msgHooks[255];
	List<ListenerInfo *> m_msgIntercepts[255];
	CStack<ListenerInfo *> m_FreeListeners;
	unsigned char m_pBase[2500];
	IRecipientFilter *m_CurRecFilter;
	bf_write m_InterceptBuffer;
	bf_write *m_OrigBuffer;
	bf_read m_ReadBuffer;
	size_t m_HookCount;
	bool m_InHook;
	bool m_BlockEndPost;
	bool m_FallbackSearch;

	Trie *m_Names;
	CellRecipientFilter m_CellRecFilter;
	bool m_InExec;
	int m_CurFlags;
	int m_CurId;
};

extern UserMessages g_UserMsgs;

#endif //_INCLUDE_SOURCEMOD_CUSERMESSAGES_H_
