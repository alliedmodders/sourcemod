// vim: set ts=4 sw=4 tw=99 noet :
// =============================================================================
// SourceMod
// Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
// =============================================================================
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License, version 3.0, as published by the
// Free Software Foundation.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
// As a special exception, AlliedModders LLC gives you permission to link the
// code of this program (as well as its derivative works) to "Half-Life 2," the
// "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
// by the Valve Corporation.  You must obey the GNU General Public License in
// all respects for all other code used.  Additionally, AlliedModders LLC grants
// this exception to all derivative works.  AlliedModders LLC defines further
// exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
// or <http://www.sourcemod.net/license.php>.
#include "RootConsoleMenu.h"
#include "sm_stringutil.h"
#include "CoreConfig.h"
#include "ConVarManager.h"
#include "logic_bridge.h"
#include <sourcemod_version.h>

RootConsoleMenu g_RootMenu;
IRootConsole *rootmenu = &g_RootMenu;

RootConsoleMenu::RootConsoleMenu()
{
}

RootConsoleMenu::~RootConsoleMenu()
{
	List<ConsoleEntry *>::iterator iter;
	for (iter=m_Menu.begin(); iter!=m_Menu.end(); iter++)
	{
		delete (*iter);
	}
	m_Menu.clear();
}

void RootConsoleMenu::OnSourceModStartup(bool late)
{
	AddRootConsoleCommand3("version", "Display version information", this);
	AddRootConsoleCommand3("credits", "Display credits listing", this);
}

void RootConsoleMenu::OnSourceModAllInitialized()
{
	sharesys->AddInterface(NULL, this);
}

void RootConsoleMenu::OnSourceModShutdown()
{
	RemoveRootConsoleCommand("credits", this);
	RemoveRootConsoleCommand("version", this);
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
	return false;
}

bool RootConsoleMenu::AddRootConsoleCommand2(const char *cmd, const char *text, IRootConsoleCommand *pHandler)
{
	return false;
}

bool RootConsoleMenu::AddRootConsoleCommand3(const char *cmd,
											 const char *text,
											 IRootConsoleCommand *pHandler)
{
	if (m_Commands.contains(cmd))
		return false;

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
			pNew->cmd = pHandler;
			m_Commands.insert(cmd, pNew);
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
		pNew->cmd = pHandler;
		m_Commands.insert(cmd, pNew);
		m_Menu.push_back(pNew);
	}

	return true;
}

bool RootConsoleMenu::RemoveRootConsoleCommand(const char *cmd, IRootConsoleCommand *pHandler)
{
	m_Commands.remove(cmd);

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

void RootConsoleMenu::GotRootCmd(const ICommandArgs *cmd)
{
	unsigned int argnum = cmd->ArgC();

	if (argnum >= 2)
	{
		const char *cmdname = cmd->Arg(1);

		ConsoleEntry *entry;
		if (m_Commands.retrieve(cmdname, &entry))
		{
			entry->cmd->OnRootConsoleCommand(cmdname, cmd);
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

void RootConsoleMenu::OnRootConsoleCommand(const char *cmdname, const ICommandArgs *command)
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
		ConsolePrint("    SourceMod Version: %s", SOURCEMOD_VERSION);
		if (g_pSourcePawn2->IsJitEnabled())
			ConsolePrint("    SourcePawn Engine: %s (build %s)", g_pSourcePawn2->GetEngineName(), g_pSourcePawn2->GetVersionString());
		else
			ConsolePrint("    SourcePawn Engine: %s (build %s NO JIT)", g_pSourcePawn2->GetEngineName(), g_pSourcePawn2->GetVersionString());
		ConsolePrint("    SourcePawn API: v1 = %d, v2 = %d", g_pSourcePawn->GetEngineAPIVersion(), g_pSourcePawn2->GetAPIVersion());
		ConsolePrint("    Compiled on: %s", SOURCEMOD_BUILD_TIME);
#if defined(SM_GENERATED_BUILD)
		ConsolePrint("    Built from: https://github.com/alliedmodders/sourcemod/commit/%s", SOURCEMOD_SHA);
		ConsolePrint("    Build ID: %s:%s", SOURCEMOD_LOCAL_REV, SOURCEMOD_SHA);
#endif
		ConsolePrint("    http://www.sourcemod.net/");
	}
}
