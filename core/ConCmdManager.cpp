/**
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

#include "ConCmdManager.h"
#include "sm_srvcmds.h"
#include "AdminCache.h"
#include "sm_stringutil.h"
#include "PlayerManager.h"
#include "Translator.h"

ConCmdManager g_ConCmds;

SH_DECL_HOOK0_void(ConCommand, Dispatch, SH_NOATTRIB, false);
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
	g_PluginSys.RemovePluginsListener(this);
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
		} else {
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
			RemoveConCmd(pInfo);
			removed.push_back(pInfo);
		}
		delete pList;
	}
}

void CommandCallback()
{
	g_ConCmds.InternalDispatch();
}

void ConCmdManager::SetCommandClient(int client)
{
	m_CmdClient = client + 1;
}

ResultType ConCmdManager::DispatchClientCommand(int client, ResultType type)
{
	const char *cmd = engine->Cmd_Argv(0);
	int args = engine->Cmd_Argc() - 1;

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

void ConCmdManager::InternalDispatch()
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

	List<CmdHook *>::iterator iter;
	CmdHook *pHook;

	/* Execute server-only commands if viable */
	if (m_CmdClient == 0 && pInfo->srvhooks.size())
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
			if (m_CmdClient 
				&& pHook->pAdmin
				&& !CheckAccess(m_CmdClient, cmd, pHook->pAdmin))
			{
				if (result < Pl_Handled)
				{
					result = Pl_Handled;
				}
				continue;
			}
			pHook->pf->PushCell(m_CmdClient);
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

bool ConCmdManager::CheckAccess(int client, const char *cmd, AdminCmdInfo *pAdmin)
{
	FlagBits cmdflags = pAdmin->eflags;
	if (cmdflags == 0)
	{
		return true;
	}

	CPlayer *player = g_Players.GetPlayerByIndex(client);
	if (!player)
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

		/* See if our other flags match */
		if ((bits & cmdflags) == cmdflags)
		{
			return true;
		}

		/* Check for overrides */
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
				} else if (rule == Command_Deny) {
					break;
				}
			}
		}
	}

	if (player->IsFakeClient()
		|| player->GetEdict() == NULL)
	{
		return false;
	}
	
	/* If we got here, the command failed... */
	char buffer[128];
	if (g_Translator.CoreTrans(client, buffer, sizeof(buffer), "No Access", NULL, NULL)
		!= Trans_Okay)
	{
		snprintf(buffer, sizeof(buffer), "You do not have access to this command");
	}

	char fullbuffer[192];
	snprintf(fullbuffer, sizeof(fullbuffer), "[SM] %s.\n", buffer);

	engine->ClientPrintf(player->GetEdict(), fullbuffer);
	
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
	} else {
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

void ConCmdManager::UpdateAdminCmdFlags(const char *cmd, OverrideType type, FlagBits bits)
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
				if (bits)
				{
					pHook->pAdmin->eflags = bits;
				} else {
					pHook->pAdmin->eflags = pHook->pAdmin->flags;
				}
			}
		}
	} else if (type == Override_CommandGroup) {
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
					if (bits)
					{
						pHook->pAdmin->eflags = bits;
					} else {
						pHook->pAdmin->eflags = pHook->pAdmin->flags;
					}
				}
			}
		}
	}
}

void ConCmdManager::RemoveConCmd(ConCmdInfo *info)
{
	/* Remove console-specific information
	 * This should always be true as of right now
	 */
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

	delete info;
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
		} else {
			SH_ADD_HOOK_STATICFUNC(ConCommand, Dispatch, pCmd, CommandCallback, false);
		}

		pInfo->pCmd = pCmd;

		sm_trie_insert(m_pCmds, name, pInfo);
		AddToCmdList(pInfo);
	}

	return pInfo;
}

void ConCmdManager::OnRootConsoleCommand(const char *command, unsigned int argcount)
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
			 iter++, id++)
		{
			PlCmdInfo &cmd = (*iter);
			if (cmd.type == Cmd_Server)
			{
				type = "server";
			} else if (cmd.type == Cmd_Console) {
				type = "console";
			} else if (cmd.type == Cmd_Admin) {
				type = "admin";
			}
			name = cmd.pInfo->pCmd->GetName();
			if (cmd.pHook->helptext.size())
			{
				help = cmd.pHook->helptext.c_str();
			} else {
				help = cmd.pInfo->pCmd->GetHelpText();
			}
			g_RootMenu.ConsolePrint("  %-17.16s %-12.11s %s", name, type, help);		
		}

		return;
	}

	g_RootMenu.ConsolePrint("[SM] Usage: sm cmds <plugin #>");
}

static int g_yam_state = 0;
void _YamState(int state)
{
	if (state == 0)
	{
		g_RootMenu.ConsolePrint("Welcome to the SourceMod Text Adventure.");
		g_RootMenu.ConsolePrint("Type sm_text to see the last message.");
		g_RootMenu.ConsolePrint("Type sm_text <word> to follow a capital word.");
		g_RootMenu.ConsolePrint("-------------------------------");
		g_RootMenu.ConsolePrint("You are at VALVE HEADQUARTERS.");
		g_RootMenu.ConsolePrint("To your LEFT is BAILOPAN, rearranging the letters to spell");
		g_RootMenu.ConsolePrint("\"A HARD VEAL QUEST\".  FORWARD is the entrance to the building.");
		g_RootMenu.ConsolePrint("To your RIGHT is your last chance to flee in terror.");
	} else if (state == 1) {
		g_RootMenu.ConsolePrint("BAILOPAN tells you that his name his pronounced");
		g_RootMenu.ConsolePrint("\"bye low pahn,\" not \"bay low pan.\"  Do you ");
		g_RootMenu.ConsolePrint("MOCK him, or NOD quietly?");
	} else if (state == 2) {
		g_RootMenu.ConsolePrint("You enter the Valve building.  You hear screams coming from within.");
		g_RootMenu.ConsolePrint("A grotesque figure lumbers up to greet you; it is Gabe Newell.");
		g_RootMenu.ConsolePrint("\"Welcome,\" he belches, \"to my lair.\"");
		g_RootMenu.ConsolePrint("Do you SHAKE Gaben's hand, WALK past him, or OFFER a donut?");
	} else if (state == 3) {
		g_RootMenu.ConsolePrint("You walk into the break room.  Alfred \"Adolf\" Reynolds and");
		g_RootMenu.ConsolePrint("Yahn \"Yeti\" Bernier are discussing something (you overhear ");
		g_RootMenu.ConsolePrint("the phrase \"and next Steam update, here's what we should break\").");
		g_RootMenu.ConsolePrint("Should you DIE in a fire, REPORT a bug, REQUEST a feature, or ");
		g_RootMenu.ConsolePrint("SPRAY them with butter?");
	}
	g_yam_state = state;
}

void _IntExt_CallYams()
{
	const char *arg = engine->Cmd_Argv(1);

	/* should be impossible */
	if (!arg || arg[0] == '\0')
	{
		_YamState(g_yam_state);
	}

	if (g_yam_state == 1)
	{
		if (strcasecmp(arg, "mock") == 0)
		{
			g_RootMenu.ConsolePrint("You mock BAILOPAN's pronounciation.  In a fit of range, ");
			g_RootMenu.ConsolePrint("he sticks an INT 3 call into your chest, rendering you broken.");
			g_RootMenu.ConsolePrint("YOU HAVE DIED.  GAME OVER.");
			g_yam_state = 0;
		} else if (strcasecmp(arg, "nod") == 0) {
			g_RootMenu.ConsolePrint("You nod quietly, and then slowly back away into the Valve headquarters.");
			_YamState(2);
		} else {
			g_RootMenu.ConsolePrint("Commands are MOCK and NOD.");
		}
	} else if (g_yam_state == 3) {
		if (strcasecmp(arg, "report") == 0)
		{
			g_RootMenu.ConsolePrint("You report a bug to Alfred and Yeti.  Immediately, both fall asleep.");
			g_RootMenu.ConsolePrint("You decay in the break room for two years while they sleep.");
			g_RootMenu.ConsolePrint("YOU HAVE DIED.  GAME OVER.");
			g_yam_state = 0;
		} else if (strcasecmp(arg, "request") == 0) {
			g_RootMenu.ConsolePrint("You request a feature to Alfred and Yeti.  They both mutter something");
			g_RootMenu.ConsolePrint("about it being implemented \"soon.\"  Then, by accident, someone sends");
			g_RootMenu.ConsolePrint("a message over \"Friends.\"  The entire building catches fire.");
			g_RootMenu.ConsolePrint("YOU HAVE DIED.  GAME OVER.");
			g_yam_state = 0;
		} else if (strcasecmp(arg, "die") == 0) {
			g_RootMenu.ConsolePrint("For no reason, you suddenly catch fire.  Alfred and Yeti find this");
			g_RootMenu.ConsolePrint("deeply disturbing, and cover your flaming corpse with Episode 2");
			g_RootMenu.ConsolePrint("advertisments.  Coming soon, with Team Fortress 2, and Portal!");
			g_RootMenu.ConsolePrint("YOU HAVE DIED.  GAME OVER.");
			g_yam_state = 0;
		} else if (strcasecmp(arg, "spray") == 0) {
			g_RootMenu.ConsolePrint("You spray Alfred and Yeti with butter.  Like Jack Thompson to an");
			g_RootMenu.ConsolePrint("ambulance, Gabe Newell instantly appears and devours them both.");
			g_RootMenu.ConsolePrint("You run away, just in time, as Gabe Newell explodes, registering ");
			g_RootMenu.ConsolePrint("a 5.6 earthquake.  Outside, world peace is achieved.");
			g_RootMenu.ConsolePrint("YOU HAVE WON.");
			g_yam_state = 0;
		}
	} else if (g_yam_state == 2) {
		if (strcasecmp(arg, "shake") == 0)
		{
			g_RootMenu.ConsolePrint("You shake Gaben's hand.  It is a terrifying and disgusting experience.");
			g_RootMenu.ConsolePrint("However, you survive, and continue on.");
			_YamState(3);
		} else if (strcasecmp(arg, "offer") == 0) {
			g_RootMenu.ConsolePrint("You offer Gabe Newell one (1) donut.  With a gleam in his eyes, ");
			g_RootMenu.ConsolePrint("he picks you up and devours you whole.");
			g_RootMenu.ConsolePrint("YOU HAVE DIED.  GAME OVER.");
			g_yam_state = 0;
		} else if (strcasecmp(arg, "walk") == 0) {
			g_RootMenu.ConsolePrint("You walk past Gabe Newell.  He can't keep up with your pace!");
			_YamState(3);
		} else {
			g_RootMenu.ConsolePrint("Commands are SHAKE, OFFER, and WALK.");
		}
	} else if (g_yam_state == 0) {
		if (strcasecmp(arg, "left") == 0)
		{
			_YamState(1);
		} else if (strcasecmp(arg, "right") == 0) {
			g_RootMenu.ConsolePrint("You run away from the Valve headquarters in sheer terror.");
			g_RootMenu.ConsolePrint("While running, you smash into an unknown person, who turns out to be your soul mate.");
			g_RootMenu.ConsolePrint("You marry and raise a family of 3 kids.");
			g_RootMenu.ConsolePrint("Many years later, you look back, and realize this was your best choice.");
			g_RootMenu.ConsolePrint("YOU HAVE WON.");
			g_yam_state = 0;
		} else if (strcasecmp(arg, "forward") == 0) {
			_YamState(2);
		} else if (arg[0] != '\0') {
			g_RootMenu.ConsolePrint("Commands are FORWARD, LEFT, and RIGHT.");
		}
	}
}

void _IntExt_EnableYams()
{
	static ConCommand *pCmd = NULL;
	if (!pCmd)
	{
		pCmd = new ConCommand("sm_text", _IntExt_CallYams, "Fountain of Yams Adventure Game", FCVAR_GAMEDLL);
		g_RootMenu.ConsolePrint("Something is now enabled...");
	} else {
		g_RootMenu.ConsolePrint("Something is already enabled...");
	}
}
