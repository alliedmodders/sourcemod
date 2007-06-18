/**
 * vim: set ts=4 :
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
