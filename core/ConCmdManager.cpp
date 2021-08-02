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

#include <bridge/include/IScriptManager.h>
#include "ChatTriggers.h"
#include "HalfLife2.h"
#include "PlayerManager.h"
#include "command_args.h"
#include "logic_bridge.h"
#include "provider.h"
#include "sm_stringutil.h"
#include "sourcemod.h"

using namespace ke;

ConCmdManager g_ConCmds;

typedef std::list<CmdHook *> PluginHookList;
void RegisterInPlugin(CmdHook *hook);

ConCmdManager::ConCmdManager()
{
}

ConCmdManager::~ConCmdManager()
{
}

void ConCmdManager::OnSourceModAllInitialized()
{
	scripts->AddPluginsListener(this);
	rootmenu->AddRootConsoleCommand3("cmds", "List console commands", this);
}

void ConCmdManager::OnSourceModShutdown()
{
	scripts->RemovePluginsListener(this);
	rootmenu->RemoveRootConsoleCommand("cmds", this);
}

void ConCmdManager::OnUnlinkConCommandBase(ConCommandBase *pBase, const char *name)
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

		if (hook->admin)
			hook->admin->group->hooks.remove(hook);

		iter = pInfo->hooks.erase(iter);
		delete hook;
	}

	RemoveConCmd(pInfo, name, false);
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
		if (hook->admin)
			hook->admin->group->hooks.remove(hook);

		if (hook->info->hooks.empty()) {
			RemoveConCmd(hook->info, hook->info->pCmd->GetName(), true);
		}
		else { // update plugin reference to next hook in line
			auto next = *hook->info->hooks.begin();
			next->info->pPlugin = next->plugin;
		}

		iter = pList->erase(iter);
		delete hook;
	}

	delete pList;
}

void CommandCallback(DISPATCH_ARGS)
{
	DISPATCH_PROLOGUE;
	EngineArgs args(command);

	AutoEnterCommand autoEnterCommand(&args);
	g_ConCmds.InternalDispatch(sCoreProviderImpl.CommandClient(), &args);
}

ConCmdInfo *ConCmdManager::FindInTrie(const char *name)
{
	ConCmdInfo *pInfo;
	if (!m_Cmds.retrieve(name, &pInfo))
		return NULL;
	return pInfo;
}

ResultType ConCmdManager::DispatchClientCommand(int client, const char *cmd, int args, ResultType type)
{
	ConCmdInfo *pInfo = FindInTrie(cmd);

	if (pInfo == NULL)
	{
		return type;
	}

	cell_t result = type;
	for (CmdHookList::iterator iter = pInfo->hooks.begin(); iter != pInfo->hooks.end(); iter++)
	{
		CmdHook *hook = *iter;

		if (hook->type == CmdHook::Server || !hook->pf->IsRunnable())
			continue;

		if (hook->admin && !CheckAccess(client, cmd, hook->admin.get()))
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

bool ConCmdManager::InternalDispatch(int client, const ICommandArgs *args)
{
	if (client)
	{
		CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
		if (!pPlayer || !pPlayer->IsConnected())
			return false;
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
		return false;
	}

	/* This is a hack to prevent say triggers from firing on messages that were 
	 * blocked because of flooding.  We won't remove this, but the hack will get 
	 * "nicer" when we expose explicit say hooks.
	 */
	if (g_ChatTriggers.WasFloodedMessage())
		return false;

	cell_t result = Pl_Continue;
	int argc = args->ArgC() - 1;

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
			if (client && hook->admin && !CheckAccess(client, cmd, hook->admin.get()))
			{
				if (result < Pl_Handled)
					result = Pl_Handled;
				continue;
			}

			// Client hooks get a client argument.
			hook->pf->PushCell(realClient);
		}

		hook->pf->PushCell(argc);

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
		return !pInfo->sourceMod;

	return false;
}

bool ConCmdManager::CheckAccess(int client, const char *cmd, AdminCmdInfo *pAdmin)
{
	if (adminsys->CheckClientCommandAccess(client, cmd, pAdmin->eflags))
	{
		return true;
	}

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer)
	{
		return false;
	}

	/* If we got here, the command failed... */
	char buffer[128];
	if (!logicore.CoreTranslate(buffer, sizeof(buffer), "%T", 2, NULL, "No Access", &client))
	{
		ke::SafeStrcpy(buffer, sizeof(buffer), "You do not have access to this command");
	}

	unsigned int replyto = g_ChatTriggers.GetReplyTo();
	if (replyto == SM_REPLY_CONSOLE)
	{
		char fullbuffer[192];
		ke::SafeSprintf(fullbuffer, sizeof(fullbuffer), "[SM] %s.\n", buffer);
		pPlayer->PrintToConsole(fullbuffer);
	}
	else if (replyto == SM_REPLY_CHAT)
	{
		char fullbuffer[192];
		ke::SafeSprintf(fullbuffer, sizeof(fullbuffer), "[SM] %s.", buffer);
		g_HL2.TextMsg(client, HUD_PRINTTALK, fullbuffer);
	}
	
	return false;
}

bool ConCmdManager::AddAdminCommand(IPluginFunction *pFunction, 
									 const char *name, 
									 const char *group,
									 int adminflags,
									 const char *description, 
									 int flags,
									 IPlugin *pPlugin)
{
	ConCmdInfo *pInfo = AddOrFindCommand(name, description, flags, pPlugin);

	if (!pInfo)
		return false;

	GroupMap::Insert i = m_CmdGrps.findForAdd(group);
	if (!i.found())
	{
		if (!m_CmdGrps.add(i, group))
			return false;
		i->value = new CommandGroup();
	}
	RefPtr<CommandGroup> cmdgroup = i->value;

	CmdHook *pHook = new CmdHook(CmdHook::Client, pInfo, pFunction, description, pPlugin);
	pHook->admin = std::make_unique<AdminCmdInfo>(cmdgroup, adminflags);

	/* First get the command group override, if any */
	bool override = adminsys->GetCommandOverride(group, 
		Override_CommandGroup,
		&(pHook->admin->eflags));

	/* Next get the command override, if any */
	if (adminsys->GetCommandOverride(name,
		Override_Command,
		&(pHook->admin->eflags)))
	{
		override = true;
	}

	/* Assign normal flags if there were no overrides */
	if (!override)
		pHook->admin->eflags = pHook->admin->flags;
	pInfo->eflags = pHook->admin->eflags;

	cmdgroup->hooks.push_back(pHook);
	pInfo->hooks.append(pHook);
	RegisterInPlugin(pHook);
	return true;
}

bool ConCmdManager::AddServerCommand(IPluginFunction *pFunction, 
									  const char *name, 
									  const char *description, 
									  int flags,
									  IPlugin *pPlugin)

{
	ConCmdInfo *pInfo = AddOrFindCommand(name, description, flags, pPlugin);

	if (!pInfo)
		return false;

	CmdHook *pHook = new CmdHook(CmdHook::Server, pInfo, pFunction, description, pPlugin);

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
			pList->emplace(iter, hook);
			return;
		}
		iter++;
	}

	pList->emplace_back(hook);
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
		GroupMap::Result r = m_CmdGrps.find(cmd);
		if (!r.found())
			return;

		RefPtr<CommandGroup> group(r->value);

		for (PluginHookList::iterator iter = group->hooks.begin(); iter != group->hooks.end(); iter++)
		{
			CmdHook *hook = *iter;
			if (!remove)
				hook->admin->eflags = bits;
			else
				hook->admin->eflags = hook->admin->flags;
			hook->info->eflags = hook->admin->eflags;
		}
	}
}

void ConCmdManager::RemoveConCmd(ConCmdInfo *info, const char *name, bool untrack)
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
			if (untrack)
				UntrackConCommandBase(info->pCmd, this);
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

ConCmdInfo *ConCmdManager::AddOrFindCommand(const char *name, const char *description, int flags, IPlugin *pPlugin)
{
	ConCmdInfo *pInfo;
	if (!m_Cmds.retrieve(name, &pInfo))
	{
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
			pInfo->pPlugin = pPlugin;
			pInfo->sourceMod = true;
		}
		else
		{
			TrackConCommandBase(pCmd, this);
			CommandHook::Callback callback = [this] (int client, const ICommandArgs *args) -> bool {
				AutoEnterCommand autoEnterCommand(args);
				return this->InternalDispatch(client, args);
			};
			pInfo->sh_hook = sCoreProviderImpl.AddCommandHook(pCmd, callback);
		}

		pInfo->pCmd = pCmd;

		m_Cmds.insert(name, pInfo);
		AddToCmdList(pInfo);
	}

	return pInfo;
}

void ConCmdManager::OnRootConsoleCommand(const char *cmdname, const ICommandArgs *command)
{
	if (command->ArgC() >= 3)
	{
		const char *text = command->Arg(2);

		IPlugin *pPlugin = scripts->FindPluginByConsoleArg(text);

		if (!pPlugin)
		{
			UTIL_ConsolePrint("[SM] Plugin \"%s\" was not found.", text);
			return;
		}

		const sm_plugininfo_t *plinfo = pPlugin->GetPublicInfo();
		const char *plname = IS_STR_FILLED(plinfo->name) ? plinfo->name : pPlugin->GetFilename();

		PluginHookList *pList;
		if (!pPlugin->GetProperty("CommandList", (void **)&pList))
		{
			UTIL_ConsolePrint("[SM] No commands found for: %s", plname);
			return;
		}
		if (pList->empty())
		{
			UTIL_ConsolePrint("[SM] No commands found for: %s", plname);
			return;
		}

		const char *type = NULL;
		const char *name;
		const char *help;
		UTIL_ConsolePrint("[SM] Listing commands for: %s", plname);
		UTIL_ConsolePrint("  %-17.16s %-8.7s %s", "[Name]", "[Type]", "[Help]");
		for (PluginHookList::iterator iter = pList->begin(); iter != pList->end(); iter++)
		{
			CmdHook *hook = *iter;
			if (hook->type == CmdHook::Server)
				type = "server";
			else 
				type = hook->info->eflags == 0 ? "console" : "admin";

			name = hook->info->pCmd->GetName();
			if (hook->helptext.length())
				help = hook->helptext.c_str();
			else
				help = hook->info->pCmd->GetHelpText();
			UTIL_ConsolePrint("  %-17.16s %-12.11s %s", name, type, help);		
		}

		return;
	}

	UTIL_ConsolePrint("[SM] Usage: sm cmds <plugin #>");
}
