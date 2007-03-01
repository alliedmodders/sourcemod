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

#include "sm_globals.h"
#include <IUserMessages.h>
#include <sh_list.h>
#include "sourcemm_api.h"
#include "sm_trie.h"
#include "CellRecipientFilter.h"

using namespace SourceHook;
using namespace SourceMod;

#define INVALID_MESSAGE_ID -1

typedef List<IUserMessageListener *>::iterator MsgIter;

class CUserMessages : 
	public IUserMessages,
	public SMGlobalClass
{
public:
	CUserMessages();
	~CUserMessages();
public: //SMGlobalClass
	void OnSourceModAllShutdown();
public: //IUserMessages
	int GetMessageIndex(const char *msg);
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
	List<IUserMessageListener *> m_msgHooks[255];
	List<IUserMessageListener *> m_msgIntercepts[255];
	unsigned char m_pBase[2500];
	IRecipientFilter *m_CurRecFilter;
	bf_write m_InterceptBuffer;
	bf_write *m_OrigBuffer;
	bf_read m_ReadBuffer;
	size_t m_HookCount;
	bool m_InHook;

	Trie *m_Names;
	CellRecipientFilter m_CellRecFilter;
	bool m_InExec;
	int m_CurFlags;
	int m_CurId;
};

extern CUserMessages g_UserMsgs;

#endif //_INCLUDE_SOURCEMOD_CUSERMESSAGES_H_
