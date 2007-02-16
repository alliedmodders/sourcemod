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

#ifndef _INCLUDE_SOURCEMOD_CCONCMDMANAGER_H_
#define _INCLUDE_SOURCEMOD_CCONCMDMANAGER_H_

#include "sm_globals.h"
#include "sourcemm_api.h"
#include "ForwardSys.h"
#include "sm_trie.h"
#include <sh_list.h>
#include <IRootConsoleMenu.h>

using namespace SourceHook;

enum CmdType
{
	Cmd_Server,
	Cmd_Console,
	Cmd_Client
};

struct ConCmdInfo
{
	ConCmdInfo()
	{
		sourceMod = false;
		pCmd = NULL;
		srvhooks = NULL;
		conhooks = NULL;
	}
	bool sourceMod;					/**< Determines whether or not concmd was created by a SourceMod plugin */
	ConCommand *pCmd;				/**< Pointer to the command itself */
	IChangeableForward *srvhooks;	/**< Hooks on this name as a server command */
	IChangeableForward *conhooks;	/**< Hooks on this name as a console command */
};

class CConCmdManager :
	public SMGlobalClass,
	public IRootConsoleCommand,
	public IPluginsListener
{
	friend void CommandCallback();
public:
	CConCmdManager();
	~CConCmdManager();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
public: //IPluginsListener
	void OnPluginLoaded(IPlugin *plugin);
	void OnPluginDestroyed(IPlugin *plugin);
public: //IRootConsoleCommand
	void OnRootConsoleCommand(const char *command, unsigned int argcount);
public:
	void AddServerCommand(IPluginFunction *pFunction, const char *name, const char *description, int flags);
	void AddConsoleCommand(IPluginFunction *pFunction, const char *name, const char *description, int flags);
	ResultType DispatchClientCommand(int client, ResultType type);
private:
	void InternalDispatch();
	ConCmdInfo *AddOrFindCommand(const char *name, const char *description, int flags);
	void SetCommandClient(int client);
	void AddToCmdList(ConCmdInfo *info);
	void RemoveConCmd(ConCmdInfo *info);
private:
	Trie *m_pCmds;
	List<ConCmdInfo *> m_CmdList;
	int m_CmdClient;
};

extern CConCmdManager g_ConCmds;

#endif // _INCLUDE_SOURCEMOD_CCONCMDMANAGER_H_

