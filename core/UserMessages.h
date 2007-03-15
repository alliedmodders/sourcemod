/**
* vim: set ts=4 :
* ===============================================================
* SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
* ===============================================================
*
* This file is not open source and may not be copied without explicit
* written permission of AlliedModders LLC.  This file may not be redistributed 
* in whole or significant part.
* For information, see LICENSE.txt or http://www.sourcemod.net/license.php
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
	void OnSourceModAllInitialized();
	void OnSourceModAllShutdown();
public: //IUserMessages
	int GetMessageIndex(const char *msg);
	bool GetMessageName(int msgid, char *buffer, size_t maxlen) const;
	bool HookUserMessage(int msg_id, IUserMessageListener *pListener, bool intercept=false);
	bool UnhookUserMessage(int msg_id, IUserMessageListener *pListener, bool intercept=false);
	bf_write *StartMessage(int msg_id, cell_t players[], unsigned int playersNum, int flags);
	bool EndMessage();
public:
	bf_write *OnStartMessage_Pre(IRecipientFilter *filter, int msg_type);
	bf_write *OnStartMessage_Post(IRecipientFilter *filter, int msg_type);
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

	Trie *m_Names;
	CellRecipientFilter m_CellRecFilter;
	bool m_InExec;
	int m_CurFlags;
	int m_CurId;
};

extern UserMessages g_UserMsgs;

#endif //_INCLUDE_SOURCEMOD_CUSERMESSAGES_H_
