/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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
	AdminCmdInfo admin;				/**< Admin info, if any */
	bool is_admin_set;				/**< Whether or not admin info is set */
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
	ResultType DispatchClientCommand(int client, const char *cmd, int args, ResultType type);
	void UpdateAdminCmdFlags(const char *cmd, OverrideType type, FlagBits bits, bool remove);
	bool LookForSourceModCommand(const char *cmd);
	bool LookForCommandAdminFlags(const char *cmd, FlagBits *pFlags);
	bool CheckCommandAccess(int client, const char *cmd, FlagBits flags);
private:
	void InternalDispatch();
	ResultType RunAdminCommand(ConCmdInfo *pInfo, int client, int args);
	ConCmdInfo *AddOrFindCommand(const char *name, const char *description, int flags);
	void SetCommandClient(int client);
	void AddToCmdList(ConCmdInfo *info);
	void RemoveConCmd(ConCmdInfo *info);
	void RemoveConCmds(List<CmdHook *> &cmdlist, IPluginContext *pContext);
	bool CheckAccess(int client, const char *cmd, AdminCmdInfo *pAdmin);
public:
	inline int GetCommandClient()
	{
		return m_CmdClient;
	}
	inline const List<ConCmdInfo *> & GetCommandList()
	{
		return m_CmdList;
	}
private:
	Trie *m_pCmds;					/* command lookup */
	Trie *m_pCmdGrps;				/* command group lookup */
	List<ConCmdInfo *> m_CmdList;	/* command list */
	int m_CmdClient;				/* current client */
	BaseStringTable m_Strings;		/* string table */
};

extern ConCmdManager g_ConCmds;

#endif // _INCLUDE_SOURCEMOD_CONCMDMANAGER_H_
