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

#include "UserMessages.h"
#include "sm_stringutil.h"
#include "logic_bridge.h"

#if SOURCE_ENGINE == SE_CSGO
#include <cstrike15_usermessage_helpers.h>
#elif SOURCE_ENGINE == SE_BLADE
#include <berimbau_usermessage_helpers.h>
#endif
#include <amtl/am-string.h>

UserMessages g_UserMsgs;

#ifdef USE_PROTOBUF_USERMESSAGES
const protobuf::Message *GetMessagePrototype(int msg_type);
#endif

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
SH_DECL_HOOK3_void(IVEngineServer, SendUserMessage, SH_NOATTRIB, 0, IRecipientFilter &, int, const protobuf::Message &);
#else
#if SOURCE_ENGINE >= SE_LEFT4DEAD
SH_DECL_HOOK3(IVEngineServer, UserMessageBegin, SH_NOATTRIB, 0, bf_write *, IRecipientFilter *, int, const char *);
#else
SH_DECL_HOOK2(IVEngineServer, UserMessageBegin, SH_NOATTRIB, 0, bf_write *, IRecipientFilter *, int);
#endif
SH_DECL_HOOK0_void(IVEngineServer, MessageEnd, SH_NOATTRIB, 0);
#endif // ==SE_CSGO || ==SE_BLADE

UserMessages::UserMessages()
#ifndef USE_PROTOBUF_USERMESSAGES
	: m_InterceptBuffer(m_pBase, 2500)
{
#else
	: m_InterceptBuffer(nullptr)
{
#endif
	m_HookCount = 0;
	m_InExec = false;
	m_InHook = false;
	m_CurFlags = 0;
	m_CurId = INVALID_MESSAGE_ID;
}

UserMessages::~UserMessages()
{
	CStack<ListenerInfo *>::iterator iter;
	for (iter=m_FreeListeners.begin(); iter!=m_FreeListeners.end(); iter++)
	{
		delete (*iter);
	}
	m_FreeListeners.popall();
}

void UserMessages::OnSourceModStartup(bool late)
{
#ifndef USE_PROTOBUF_USERMESSAGES
	/* -1 means SourceMM was unable to get the user message list */
	m_FallbackSearch = (g_SMAPI->GetUserMessageCount() == -1);
#endif
}

void UserMessages::OnSourceModAllInitialized()
{
	sharesys->AddInterface(NULL, this);
}

void UserMessages::OnSourceModAllShutdown()
{
	if (m_HookCount)
	{
#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
		SH_REMOVE_HOOK(IVEngineServer, SendUserMessage, engine, SH_MEMBER(this, &UserMessages::OnSendUserMessage_Pre), false);
		SH_REMOVE_HOOK(IVEngineServer, SendUserMessage, engine, SH_MEMBER(this, &UserMessages::OnSendUserMessage_Post), true);
#else
		SH_REMOVE_HOOK(IVEngineServer, UserMessageBegin, engine, SH_MEMBER(this, &UserMessages::OnStartMessage_Pre), false);
		SH_REMOVE_HOOK(IVEngineServer, UserMessageBegin, engine, SH_MEMBER(this, &UserMessages::OnStartMessage_Post), true);
		SH_REMOVE_HOOK(IVEngineServer, MessageEnd, engine, SH_MEMBER(this, &UserMessages::OnMessageEnd_Pre), false);
		SH_REMOVE_HOOK(IVEngineServer, MessageEnd, engine, SH_MEMBER(this, &UserMessages::OnMessageEnd_Post), true);
#endif
	}
	m_HookCount = 0;
}

int UserMessages::GetMessageIndex(const char *msg)
{
#if SOURCE_ENGINE == SE_CSGO
	// Can split this per engine and/or game later
	return g_Cstrike15UsermessageHelpers.GetIndex(msg);
#elif SOURCE_ENGINE == SE_BLADE
	return g_BerimbauUsermessageHelpers.GetIndex(msg);
#else

	int msgid;
	if (!m_Names.retrieve(msg, &msgid))
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
					m_Names.insert(msg, msgid);
					return msgid;
				}
				msgid++;
			}
		}

		msgid = g_SMAPI->FindUserMessage(msg);

		if (msgid != INVALID_MESSAGE_ID)
			m_Names.insert(msg, msgid);
	}

	return msgid;
#endif
}

bool UserMessages::GetMessageName(int msgid, char *buffer, size_t maxlength) const
{
#ifdef USE_PROTOBUF_USERMESSAGES
#if SOURCE_ENGINE == SE_CSGO
	const char *pszName = g_Cstrike15UsermessageHelpers.GetName(msgid);
#elif SOURCE_ENGINE == SE_BLADE
	const char *pszName = g_BerimbauUsermessageHelpers.GetName(msgid);
#endif
	if (!pszName)
		return false;

	ke::SafeStrcpy(buffer, maxlength, pszName);
	return true;
#else
	if (m_FallbackSearch)
	{
		int size;
		return gamedll->GetUserMessageInfo(msgid, buffer, maxlength, size);
	}

	const char *msg = g_SMAPI->GetUserMessage(msgid);

	if (msg)
	{
		ke::SafeStrcpy(buffer, maxlength, msg);
		return true;
	}

	return false;
#endif
}

bf_write *UserMessages::StartBitBufMessage(int msg_id, const cell_t players[], unsigned int playersNum, int flags)
{
#ifdef USE_PROTOBUF_USERMESSAGES
	return NULL;
#else
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
#if SOURCE_ENGINE >= SE_LEFT4DEAD
		buffer = ENGINE_CALL(UserMessageBegin)(static_cast<IRecipientFilter *>(&m_CellRecFilter), msg_id, g_SMAPI->GetUserMessage(msg_id));
#else
		buffer = ENGINE_CALL(UserMessageBegin)(static_cast<IRecipientFilter *>(&m_CellRecFilter), msg_id);
#endif
	} else {
#if SOURCE_ENGINE >= SE_LEFT4DEAD
		buffer = engine->UserMessageBegin(static_cast<IRecipientFilter *>(&m_CellRecFilter), msg_id, g_SMAPI->GetUserMessage(msg_id));
#else
		buffer = engine->UserMessageBegin(static_cast<IRecipientFilter *>(&m_CellRecFilter), msg_id);
#endif
	}

	return buffer;
#endif // USE_PROTOBUF_USERMESSAGES
}

google::protobuf::Message *UserMessages::StartProtobufMessage(int msg_id, const cell_t players[], unsigned int playersNum, int flags)
{
#ifndef USE_PROTOBUF_USERMESSAGES
	return NULL;
#else
	PbHandle buffer;

	if (m_InExec || m_InHook)
	{
		return NULL;
	}
	if (msg_id < 0 || msg_id >= 255)
	{
		return NULL;
	}

	m_CurId = msg_id;
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
		// direct message creation, return buffer "from engine". keep track
		m_FakeEngineBuffer =
			PbHandle(GetMessagePrototype(msg_id)->New(), PbHandle::Owned, PbHandle::Local);
		buffer = m_FakeEngineBuffer.AsUnowned();
	} else {
		char messageName[32];
		if (!GetMessageName(msg_id, messageName, sizeof(messageName)))
		{
			m_InExec = false;
			return NULL;
		}

		// The message returned here should always local.
		auto msg = OnStartMessage_Pre(static_cast<IRecipientFilter *>(&m_CellRecFilter), msg_id, messageName);
		switch (m_FakeMetaRes)
		{
		case MRES_IGNORED:
		case MRES_HANDLED:
			m_FakeEngineBuffer =
				PbHandle(GetMessagePrototype(msg_id)->New(), PbHandle::Owned, PbHandle::Local);
			buffer = m_FakeEngineBuffer.AsUnowned();
			break;		
		case MRES_OVERRIDE:
			m_FakeEngineBuffer =
				PbHandle(GetMessagePrototype(msg_id)->New(), PbHandle::Owned, PbHandle::Local);
		// fallthrough
		case MRES_SUPERCEDE:
			buffer = std::move(msg);
			break;
		}
		
		OnStartMessage_Post(static_cast<IRecipientFilter *>(&m_CellRecFilter), msg_id, messageName);
	}

	assert(!buffer.is_owned());
	return buffer.GetLocalMessage();
#endif // USE_PROTOBUF_USERMESSAGES
}

bool UserMessages::EndMessage()
{
	if (!m_InExec)
	{
		return false;
	}

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	PbHandle localBuffer = std::move(m_FakeEngineBuffer);
	if (m_CurFlags & USERMSG_BLOCKHOOKS)
	{
		PbHandle priv = localBuffer.ToPrivate(m_CurId);
		ENGINE_CALL(SendUserMessage)(static_cast<IRecipientFilter &>(m_CellRecFilter), m_CurId,
		                             *priv.GetPrivateMessage());
	} else {
		OnMessageEnd_Pre();

		switch (m_FakeMetaRes)
		{
		case MRES_IGNORED:
		case MRES_HANDLED:
		case MRES_OVERRIDE:
		{
			PbHandle priv = localBuffer.ToPrivate(m_CurId);
			engine->SendUserMessage(static_cast<IRecipientFilter &>(m_CellRecFilter), m_CurId,
			                        *priv.GetPrivateMessage());
			break;
		}
		//case MRES_SUPERCEDE:
		}

		OnMessageEnd_Post();
	}
#else
	if (m_CurFlags & USERMSG_BLOCKHOOKS)
	{
		ENGINE_CALL(MessageEnd)();
	} else {
		engine->MessageEnd();
	}
#endif // SE_CSGO || SE_BLADE

	m_InExec = false;
	m_CurFlags = 0;
	m_CellRecFilter.Reset();

	return true;
}

UserMessageType UserMessages::GetUserMessageType() const
{
#ifdef USE_PROTOBUF_USERMESSAGES
	return UM_Protobuf;
#else
	return UM_BitBuf;
#endif
}

bool UserMessages::HookUserMessage2(int msg_id,
									IUserMessageListener *pListener,
									bool intercept)
{
#ifdef USE_PROTOBUF_USERMESSAGES
	return InternalHook(msg_id, (IProtobufUserMessageListener *)pListener, intercept, true);
#else
	return InternalHook(msg_id, (IBitBufUserMessageListener *)pListener, intercept, true);
#endif
}

bool UserMessages::UnhookUserMessage2(int msg_id,
									  IUserMessageListener *pListener,
									  bool intercept)
{
#ifdef USE_PROTOBUF_USERMESSAGES
	return InternalUnhook(msg_id, (IProtobufUserMessageListener *)pListener, intercept, true);
#else
	return InternalUnhook(msg_id, (IBitBufUserMessageListener *)pListener, intercept, true);
#endif
}

bool UserMessages::HookUserMessage(int msg_id, IUserMessageListener *pListener, bool intercept)
{
#ifdef USE_PROTOBUF_USERMESSAGES
	return InternalHook(msg_id, (IProtobufUserMessageListener *)pListener, intercept, false);
#else
	return InternalHook(msg_id, (IBitBufUserMessageListener *)pListener, intercept, false);
#endif
}

bool UserMessages::UnhookUserMessage(int msg_id, IUserMessageListener *pListener, bool intercept)
{	
#ifdef USE_PROTOBUF_USERMESSAGES
	return InternalUnhook(msg_id, (IProtobufUserMessageListener *)pListener, intercept, false);
#else
	return InternalUnhook(msg_id, (IBitBufUserMessageListener *)pListener, intercept, false);
#endif
}

#ifdef USE_PROTOBUF_USERMESSAGES
bool UserMessages::InternalHook(int msg_id, IProtobufUserMessageListener *pListener, bool intercept, bool isNew)
#else
bool UserMessages::InternalHook(int msg_id, IBitBufUserMessageListener *pListener, bool intercept, bool isNew)
#endif
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
	pInfo->IsNew = isNew;

	if (!m_HookCount++)
	{
#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
		SH_ADD_HOOK(IVEngineServer, SendUserMessage, engine, SH_MEMBER(this, &UserMessages::OnSendUserMessage_Pre), false);
		SH_ADD_HOOK(IVEngineServer, SendUserMessage, engine, SH_MEMBER(this, &UserMessages::OnSendUserMessage_Post), true);
#else
		SH_ADD_HOOK(IVEngineServer, UserMessageBegin, engine, SH_MEMBER(this, &UserMessages::OnStartMessage_Pre), false);
		SH_ADD_HOOK(IVEngineServer, UserMessageBegin, engine, SH_MEMBER(this, &UserMessages::OnStartMessage_Post), true);
		SH_ADD_HOOK(IVEngineServer, MessageEnd, engine, SH_MEMBER(this, &UserMessages::OnMessageEnd_Pre), false);
		SH_ADD_HOOK(IVEngineServer, MessageEnd, engine, SH_MEMBER(this, &UserMessages::OnMessageEnd_Post), true);
#endif
	}

	if (intercept)
	{
		m_msgIntercepts[msg_id].push_back(pInfo);
	} else {
		m_msgHooks[msg_id].push_back(pInfo);
	}

	return true;
}

#ifdef USE_PROTOBUF_USERMESSAGES
const protobuf::Message *GetMessagePrototype(int msg_type)
{
#if SOURCE_ENGINE == SE_CSGO
	return g_Cstrike15UsermessageHelpers.GetPrototype(msg_type);
#elif SOURCE_ENGINE == SE_BLADE
	return g_BerimbauUsermessageHelpers.GetPrototype(msg_type);
#endif
}
#endif

#ifdef USE_PROTOBUF_USERMESSAGES
bool UserMessages::InternalUnhook(int msg_id, IProtobufUserMessageListener *pListener, bool intercept, bool isNew)
#else
bool UserMessages::InternalUnhook(int msg_id, IBitBufUserMessageListener *pListener, bool intercept, bool isNew)
#endif
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
		if (pInfo->Callback == pListener && pInfo->IsNew == isNew)
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
#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
		SH_REMOVE_HOOK(IVEngineServer, SendUserMessage, engine, SH_MEMBER(this, &UserMessages::OnSendUserMessage_Pre), false);
		SH_REMOVE_HOOK(IVEngineServer, SendUserMessage, engine, SH_MEMBER(this, &UserMessages::OnSendUserMessage_Post), true);
#else
		SH_REMOVE_HOOK(IVEngineServer, UserMessageBegin, engine, SH_MEMBER(this, &UserMessages::OnStartMessage_Pre), false);
		SH_REMOVE_HOOK(IVEngineServer, UserMessageBegin, engine, SH_MEMBER(this, &UserMessages::OnStartMessage_Post), true);
		SH_REMOVE_HOOK(IVEngineServer, MessageEnd, engine, SH_MEMBER(this, &UserMessages::OnMessageEnd_Pre), false);
		SH_REMOVE_HOOK(IVEngineServer, MessageEnd, engine, SH_MEMBER(this, &UserMessages::OnMessageEnd_Post), true);
#endif
	}
}

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
void UserMessages::OnSendUserMessage_Pre(IRecipientFilter &filter, int msg_type, const protobuf::Message &msg)
{
#if SOURCE_ENGINE == SE_CSGO
	const char *pszName = g_Cstrike15UsermessageHelpers.GetName(msg_type);
#elif SOURCE_ENGINE == SE_BLADE
	const char *pszName = g_BerimbauUsermessageHelpers.GetName(msg_type);
#endif

	OnStartMessage_Pre(&filter, msg_type, pszName);

	if (m_FakeMetaRes == MRES_SUPERCEDE)
	{
		int size = msg.ByteSize();
		uint8 *data = (uint8 *)stackalloc(size);
		msg.SerializePartialToArray(data, size);
		m_InterceptBuffer->ParsePartialFromArray(data, size);
	}
	else
	{
		m_FakeEngineBuffer =
			PbHandle(&const_cast<protobuf::Message &>(msg), PbHandle::Unowned, PbHandle::Private);
	}

	OnStartMessage_Post(&filter, msg_type, pszName);

	OnMessageEnd_Pre();
	if (m_FakeMetaRes == MRES_SUPERCEDE)
		RETURN_META(MRES_SUPERCEDE);

	RETURN_META(MRES_IGNORED);
}

void UserMessages::OnSendUserMessage_Post(IRecipientFilter &filter, int msg_type, const protobuf::Message &msg)
{
	OnMessageEnd_Post();
	RETURN_META(MRES_IGNORED);
}
#endif

#ifdef USE_PROTOBUF_USERMESSAGES
#define UM_RETURN_META_VALUE(res, val) \
	m_FakeMetaRes = res;               \
	return val;

#define UM_RETURN_META(res)            \
	m_FakeMetaRes = res;               \
	return;
#else
#define UM_RETURN_META_VALUE(res, val) \
	RETURN_META_VALUE(res, val)

#define UM_RETURN_META(res)            \
	RETURN_META(res)
#endif

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
PbHandle UserMessages::OnStartMessage_Pre(IRecipientFilter *filter, int msg_type, const char *msg_name)
#elif SOURCE_ENGINE >= SE_LEFT4DEAD
bf_write *UserMessages::OnStartMessage_Pre(IRecipientFilter *filter, int msg_type, const char *msg_name)
#else
bf_write *UserMessages::OnStartMessage_Pre(IRecipientFilter *filter, int msg_type)
#endif
{
	bool is_intercept_empty = m_msgIntercepts[msg_type].empty();
	bool is_hook_empty = m_msgHooks[msg_type].empty();

	if ((is_intercept_empty && is_hook_empty)
		|| (m_InExec && (m_CurFlags & USERMSG_BLOCKHOOKS)))
	{
		m_InHook = false;
		UM_RETURN_META_VALUE(MRES_IGNORED, nullptr);
	}

	m_CurId = msg_type;
	m_CurRecFilter = filter;
	m_InHook = true;
	m_BlockEndPost = false;

	if (!is_intercept_empty)
	{
#ifdef USE_PROTOBUF_USERMESSAGES
		m_InterceptBuffer =
			PbHandle(GetMessagePrototype(msg_type)->New(), PbHandle::Owned, PbHandle::Local);
		UM_RETURN_META_VALUE(MRES_SUPERCEDE, m_InterceptBuffer.AsUnowned());
#else
		m_InterceptBuffer.Reset();
		UM_RETURN_META_VALUE(MRES_SUPERCEDE, &m_InterceptBuffer);
#endif
	}

	UM_RETURN_META_VALUE(MRES_IGNORED, nullptr);
}

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
void* UserMessages::OnStartMessage_Post(IRecipientFilter *filter, int msg_type, const char *msg_name)
#elif SOURCE_ENGINE >= SE_LEFT4DEAD
bf_write *UserMessages::OnStartMessage_Post(IRecipientFilter *filter, int msg_type, const char *msg_name)
#else
bf_write *UserMessages::OnStartMessage_Post(IRecipientFilter *filter, int msg_type)
#endif
{
	if (!m_InHook)
	{
		UM_RETURN_META_VALUE(MRES_IGNORED, NULL);
	}

#ifdef USE_PROTOBUF_USERMESSAGES
	if (m_FakeMetaRes == MRES_SUPERCEDE)
		m_OrigBuffer = m_InterceptBuffer.AsUnowned();
	else
		m_OrigBuffer = m_FakeEngineBuffer.AsUnowned();
#else
	m_OrigBuffer = META_RESULT_ORIG_RET(bf_write *);
#endif

	UM_RETURN_META_VALUE(MRES_IGNORED, NULL);
}

void UserMessages::OnMessageEnd_Post()
{
	if (!m_InHook)
	{
		UM_RETURN_META(MRES_IGNORED);
	}

	MsgList *pList;
	MsgIter iter;
	ListenerInfo *pInfo;

	m_InHook = false;

	pList = &m_msgIntercepts[m_CurId];
	for (iter=pList->begin(); iter!=pList->end(); )
	{
		pInfo = (*iter);
		if (m_BlockEndPost && !pInfo->IsNew)
		{
			continue;
		}
		pInfo->IsHooked = true;
		pInfo->Callback->OnUserMessageSent(m_CurId);
		if (pInfo->IsNew)
		{
			pInfo->Callback->OnPostUserMessage(m_CurId, !m_BlockEndPost);
		}

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

	pList = &m_msgHooks[m_CurId];
	for (iter=pList->begin(); iter!=pList->end(); )
	{
		pInfo = (*iter);
		if (m_BlockEndPost && !pInfo->IsNew)
		{
			continue;
		}
		pInfo->IsHooked = true;
		pInfo->Callback->OnUserMessageSent(m_CurId);
		if (pInfo->IsNew)
		{
			pInfo->Callback->OnPostUserMessage(m_CurId, !m_BlockEndPost);
		}

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
		UM_RETURN_META(MRES_IGNORED);
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

#ifdef USE_PROTOBUF_USERMESSAGES
		PbHandle local = m_InterceptBuffer.ToLocal(m_CurId);
		res = pInfo->Callback->InterceptUserMessage(m_CurId, local.GetLocalMessage(), m_CurRecFilter);
		m_InterceptBuffer.CopyFrom(local);
#else
		res = pInfo->Callback->InterceptUserMessage(m_CurId, &m_InterceptBuffer, m_CurRecFilter);
#endif

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

	if (!handled && intercepted)
	{
#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
		PbHandle priv = m_InterceptBuffer.ToPrivate(m_CurId);
		ENGINE_CALL(SendUserMessage)(static_cast<IRecipientFilter &>(*m_CurRecFilter), m_CurId, *priv.GetPrivateMessage());
#else
		bf_write *engine_bfw;
#if SOURCE_ENGINE >= SE_LEFT4DEAD
		engine_bfw = ENGINE_CALL(UserMessageBegin)(m_CurRecFilter, m_CurId, g_SMAPI->GetUserMessage(m_CurId));
#else
		engine_bfw = ENGINE_CALL(UserMessageBegin)(m_CurRecFilter, m_CurId);
#endif
		m_ReadBuffer.StartReading(m_InterceptBuffer.GetBasePointer(), m_InterceptBuffer.GetNumBytesWritten());
		engine_bfw->WriteBitsFromBuffer(&m_ReadBuffer, m_InterceptBuffer.GetNumBitsWritten());
		ENGINE_CALL(MessageEnd)();
#endif // SE_CSGO || SE_BLADE
	}

	{
#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
		PbHandle tmp_msg(GetMessagePrototype(m_CurId)->New(), PbHandle::Owned, PbHandle::Local);
		tmp_msg.CopyFrom(m_OrigBuffer);
		auto pTempMsg = tmp_msg.GetLocalMessage();
#else
		bf_write *pTempMsg = m_OrigBuffer;
#endif

		pList = &m_msgHooks[m_CurId];
		for (iter=pList->begin(); iter!=pList->end(); )
		{
			pInfo = (*iter);
			pInfo->IsHooked = true;
			pInfo->Callback->OnUserMessage(m_CurId, pTempMsg, m_CurRecFilter);

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

	UM_RETURN_META((intercepted) ? MRES_SUPERCEDE : MRES_IGNORED);
supercede:
	m_BlockEndPost = true;
	UM_RETURN_META(MRES_SUPERCEDE);
}
