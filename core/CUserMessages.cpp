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

#include "CUserMessages.h"

CUserMessages g_UserMsgs;

SH_DECL_HOOK2(IVEngineServer, UserMessageBegin, SH_NOATTRIB, 0, bf_write *, IRecipientFilter *, int);
SH_DECL_HOOK0_void(IVEngineServer, MessageEnd, SH_NOATTRIB, 0);

//:TODO: USE NEW MM IFACE TO SEARCH FOR MESSAGE NAMES ETC! faluco--> i'll do this when i have some spare time

CUserMessages::CUserMessages() : m_InterceptBuffer(m_pBase, 2500)
{
	m_Names = sm_trie_create();
	m_HookCount = 0;
	m_InExec = false;
	m_InHook = false;
	m_CurFlags = 0;
	m_CurId = INVALID_MESSAGE_ID;
}

CUserMessages::~CUserMessages()
{
	sm_trie_destroy(m_Names);
}

void CUserMessages::OnSourceModAllInitialized()
{
	g_ShareSys.AddInterface(NULL, this);
}

void CUserMessages::OnSourceModAllShutdown()
{
	if (m_HookCount)
	{
		SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, UserMessageBegin, engine, this, &CUserMessages::OnStartMessage_Pre, false);
		SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, UserMessageBegin, engine, this, &CUserMessages::OnStartMessage_Post, true);
		SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, MessageEnd, engine, this, &CUserMessages::OnMessageEnd_Pre, false);
		SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, MessageEnd, engine, this, &CUserMessages::OnMessageEnd_Post, true);
	}
	m_HookCount = 0;
}

int CUserMessages::GetMessageIndex(const char *msg)
{
	int msgid;
	if (!sm_trie_retrieve(m_Names, msg, reinterpret_cast<void **>(&msgid)))
	{
		char buf[255];
		int dummy;
		msgid = 0;

		while (gamedll->GetUserMessageInfo(msgid, buf, sizeof(buf), dummy))
		{
			if (strcmp(msg, buf) == 0)
			{
				sm_trie_insert(m_Names, msg, reinterpret_cast<void *>(msgid));
				return msgid;
			}
			msgid++;
		}

		return INVALID_MESSAGE_ID;
	}

	return msgid;
}

bool CUserMessages::GetMessageName(int msgid, char *buffer, size_t maxlen) const
{
	int dummy;

	return gamedll->GetUserMessageInfo(msgid, buffer, maxlen, dummy);
}

bf_write *CUserMessages::StartMessage(int msg_id, cell_t players[], unsigned int playersNum, int flags)
{
	bf_write *buffer;
	
	if (m_InExec)
	{
		return NULL;
	}
	if (msg_id < 0 || msg_id >= 255)
	{
		return NULL;
	}

	m_CellRecFilter.SetRecipientPtr(players, playersNum);

	m_CurFlags = flags;
	if (m_CurFlags & USERMSG_INITMSG)
	{
		m_CellRecFilter.SetInitMessage(true);
	}
	if (m_CurFlags & USERMSG_RELIABLE)
	{
		m_CellRecFilter.SetReliable(true);
	}

	m_InExec = true;

	if (m_CurFlags & (USERMSG_PASSTHRU_ALL|USERMSG_PASSTHRU))
	{
		buffer = engine->UserMessageBegin(static_cast<IRecipientFilter *>(&m_CellRecFilter), msg_id);
	} else {
		buffer = ENGINE_CALL(UserMessageBegin)(static_cast<IRecipientFilter *>(&m_CellRecFilter), msg_id);
	}

	return buffer;
}

bool CUserMessages::EndMessage()
{
	if (!m_InExec)
	{
		return false;
	}

	if (m_CurFlags & (USERMSG_PASSTHRU_ALL|USERMSG_PASSTHRU))
	{
		engine->MessageEnd();
	} else {
		ENGINE_CALL(MessageEnd)();
	}

	m_InExec = false;
	m_CurFlags = 0;
	m_CellRecFilter.ResetFilter();

	return true;
}

bool CUserMessages::HookUserMessage(int msg_id, IUserMessageListener *pListener, bool intercept)
{
	if (msg_id < 0 || msg_id >= 255)
	{
		return false;
	}

	if (!m_HookCount++)
	{
		SH_ADD_HOOK_MEMFUNC(IVEngineServer, UserMessageBegin, engine, this, &CUserMessages::OnStartMessage_Pre, false);
		SH_ADD_HOOK_MEMFUNC(IVEngineServer, UserMessageBegin, engine, this, &CUserMessages::OnStartMessage_Post, true);
		SH_ADD_HOOK_MEMFUNC(IVEngineServer, MessageEnd, engine, this, &CUserMessages::OnMessageEnd_Pre, false);
		SH_ADD_HOOK_MEMFUNC(IVEngineServer, MessageEnd, engine, this, &CUserMessages::OnMessageEnd_Post, true);
	}

	if (intercept)
	{
		m_msgIntercepts[msg_id].push_back(pListener);
	} else {
		m_msgHooks[msg_id].push_back(pListener);
	}

	return true;
}

bool CUserMessages::UnhookUserMessage(int msg_id, IUserMessageListener *pListener, bool intercept)
{
	//:TODO: restrict user from unhooking stuff during a message hook to avoid iterator mess
	List<IUserMessageListener *> *lst;
	IUserMessageListener *listener;
	MsgIter iter;
	bool deleted = false;

	if (msg_id < 0 || msg_id >= 255)
	{
		return false;
	}

	lst = (intercept) ? &m_msgIntercepts[msg_id] : &m_msgHooks[msg_id];
	for (iter=lst->begin(); iter!=lst->end(); iter++)
	{
		listener = (*iter);
		if (listener == pListener)
		{
			lst->erase(iter);
			deleted = true;
			break;
		}
	}

	if (deleted && !(--m_HookCount))
	{
		SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, UserMessageBegin, engine, this, &CUserMessages::OnStartMessage_Pre, false);
		SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, UserMessageBegin, engine, this, &CUserMessages::OnStartMessage_Post, true);
		SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, MessageEnd, engine, this, &CUserMessages::OnMessageEnd_Pre, false);
		SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, MessageEnd, engine, this, &CUserMessages::OnMessageEnd_Post, true);
	}

	return (deleted) ? true : false;
}

bf_write *CUserMessages::OnStartMessage_Pre(IRecipientFilter *filter, int msg_type)
{
	bool is_intercept_empty = m_msgIntercepts[msg_type].empty();
	bool is_hook_empty = m_msgHooks[msg_type].empty();

	if (is_intercept_empty && is_hook_empty)
	{
		m_InHook = false;
		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}

	if ((m_InExec) && !(m_CurFlags & USERMSG_PASSTHRU_ALL))
	{
		m_InHook = false;
		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}

	m_CurId = msg_type;
	m_CurRecFilter = filter;
	m_InterceptBuffer.Reset();
	m_InHook = true;

	if (!is_intercept_empty)
	{
		RETURN_META_VALUE(MRES_SUPERCEDE, &m_InterceptBuffer);
	}

	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

bf_write *CUserMessages::OnStartMessage_Post(IRecipientFilter *filter, int msg_type)
{
	if (!m_InHook)
	{
		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}

	m_OrigBuffer = META_RESULT_ORIG_RET(bf_write *);

	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

void CUserMessages::OnMessageEnd_Post()
{
	if (!m_InHook || (META_RESULT_STATUS == MRES_SUPERCEDE))
	{
		RETURN_META(MRES_IGNORED);
	}

	List<IUserMessageListener *> *lst;
	IUserMessageListener *listener;
	MsgIter iter;

	lst = &m_msgIntercepts[m_CurId];
	for (iter=lst->begin(); iter!=lst->end(); iter++)
	{
		listener = (*iter);
		listener->OnUserMessageSent(m_CurId);
	}

	lst = &m_msgHooks[m_CurId];
	for (iter=lst->begin(); iter!=lst->end(); iter++)
	{
		listener = (*iter);
		listener->OnUserMessageSent(m_CurId);
	}
}

void CUserMessages::OnMessageEnd_Pre()
{
	if (!m_InHook)
	{
		RETURN_META(MRES_IGNORED);
	}

	List<IUserMessageListener *> *lst;
	IUserMessageListener *listener;
	bf_write *engine_bfw;
	MsgIter iter;
	ResultType res;
	bool intercepted = false;
	bool handled = false;

	lst = &m_msgIntercepts[m_CurId];
	for (iter=lst->begin(); iter!=lst->end(); iter++)
	{
		listener = (*iter);
		res = listener->InterceptUserMessage(m_CurId, &m_InterceptBuffer, m_CurRecFilter);
		if (res == Pl_Stop)
		{
			goto supercede;
		} else if (res == Pl_Handled) {
			handled = true;
		}
		intercepted = true;
	}

	if (handled)
	{
		goto supercede;
	}

	if (intercepted)
	{
		engine_bfw = ENGINE_CALL(UserMessageBegin)(m_CurRecFilter, m_CurId);
		m_ReadBuffer.StartReading(m_InterceptBuffer.GetBasePointer(), m_InterceptBuffer.GetNumBytesWritten());
		engine_bfw->WriteBitsFromBuffer(&m_ReadBuffer, m_InterceptBuffer.GetNumBitsWritten());
		ENGINE_CALL(MessageEnd)();
		goto supercede;
	}

	lst = &m_msgHooks[m_CurId];
	for (iter=lst->begin(); iter!=lst->end(); iter++)
	{
		listener = (*iter);
		listener->OnUserMessage(m_CurId, m_OrigBuffer, m_CurRecFilter);
	}

	RETURN_META(MRES_IGNORED);
supercede:
	RETURN_META(MRES_SUPERCEDE);
}
