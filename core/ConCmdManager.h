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

#ifndef _INCLUDE_SOURCEMOD_CONCMDMANAGER_H_
#define _INCLUDE_SOURCEMOD_CONCMDMANAGER_H_

#include "sm_globals.h"
#include "sourcemm_api.h"
#include "ForwardSys.h"
#include "sm_trie.h"
#include "sm_memtable.h"
#include <sh_list.h>
#include <sh_string.h>
#include <IRootConsoleMenu.h>
#include <IAdminSystem.h>

using namespace SourceHook;

enum CmdType
{
	Cmd_Server,
	Cmd_Console,
	Cmd_Admin,
};

struct AdminCmdInfo
{
	AdminCmdInfo()
	{
		cmdGrpId = -1;
		flags = 0;
		eflags = 0;
	}
	int cmdGrpId;			/* index into cmdgroup string table */
	FlagBits flags;			/* default flags */
	FlagBits eflags;		/* effective flags */
};

struct CmdHook
{
	CmdHook()
	{
		pf = NULL;
		pAdmin = NULL;
	}
	IPluginFunction *pf;	/* function hook */
	String helptext;		/* help text */
	AdminCmdInfo *pAdmin;	/* admin requirements, if any */
};

struct ConCmdInfo
{
	ConCmdInfo()
	{
		sourceMod = false;
		pCmd = NULL;
	}
	bool sourceMod;					/**< Determines whether or not concmd was created by a SourceMod plugin */
	ConCommand *pCmd;				/**< Pointer to the command itself */
	List<CmdHook *> srvhooks;		/**< Hooks as a server command */
	List<CmdHook *> conhooks;		/**< Hooks as a console command */
};

class ConCmdManager :
	public SMGlobalClass,
	public IRootConsoleCommand,
	public IPluginsListener
{
	friend void CommandCallback();
public:
	ConCmdManager();
	~ConCmdManager();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
	void OnSourceModPluginsLoaded();
public: //IPluginsListener
	void OnPluginDestroyed(IPlugin *plugin);
public: //IRootConsoleCommand
	void OnRootConsoleCommand(const char *command, unsigned int argcount);
public:
	bool AddServerCommand(IPluginFunction *pFunction, const char *name, const char *description, int flags);
	bool AddConsoleCommand(IPluginFunction *pFunction, const char *name, const char *description, int flags);
	bool AddAdminCommand(IPluginFunction *pFunction, 
						 const char *name, 
						 const char *group,
						 int adminflags,
						 const char *description, 
						 int flags);
	ResultType DispatchClientCommand(int client, ResultType type);
	void UpdateAdminCmdFlags(const char *cmd, OverrideType type, FlagBits bits);
	void NotifyExecDone(const char *file);
private:
	void InternalDispatch();
	ResultType RunAdminCommand(ConCmdInfo *pInfo, int client, int args);
	ConCmdInfo *AddOrFindCommand(const char *name, const char *description, int flags);
	void SetCommandClient(int client);
	void AddToCmdList(ConCmdInfo *info);
	void RemoveConCmd(ConCmdInfo *info);
	void RemoveConCmds(List<CmdHook *> &cmdlist, IPluginContext *pContext);
	bool CheckAccess(int client, const char *cmd, AdminCmdInfo *pAdmin);
	void OnExecCmd();
private:
	Trie *m_pCmds;					/* command lookup */
	Trie *m_pCmdGrps;				/* command group lookup */
	List<ConCmdInfo *> m_CmdList;	/* command list */
	int m_CmdClient;				/* current client */
	BaseStringTable m_Strings;		/* string table */
	ConVar *m_pServerCfgFile;		/* servercfgfile cvar */
	ConCommand *m_pExecCmd;			/* "exec" command */
	IForward *m_pServerCfgFwd;		/* server config forward */
	bool m_bServerCfgDone;			/* marks whether a servercfg was detected */
};

extern ConCmdManager g_ConCmds;

#endif // _INCLUDE_SOURCEMOD_CONCMDMANAGER_H_
