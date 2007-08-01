/**
 * vim: set ts=4 :
 * ================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
 * ================================================================
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License, 
 * version 3.0, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to 
 * link the code of this program (as well as its derivative works) to 
 * "Half-Life 2," the "Source Engine," the "SourcePawn JIT," and any 
 * Game MODs that run on software by the Valve Corporation.  You must 
 * obey the GNU General Public License in all respects for all other 
 * code used. Additionally, AlliedModders LLC grants this exception 
 * to all derivative works. AlliedModders LLC defines further 
 * exceptions, found in LICENSE.txt (as of this writing, version 
 * JULY-31-2007), or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "sm_srvcmds.h"
#include "sm_version.h"
#include "sm_stringutil.h"
#include "HandleSys.h"
#include "CoreConfig.h"

RootConsoleMenu g_RootMenu;

ConVar sourcemod_version("sourcemod_version", SVN_FULL_VERSION, FCVAR_SPONLY|FCVAR_REPLICATED|FCVAR_NOTIFY, "SourceMod Version");

RootConsoleMenu::RootConsoleMenu()
{
	m_pCommands = sm_trie_create();
	m_CfgExecDone = false;
}

RootConsoleMenu::~RootConsoleMenu()
{
	sm_trie_destroy(m_pCommands);

	List<ConsoleEntry *>::iterator iter;
	for (iter=m_Menu.begin(); iter!=m_Menu.end(); iter++)
	{
		delete (*iter);
	}
	m_Menu.clear();
}

extern void _IntExt_OnHostnameChanged(ConVar *pConVar, char const *oldValue);

void RootConsoleMenu::OnSourceModStartup(bool late)
{
	ConCommandBaseMgr::OneTimeInit(this);
	AddRootConsoleCommand("version", "Display version information", this);
	AddRootConsoleCommand("credits", "Display credits listing", this);

	ConVar *pHost = icvar->FindVar("hostname");
	pHost->InstallChangeCallback(_IntExt_OnHostnameChanged);
}

void RootConsoleMenu::OnSourceModShutdown()
{
	RemoveRootConsoleCommand("credits", this);
	RemoveRootConsoleCommand("version", this);
}

bool RootConsoleMenu::RegisterConCommandBase(ConCommandBase *pCommand)
{
	META_REGCVAR(pCommand);

	/* Override values of convars created by SourceMod convar manager if specified on command line */
	const char *cmdLineValue = icvar->GetCommandLineValue(pCommand->GetName());
	if (cmdLineValue && !pCommand->IsCommand())
	{
		static_cast<ConVar *>(pCommand)->SetValue(cmdLineValue);
	}

	return true;
}

void RootConsoleMenu::ConsolePrint(const char *fmt, ...)
{
	char buffer[512];

	va_list ap;
	va_start(ap, fmt);
	size_t len = vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	if (len >= sizeof(buffer) - 1)
	{
		buffer[510] = '\n';
		buffer[511] = '\0';
	} else {
		buffer[len++] = '\n';
		buffer[len] = '\0';
	}
	
	META_CONPRINT(buffer);
}

bool RootConsoleMenu::AddRootConsoleCommand(const char *cmd, const char *text, IRootConsoleCommand *pHandler)
{
	if (sm_trie_retrieve(m_pCommands, cmd, NULL))
	{
		return false;
	}

	sm_trie_insert(m_pCommands, cmd, pHandler);

	/* Sort this into the menu */
	List<ConsoleEntry *>::iterator iter = m_Menu.begin();
	ConsoleEntry *pEntry;
	bool inserted = false;
	while (iter != m_Menu.end())
	{
		pEntry = (*iter);
		if (strcmp(cmd, pEntry->command.c_str()) < 0)
		{
			ConsoleEntry *pNew = new ConsoleEntry;
			pNew->command.assign(cmd);
			pNew->description.assign(text);
			m_Menu.insert(iter, pNew);
			inserted = true;
			break;
		}
		iter++;
	}

	if (!inserted)
	{
		ConsoleEntry *pNew = new ConsoleEntry;
		pNew->command.assign(cmd);
		pNew->description.assign(text);
		m_Menu.push_back(pNew);
	}

	return true;
}

bool RootConsoleMenu::RemoveRootConsoleCommand(const char *cmd, IRootConsoleCommand *pHandler)
{
	/* Sanity tests */
	IRootConsoleCommand *object;
	if (sm_trie_retrieve(m_pCommands, cmd, (void **)&object))
	{
		if (object != pHandler)
		{
			return false;
		}
	} else {
		return false;
	}

	sm_trie_delete(m_pCommands, cmd);

	List<ConsoleEntry *>::iterator iter;
	ConsoleEntry *pEntry;
	for (iter=m_Menu.begin(); iter!=m_Menu.end(); iter++)
	{
		pEntry = (*iter);
		if (pEntry->command.compare(cmd) == 0)
		{
			delete pEntry;
			m_Menu.erase(iter);
			break;
		}
	}

	return true;
}

void RootConsoleMenu::DrawGenericOption(const char *cmd, const char *text)
{
	char buffer[255];
	size_t len, cmdlen = strlen(cmd);

	len = UTIL_Format(buffer, sizeof(buffer), "    %s", cmd);
	if (cmdlen < 16)
	{
		size_t num = 16 - cmdlen;
		for (size_t i = 0; i < num; i++)
		{
			buffer[len++] = ' ';
		}
		len += snprintf(&buffer[len], sizeof(buffer) - len, " - %s", text);
		ConsolePrint("%s", buffer);
	}
}

extern void _IntExt_EnableYams();

void RootConsoleMenu::GotRootCmd()
{
	unsigned int argnum = GetArgumentCount();

	if (argnum >= 2)
	{
		const char *cmd = GetArgument(1);
		if (strcmp(cmd, "text") == 0)
		{
			_IntExt_EnableYams();
			return;
		} else if (strcmp(cmd, "internal") == 0) {
			if (argnum >= 3)
			{
				const char *arg = GetArgument(2);
				if (strcmp(arg, "1") == 0)
				{
					SM_ConfigsExecuted_Global();
				} else if (strcmp(arg, "2") == 0) {
					if (argnum >= 4)
					{
						SM_ConfigsExecuted_Plugin(atoi(GetArgument(3)));
					}
				}
			}
			return;
		}

		IRootConsoleCommand *pHandler;
		if (sm_trie_retrieve(m_pCommands, cmd, (void **)&pHandler))
		{
			pHandler->OnRootConsoleCommand(cmd, argnum);
			return;
		}
	}

	ConsolePrint("SourceMod Menu:");
	ConsolePrint("Usage: sm <command> [arguments]");

	List<ConsoleEntry *>::iterator iter;
	ConsoleEntry *pEntry;
	for (iter=m_Menu.begin(); iter!=m_Menu.end(); iter++)
	{
		pEntry = (*iter);
		DrawGenericOption(pEntry->command.c_str(), pEntry->description.c_str());
	}
}

const char *RootConsoleMenu::GetArgument(unsigned int argno)
{
	return engine->Cmd_Argv(argno);
}

const char *RootConsoleMenu::GetArguments()
{
	return engine->Cmd_Args();
}

unsigned int RootConsoleMenu::GetArgumentCount()
{
	return engine->Cmd_Argc();
}

void RootConsoleMenu::OnRootConsoleCommand(const char *cmd, unsigned int argcount)
{
	if (strcmp(cmd, "credits") == 0)
	{
		ConsolePrint(" SourceMod was developed by AlliedModders, LLC.");
		ConsolePrint(" Development would not have been possible without the following people:");
		ConsolePrint("  David \"BAILOPAN\" Anderson, lead developer");
		ConsolePrint("  Borja \"faluco\" Ferrer, Core developer");
		ConsolePrint("  Scott \"Damaged Soul\" Ehlert, Core developer");
		ConsolePrint("  Pavol \"PM OnoTo\" Marko, SourceHook developer");
		ConsolePrint(" Special thanks to Viper of GameConnect");
		ConsolePrint(" Special thanks to Mani of Mani-Admin-Plugin");
		ConsolePrint(" http://www.sourcemod.net/");
	} else if (strcmp(cmd, "version") == 0) {
		ConsolePrint(" SourceMod Version Information:");
		ConsolePrint("    SourceMod Version: %s", SVN_FULL_VERSION);
		ConsolePrint("    JIT Version: %s, %s", g_pVM->GetVMName(), g_pVM->GetVersionString());
		ConsolePrint("    JIT Settings: %s", g_pVM->GetCPUOptimizations());
		ConsolePrint("    http://www.sourcemod.net/");
	}
}

CON_COMMAND(sm, "SourceMod Menu")
{
	g_RootMenu.GotRootCmd();
}

CON_COMMAND(sm_dump_handles, "Dumps Handle usage to a file for finding Handle leaks")
{
	if (engine->Cmd_Argc() < 2)
	{
		g_RootMenu.ConsolePrint("Usage: sm_dump_handles <file>");
		return;
	}

	const char *arg = engine->Cmd_Argv(1);
	FILE *fp = fopen(arg, "wt");
	if (!fp)
	{
		g_RootMenu.ConsolePrint("Could not find file \"%s\"", arg);
		return;
	}

	g_HandleSys.Dump(fp);

	fclose(fp);
}
