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

#ifndef _INCLUDE_SOURCEMOD_SERVERCOMMANDS_H_
#define _INCLUDE_SOURCEMOD_SERVERCOMMANDS_H_

#include "sourcemod.h"
#include <IRootConsoleMenu.h>
#include "PluginSys.h"
#include "sourcemm_api.h"
#include <sh_list.h>
#include <sh_string.h>
#include "sm_trie.h"

using namespace SourceMod;
using namespace SourceHook;

struct ConsoleEntry
{
	String command;
	String description;
};

class RootConsoleMenu : 
	public IConCommandBaseAccessor,
	public SMGlobalClass,
	public IRootConsoleCommand,
	public IRootConsole
{
public:
	RootConsoleMenu();
	~RootConsoleMenu();
public: //IConCommandBaseAccessor
	bool RegisterConCommandBase(ConCommandBase *pCommand);
public: //SMGlobalClass
	void OnSourceModStartup(bool late);
	void OnSourceModShutdown();
public: //IRootConsoleCommand
	void OnRootConsoleCommand(const char *cmd, unsigned int argcount);
public: //IRootConsole
	bool AddRootConsoleCommand(const char *cmd, const char *text, IRootConsoleCommand *pHandler);
	bool RemoveRootConsoleCommand(const char *cmd, IRootConsoleCommand *pHandler);
	void ConsolePrint(const char *fmt, ...);
	const char *GetArgument(unsigned int argno);
	unsigned int GetArgumentCount();
	const char *GetArguments();
	void DrawGenericOption(const char *cmd, const char *text);
public:
	void GotRootCmd();
private:
	bool m_CfgExecDone;
	Trie *m_pCommands;
	List<ConsoleEntry *> m_Menu;
};

extern RootConsoleMenu g_RootMenu;

#endif //_INCLUDE_SOURCEMOD_SERVERCOMMANDS_H_
