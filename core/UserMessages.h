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

#ifndef _INCLUDE_SOURCEMOD_CUSERMESSAGES_H_
#define _INCLUDE_SOURCEMOD_CUSERMESSAGES_H_

#include <IUserMessages.h>
#include "sourcemm_api.h"
#include <sm_stringhashmap.h>
#include "sm_stringutil.h"
#include "CellRecipientFilter.h"
#include "sm_globals.h"
#include <sh_list.h>
#include <sh_stack.h>

using namespace SourceHook;
using namespace SourceMod;

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
#define USE_PROTOBUF_USERMESSAGES
#endif

#ifdef USE_PROTOBUF_USERMESSAGES
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <netmessages.pb.h>
#include "pb_handle.h"

using namespace google;
#else
#include <bitbuf.h>
#endif

#define INVALID_MESSAGE_ID -1

struct ListenerInfo
{
#ifdef USE_PROTOBUF_USERMESSAGES
	IProtobufUserMessageListener *Callback;
#else
	IBitBufUserMessageListener *Callback;
#endif
	bool IsHooked;
	bool KillMe;
	bool IsNew;
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
	bf_write *StartBitBufMessage(int msg_id, const cell_t players[], unsigned int playersNum, int flags);
	google::protobuf::Message *StartProtobufMessage(int msg_id, const cell_t players[], unsigned int playersNum, int flags);
	bool EndMessage();
	bool HookUserMessage2(int msg_id,
		IUserMessageListener *pListener,
		bool intercept=false);
	bool UnhookUserMessage2(int msg_id,
		IUserMessageListener *pListener,
		bool intercept=false);
	UserMessageType GetUserMessageType() const;
public:
#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	void OnSendUserMessage_Pre(IRecipientFilter &filter, int msg_type, const protobuf::Message &msg);
	void OnSendUserMessage_Post(IRecipientFilter &filter, int msg_type, const protobuf::Message &msg);
#endif

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	PbHandle OnStartMessage_Pre(IRecipientFilter *filter, int msg_type, const char *msg_name);
	void* OnStartMessage_Post(IRecipientFilter *filter, int msg_type, const char *msg_name);
#elif SOURCE_ENGINE >= SE_LEFT4DEAD
	bf_write *OnStartMessage_Pre(IRecipientFilter *filter, int msg_type, const char *msg_name);
	bf_write *OnStartMessage_Post(IRecipientFilter *filter, int msg_type, const char *msg_name);
#else
	bf_write *OnStartMessage_Pre(IRecipientFilter *filter, int msg_type);
	bf_write *OnStartMessage_Post(IRecipientFilter *filter, int msg_type);
#endif
	void OnMessageEnd_Pre();
	void OnMessageEnd_Post();
private:
#ifdef USE_PROTOBUF_USERMESSAGES
	bool InternalHook(int msg_id, IProtobufUserMessageListener *pListener, bool intercept, bool isNew);
	bool InternalUnhook(int msg_id, IProtobufUserMessageListener *pListener, bool intercept, bool isNew);
#else
	bool InternalHook(int msg_id, IBitBufUserMessageListener *pListener, bool intercept, bool isNew);
	bool InternalUnhook(int msg_id, IBitBufUserMessageListener *pListener, bool intercept, bool isNew);
#endif
	void _DecRefCounter();
private:
	List<ListenerInfo *> m_msgHooks[255];
	List<ListenerInfo *> m_msgIntercepts[255];
	CStack<ListenerInfo *> m_FreeListeners;
	IRecipientFilter *m_CurRecFilter;
#ifndef USE_PROTOBUF_USERMESSAGES
	unsigned char m_pBase[2500];
	bf_write m_InterceptBuffer;
	bf_write *m_OrigBuffer;
	bf_read m_ReadBuffer;
#else
	// The engine used to provide this. Now we track it.
	PbHandle m_OrigBuffer;
	PbHandle m_FakeEngineBuffer;
	META_RES m_FakeMetaRes;

	PbHandle m_InterceptBuffer;
#endif
	size_t m_HookCount;
	bool m_InHook;
	bool m_BlockEndPost;
#ifndef USE_PROTOBUF_USERMESSAGES
	bool m_FallbackSearch;

	StringHashMap<int> m_Names;
#endif
	CellRecipientFilter m_CellRecFilter;
	bool m_InExec;
	int m_CurFlags;
	int m_CurId;
};

extern UserMessages g_UserMsgs;

#endif //_INCLUDE_SOURCEMOD_CUSERMESSAGES_H_
