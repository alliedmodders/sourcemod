#include "CConCmdManager.h"
#include "sm_srvcmds.h"

CConCmdManager g_ConCmds;

SH_DECL_HOOK0_void(ConCommand, Dispatch, SH_NOATTRIB, false);
SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, false, int);

struct PlCmdInfo
{
	ConCmdInfo *pInfo;
	CmdType type;
};

typedef List<PlCmdInfo> CmdList;

char *sm_strdup(const char *str)
{
	char *ptr = new char[strlen(str)+1];
	strcpy(ptr, str);
	return ptr;
}

CConCmdManager::CConCmdManager()
{
	m_pCmds = sm_trie_create();
}

CConCmdManager::~CConCmdManager()
{
	sm_trie_destroy(m_pCmds);
}

void CConCmdManager::OnSourceModAllInitialized()
{
	g_PluginSys.AddPluginsListener(this);
	g_RootMenu.AddRootConsoleCommand("cmds", "List console commands", this);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, serverClients, this, &CConCmdManager::SetCommandClient, false);
}

void CConCmdManager::OnSourceModShutdown()
{
	/* All commands should already be removed by the time we're done */
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, serverClients, this, &CConCmdManager::SetCommandClient, false);
	g_RootMenu.RemoveRootConsoleCommand("cmds", this);
	g_PluginSys.RemovePluginsListener(this);
}

void CConCmdManager::OnPluginLoaded(IPlugin *plugin)
{
	/* Nothing yet... */
}

void CConCmdManager::OnPluginDestroyed(IPlugin *plugin)
{
	CmdList *pList;
	List<ConCmdInfo *> removed;
	if (plugin->GetProperty("CommandList", (void **)&pList, true))
	{
		CmdList::iterator iter;
		for (iter=pList->begin();
			 iter!=pList->end();
			 iter++)
		{
			PlCmdInfo &cmd = (*iter);
			if (cmd.type == Cmd_Server
				|| cmd.type == Cmd_Console)
			{
				ConCmdInfo *pInfo = cmd.pInfo;
				/* See if this is being removed */
				if (removed.find(pInfo) != removed.end())
				{
					continue;
				}
				/* See if there are still hooks */
				if (pInfo->srvhooks
					&& pInfo->srvhooks->GetFunctionCount())
				{
					continue;
				}
				if (pInfo->conhooks
					&& pInfo->conhooks->GetFunctionCount())
				{
					continue;
				}
				/* Remove the command */
				RemoveConCmd(pInfo);
				removed.push_back(pInfo);
			}
		}
		delete pList;
	}
}

void CommandCallback()
{
	g_ConCmds.InternalDispatch();
}

void CConCmdManager::SetCommandClient(int client)
{
	m_CmdClient = client + 1;
}

ResultType CConCmdManager::DispatchClientCommand(int client, ResultType type)
{
	const char *cmd = engine->Cmd_Argv(0);
	int args = engine->Cmd_Argc() - 1;

	ConCmdInfo *pInfo;
	if (sm_trie_retrieve(m_pCmds, cmd, (void **)&pInfo))
	{
		cell_t result = type;
		if (pInfo->conhooks && pInfo->conhooks->GetFunctionCount())
		{
			pInfo->conhooks->PushCell(client);
			pInfo->conhooks->PushCell(args);
			pInfo->conhooks->Execute(&result);
		}
		if (result >= Pl_Stop)
		{
			return Pl_Stop;
		}
		type = (ResultType)result;
	}

	return type;
}

void CConCmdManager::InternalDispatch()
{
	/**
	 * Note: Console commands will EITHER go through IServerGameDLL::ClientCommand,
	 * OR this dispatch.  They will NEVER go through both.
	 * -- 
	 * Whether or not it goes through the callback is determined by FCVAR_GAMEDLL
	 */

	const char *cmd = engine->Cmd_Argv(0);
	ConCmdInfo *pInfo;
	if (!sm_trie_retrieve(m_pCmds, cmd, (void **)&pInfo))
	{
		return;
	}

	cell_t result = Pl_Continue;
	int args = engine->Cmd_Argc() - 1;

	if (m_CmdClient == 0)
	{
		if (pInfo->srvhooks && pInfo->srvhooks->GetFunctionCount())
		{
			pInfo->srvhooks->PushCell(args);
			pInfo->srvhooks->Execute(&result);
		}
	
		/* Check if there's an early stop */
		if (result >= Pl_Stop)
		{
			if (!pInfo->sourceMod)
			{
				RETURN_META(MRES_SUPERCEDE);
			}
			return;
		}
	}

	/* Execute console command hooks */
	if (pInfo->conhooks && pInfo->conhooks->GetFunctionCount())
	{
		pInfo->conhooks->PushCell(m_CmdClient);
		pInfo->conhooks->PushCell(args);
		pInfo->conhooks->Execute(&result);
	}

	if (result >= Pl_Handled)
	{
		if (!pInfo->sourceMod)
		{
			RETURN_META(MRES_SUPERCEDE);
		}
		return;
	}
}

void CConCmdManager::AddConsoleCommand(IPluginFunction *pFunction, 
									   const char *name, 
									   const char *description, 
									   int flags)
{
	ConCmdInfo *pInfo = AddOrFindCommand(name, description, flags);

	if (!pInfo->conhooks)
	{
		pInfo->conhooks = g_Forwards.CreateForwardEx(NULL, ET_Hook, 2, NULL, Param_Cell, Param_Cell);
	}

	pInfo->conhooks->AddFunction(pFunction);

	/* Add to the plugin */
	CmdList *pList;
	IPlugin *pPlugin = g_PluginSys.GetPluginByCtx(pFunction->GetParentContext()->GetContext());
	if (!pPlugin->GetProperty("CommandList", (void **)&pList))
	{
		pList = new CmdList();
		pPlugin->SetProperty("CommandList", pList);
	}
	PlCmdInfo info;
	info.pInfo = pInfo;
	info.type = Cmd_Console;
	pList->push_back(info);
}

void CConCmdManager::AddServerCommand(IPluginFunction *pFunction, 
									  const char *name, 
									  const char *description, 
									  int flags)

{
	ConCmdInfo *pInfo = AddOrFindCommand(name, description, flags);

	if (!pInfo->srvhooks)
	{
		pInfo->srvhooks = g_Forwards.CreateForwardEx(NULL, ET_Hook, 1, NULL, Param_Cell);
	}

	pInfo->srvhooks->AddFunction(pFunction);

	/* Add to the plugin */
	CmdList *pList;
	IPlugin *pPlugin = g_PluginSys.GetPluginByCtx(pFunction->GetParentContext()->GetContext());
	if (!pPlugin->GetProperty("CommandList", (void **)&pList))
	{
		pList = new CmdList();
		pPlugin->SetProperty("CommandList", pList);
	}
	PlCmdInfo info;
	info.pInfo = pInfo;
	info.type = Cmd_Server;
	pList->push_back(info);
}

void CConCmdManager::AddToCmdList(ConCmdInfo *info)
{
	List<ConCmdInfo *>::iterator iter = m_CmdList.begin();
	ConCmdInfo *pInfo;
	bool inserted = false;
	const char *orig;

	if (info->pCmd)
	{
		orig = info->pCmd->GetName();
	}

	/* Insert this into the help list, SORTED alphabetically. */
	while (iter != m_CmdList.end())
	{
		const char *cmd = NULL;
		pInfo = (*iter);
		if (pInfo->pCmd)
		{
			cmd = pInfo->pCmd->GetName();
		}
		if (strcmp(orig, cmd) < 0)
		{
			m_CmdList.insert(iter, info);
			inserted = true;
			break;
		}
		iter++;
	}

	if (!inserted)
	{
		m_CmdList.push_back(info);
	}
}

void CConCmdManager::RemoveConCmd(ConCmdInfo *info)
{
	/* Remove console-specific information */
	if (info->pCmd)
	{
		/* Remove from the trie */
		sm_trie_delete(m_pCmds, info->pCmd->GetName());

		if (info->sourceMod)
		{
			/* Unlink from SourceMM */
			g_SMAPI->UnregisterConCmdBase(g_PLAPI, info->pCmd);
			/* Delete the command's memory */
			char *new_help = const_cast<char *>(info->pCmd->GetHelpText());
			char *new_name = const_cast<char *>(info->pCmd->GetName());
			delete [] new_help;
			delete [] new_name;
			delete info->pCmd;
		} else {
			/* Remove the external hook */
			SH_REMOVE_HOOK_STATICFUNC(ConCommand, Dispatch, info->pCmd, CommandCallback, false);
		}
	}
	
	/* Remove from list */
	m_CmdList.remove(info);
	
	/* Free forwards */
	if (info->srvhooks)
	{
		g_Forwards.ReleaseForward(info->srvhooks);
	}
}

ConCmdInfo *CConCmdManager::AddOrFindCommand(const char *name, const char *description, int flags)
{
	ConCmdInfo *pInfo;
	if (!sm_trie_retrieve(m_pCmds, name, (void **)&pInfo))
	{
		pInfo = new ConCmdInfo();
		/* Find the commandopan */
		ConCommandBase *pBase = icvar->GetCommands();
		ConCommand *pCmd = NULL;
		while (pBase)
		{
			if (pBase->IsCommand()
				&& (strcmp(pBase->GetName(), name) == 0))
			{
				pCmd = (ConCommand *)pBase;
				break;
			}
			pBase = const_cast<ConCommandBase *>(pBase->GetNext());
		}

		if (!pCmd)
		{
			/* Note that we have to duplicate because the source might not be 
			 * a static string, and these expect static memory.
			 */
			if (!description)
			{
				description = "";
			}
			char *new_name = sm_strdup(name);
			char *new_help = sm_strdup(description);
			pCmd = new ConCommand(new_name, CommandCallback, new_help, flags);
			pInfo->sourceMod = true;
		} else {
			SH_ADD_HOOK_STATICFUNC(ConCommand, Dispatch, pCmd, CommandCallback, false);
		}

		pInfo->pCmd = pCmd;

		sm_trie_insert(m_pCmds, name, pInfo);
		AddToCmdList(pInfo);
	}

	return pInfo;
}

void CConCmdManager::OnRootConsoleCommand(const char *command, unsigned int argcount)
{
	if (argcount >= 3)
	{
		const char *text = engine->Cmd_Argv(2);
		int id = atoi(text);
		CPlugin *pPlugin = g_PluginSys.GetPluginByOrder(id);

		if (!pPlugin)
		{
			g_RootMenu.ConsolePrint("[SM] Plugin index %d not found.", id);
			return;
		}

		CmdList *pList;
		if (!pPlugin->GetProperty("CommandList", (void **)&pList))
		{
			g_RootMenu.ConsolePrint("[SM] No commands found for %s", pPlugin->GetFilename());
			return;
		}
		if (!pList->size())
		{
			g_RootMenu.ConsolePrint("[SM] No commands found for %s", pPlugin->GetFilename());
			return;
		}

		CmdList::iterator iter;
		const char *type = NULL;
		const char *name;
		const char *help;
		g_RootMenu.ConsolePrint(" %-17.16s %-8.7s %s", "[Name]", "[Type]", "[Help]");
		for (iter=pList->begin();
			 iter!=pList->end();
			 iter++)
		{
			PlCmdInfo &cmd = (*iter);
			if (cmd.type == Cmd_Server)
			{
				type = "server";
			} else if (cmd.type == Cmd_Console) {
				type = "console";
			} else if (cmd.type == Cmd_Client) {
				type = "client";
			}
			name = cmd.pInfo->pCmd->GetName();
			help = cmd.pInfo->pCmd->GetHelpText();
			g_RootMenu.ConsolePrint(" %-17.16s %-12.11s %s", name, type, help);		
		}

		return;
	}

	g_RootMenu.ConsolePrint("[SM] Usage: sm cmds <plugin #>");
}
