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

#include "UserMessages.h"
#include "sm_stringutil.h"

UserMessages g_UserMsgs;

SH_DECL_HOOK2(IVEngineServer, UserMessageBegin, SH_NOATTRIB, 0, bf_write *, IRecipientFilter *, int);
SH_DECL_HOOK0_void(IVEngineServer, MessageEnd, SH_NOATTRIB, 0);

UserMessages::UserMessages() : m_InterceptBuffer(m_pBase, 2500)
{
	m_Names = sm_trie_create();
	m_HookCount = 0;
	m_InExec = false;
	m_InHook = false;
	m_CurFlags = 0;
	m_CurId = INVALID_MESSAGE_ID;
}

UserMessages::~UserMessages()
{
	sm_trie_destroy(m_Names);

	CStack<ListenerInfo *>::iterator iter;
	for (iter=m_FreeListeners.begin(); iter!=m_FreeListeners.end(); iter++)
	{
		delete (*iter);
	}
	m_FreeListeners.popall();
}

void UserMessages::OnSourceModStartup(bool late)
{
	/* -1 means SourceMM was unable to get the user message list */
	m_FallbackSearch = (g_SMAPI->GetUserMessageCount() == -1);
}

void UserMessages::OnSourceModAllInitialized()
{
	g_ShareSys.AddInterface(NULL, this);
}

void UserMessages::OnSourceModAllShutdown()
{
	if (m_HookCount)
	{
		SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, UserMessageBegin, engine, this, &UserMessages::OnStartMessage_Pre, false);
		SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, UserMessageBegin, engine, this, &UserMessages::OnStartMessage_Post, true);
		SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, MessageEnd, engine, this, &UserMessages::OnMessageEnd_Pre, false);
		SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, MessageEnd, engine, this, &UserMessages::OnMessageEnd_Post, true);
	}
	m_HookCount = 0;
}

int UserMessages::GetMessageIndex(const char *msg)
{
	int msgid;

	if (!sm_trie_retrieve(m_Names, msg, reinterpret_cast<void **>(&msgid)))
	{
		if (m_FallbackSearch)
		{
			char msgbuf[64];
			int size;
			msgid = 0;

			while (gamedll->GetUserMessageInfo(msgid, msgbuf, sizeof(msgbuf), size))
			{
				if (strcmp(msgbuf, msg) == 0)
				{
					sm_trie_insert(m_Names, msg, reinterpret_cast<void *>(msgid));
					return msgid;
				}
				msgid++;
			}
		}

		msgid = g_SMAPI->FindUserMessage(msg);

		if (msgid != INVALID_MESSAGE_ID)
		{
			sm_trie_insert(m_Names, msg, reinterpret_cast<void *>(msgid));
		}
	}

	return msgid;
}

bool UserMessages::GetMessageName(int msgid, char *buffer, size_t maxlength) const
{
	if (m_FallbackSearch)
	{
		int size;
		return gamedll->GetUserMessageInfo(msgid, buffer, maxlength, size);
	}

	const char *msg = g_SMAPI->GetUserMessage(msgid);

	if (msg)
	{
		strncopy(buffer, msg, maxlength);
		return true;
	}

	return false;
}

bf_write *UserMessages::StartMessage(int msg_id, const cell_t players[], unsigned int playersNum, int flags)
{
	bf_write *buffer;

	if (m_InExec || m_InHook)
	{
		return NULL;
	}
	if (msg_id < 0 || msg_id >= 255)
	{
		return NULL;
	}

	m_CellRecFilter.Initialize(players, playersNum);

	m_CurFlags = flags;
	if (m_CurFlags & USERMSG_INITMSG)
	{
		m_CellRecFilter.SetToInit(true);
	}
	if (m_CurFlags & USERMSG_RELIABLE)
	{
		m_CellRecFilter.SetToReliable(true);
	}

	m_InExec = true;

	if (m_CurFlags & USERMSG_BLOCKHOOKS)
	{
		buffer = ENGINE_CALL(UserMessageBegin)(static_cast<IRecipientFilter *>(&m_CellRecFilter), msg_id);
	} else {
		buffer = engine->UserMessageBegin(static_cast<IRecipientFilter *>(&m_CellRecFilter), msg_id);
	}

	return buffer;
}

bool UserMessages::EndMessage()
{
	if (!m_InExec)
	{
		return false;
	}

	if (m_CurFlags & USERMSG_BLOCKHOOKS)
	{
		ENGINE_CALL(MessageEnd)();
	} else {
		engine->MessageEnd();
	}

	m_InExec = false;
	m_CurFlags = 0;
	m_CellRecFilter.Reset();

	return true;
}

bool UserMessages::HookUserMessage(int msg_id, IUserMessageListener *pListener, bool intercept)
{
	if (msg_id < 0 || msg_id >= 255)
	{
		return false;
	}

	ListenerInfo *pInfo;
	if (m_FreeListeners.empty())
	{
		pInfo = new ListenerInfo;
	} else {
		pInfo = m_FreeListeners.front();
		m_FreeListeners.pop();
	}

	pInfo->Callback = pListener;
	pInfo->IsHooked = false;
	pInfo->KillMe = false;

	if (!m_HookCount++)
	{
		SH_ADD_HOOK_MEMFUNC(IVEngineServer, UserMessageBegin, engine, this, &UserMessages::OnStartMessage_Pre, false);
		SH_ADD_HOOK_MEMFUNC(IVEngineServer, UserMessageBegin, engine, this, &UserMessages::OnStartMessage_Post, true);
		SH_ADD_HOOK_MEMFUNC(IVEngineServer, MessageEnd, engine, this, &UserMessages::OnMessageEnd_Pre, false);
		SH_ADD_HOOK_MEMFUNC(IVEngineServer, MessageEnd, engine, this, &UserMessages::OnMessageEnd_Post, true);
	}

	if (intercept)
	{
		m_msgIntercepts[msg_id].push_back(pInfo);
	} else {
		m_msgHooks[msg_id].push_back(pInfo);
	}

	return true;
}

bool UserMessages::UnhookUserMessage(int msg_id, IUserMessageListener *pListener, bool intercept)
{	
	MsgList *pList;
	MsgIter iter;
	ListenerInfo *pInfo;
	bool deleted = false;

	if (msg_id < 0 || msg_id >= 255)
	{
		return false;
	}

	pList = (intercept) ? &m_msgIntercepts[msg_id] : &m_msgHooks[msg_id];
	for (iter=pList->begin(); iter!=pList->end(); iter++)
	{
		pInfo = (*iter);
		if (pInfo->Callback == pListener)
		{
			if (pInfo->IsHooked)
			{
				pInfo->KillMe = true;
				return true;
			}
			pList->erase(iter);
			deleted = true;
			break;
		}
	}

	if (deleted)
	{
		_DecRefCounter();
	}

	return deleted;
}

void UserMessages::_DecRefCounter()
{
	if (--m_HookCount == 0)
	{
		SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, UserMessageBegin, engine, this, &UserMessages::OnStartMessage_Pre, false);
		SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, UserMessageBegin, engine, this, &UserMessages::OnStartMessage_Post, true);
		SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, MessageEnd, engine, this, &UserMessages::OnMessageEnd_Pre, false);
		SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, MessageEnd, engine, this, &UserMessages::OnMessageEnd_Post, true);
	}
}

bf_write *UserMessages::OnStartMessage_Pre(IRecipientFilter *filter, int msg_type)
{
	bool is_intercept_empty = m_msgIntercepts[msg_type].empty();
	bool is_hook_empty = m_msgHooks[msg_type].empty();

	if ((is_intercept_empty && is_hook_empty)
		|| (m_InExec && (m_CurFlags & USERMSG_BLOCKHOOKS)))
	{
		m_InHook = false;
		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}

	m_CurId = msg_type;
	m_CurRecFilter = filter;
	m_InHook = true;
	m_BlockEndPost = false;

	if (!is_intercept_empty)
	{
		m_InterceptBuffer.Reset();
		RETURN_META_VALUE(MRES_SUPERCEDE, &m_InterceptBuffer);
	}

	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

bf_write *UserMessages::OnStartMessage_Post(IRecipientFilter *filter, int msg_type)
{
	if (!m_InHook)
	{
		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}

	m_OrigBuffer = META_RESULT_ORIG_RET(bf_write *);

	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

void UserMessages::OnMessageEnd_Post()
{
	if (!m_InHook || m_BlockEndPost)
	{
		RETURN_META(MRES_IGNORED);
	}

	MsgList *pList;
	MsgIter iter;
	ListenerInfo *pInfo;

	pList = &m_msgIntercepts[m_CurId];
	for (iter=pList->begin(); iter!=pList->end(); )
	{
		pInfo = (*iter);
		pInfo->IsHooked = true;
		pInfo->Callback->OnUserMessageSent(m_CurId);

		if (pInfo->KillMe)
		{
			iter = pList->erase(iter);
			m_FreeListeners.push(pInfo);
			_DecRefCounter();
			continue;
		}

		pInfo->IsHooked = false;
		iter++;
	}

	m_InHook = false;

	pList = &m_msgHooks[m_CurId];
	for (iter=pList->begin(); iter!=pList->end(); )
	{
		pInfo = (*iter);
		pInfo->IsHooked = true;
		pInfo->Callback->OnUserMessageSent(m_CurId);

		if (pInfo->KillMe)
		{
			iter = pList->erase(iter);
			m_FreeListeners.push(pInfo);
			_DecRefCounter();
			continue;
		}

		pInfo->IsHooked = false;
		iter++;
	}
}

void UserMessages::OnMessageEnd_Pre()
{
	if (!m_InHook)
	{
		RETURN_META(MRES_IGNORED);
	}

	MsgList *pList;
	MsgIter iter;
	ListenerInfo *pInfo;

	ResultType res;
	bool intercepted = false;
	bool handled = false;

	pList = &m_msgIntercepts[m_CurId];
	for (iter=pList->begin(); iter!=pList->end(); )
	{
		pInfo = (*iter);
		pInfo->IsHooked = true;
		res = pInfo->Callback->InterceptUserMessage(m_CurId, &m_InterceptBuffer, m_CurRecFilter);

		intercepted = true;

		switch (res)
		{
		case Pl_Stop:
			{
				if (pInfo->KillMe)
				{
					pList->erase(iter);
					m_FreeListeners.push(pInfo);
					_DecRefCounter();
					goto supercede;
				}
				pInfo->IsHooked = false;
				goto supercede;
			}
		case Pl_Handled:
			{
				handled = true;
				if (pInfo->KillMe)
				{
					iter = pList->erase(iter);
					m_FreeListeners.push(pInfo);
					_DecRefCounter();
					continue;
				}
				break;
			}
		default:
			{
				if (pInfo->KillMe)
				{
					iter = pList->erase(iter);
					m_FreeListeners.push(pInfo);
					_DecRefCounter();
					continue;
				}
				break;
			}
		}
		pInfo->IsHooked = false;
		iter++;
	}

	if (handled)
	{
		goto supercede;
	}

	if (intercepted)
	{
		bf_write *engine_bfw;

		engine_bfw = ENGINE_CALL(UserMessageBegin)(m_CurRecFilter, m_CurId);
		m_ReadBuffer.StartReading(m_InterceptBuffer.GetBasePointer(), m_InterceptBuffer.GetNumBytesWritten());
		engine_bfw->WriteBitsFromBuffer(&m_ReadBuffer, m_InterceptBuffer.GetNumBitsWritten());
		ENGINE_CALL(MessageEnd)();
	}

	pList = &m_msgHooks[m_CurId];
	for (iter=pList->begin(); iter!=pList->end(); )
	{
		pInfo = (*iter);
		pInfo->IsHooked = true;
		pInfo->Callback->OnUserMessage(m_CurId, m_OrigBuffer, m_CurRecFilter);

		if (pInfo->KillMe)
		{
			iter = pList->erase(iter);
			m_FreeListeners.push(pInfo);
			_DecRefCounter();
			continue;
		}

		pInfo->IsHooked = false;
		iter++;
	}

	RETURN_META((intercepted) ? MRES_SUPERCEDE : MRES_IGNORED);
supercede:
	m_InHook = false;
	m_BlockEndPost = true;
	RETURN_META(MRES_SUPERCEDE);
}
