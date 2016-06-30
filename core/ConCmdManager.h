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

#ifndef _INCLUDE_SOURCEMOD_CONCMDMANAGER_H_
#define _INCLUDE_SOURCEMOD_CONCMDMANAGER_H_

#include "sm_globals.h"
#include "sourcemm_api.h"
#include <IForwardSys.h>
#include <sh_list.h>
#include <sh_string.h>
#include <IRootConsoleMenu.h>
#include <IAdminSystem.h>
#include "concmd_cleaner.h"
#include "GameHooks.h"
#include <sm_stringhashmap.h>
#include <am-utility.h>
#include <am-inlinelist.h>
#include <am-linkedlist.h>
#include <am-refcounting.h>

using namespace SourceHook;

struct CmdHook;
struct ConCmdInfo;

struct CommandGroup : public ke::Refcounted<CommandGroup>
{
	ke::LinkedList<CmdHook *> hooks;
};

struct AdminCmdInfo
{
	AdminCmdInfo(const ke::RefPtr<CommandGroup> &group, FlagBits flags)
		: group(group),
		  flags(flags),
		  eflags(0)
	{
	}
	ke::RefPtr<CommandGroup> group;
	FlagBits flags;			/* default flags */
	FlagBits eflags;		/* effective flags */
};

struct CmdHook : public ke::InlineListNode<CmdHook>
{
	enum Type {
		Server,
		Client
	};

	CmdHook(Type type, ConCmdInfo *cmd, IPluginFunction *fun, const char *description)
		: type(type),
		  info(cmd),
		  pf(fun),
		  helptext(description)
	{
	}

	Type type;
	ConCmdInfo *info;
	IPluginFunction *pf;				/* function hook */
	ke::AString helptext;				/* help text */
	ke::AutoPtr<AdminCmdInfo> admin;	/* admin requirements, if any */
};

typedef ke::InlineList<CmdHook> CmdHookList;

struct ConCmdInfo
{
	ConCmdInfo()
	{
		sourceMod = false;
		pCmd = NULL;
		eflags = 0;
	}
	bool sourceMod;					/**< Determines whether or not concmd was created by a SourceMod plugin */
	ConCommand *pCmd;				/**< Pointer to the command itself */
	CmdHookList hooks;				/**< Hook list */
	FlagBits eflags;				/**< Effective admin flags */
	ke::RefPtr<CommandHook> sh_hook;   /**< SourceHook hook, if any. */
};

typedef List<ConCmdInfo *> ConCmdList;

class ConCmdManager :
	public SMGlobalClass,
	public IRootConsoleCommand,
	public IPluginsListener,
	public IConCommandTracker
{
	friend void CommandCallback(DISPATCH_ARGS);
public:
	ConCmdManager();
	~ConCmdManager();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
public: //IPluginsListener
	void OnPluginDestroyed(IPlugin *plugin);
public: //IRootConsoleCommand
	void OnRootConsoleCommand(const char *cmdname, const ICommandArgs *command) override;
public: //IConCommandTracker
	void OnUnlinkConCommandBase(ConCommandBase *pBase, const char *name) override;
public:
	bool AddServerCommand(IPluginFunction *pFunction, const char *name, const char *description, int flags);
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
private:
	bool InternalDispatch(int client, const ICommandArgs *args);
	ResultType RunAdminCommand(ConCmdInfo *pInfo, int client, int args);
	ConCmdInfo *AddOrFindCommand(const char *name, const char *description, int flags);
	void AddToCmdList(ConCmdInfo *info);
	void RemoveConCmd(ConCmdInfo *info, const char *cmd, bool untrack);
	bool CheckAccess(int client, const char *cmd, AdminCmdInfo *pAdmin);

	// Case insensitive
	ConCmdList::iterator FindInList(const char *name);

	// Case sensitive
	ConCmdInfo *FindInTrie(const char *name);
public:
	inline const List<ConCmdInfo *> & GetCommandList()
	{
		return m_CmdList;
	}
private:
	typedef StringHashMap<ke::RefPtr<CommandGroup> > GroupMap;

	StringHashMap<ConCmdInfo *> m_Cmds; /* command lookup */
	GroupMap m_CmdGrps;				/* command group map */
	ConCmdList m_CmdList;			/* command list */
};

extern ConCmdManager g_ConCmds;

#endif // _INCLUDE_SOURCEMOD_CONCMDMANAGER_H_

