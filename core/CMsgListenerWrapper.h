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

#ifndef _INCLUDE_SOURCEMOD_CMSGLISTENERWRAPPER_H_
#define _INCLUDE_SOURCEMOD_CMSGLISTENERWRAPPER_H_

extern int g_MsgPlayers[256];

class MsgListenerWrapper : public IUserMessageListener
{
public:
	void InitListener(int msgid, IPluginFunction *hook, IPluginFunction *notify, bool intercept);
	bool IsInterceptHook() const;
	int GetMessageId() const;
	IPluginFunction *GetHookedFunction() const;
	IPluginFunction *GetNotifyFunction() const;
public: //IUserMessageListener
	void OnUserMessage(int msg_id, bf_write *bf, IRecipientFilter *pFilter);
	ResultType InterceptUserMessage(int msg_id, bf_write *bf, IRecipientFilter *pFilter);
	void OnUserMessageSent(int msg_id);
private:
	size_t PreparePlArray(int *pl_array, IRecipientFilter *pFilter);
private:
	IPluginFunction *m_Hook;
	IPluginFunction *m_Intercept;
	IPluginFunction *m_Notify;
	bool m_IsInterceptHook;
	int m_MsgId;
};

inline size_t MsgListenerWrapper::PreparePlArray(int *pl_array, IRecipientFilter *pFilter)
{
	size_t size = static_cast<size_t>(pFilter->GetRecipientCount());

	for (size_t i=0; i<size; i++)
	{
		pl_array[i] = pFilter->GetRecipientIndex(i);
	}

	return size;
}

inline bool MsgListenerWrapper::IsInterceptHook() const
{
	return m_IsInterceptHook;
}

inline int MsgListenerWrapper::GetMessageId() const
{
	return m_MsgId;
}

inline IPluginFunction *MsgListenerWrapper::GetHookedFunction() const
{
	if (m_Hook)
	{
		return m_Hook;
	} else {
		return m_Intercept;
	}
}

inline IPluginFunction *MsgListenerWrapper::GetNotifyFunction() const
{
	return m_Notify;
}

inline void MsgListenerWrapper::InitListener(int msgid, IPluginFunction *hook, IPluginFunction *notify, bool intercept)
{
	if (intercept)
	{
		m_Intercept = hook;
		m_Hook = NULL;
	} else {
		m_Hook = hook;
		m_Intercept = NULL;
	}

	if (notify)
	{
		m_Notify = notify;
	} else {
		m_Notify = NULL;
	}

	m_MsgId = msgid;
	m_IsInterceptHook = intercept;
}

inline void MsgListenerWrapper::OnUserMessage(int msg_id, bf_write *bf, IRecipientFilter *pFilter)
{
	cell_t res;
	size_t size = PreparePlArray(g_MsgPlayers, pFilter);

	m_Hook->PushCell(msg_id);
	m_Hook->PushCell(0); //:TODO: push handle!
	m_Hook->PushArray(g_MsgPlayers, size, NULL);
	m_Hook->PushCell(size);
	m_Hook->PushCell(pFilter->IsReliable());
	m_Hook->PushCell(pFilter->IsInitMessage());
	m_Hook->Execute(&res);
}

inline ResultType MsgListenerWrapper::InterceptUserMessage(int msg_id, bf_write *bf, IRecipientFilter *pFilter)
{
	cell_t res = static_cast<cell_t>(Pl_Continue);
	size_t size = PreparePlArray(g_MsgPlayers, pFilter);

	m_Intercept->PushCell(msg_id);
	m_Intercept->PushCell(0); //:TODO: push handle!
	m_Intercept->PushArray(g_MsgPlayers, size, NULL);
	m_Intercept->PushCell(size);
	m_Intercept->PushCell(pFilter->IsReliable());
	m_Intercept->PushCell(pFilter->IsInitMessage());
	m_Intercept->Execute(&res);

	return static_cast<ResultType>(res);
}

inline void MsgListenerWrapper::OnUserMessageSent(int msg_id)
{
	cell_t res;

	if (!m_Notify)
	{
		return;
	}

	m_Notify->PushCell(msg_id);
	m_Notify->Execute(&res);
}

#endif //_INCLUDE_SOURCEMOD_CMSGLISTENERWRAPPER_H_
