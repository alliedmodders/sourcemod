#include "ChatTriggers.h"
#include "sm_stringutil.h"
#include "TextParsers.h"
#include "ConCmdManager.h"

/* :HACKHACK: We can't SH_DECL here because ConCmdManager.cpp does */
extern bool __SourceHook_FHRemoveConCommandDispatch(void *, bool, class fastdelegate::FastDelegate0<void>);
extern int __SourceHook_FHAddConCommandDispatch(void *, bool, class fastdelegate::FastDelegate0<void>);

ChatTriggers g_ChatTriggers;

ChatTriggers::ChatTriggers() : m_pSayCmd(NULL), m_bWillProcessInPost(false), 
	m_ReplyTo(SM_REPLY_CONSOLE)
{
	m_PubTrigger = sm_strdup("!");
	m_PrivTrigger = sm_strdup("/");
	m_PubTriggerSize = 1;
	m_PrivTriggerSize = 1;
}

ChatTriggers::~ChatTriggers()
{
	delete [] m_PubTrigger;
	m_PubTrigger = NULL;
	delete [] m_PrivTrigger;
	m_PrivTrigger = NULL;
}

ConfigResult ChatTriggers::OnSourceModConfigChanged(const char *key, 
													const char *value, 
													ConfigSource source,
													char *error, 
													size_t maxlength)
{
	if (strcmp(key, "PublicChatTrigger") == 0)
	{
		delete [] m_PubTrigger;
		m_PubTrigger = sm_strdup(value);
		m_PubTriggerSize = strlen(m_PubTrigger);
		return ConfigResult_Accept;
	} else if (strcmp(key, "SilentChatTrigger") == 0) {
		delete [] m_PrivTrigger;
		m_PrivTrigger = sm_strdup(value);
		m_PrivTriggerSize = strlen(m_PrivTrigger);
		return ConfigResult_Accept;
	}

	return ConfigResult_Ignore;
}

void ChatTriggers::OnSourceModGameInitialized()
{
	unsigned int total = 2;
	ConCommandBase *pCmd = icvar->GetCommands();
	const char *name;
	while (pCmd)
	{
		if (pCmd->IsCommand())
		{
			name = pCmd->GetName();
			if (!m_pSayCmd && strcmp(name, "say") == 0)
			{
				m_pSayCmd = (ConCommand *)pCmd;
				if (--total == 0)
				{
					break;
				}
			} else if (!m_pSayTeamCmd && strcmp(name, "say_team") == 0) {
				m_pSayTeamCmd = (ConCommand *)pCmd;
				if (--total == 0)
				{
					break;
				}
			}
		}
		pCmd = const_cast<ConCommandBase *>(pCmd->GetNext());
	}

	if (m_pSayCmd)
	{
		SH_ADD_HOOK_MEMFUNC(ConCommand, Dispatch, m_pSayCmd, this, &ChatTriggers::OnSayCommand_Pre, false);
		SH_ADD_HOOK_MEMFUNC(ConCommand, Dispatch, m_pSayCmd, this, &ChatTriggers::OnSayCommand_Post, true);
	}
	if (m_pSayTeamCmd)
	{
		SH_ADD_HOOK_MEMFUNC(ConCommand, Dispatch, m_pSayTeamCmd, this, &ChatTriggers::OnSayCommand_Pre, false);
		SH_ADD_HOOK_MEMFUNC(ConCommand, Dispatch, m_pSayTeamCmd, this, &ChatTriggers::OnSayCommand_Post, true);
	}
}

void ChatTriggers::OnSourceModShutdown()
{
	if (m_pSayTeamCmd)
	{
		SH_REMOVE_HOOK_MEMFUNC(ConCommand, Dispatch, m_pSayTeamCmd, this, &ChatTriggers::OnSayCommand_Post, true);
		SH_REMOVE_HOOK_MEMFUNC(ConCommand, Dispatch, m_pSayTeamCmd, this, &ChatTriggers::OnSayCommand_Pre, false);
	}
	if (m_pSayCmd)
	{
		SH_REMOVE_HOOK_MEMFUNC(ConCommand, Dispatch, m_pSayCmd, this, &ChatTriggers::OnSayCommand_Post, true);
		SH_REMOVE_HOOK_MEMFUNC(ConCommand, Dispatch, m_pSayCmd, this, &ChatTriggers::OnSayCommand_Pre, false);
	}
}

void ChatTriggers::OnSayCommand_Pre()
{
	int client = g_ConCmds.GetCommandClient();

	/* The server console cannot do this */
	if (client == 0)
	{
		RETURN_META(MRES_IGNORED);
	}

	const char *args = engine->Cmd_Args();

	/* Handle quoted string sets */
	bool is_quoted = false;
	if (args[0] == '"')
	{
		args++;
		is_quoted = true;
	}

	bool is_trigger = false;
	bool is_silent = false;

	/* Check for either trigger */
	if (m_PubTriggerSize && strncmp(args, m_PubTrigger, m_PubTriggerSize) == 0)
	{
		is_trigger = true;
	} else if (m_PrivTriggerSize && strncmp(args, m_PrivTrigger, m_PrivTriggerSize) == 0) {
		is_trigger = true;
		is_silent = true;
	}

	if (!is_trigger)
	{
		RETURN_META(MRES_IGNORED);
	}

	/* If we're a public command, process later */
	if (!is_silent)
	{
		/* We have to process this in _post_ instead.  Darn. */
		m_bWillProcessInPost = true;
		RETURN_META(MRES_IGNORED);
	}

	/* Otherwise, process now */
	if (ProcessTrigger(engine->PEntityOfEntIndex(client), &args[m_PrivTriggerSize], is_quoted))
	{
		/* If we succeed, block the original say! */
		RETURN_META(MRES_SUPERCEDE);
	}

	RETURN_META(MRES_IGNORED);
}

void ChatTriggers::OnSayCommand_Post()
{
	if (m_bWillProcessInPost)
	{
		/* Reset this for re-entrancy */
		m_bWillProcessInPost = false;

		/* Get our arguments */
		const char *args = engine->Cmd_Args();

		/* Handle quotations */
		bool is_quoted = false;
		if (args[0] == '"')
		{
			args++;
			is_quoted = true;
		}

		int client = g_ConCmds.GetCommandClient();

		ProcessTrigger(engine->PEntityOfEntIndex(client), &args[m_PubTriggerSize], is_quoted);
	}
}

bool ChatTriggers::ProcessTrigger(edict_t *pEdict, const char *args, bool is_quoted)
{
	/* Eat up whitespace */
	while (*args != '\0' && IsWhitespace(args))
	{
		args++;
	}

	/* Check if we're still valid */
	if (*args == '\0')
	{
		return false;
	}

	/* Extract a command.  This is kind of sloppy. */
	char cmd_buf[64];
	size_t cmd_len = 0;
	const char *inptr = args;
	while (*inptr != '\0' 
			&& !IsWhitespace(inptr) 
			&& cmd_len < sizeof(cmd_buf) - 1)
	{
		cmd_buf[cmd_len++] = *inptr++;
	}
	cmd_buf[cmd_len] = '\0';

	/* See if we have this registered */
	bool prepended = false;
	if (!g_ConCmds.LookForSourceModCommand(cmd_buf))
	{
		/* Check if we had an "sm_" prefix */
		if (strncmp(cmd_buf, "sm_", 3) == 0)
		{
			return false;
		}

		/* Now, prepend.  Don't worry about the buffers.  This will 
		 * work because the sizes are limited from earlier.
		 */
		char new_buf[80];
		strcpy(new_buf, "sm_");
		strncopy(&new_buf[3], cmd_buf, sizeof(new_buf)-3);

		/* Recheck */
		if (!g_ConCmds.LookForSourceModCommand(new_buf))
		{
			return false;
		}
		
		prepended = true;
	}

	/* See if we need to do extra string manipulation */
	if (is_quoted || prepended)
	{
		static char buffer[300];
		size_t len;

		/* Check if we need to prepend sm_ */
		if (prepended)
		{
			len = UTIL_Format(buffer, sizeof(buffer), "sm_%s", args);
		} else {
			len = strncopy(buffer, args, sizeof(buffer));
		}
		
		/* Check if we need to strip a quote */
		if (is_quoted)
		{
			if (buffer[len-1] == '"')
			{
				buffer[--len] = '\0';
			}
		}

		args = buffer;
	}

	/* Finally, execute! */
	unsigned int old = SetReplyTo(SM_REPLY_CHAT);
	serverpluginhelpers->ClientCommand(pEdict, args);
	SetReplyTo(old);

	return true;
}

unsigned int ChatTriggers::SetReplyTo(unsigned int reply)
{
	unsigned int old = m_ReplyTo;

	m_ReplyTo = reply;

	return old;
}

unsigned int ChatTriggers::GetReplyTo()
{
	return m_ReplyTo;
}
