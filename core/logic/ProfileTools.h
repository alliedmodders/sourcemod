// vim: set ts=4 sw=4 tw=99 noet :
// =============================================================================
// SourceMod
// Copyright (C) 2004-2014 AlliedModders LLC.  All rights reserved.
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

#ifndef _include_sourcemod_logic_profile_tool_manager_h_
#define _include_sourcemod_logic_profile_tool_manager_h_

#include <sp_vm_api.h>
#include <am-vector.h>
#include <IShareSys.h>
#include <IRootConsoleMenu.h>
#include "common_logic.h"

using namespace SourcePawn;

class ProfileToolManager
	: public SMGlobalClass,
	  public IRootConsoleCommand
{
public:
	ProfileToolManager();

	// SMGlobalClass
	void OnSourceModAllInitialized() override;
	void OnSourceModShutdown() override;

	// IRootConsoleCommand
	void OnRootConsoleCommand(const char *cmdname, const ICommandArgs *args) override;

	void RegisterTool(IProfilingTool *tool) {
		tools_.push_back(tool);
	}

	bool IsActive() const {
		return !!active_;
	}

	// If we get problems with plugins not being able to balance the profile
	// scopes, we should add a safety net that automatically clears any pending
	// scopes.
	void EnterScope(const char *group, const char *name) {
		if (active_)
			active_->EnterScope(group, name);
	}
	void LeaveScope() {
		if (active_)
			active_->LeaveScope();
	}

	IProfilingTool *FindToolByName(const char *name);

private:
	void StartFromConsole(IProfilingTool *tool);

private:
	std::vector<IProfilingTool *> tools_;
	IProfilingTool *active_;
	IProfilingTool *default_;
	bool enabled_;
};

extern ProfileToolManager g_ProfileToolManager;

#endif // _include_sourcemod_logic_profile_tool_manager_h_
