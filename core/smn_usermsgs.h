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

#endif //_INCLUDE_SOURCEMOD_CMSGLISTENERWRAPPER_H_
