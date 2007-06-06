#ifndef _INCLUDE_SOURCEMOD_CHAT_TRIGGERS_H_
#define _INCLUDE_SOURCEMOD_CHAT_TRIGGERS_H_

#include "sm_globals.h"
#include "sourcemm_api.h"

#define SM_REPLY_CONSOLE	0
#define SM_REPLY_CHAT		1

class ChatTriggers : public SMGlobalClass
{
public:
	ChatTriggers();
public: //SMGlobalClass
	void OnSourceModGameInitialized();
	void OnSourceModShutdown();
private: //ConCommand
	void OnSayCommand_Pre();
	void OnSayCommand_Post();
public:
	unsigned int GetReplyTo();
	unsigned int SetReplyTo(unsigned int reply);
private:
	bool ProcessTrigger(edict_t *pEdict, const char *args, bool is_quoted);
private:
	ConCommand *m_pSayCmd;
	char *m_PubTrigger;
	size_t m_PubTriggerSize;
	char *m_PrivTrigger;
	size_t m_PrivTriggerSize;
	bool m_bWillProcessInPost;
	unsigned int m_ReplyTo;
};

extern ChatTriggers g_ChatTriggers;

#endif //_INCLUDE_SOURCEMOD_CHAT_TRIGGERS_H_
