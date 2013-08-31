/**
 * vim: set ts=4 sw=4 tw=99 noet :
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
#include "HalfLife2.h"
#include "ChatTriggers.h"
#include "logic_bridge.h"

ConCmdManager g_ConCmds;

#if SOURCE_ENGINE == SE_DOTA
	SH_DECL_HOOK2_void(ConCommand, Dispatch, SH_NOATTRIB, false, const CCommandContext &, const CCommand &);
#elif SOURCE_ENGINE >= SE_ORANGEBOX
	SH_DECL_HOOK1_void(ConCommand, Dispatch, SH_NOATTRIB, false, const CCommand &);
#else
	SH_DECL_HOOK0_void(ConCommand, Dispatch, SH_NOATTRIB, false);
#endif

SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, false, int);

typedef ke::LinkedList<CmdHook *> PluginHookList;
void RegisterInPlugin(CmdHook *hook);

ConCmdManager::ConCmdManager() : m_Strings(1024)
{
	m_CmdClient = 0;
}

ConCmdManager::~ConCmdManager()
{
}

void ConCmdManager::OnSourceModAllInitialized()
{
	scripts->AddPluginsListener(this);
	g_RootMenu.AddRootConsoleCommand("cmds", "List console commands", this);
	SH_ADD_HOOK(IServerGameClients, SetCommandClient, serverClients, SH_MEMBER(this, &ConCmdManager::SetCommandClient), false);
}

void ConCmdManager::OnSourceModShutdown()
{
	scripts->RemovePluginsListener(this);
	/* All commands should already be removed by the time we're done */
	SH_REMOVE_HOOK(IServerGameClients, SetCommandClient, serverClients, SH_MEMBER(this, &ConCmdManager::SetCommandClient), false);
	g_RootMenu.RemoveRootConsoleCommand("cmds", this);
}

void ConCmdManager::OnUnlinkConCommandBase(ConCommandBase *pBase, const char *name, bool is_read_safe)
{
	/* Whoa, first get its information struct */
	ConCmdInfo *pInfo;

	if (!m_Cmds.retrieve(name, &pInfo))
		return;

	CmdHookList::iterator iter = pInfo->hooks.begin();
	while (iter != pInfo->hooks.end())
	{
		CmdHook *hook = *iter;

		IPluginContext *pContext = hook->pf->GetParentContext();
		IPlugin *pPlugin = scripts->FindPluginByContext(pContext->GetContext());

		// The list is guaranteed to exist.
		PluginHookList *list;
		pPlugin->GetProperty("CommandList", (void **)&list, false);
		for (PluginHookList::iterator hiter = list->begin(); hiter != list->end(); hiter++)
		{
			if (*hiter == hook)
			{
				list->erase(hiter);
				break;
			}
		}

		iter = pInfo->hooks.erase(iter);
		delete hook;
	}

	RemoveConCmd(pInfo, name, is_read_safe, false);
}

void ConCmdManager::OnPluginDestroyed(IPlugin *plugin)
{
	PluginHookList *pList;
	if (!plugin->GetProperty("CommandList", (void **)&pList, true))
		return;

	PluginHookList::iterator iter = pList->begin();
	while (iter != pList->end())
	{
		CmdHook *hook = *iter;

		hook->info->hooks.remove(hook);

		if (hook->info->hooks.empty())
			RemoveConCmd(hook->info, hook->info->pCmd->GetName(), true, true);

		iter = pList->erase(iter);
		delete hook;
	}

	delete pList;
}

#if SOURCE_ENGINE == SE_DOTA
void CommandCallback(const CCommandContext &context, const CCommand &command)
{
#elif SOURCE_ENGINE >= SE_ORANGEBOX
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

ConCmdInfo *ConCmdManager::FindInTrie(const char *name)
{
	ConCmdInfo *pInfo;
	if (!m_Cmds.retrieve(name, &pInfo))
		return NULL;
	return pInfo;
}

ConCmdList::iterator ConCmdManager::FindInList(const char *cmd)
{
	List<ConCmdInfo *>::iterator iter = m_CmdList.begin();

	while (iter != m_CmdList.end())
	{
		if (strcasecmp((*iter)->pCmd->GetName(), cmd) == 0)
			break;
		iter++;
	}

	return iter;
}

ResultType ConCmdManager::DispatchClientCommand(int client, const char *cmd, int args, ResultType type)
{
	ConCmdInfo *pInfo;

	if ((pInfo = FindInTrie(cmd)) == NULL)
	{
		ConCmdList::iterator item = FindInList(cmd);
		if (item == m_CmdList.end())
			return type;
		pInfo = *item;
	}

	cell_t result = type;
	for (CmdHookList::iterator iter = pInfo->hooks.begin(); iter != pInfo->hooks.end(); iter++)
	{
		CmdHook *hook = *iter;

		if (hook->type == CmdHook::Server || !hook->pf->IsRunnable())
			continue;

		if (hook->admin && !CheckAccess(client, cmd, hook->admin))
		{
			if (result < Pl_Handled)
				result = Pl_Handled;
			continue;
		}

		hook->pf->PushCell(client);
		hook->pf->PushCell(args);

		cell_t tempres = result;
		if (hook->pf->Execute(&tempres) == SP_ERROR_NONE)
		{
			if (tempres > result)
				result = tempres;
			if (result == Pl_Stop)
				break;
		}
	}

	return (ResultType)result;
}

void ConCmdManager::InternalDispatch(const CCommand &command)
{
	int client = m_CmdClient;

	if (client)
	{
		CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
		if (!pPlayer || !pPlayer->IsConnected())
			return;
	}

	/**
	 * Note: Console commands will EITHER go through IServerGameDLL::ClientCommand,
	 * OR this dispatch.  They will NEVER go through both.
	 * -- 
	 * Whether or not it goes through the callback is determined by FCVAR_GAMEDLL
	 */
	const char *cmd = g_HL2.CurrentCommandName();

	ConCmdInfo *pInfo = FindInTrie(cmd);
	if (pInfo == NULL)
	{
        /* Unfortunately, we now have to do a slow lookup because Valve made client commands 
         * case-insensitive.  We can't even use our sortedness.
         */
        if (client == 0 && !engine->IsDedicatedServer())
            return;

		ConCmdList::iterator item = FindInList(cmd);
		if (item == m_CmdList.end())
			return;

		pInfo = *item;
	}

	/* This is a hack to prevent say triggers from firing on messages that were 
	 * blocked because of flooding.  We won't remove this, but the hack will get 
	 * "nicer" when we expose explicit say hooks.
	 */
	if (g_ChatTriggers.WasFloodedMessage())
		return;

	cell_t result = Pl_Continue;
	int args = command.ArgC() - 1;

	// On a listen server, sometimes the server host's client index can be set
	// as 0. So index 1 is passed to the command callback to correct this
	// potential problem.
	int realClient = (!engine->IsDedicatedServer() && client == 0)
	                 ? g_Players.ListenClient()
	                 : client;
	int dedicatedClient = engine->IsDedicatedServer() ? 0 : g_Players.ListenClient();

	for (CmdHookList::iterator iter = pInfo->hooks.begin(); iter != pInfo->hooks.end(); iter++)
	{
		CmdHook *hook = *iter;

		if (!hook->pf->IsRunnable())
			continue;

		if (hook->type == CmdHook::Server)
		{
			// Don't execute unless we're in the server console.
			if (realClient != dedicatedClient)
				continue;
		} else {
			// Check admin rights if needed. realClient isn't needed since we
			// should bypass admin checks if client == 0 anyway.
			if (client && hook->admin && !CheckAccess(client, cmd, hook->admin))
			{
				if (result < Pl_Handled)
					result = Pl_Handled;
				continue;
			}

			// Client hooks get a client argument.
			hook->pf->PushCell(realClient);
		}

		hook->pf->PushCell(args);

		cell_t tempres = result;
		if (hook->pf->Execute(&tempres) == SP_ERROR_NONE)
		{
			if (tempres > result)
				result = tempres;
			if (result == Pl_Stop)
				break;
		}
	}

	if (result >= Pl_Handled)
	{
		if (!pInfo->sourceMod)
			RETURN_META(MRES_SUPERCEDE);
		return;
	}
}

bool ConCmdManager::CheckClientCommandAccess(int client, const char *cmd, FlagBits cmdflags)
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

	return CheckAdminCommandAccess(player->GetAdminId(), cmd, cmdflags);
}

bool ConCmdManager::CheckAdminCommandAccess(AdminId adm, const char *cmd, FlagBits cmdflags)
{
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
	if (CheckClientCommandAccess(client, cmd, pAdmin->eflags))
	{
		return true;
	}

#if SOURCE_ENGINE != SE_DOTA
	edict_t *pEdict = PEntityOfEntIndex(client);
#endif
	
	/* If we got here, the command failed... */
	char buffer[128];
	if (!logicore.CoreTranslate(buffer, sizeof(buffer), "%T", 2, NULL, "No Access", &client))
	{
		UTIL_Format(buffer, sizeof(buffer), "You do not have access to this command");
	}

	unsigned int replyto = g_ChatTriggers.GetReplyTo();
	if (replyto == SM_REPLY_CONSOLE)
	{
		char fullbuffer[192];
		UTIL_Format(fullbuffer, sizeof(fullbuffer), "[SM] %s.\n", buffer);
#if SOURCE_ENGINE == SE_DOTA
		engine->ClientPrintf(client, fullbuffer);
#else
		engine->ClientPrintf(pEdict, fullbuffer);
#endif
	}
	else if (replyto == SM_REPLY_CHAT)
	{
		char fullbuffer[192];
		UTIL_Format(fullbuffer, sizeof(fullbuffer), "[SM] %s.", buffer);
		g_HL2.TextMsg(client, HUD_PRINTTALK, fullbuffer);
	}
	
	return false;
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
		return false;

	CmdHook *pHook = new CmdHook(CmdHook::Client, pInfo, pFunction, description);

	pHook->admin = new AdminCmdInfo();

	int grpid;
	if (!m_CmdGrps.retrieve(group, &grpid))
	{
		grpid = m_Strings.AddString(group);
		m_CmdGrps.insert(group, grpid);
	}

	pHook->admin->cmdGrpId = grpid;
	pHook->admin->flags = adminflags;

	/* First get the command group override, if any */
	bool override = g_Admins.GetCommandOverride(group, 
		Override_CommandGroup,
		&(pHook->admin->eflags));

	/* Next get the command override, if any */
	if (g_Admins.GetCommandOverride(name,
		Override_Command,
		&(pHook->admin->eflags)))
	{
		override = true;
	}

	/* Assign normal flags if there were no overrides */
	if (!override)
		pHook->admin->eflags = pHook->admin->flags;
	pInfo->eflags = pHook->admin->eflags;

	pInfo->hooks.append(pHook);
	RegisterInPlugin(pHook);
	return true;
}

bool ConCmdManager::AddServerCommand(IPluginFunction *pFunction, 
									  const char *name, 
									  const char *description, 
									  int flags)

{
	ConCmdInfo *pInfo = AddOrFindCommand(name, description, flags);

	if (!pInfo)
		return false;

	CmdHook *pHook = new CmdHook(CmdHook::Server, pInfo, pFunction, description);

	pInfo->hooks.append(pHook);
	RegisterInPlugin(pHook);
	return true;
}

void RegisterInPlugin(CmdHook *hook)
{
	PluginHookList *pList;

	IPlugin *pPlugin = scripts->FindPluginByContext(hook->pf->GetParentContext());
	if (!pPlugin->GetProperty("CommandList", (void **)&pList))
	{
		pList = new PluginHookList();
		pPlugin->SetProperty("CommandList", pList);
	}

	const char *orig = hook->info->pCmd->GetName();

	/* Insert this into the help list, SORTED alphabetically. */
	PluginHookList::iterator iter = pList->begin();
	while (iter != pList->end())
	{
		const char *cmd = (*iter)->info->pCmd->GetName();
		if (strcmp(orig, cmd) < 0)
		{
			pList->insertBefore(iter, hook);
			return;
		}
		iter++;
	}

	pList->append(hook);
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
	if (type == Override_Command)
	{
		ConCmdInfo *pInfo;
		if (!m_Cmds.retrieve(cmd, &pInfo))
			return;

		for (CmdHookList::iterator iter = pInfo->hooks.begin(); iter != pInfo->hooks.end(); iter++)
		{
			if (!iter->admin)
				continue;

			if (!remove)
				iter->admin->eflags = bits;
			else
				iter->admin->eflags = iter->admin->flags;
			pInfo->eflags = iter->admin->eflags;
		}
	}
	else if (type == Override_CommandGroup)
	{
		int grpid;
		if (!m_CmdGrps.retrieve(cmd, &grpid))
			return;

		/* This is bad :( loop through all commands */
		List<ConCmdInfo *>::iterator iter;
		for (iter=m_CmdList.begin(); iter!=m_CmdList.end(); iter++)
		{
			ConCmdInfo *pInfo = *iter;
			for (CmdHookList::iterator citer = pInfo->hooks.begin(); citer != pInfo->hooks.end(); citer++)
			{
				CmdHook *hook = (*citer);
				if (!hook->admin || hook->admin->cmdGrpId != grpid)
					continue;

				if (remove)
					hook->admin->eflags = bits;
				else
					hook->admin->eflags = hook->admin->flags;
				pInfo->eflags = hook->admin->eflags;
			}
		}
	}
}

void ConCmdManager::RemoveConCmd(ConCmdInfo *info, const char *name, bool is_read_safe, bool untrack)
{
	/* Remove from the trie */
	m_Cmds.remove(name);

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
				SH_REMOVE_HOOK(ConCommand, Dispatch, info->pCmd, SH_STATIC(CommandCallback), false);
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
	if (!m_Cmds.retrieve(cmd, &pInfo))
		return false;

	return pInfo->sourceMod && !pInfo->hooks.empty();
}

bool ConCmdManager::LookForCommandAdminFlags(const char *cmd, FlagBits *pFlags)
{
	ConCmdInfo *pInfo;
	if (!m_Cmds.retrieve(cmd, &pInfo))
		return false;

	*pFlags = pInfo->eflags;
	return true;
}

ConCmdInfo *ConCmdManager::AddOrFindCommand(const char *name, const char *description, int flags)
{
	ConCmdInfo *pInfo;
	if (!m_Cmds.retrieve(name, &pInfo))
	{
		ConCmdList::iterator item = FindInList(name);
		if (item != m_CmdList.end())
			return *item;

		pInfo = new ConCmdInfo();
		/* Find the commandopan */
		ConCommand *pCmd = FindCommand(name);

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
			SH_ADD_HOOK(ConCommand, Dispatch, pCmd, SH_STATIC(CommandCallback), false);
		}

		pInfo->pCmd = pCmd;

		m_Cmds.insert(name, pInfo);
		AddToCmdList(pInfo);
	}

	return pInfo;
}

void ConCmdManager::OnRootConsoleCommand(const char *cmdname, const CCommand &command)
{
	if (command.ArgC() >= 3)
	{
		const char *text = command.Arg(2);

		IPlugin *pPlugin = scripts->FindPluginByConsoleArg(text);

		if (!pPlugin)
		{
			g_RootMenu.ConsolePrint("[SM] Plugin \"%s\" was not found.", text);
			return;
		}

		const sm_plugininfo_t *plinfo = pPlugin->GetPublicInfo();
		const char *plname = IS_STR_FILLED(plinfo->name) ? plinfo->name : pPlugin->GetFilename();

		PluginHookList *pList;
		if (!pPlugin->GetProperty("CommandList", (void **)&pList))
		{
			g_RootMenu.ConsolePrint("[SM] No commands found for: %s", plname);
			return;
		}
		if (pList->empty())
		{
			g_RootMenu.ConsolePrint("[SM] No commands found for: %s", plname);
			return;
		}

		const char *type = NULL;
		const char *name;
		const char *help;
		g_RootMenu.ConsolePrint("[SM] Listing commands for: %s", plname);
		g_RootMenu.ConsolePrint("  %-17.16s %-8.7s %s", "[Name]", "[Type]", "[Help]");
		for (PluginHookList::iterator iter = pList->begin(); iter != pList->end(); iter++)
		{
			CmdHook *hook = *iter;
			if (hook->type == CmdHook::Server)
				type = "server";
			else 
				type = hook->info->eflags == 0 ? "console" : "admin";

			name = hook->info->pCmd->GetName();
			if (hook->helptext.length())
				help = hook->helptext.chars();
			else
				help = hook->info->pCmd->GetHelpText();
			g_RootMenu.ConsolePrint("  %-17.16s %-12.11s %s", name, type, help);		
		}

		return;
	}

	g_RootMenu.ConsolePrint("[SM] Usage: sm cmds <plugin #>");
}
