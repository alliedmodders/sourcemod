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

#include "ProfileTools.h"
#include <stdarg.h>

ProfileToolManager g_ProfileToolManager;

ProfileToolManager::ProfileToolManager()
	: active_(nullptr)
{
}

void
ProfileToolManager::OnSourceModAllInitialized()
{
	rootmenu->AddRootConsoleCommand2("prof", "Profiling", this);
}

void
ProfileToolManager::OnSourceModShutdown()
{
	rootmenu->RemoveRootConsoleCommand("prof", this);
}

IProfilingTool *
ProfileToolManager::FindToolByName(const char *name)
{
	for (size_t i = 0; i < tools_.length(); i++) {
		if (strcmp(tools_[i]->Name(), name) == 0)
			return tools_[i];
	}
	return nullptr;
}

static void
render_help(const char *fmt, ...)
{
	char buffer[2048];

	va_list ap;
	va_start(ap, fmt);
	smcore.FormatArgs(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	rootmenu->ConsolePrint("%s", buffer);
}

void
ProfileToolManager::OnRootConsoleCommand2(const char *cmdname, const ICommandArgs *args)
{
	if (tools_.length() == 0) {
		rootmenu->ConsolePrint("No profiling tools are enabled.");
		return;
	}

	if (args->ArgC() >= 3) {
		cmdname = args->Arg(2);

		if (strcmp(cmdname, "list") == 0) {
			rootmenu->ConsolePrint("Profiling tools:");
			for (size_t i = 0; i < tools_.length(); i++) {
				rootmenu->DrawGenericOption(tools_[i]->Name(), tools_[i]->Description());
			}
			return;
		}
		if (strcmp(cmdname, "stop") == 0) {
			if (!active_) {
				rootmenu->ConsolePrint("No profiler is active.");
				return;
			}
			g_pSourcePawn2->DisableProfiling();
			g_pSourcePawn2->SetProfilingTool(nullptr);
			active_->Stop();
			active_->RenderHelp(render_help);
			return;
		}

		if (args->ArgC() < 4) {
			rootmenu->ConsolePrint("You must specify a profiling tool name.");
			return;
		}

		const char *toolname = args->Arg(3);
		if (strcmp(cmdname, "start") == 0) {
			if (active_) {
				rootmenu->ConsolePrint("A profile is already active using %s.", active_->Name());
				return;
			}
			if ((active_ = FindToolByName(toolname)) == nullptr) {
				rootmenu->ConsolePrint("No tool with the name \"%s\" was found.", toolname);
				return;
			}
			if (!active_->Start()) {
				rootmenu->ConsolePrint("Failed to attach to or start %s.", active_->Name());
				active_ = nullptr;
				return;
			}
			g_pSourcePawn2->SetProfilingTool(active_);
			g_pSourcePawn2->EnableProfiling();
			rootmenu->ConsolePrint("Started profiling with %s.", active_->Name());
			return;
		}
		if (strcmp(cmdname, "help") == 0) {
			IProfilingTool *tool = FindToolByName(toolname);
			if (!tool) {
				rootmenu->ConsolePrint("No tool with the name \"%s\" was found.", toolname);
				return;
			}
			tool->RenderHelp(render_help);
			return;
		}
	}

	rootmenu->ConsolePrint("Profiling commands:");
	rootmenu->DrawGenericOption("list", "List all available profiling tools.");
	rootmenu->DrawGenericOption("start", "Start a profile with a given tool.");
	rootmenu->DrawGenericOption("stop", "Stop the current profile session.");
	rootmenu->DrawGenericOption("help", "Display help text for a profiler.");
}
