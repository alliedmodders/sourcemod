/**
 * vim: set ts=4 sw=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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

#include "sm_srvcmds.h"
#include <sourcemod_version.h>
#include "sm_stringutil.h"
#include "HandleSys.h"
#include "CoreConfig.h"
#include "ConVarManager.h"
#include "ShareSys.h"

RootConsoleMenu g_RootMenu;

ConVar sourcemod_version("sourcemod_version", SM_VERSION_STRING, FCVAR_SPONLY|FCVAR_REPLICATED|FCVAR_NOTIFY, "SourceMod Version");

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

void RootConsoleMenu::OnSourceModStartup(bool late)
{
#if SOURCE_ENGINE >= SE_ORANGEBOX
	g_pCVar = icvar;
#endif
	CONVAR_REGISTER(this);
	AddRootConsoleCommand("version", "Display version information", this);
	AddRootConsoleCommand("credits", "Display credits listing", this);
}

void RootConsoleMenu::OnSourceModAllInitialized()
{
	g_ShareSys.AddInterface(NULL, this);
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
	return _AddRootConsoleCommand(cmd, text, pHandler, false);
}

bool RootConsoleMenu::AddRootConsoleCommand2(const char *cmd, const char *text, IRootConsoleCommand *pHandler)
{
	return _AddRootConsoleCommand(cmd, text, pHandler, true);
}

bool RootConsoleMenu::_AddRootConsoleCommand(const char *cmd,
											 const char *text,
											 IRootConsoleCommand *pHandler,
											 bool version2)
{
	if (sm_trie_retrieve(m_pCommands, cmd, NULL))
	{
		return false;
	}

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
			pNew->version2 = version2;
			pNew->cmd = pHandler;
			sm_trie_insert(m_pCommands, cmd, pNew);
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
		pNew->version2 = version2;
		pNew->cmd = pHandler;
		sm_trie_insert(m_pCommands, cmd, pNew);
		m_Menu.push_back(pNew);
	}

	return true;
}

bool RootConsoleMenu::RemoveRootConsoleCommand(const char *cmd, IRootConsoleCommand *pHandler)
{
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

const char *RootConsoleMenu::GetInterfaceName()
{
	return SMINTERFACE_ROOTCONSOLE_NAME;
}

unsigned int RootConsoleMenu::GetInterfaceVersion()
{
	return SMINTERFACE_ROOTCONSOLE_VERSION;
}

#if SOURCE_ENGINE==SE_EPISODEONE || SOURCE_ENGINE==SE_DARKMESSIAH
class CCommandArgs : public ICommandArgs
{
public:
	CCommandArgs(const CCommand& _cmd)
	{
	}
	const char *Arg(int n) const
	{
		return engine->Cmd_Argv(n);
	}
	int ArgC() const
	{
		return engine->Cmd_Argc();
	}
	const char *ArgS() const
	{
		return engine->Cmd_Args();
	}
};
#else
class CCommandArgs : public ICommandArgs
{
	const CCommand *cmd;
public:
	CCommandArgs(const CCommand& _cmd) : cmd(&_cmd)
	{
	}
	const char *Arg(int n) const
	{
		return cmd->Arg(n);
	}
	int ArgC() const
	{
		return cmd->ArgC();
	}
	const char *ArgS() const
	{
		return cmd->ArgS();
	}
};
#endif

void RootConsoleMenu::GotRootCmd(const CCommand &cmd)
{
	unsigned int argnum = cmd.ArgC();

	if (argnum >= 2)
	{
		const char *cmdname = cmd.Arg(1);
		if (strcmp(cmdname, "internal") == 0)
		{
			if (argnum >= 3)
			{
				const char *arg = cmd.Arg(2);
				if (strcmp(arg, "1") == 0)
				{
					SM_ConfigsExecuted_Global();
				}
				else if (strcmp(arg, "2") == 0)
				{
					if (argnum >= 4)
					{
						SM_ConfigsExecuted_Plugin(atoi(cmd.Arg(3)));
					}
				}
			}
			return;
		}

		CCommandArgs ocmd(cmd);

		ConsoleEntry *entry;
		if (sm_trie_retrieve(m_pCommands, cmdname, (void **)&entry))
		{
			if (entry->version2)
			{
				entry->cmd->OnRootConsoleCommand2(cmdname, &ocmd);
			}
			else
			{
				entry->cmd->OnRootConsoleCommand(cmdname, cmd);
			}
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

void RootConsoleMenu::OnRootConsoleCommand(const char *cmdname, const CCommand &command)
{
	if (strcmp(cmdname, "credits") == 0)
	{
		ConsolePrint(" SourceMod was developed by AlliedModders, LLC.");
		ConsolePrint(" Development would not have been possible without the following people:");
		ConsolePrint("  David \"BAILOPAN\" Anderson");
		ConsolePrint("  Matt \"pRED\" Woodrow");
		ConsolePrint("  Scott \"DS\" Ehlert");
		ConsolePrint("  Fyren");
		ConsolePrint("  Nicholas \"psychonic\" Hastings");
		ConsolePrint("  Asher \"asherkin\" Baker");
		ConsolePrint("  Borja \"faluco\" Ferrer");
		ConsolePrint("  Pavol \"PM OnoTo\" Marko");
		ConsolePrint(" Special thanks to Liam, ferret, and Mani");
		ConsolePrint(" Special thanks to Viper and SteamFriends");
		ConsolePrint(" http://www.sourcemod.net/");
	}
	else if (strcmp(cmdname, "version") == 0)
	{
		ConsolePrint(" SourceMod Version Information:");
		ConsolePrint("    SourceMod Version: %s", SM_VERSION_STRING);
		ConsolePrint("    SourcePawn Engine: %s (build %s)", g_pSourcePawn2->GetEngineName(), g_pSourcePawn2->GetVersionString());
		ConsolePrint("    SourcePawn API: v1 = %d, v2 = %d", g_pSourcePawn->GetEngineAPIVersion(), g_pSourcePawn2->GetAPIVersion());
		ConsolePrint("    Compiled on: %s %s", __DATE__, __TIME__);
		ConsolePrint("    Build ID: %s", SM_BUILD_UNIQUEID);
		ConsolePrint("    http://www.sourcemod.net/");
	}
}

CON_COMMAND(sm, "SourceMod Menu")
{
#if SOURCE_ENGINE <= SE_DARKMESSIAH
	CCommand args;
#endif
	g_RootMenu.GotRootCmd(args);
}

FILE *g_pHndlLog = NULL;

void write_handles_to_log(const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	vfprintf(g_pHndlLog, fmt, ap);
	fprintf(g_pHndlLog, "\n");
	va_end(ap);
}

void write_handles_to_game(const char *fmt, ...)
{
	size_t len;
	va_list ap;
	char buffer[1024];
	
	va_start(ap, fmt);
	len = UTIL_FormatArgs(buffer, sizeof(buffer)-2, fmt, ap);
	va_end(ap);

	buffer[len] = '\n';
	buffer[len+1] = '\0';

	engine->LogPrint(buffer);
}

CON_COMMAND(sm_dump_handles, "Dumps Handle usage to a file for finding Handle leaks")
{
#if SOURCE_ENGINE <= SE_DARKMESSIAH
	CCommand args;
#endif
	if (args.ArgC() < 2)
	{
		g_RootMenu.ConsolePrint("Usage: sm_dump_handles <file> or <log> for game logs");
		return;
	}

	if (strcmp(args.Arg(1), "log") != 0)
	{
		const char *arg = args.Arg(1);
		FILE *fp = fopen(arg, "wt");
		if (!fp)
		{
			g_RootMenu.ConsolePrint("Could not find file \"%s\"", arg);
			return;
		}

		g_pHndlLog = fp;
		g_HandleSys.Dump(write_handles_to_log);
		g_pHndlLog = NULL;

		fclose(fp);
	}
	else
	{
		g_HandleSys.Dump(write_handles_to_game);
	}
}

