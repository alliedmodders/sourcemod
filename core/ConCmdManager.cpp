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

#include "ConCmdManager.h"
#include "sm_srvcmds.h"
#include "AdminCache.h"
#include "sm_stringutil.h"
#include "PlayerManager.h"
#include "Translator.h"
#include "HalfLife2.h"
#include "ChatTriggers.h"

ConCmdManager g_ConCmds;

#if defined ORANGEBOX_BUILD
	SH_DECL_HOOK1_void(ConCommand, Dispatch, SH_NOATTRIB, false, const CCommand &);
#else
	SH_DECL_HOOK0_void(ConCommand, Dispatch, SH_NOATTRIB, false);
#endif

SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, false, int);

struct PlCmdInfo
{
	ConCmdInfo *pInfo;
	CmdHook *pHook;
	CmdType type;
};
typedef List<PlCmdInfo> CmdList;

void AddToPlCmdList(CmdList *pList, const PlCmdInfo &info);

ConCmdManager::ConCmdManager() : m_Strings(1024)
{
	m_pCmds = sm_trie_create();
	m_pCmdGrps = sm_trie_create();
	m_CmdClient = 0;
}

ConCmdManager::~ConCmdManager()
{
	sm_trie_destroy(m_pCmds);
	sm_trie_destroy(m_pCmdGrps);
}

void ConCmdManager::OnSourceModAllInitialized()
{
	g_PluginSys.AddPluginsListener(this);
	g_RootMenu.AddRootConsoleCommand("cmds", "List console commands", this);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, serverClients, this, &ConCmdManager::SetCommandClient, false);
}

void ConCmdManager::OnSourceModShutdown()
{
	/* All commands should already be removed by the time we're done */
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, serverClients, this, &ConCmdManager::SetCommandClient, false);
	g_RootMenu.RemoveRootConsoleCommand("cmds", this);
}

void ConCmdManager::OnUnlinkConCommandBase(ConCommandBase *pBase, const char *name, bool is_read_safe)
{
	/* Whoa, first get its information struct */
	ConCmdInfo *pInfo;

	if (!sm_trie_retrieve(m_pCmds, name, (void **)&pInfo))
	{
		return;
	}

	RemoveConCmds(pInfo->srvhooks);
	RemoveConCmds(pInfo->conhooks);

	RemoveConCmd(pInfo, name, is_read_safe, false);
}

void ConCmdManager::RemoveConCmds(List<CmdHook *> &cmdlist)
{
	List<CmdHook *>::iterator iter = cmdlist.begin();

	while (iter != cmdlist.end())
	{
		CmdHook *pHook = (*iter);
		IPluginContext *pContext = pHook->pf->GetParentContext();
		IPlugin *pPlugin = g_PluginSys.GetPluginByCtx(pContext->GetContext());
		CmdList *pList = NULL;
		
		//gaben
		if (!pPlugin->GetProperty("CommandList", (void **)&pList, false) || !pList)
		{
			continue;
		}

		CmdList::iterator p_iter = pList->begin();
		while (p_iter != pList->end())
		{
			PlCmdInfo &cmd = (*p_iter);
			if (cmd.pHook == pHook)
			{
				p_iter = pList->erase(p_iter);
			}
			else
			{
				p_iter++;
			}
		}

		delete pHook->pAdmin;
		delete pHook;
		
		iter = cmdlist.erase(iter);
	}
}

void ConCmdManager::RemoveConCmds(List<CmdHook *> &cmdlist, IPluginContext *pContext)
{
	List<CmdHook *>::iterator iter = cmdlist.begin();
	CmdHook *pHook;

	while (iter != cmdlist.end())
	{
		pHook = (*iter);
		if (pHook->pf->GetParentContext() == pContext)
		{
			delete pHook->pAdmin;
			delete pHook;
			iter = cmdlist.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}

void ConCmdManager::OnPluginDestroyed(IPlugin *plugin)
{
	CmdList *pList;
	List<ConCmdInfo *> removed;
	if (plugin->GetProperty("CommandList", (void **)&pList, true))
	{
		IPluginContext *pContext = plugin->GetBaseContext();
		CmdList::iterator iter;
		/* First pass!
	 	 * Only bother if there's an actual command list on this plugin... 
	 	 */
		for (iter=pList->begin();
			iter!=pList->end();
			iter++)
		{
			PlCmdInfo &cmd = (*iter);
			ConCmdInfo *pInfo = cmd.pInfo;

			/* Has this chain already been fully cleaned/removed? */
			if (removed.find(pInfo) != removed.end())
			{
				continue;
			}

			/* Remove any hooks from us on this command */
			RemoveConCmds(pInfo->conhooks, pContext);
			RemoveConCmds(pInfo->srvhooks, pContext);

			/* See if there are still hooks */
			if (pInfo->srvhooks.size())
			{
				continue;
			}
			if (pInfo->conhooks.size())
			{
				continue;
			}

			/* Remove the command, it should be safe now */
			RemoveConCmd(pInfo, pInfo->pCmd->GetName(), true, true);
			removed.push_back(pInfo);
		}
		delete pList;
	}
}
#if defined ORANGEBOX_BUILD
void CommandCallback(const CCommand &command)
{
#else
void CommandCallback()
{
	CCommand command;
#endif

	g_HL2.PushCommandStack(&command);

	g_ConCmds.InternalDispatch(command);

	g_HL2.PopCommandStack();
}

void ConCmdManager::SetCommandClient(int client)
{
	m_CmdClient = client + 1;
}

ResultType ConCmdManager::DispatchClientCommand(int client, const char *cmd, int args, ResultType type)
{
	ConCmdInfo *pInfo;
	if (sm_trie_retrieve(m_pCmds, cmd, (void **)&pInfo))
	{
		cell_t result = type;
		cell_t tempres = result;
		List<CmdHook *>::iterator iter;
		CmdHook *pHook;
		for (iter=pInfo->conhooks.begin();
			 iter!=pInfo->conhooks.end();
			 iter++)
		{
			pHook = (*iter);
			if (!pHook->pf->IsRunnable())
			{
				continue;
			}
			if (pHook->pAdmin && !CheckAccess(client, cmd, pHook->pAdmin))
			{
				if (result < Pl_Handled)
				{
					result = Pl_Handled;
				}
				continue;
			}
			pHook->pf->PushCell(client);
			pHook->pf->PushCell(args);
			if (pHook->pf->Execute(&tempres) == SP_ERROR_NONE)
			{
				if (tempres > result)
				{
					result = tempres;
				}
				if (result == Pl_Stop)
				{
					break;
				}
			}
		}
		type = (ResultType)result;
	}

	return type;
}

void ConCmdManager::InternalDispatch(const CCommand &command)
{
	int client = m_CmdClient;

	if (client)
	{
		CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
		if (!pPlayer || !pPlayer->IsConnected())
		{
			return;
		}
	}

	/**
	 * Note: Console commands will EITHER go through IServerGameDLL::ClientCommand,
	 * OR this dispatch.  They will NEVER go through both.
	 * -- 
	 * Whether or not it goes through the callback is determined by FCVAR_GAMEDLL
	 */
	const char *cmd = g_HL2.CurrentCommandName();

	ConCmdInfo *pInfo;
	if (!sm_trie_retrieve(m_pCmds, cmd, (void **)&pInfo))
	{
		return;
	}

	/* This is a hack to prevent say triggers from firing on messages that were 
	 * blocked because of flooding.  We won't remove this, but the hack will get 
	 * "nicer" when we expose explicit say hooks.
	 */
	if (g_ChatTriggers.WasFloodedMessage())
	{
		return;
	}

	cell_t result = Pl_Continue;
	int args = command.ArgC() - 1;

	List<CmdHook *>::iterator iter;
	CmdHook *pHook;

	/* Execute server-only commands if viable */
	if (client == 0 && pInfo->srvhooks.size())
	{
		cell_t tempres = result;
		for (iter=pInfo->srvhooks.begin();
			 iter!=pInfo->srvhooks.end();
			 iter++)
		{
			pHook = (*iter);
			if (!pHook->pf->IsRunnable())
			{
				continue;
			}
			pHook->pf->PushCell(args);
			if (pHook->pf->Execute(&tempres) == SP_ERROR_NONE)
			{
				if (tempres > result)
				{
					result = tempres;
				}
				if (result == Pl_Stop)
				{
					break;
				}
			}
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

	/* Execute console commands */
	if (pInfo->conhooks.size())
	{
		cell_t tempres = result;
		for (iter=pInfo->conhooks.begin();
			iter!=pInfo->conhooks.end();
			iter++)
		{
			pHook = (*iter);
			if (!pHook->pf->IsRunnable())
			{
				continue;
			}
			if (client 
				&& pHook->pAdmin
				&& !CheckAccess(client, cmd, pHook->pAdmin))
			{
				if (result < Pl_Handled)
				{
					result = Pl_Handled;
				}
				continue;
			}

			/* On a listen server, sometimes the server host's client index can be set as 0.
			 * So index 1 is passed to the command callback to correct this potential problem.
			 */
			if (!engine->IsDedicatedServer())
			{
				client = g_Players.ListenClient();
			}

			pHook->pf->PushCell(client);
			pHook->pf->PushCell(args);

			if (pHook->pf->Execute(&tempres) == SP_ERROR_NONE)
			{
				if (tempres > result)
				{
					result = tempres;
				}
				if (result == Pl_Stop)
				{
					break;
				}
			}
		}
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

bool ConCmdManager::CheckCommandAccess(int client, const char *cmd, FlagBits cmdflags)
{
	if (cmdflags == 0 || client == 0)
	{
		return true;
	}

	/* If running listen server, then client 1 is the server host and should have 'root' access */
	if (client == 1 && !engine->IsDedicatedServer())
	{
		return true;
	}

	CPlayer *player = g_Players.GetPlayerByIndex(client);
	if (!player 
		|| player->GetEdict() == NULL
		|| player->IsFakeClient())
	{
		return false;
	}

	AdminId adm = player->GetAdminId();
	if (adm != INVALID_ADMIN_ID)
	{
		FlagBits bits = g_Admins.GetAdminFlags(adm, Access_Effective);

		/* root knows all, WHOA */
		if ((bits & ADMFLAG_ROOT) == ADMFLAG_ROOT)
		{
			return true;
		}

		/* Check for overrides
		 * :TODO: is it worth optimizing this?
		 */
		unsigned int groups = g_Admins.GetAdminGroupCount(adm);
		GroupId gid;
		OverrideRule rule;
		bool override = false;
		for (unsigned int i=0; i<groups; i++)
		{
			gid = g_Admins.GetAdminGroup(adm, i, NULL);
			/* First get group-level override */
			override = g_Admins.GetGroupCommandOverride(gid, cmd, Override_CommandGroup, &rule);
			/* Now get the specific command override */
			if (g_Admins.GetGroupCommandOverride(gid, cmd, Override_Command, &rule))
			{
				override = true;
			}
			if (override)
			{
				if (rule == Command_Allow)
				{
					return true;
				}
				else if (rule == Command_Deny)
				{
					return false;
				}
			}
		}

		/* See if our other flags match */
		if ((bits & cmdflags) == cmdflags)
		{
			return true;
		}
	}

	return false;
}

bool ConCmdManager::CheckAccess(int client, const char *cmd, AdminCmdInfo *pAdmin)
{
	if (CheckCommandAccess(client, cmd, pAdmin->eflags))
	{
		return true;
	}

	edict_t *pEdict = engine->PEntityOfEntIndex(client);
	
	/* If we got here, the command failed... */
	char buffer[128];
	if (g_Translator.CoreTrans(client, buffer, sizeof(buffer), "No Access", NULL, NULL)
		!= Trans_Okay)
	{
		UTIL_Format(buffer, sizeof(buffer), "You do not have access to this command");
	}

	unsigned int replyto = g_ChatTriggers.GetReplyTo();
	if (replyto == SM_REPLY_CONSOLE)
	{
		char fullbuffer[192];
		UTIL_Format(fullbuffer, sizeof(fullbuffer), "[SM] %s.\n", buffer);
		engine->ClientPrintf(pEdict, fullbuffer);
	}
	else if (replyto == SM_REPLY_CHAT)
	{
		char fullbuffer[192];
		UTIL_Format(fullbuffer, sizeof(fullbuffer), "[SM] %s.", buffer);
		g_HL2.TextMsg(client, HUD_PRINTTALK, fullbuffer);
	}
	
	return false;
}

bool ConCmdManager::AddConsoleCommand(IPluginFunction *pFunction, 
									   const char *name, 
									   const char *description, 
									   int flags)
{
	ConCmdInfo *pInfo = AddOrFindCommand(name, description, flags);

	if (!pInfo)
	{
		return false;
	}

	CmdHook *pHook = new CmdHook();

	pHook->pf = pFunction;
	if (description && description[0])
	{
		pHook->helptext.assign(description);
	}
	pInfo->conhooks.push_back(pHook);

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
	info.pHook = pHook;
	AddToPlCmdList(pList, info);

	return true;
}

bool ConCmdManager::AddAdminCommand(IPluginFunction *pFunction, 
									 const char *name, 
									 const char *group,
									 int adminflags,
									 const char *description, 
									 int flags)
{
	ConCmdInfo *pInfo = AddOrFindCommand(name, description, flags);

	if (!pInfo)
	{
		return false;
	}

	CmdHook *pHook = new CmdHook();
	AdminCmdInfo *pAdmin = new AdminCmdInfo();

	pHook->pf = pFunction;
	if (description && description[0])
	{
		pHook->helptext.assign(description);
	}
	pHook->pAdmin = pAdmin;

	void *object;
	int grpid;
	if (!sm_trie_retrieve(m_pCmdGrps, group, (void **)&object))
	{
		grpid = m_Strings.AddString(group);
		sm_trie_insert(m_pCmdGrps, group, (void *)grpid);
	}
	else
	{
		grpid = (int)object;
	}

	pAdmin->cmdGrpId = grpid;
	pAdmin->flags = adminflags;

	/* First get the command group override, if any */
	bool override = g_Admins.GetCommandOverride(group, 
		Override_CommandGroup,
		&(pAdmin->eflags));

	/* Next get the command override, if any */
	if (g_Admins.GetCommandOverride(name,
		Override_Command,
		&(pAdmin->eflags)))
	{
		override = true;
	}

	/* Assign normal flags if there were no overrides */
	if (!override)
	{
		pAdmin->eflags = pAdmin->flags;
	}

	/* Finally, add the hook */
	pInfo->conhooks.push_back(pHook);
	pInfo->admin = *(pHook->pAdmin);
	pInfo->is_admin_set = true;

	/* Now add to the plugin */
	CmdList *pList;
	IPlugin *pPlugin = g_PluginSys.GetPluginByCtx(pFunction->GetParentContext()->GetContext());
	if (!pPlugin->GetProperty("CommandList", (void **)&pList))
	{
		pList = new CmdList();
		pPlugin->SetProperty("CommandList", pList);
	}
	PlCmdInfo info;
	info.pInfo = pInfo;
	info.type = Cmd_Admin;
	info.pHook = pHook;
	AddToPlCmdList(pList, info);

	return true;
}

bool ConCmdManager::AddServerCommand(IPluginFunction *pFunction, 
									  const char *name, 
									  const char *description, 
									  int flags)

{
	ConCmdInfo *pInfo = AddOrFindCommand(name, description, flags);

	if (!pInfo)
	{
		return false;
	}

	CmdHook *pHook = new CmdHook();

	pHook->pf = pFunction;
	if (description && description[0])
	{
		pHook->helptext.assign(description);
	}

	pInfo->srvhooks.push_back(pHook);

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
	info.pHook = pHook;
	AddToPlCmdList(pList, info);

	return true;
}

void AddToPlCmdList(CmdList *pList, const PlCmdInfo &info)
{
	CmdList::iterator iter = pList->begin();
	bool inserted = false;
	const char *orig = NULL;

	orig = info.pInfo->pCmd->GetName();

	/* Insert this into the help list, SORTED alphabetically. */
	while (iter != pList->end())
	{
		PlCmdInfo &obj = (*iter);
		const char *cmd = obj.pInfo->pCmd->GetName();
		if (strcmp(orig, cmd) < 0)
		{
			pList->insert(iter, info);
			inserted = true;
			break;
		}
		iter++;
	}

	if (!inserted)
	{
		pList->push_back(info);
	}
}

void ConCmdManager::AddToCmdList(ConCmdInfo *info)
{
	List<ConCmdInfo *>::iterator iter = m_CmdList.begin();
	ConCmdInfo *pInfo;
	bool inserted = false;
	const char *orig = NULL;

	orig = info->pCmd->GetName();

	/* Insert this into the help list, SORTED alphabetically. */
	while (iter != m_CmdList.end())
	{
		const char *cmd = NULL;
		pInfo = (*iter);
		cmd = pInfo->pCmd->GetName();
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

void ConCmdManager::UpdateAdminCmdFlags(const char *cmd, OverrideType type, FlagBits bits, bool remove)
{
	ConCmdInfo *pInfo;

	if (type == Override_Command)
	{
		if (!sm_trie_retrieve(m_pCmds, cmd, (void **)&pInfo))
		{
			return;
		}

		List<CmdHook *>::iterator iter;
		CmdHook *pHook;

		for (iter=pInfo->conhooks.begin(); iter!=pInfo->conhooks.end(); iter++)
		{
			pHook = (*iter);
			if (pHook->pAdmin)
			{
				if (!remove)
				{
					pHook->pAdmin->eflags = bits;
				} else {
					pHook->pAdmin->eflags = pHook->pAdmin->flags;
				}
				pInfo->admin = *(pHook->pAdmin);
			}
		}
		pInfo->is_admin_set = true;
	}
	else if (type == Override_CommandGroup)
	{
		void *object;
		if (!sm_trie_retrieve(m_pCmdGrps, cmd, &object))
		{
			return;
		}
		int grpid = (int)object;

		/* This is bad :( loop through all commands */
		List<ConCmdInfo *>::iterator iter;
		CmdHook *pHook;
		for (iter=m_CmdList.begin(); iter!=m_CmdList.end(); iter++)
		{
			pInfo = (*iter);
			for (List<CmdHook *>::iterator citer=pInfo->conhooks.begin();
				 citer!=pInfo->conhooks.end();
				 citer++)
			{
				pHook = (*citer);
				if (pHook->pAdmin && pHook->pAdmin->cmdGrpId == grpid)
				{
					if (remove)
					{
						pHook->pAdmin->eflags = bits;
					} else {
						pHook->pAdmin->eflags = pHook->pAdmin->flags;
					}
					pInfo->admin = *(pHook->pAdmin);
				}
			}
		}
		pInfo->is_admin_set = true;
	}
}

void ConCmdManager::RemoveConCmd(ConCmdInfo *info, const char *name, bool is_read_safe, bool untrack)
{
	/* Remove from the trie */
	sm_trie_delete(m_pCmds, name);

	/* Remove console-specific information
	 * This should always be true as of right now
	 */
	if (info->pCmd)
	{
		if (info->sourceMod)
		{
			/* Unlink from SourceMM */
			g_SMAPI->UnregisterConCommandBase(g_PLAPI, info->pCmd);
			/* Delete the command's memory */
			char *new_help = const_cast<char *>(info->pCmd->GetHelpText());
			char *new_name = const_cast<char *>(info->pCmd->GetName());
			delete [] new_help;
			delete [] new_name;
			delete info->pCmd;
		}
		else
		{
			if (is_read_safe)
			{
				/* Remove the external hook */
				SH_REMOVE_HOOK_STATICFUNC(ConCommand, Dispatch, info->pCmd, CommandCallback, false);
			}
			if (untrack)
			{
				UntrackConCommandBase(info->pCmd, this);
			}
		}
	}
	
	/* Remove from list */
	m_CmdList.remove(info);

	delete info;
}

bool ConCmdManager::LookForSourceModCommand(const char *cmd)
{
	ConCmdInfo *pInfo;

	if (!sm_trie_retrieve(m_pCmds, cmd, (void **)&pInfo))
	{
		return false;
	}

	return pInfo->sourceMod && (pInfo->conhooks.size() > 0);
}

bool ConCmdManager::LookForCommandAdminFlags(const char *cmd, FlagBits *pFlags)
{
	ConCmdInfo *pInfo;

	if (!sm_trie_retrieve(m_pCmds, cmd, (void **)&pInfo))
	{
		return false;
	}

	*pFlags = pInfo->admin.eflags;

	return pInfo->is_admin_set;
}

ConCmdInfo *ConCmdManager::AddOrFindCommand(const char *name, const char *description, int flags)
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
			if (strcmp(pBase->GetName(), name) == 0)
			{
				/* Don't want to return convar with same name */
				if (!pBase->IsCommand())
				{
					return NULL;
				}

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
		}
		else
		{
			TrackConCommandBase(pCmd, this);
			SH_ADD_HOOK_STATICFUNC(ConCommand, Dispatch, pCmd, CommandCallback, false);
		}

		pInfo->pCmd = pCmd;
		pInfo->is_admin_set = false;

		sm_trie_insert(m_pCmds, name, pInfo);
		AddToCmdList(pInfo);
	}

	return pInfo;
}

void ConCmdManager::OnRootConsoleCommand(const char *cmdname, const CCommand &command)
{
	if (command.ArgC() >= 3)
	{
		const char *text = command.Arg(2);

		CPlugin *pPlugin = g_PluginSys.FindPluginByConsoleArg(text);

		if (!pPlugin)
		{
			g_RootMenu.ConsolePrint("[SM] Plugin \"%s\" was not found.", text);
			return;
		}

		const sm_plugininfo_t *plinfo = pPlugin->GetPublicInfo();
		const char *plname = IS_STR_FILLED(plinfo->name) ? plinfo->name : pPlugin->GetFilename();

		CmdList *pList;
		if (!pPlugin->GetProperty("CommandList", (void **)&pList))
		{
			g_RootMenu.ConsolePrint("[SM] No commands found for: %s", plname);
			return;
		}
		if (!pList->size())
		{
			g_RootMenu.ConsolePrint("[SM] No commands found for: %s", plname);
			return;
		}

		CmdList::iterator iter;
		const char *type = NULL;
		const char *name;
		const char *help;
		g_RootMenu.ConsolePrint("[SM] Listing %d commands for: %s", pList->size(), plname);
		g_RootMenu.ConsolePrint("  %-17.16s %-8.7s %s", "[Name]", "[Type]", "[Help]");
		for (iter=pList->begin();
			 iter!=pList->end();
			 iter++)
		{
			PlCmdInfo &cmd = (*iter);
			if (cmd.type == Cmd_Server)
			{
				type = "server";
			}
			else if (cmd.type == Cmd_Console)
			{
				type = "console";
			}
			else if (cmd.type == Cmd_Admin)
			{
				type = "admin";
			}
			name = cmd.pInfo->pCmd->GetName();
			if (cmd.pHook->helptext.size())
			{
				help = cmd.pHook->helptext.c_str();
			}
			else
			{
				help = cmd.pInfo->pCmd->GetHelpText();
			}
			g_RootMenu.ConsolePrint("  %-17.16s %-12.11s %s", name, type, help);		
		}

		return;
	}

	g_RootMenu.ConsolePrint("[SM] Usage: sm cmds <plugin #>");
}
