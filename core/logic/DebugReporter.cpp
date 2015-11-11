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

#include <IPluginSys.h>
#include <stdarg.h>
#include "DebugReporter.h"
#include "Logger.h"
#include <am-string.h>
#include <am-vector.h>
#include "PluginSys.h"
#include "common_logic.h"
#include <bridge/include/CoreProvider.h>

DebugReport g_DbgReporter;

void DebugReport::OnSourceModStartup(bool late)
{
	if (!g_pConsoleDebugger)
		return;

	const char *enabled = bridge->GetCoreConfigValue("EnableConsoleDebugger");
	if (enabled != nullptr && !strcasecmp(enabled, "yes")) {
		g_pConsoleDebugger->SetEnabled(true);
	}
}

void DebugReport::OnSourceModAllInitialized()
{
	g_pSourcePawn->SetDebugListener(this);

	rootmenu->AddRootConsoleCommand3("debug", "Debug Plugins", this);
}

void DebugReport::OnSourceModShutdown()
{
	rootmenu->RemoveRootConsoleCommand("debug", this);
}

void DebugReport::OnDebugSpew(const char *msg, ...)
{
	va_list ap;
	char buffer[512];

	va_start(ap, msg);
	ke::SafeVsprintf(buffer, sizeof(buffer), msg, ap);
	va_end(ap);

	g_Logger.LogMessage("[SM] %s", buffer);
}

void DebugReport::GenerateError(IPluginContext *ctx, cell_t func_idx, int err, const char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	GenerateErrorVA(ctx, func_idx, err, message, ap);
	va_end(ap);
}

void DebugReport::GenerateErrorVA(IPluginContext *ctx, cell_t func_idx, int err, const char *message, va_list ap)
{
	char buffer[512];
	ke::SafeVsprintf(buffer, sizeof(buffer), message, ap);

	const char *plname = pluginsys->FindPluginByContext(ctx->GetContext())->GetFilename();
	const char *error = g_pSourcePawn2->GetErrorString(err);

	if (error)
	{
		g_Logger.LogError("[SM] Plugin \"%s\" encountered error %d: %s", plname, err, error);
	} else {
		g_Logger.LogError("[SM] Plugin \"%s\" encountered unknown error %d", plname, err);
	}

	g_Logger.LogError("[SM] %s", buffer);

	if (func_idx != -1)
	{
		if (func_idx & 1)
		{
			func_idx >>= 1;
			sp_public_t *function;
			if (ctx->GetRuntime()->GetPublicByIndex(func_idx, &function) == SP_ERROR_NONE)
			{
				g_Logger.LogError("[SM] Unable to call function \"%s\" due to above error(s).", function->name);
			}
		}
	}
}

void DebugReport::GenerateCodeError(IPluginContext *pContext, uint32_t code_addr, int err, const char *message, ...)
{
	va_list ap;
	char buffer[512];

	va_start(ap, message);
	ke::SafeVsprintf(buffer, sizeof(buffer), message, ap);
	va_end(ap);

	const char *plname = pluginsys->FindPluginByContext(pContext->GetContext())->GetFilename();
	const char *error = g_pSourcePawn2->GetErrorString(err);

	if (error)
	{
		g_Logger.LogError("[SM] Plugin \"%s\" encountered error %d: %s", plname, err, error);
	} else {
		g_Logger.LogError("[SM] Plugin \"%s\" encountered unknown error %d", plname, err);
	}

	g_Logger.LogError("[SM] %s", buffer);

	IPluginDebugInfo *pDebug;
	if ((pDebug = pContext->GetRuntime()->GetDebugInfo()) == NULL)
	{
		g_Logger.LogError("[SM] Debug mode is not enabled for \"%s\"", plname);
		g_Logger.LogError("[SM] To enable debug mode, edit plugin_settings.cfg, or type: sm plugins debug %d on",
			_GetPluginIndex(pContext));
		return;
	}

	const char *name;
	if (pDebug->LookupFunction(code_addr, &name) == SP_ERROR_NONE)
	{
		g_Logger.LogError("[SM] Unable to call function \"%s\" due to above error(s).", name);
	} else {
		g_Logger.LogError("[SM] Unable to call function (name unknown, address \"%x\").", code_addr);
	}
}

int DebugReport::_GetPluginIndex(IPluginContext *ctx)
{
	int id = 1;
	IPluginIterator *iter = pluginsys->GetPluginIterator();

	for (; iter->MorePlugins(); iter->NextPlugin(), id++)
	{
		IPlugin *pl = iter->GetPlugin();
		if (pl->GetBaseContext() == ctx)
		{
			iter->Release();
			return id;
		}
	}

	iter->Release();

	/* If we don't know which plugin this is, it's one being loaded.  Fake its index for now. */

	return pluginsys->GetPluginCount() + 1;
}

void DebugReport::ReportError(const IErrorReport &report, IFrameIterator &iter)
{
	// Don't log an error if a function wasn't runnable.
	// This is necassary due to the way SM is handling and exposing
	// scripted functions. It's too late to change that now.
	if (report.Code() == SP_ERROR_NOT_RUNNABLE)
		return;

	const char *blame = nullptr;
	if (report.Blame()) 
	{
		blame = report.Blame()->DebugName();
	} else {
	    // Find the nearest plugin to blame.
		for (; !iter.Done(); iter.Next()) 
		{
			if (iter.IsScriptedFrame()) 
			{
				IPlugin *plugin = pluginsys->FindPluginByContext(iter.Context()->GetContext());
				if (plugin)
				{
					blame = plugin->GetFilename();
				} else {
					blame = iter.Context()->GetRuntime()->GetFilename();
				}
				break;
			}
		}
	}

	iter.Reset();

	g_Logger.LogError("[SM] Exception reported: %s", report.Message());

	if (blame) 
	{
		g_Logger.LogError("[SM] Blaming: %s", blame);
	}

	if (!iter.Done()) 
	{
		g_Logger.LogError("[SM] Call stack trace:");

		for (int index = 0; !iter.Done(); iter.Next(), index++) 
		{
			const char *fn = iter.FunctionName();
			if (!fn)
			{
				fn = "<unknown function>";
			}
			if (iter.IsNativeFrame()) 
			{
				g_Logger.LogError("[SM]   [%d] %s", index, fn);
				continue;
			}
			if (iter.IsScriptedFrame()) 
			{
				const char *file = iter.FilePath();
				if (!file)
				{
					file = "<unknown>";
				}
				g_Logger.LogError("[SM]   [%d] Line %d, %s::%s",
						index,
						iter.LineNumber(),
						file,
						fn);
			}
		}
	}
}

void DebugReport::OnRootConsoleCommand(const char *cmdname, const ICommandArgs *command)
{
	if (!g_pConsoleDebugger || !g_pConsoleDebugger->IsEnabled()) {
		rootmenu->ConsolePrint("[SM] Console debugger not available.");
		return;
	}

	int argcount = command->ArgC();
	if (argcount >= 3) {
		const char *cmd = command->Arg(2);
		if (strcmp(cmd, "start") == 0) {
			if (argcount < 4) {
				rootmenu->ConsolePrint("[SM] Usage: sm debug start <#|file>");
				return;
			}

			const char *arg = command->Arg(3);
			CPlugin *pl = (CPlugin *)g_PluginSys.FindPluginByConsoleArg(arg);

			if (!pl) {
				rootmenu->ConsolePrint("[SM] Plugin %s is not loaded.", arg);
				return;
			}

			char name[PLATFORM_MAX_PATH];
			const sm_plugininfo_t *info = pl->GetPublicInfo();
			if (pl->GetStatus() <= Plugin_Paused)
				strcpy(name, (info->name[0] != '\0') ? info->name : pl->GetFilename());
			else
				strcpy(name, pl->GetFilename());

			if (g_pConsoleDebugger->StartDebugger(pl->GetBaseContext()))
				rootmenu->ConsolePrint("[SM] Pausing plugin %s for debugging. Will halt on next instruction.", name);
			else
				rootmenu->ConsolePrint("[SM] Failed to pause plugin %s for debugging.", name);

			return;
		}
		else if (strcmp(cmd, "next") == 0) {
			
			if (g_pConsoleDebugger->DebugNextLoadedPlugin())
				rootmenu->ConsolePrint("[SM] Will halt on the first instruction of the next loaded plugin.");
			else
				rootmenu->ConsolePrint("[SM] Failed to mark next loaded plugin for debugging.");
			return;
		}
		else if (strcmp(cmd, "bp") == 0) {

			if (argcount >= 5) {
				const char *plugin = command->Arg(3);
				CPlugin *pl = (CPlugin *)g_PluginSys.FindPluginByConsoleArg(plugin);

				if (!pl) {
					rootmenu->ConsolePrint("[SM] Plugin %s is not loaded.", plugin);
					return;
				}

				const char *arg = command->Arg(4);
				if (strcmp(arg, "list") == 0) {
					ke::Vector<IBreakpoint *> *breakpoints;
					breakpoints = g_pConsoleDebugger->GetBreakpoints(pl->GetBaseContext());

					if (!breakpoints) {
						rootmenu->ConsolePrint("[SM] Debugger is not active on plugin %s.", plugin);
						return;
					}

					rootmenu->ConsolePrint("[SM] Listing %d breakpoint(s) for plugin %s:", breakpoints->length(), pl->GetFilename());

					char line[256];
					IBreakpoint *bp;
					for (unsigned int i = 0; i < breakpoints->length(); i++) {
						bp = breakpoints->at(i);
						ke::SafeSprintf(line, sizeof(line), "[SM] %2d  ", i + 1);

						if (bp->line() > 0)
							ke::SafeSprintf(line, sizeof(line), "%sline: %d", line, bp->line());

						if (bp->temporary())
							ke::SafeSprintf(line, sizeof(line), "%s  (TEMP)", line);

						if (bp->filename() != nullptr)
							ke::SafeSprintf(line, sizeof(line), "%s\tfile: %s", line, bp->filename());

						if (bp->name() != nullptr)
							ke::SafeSprintf(line, sizeof(line), "%s\tfunc: %s", line, bp->name());

						rootmenu->ConsolePrint("%s", line);
					}

					return;
				}
				else if (strcmp(arg, "add") == 0) {
					if (argcount < 6) {
						rootmenu->ConsolePrint("[SM] Usage: sm debug bp <#|file> add <file:line | file:function>");
						return;
					}

					const char *bpline = command->Arg(5);
					IBreakpoint *bp = g_pConsoleDebugger->AddBreakpoint(pl->GetBaseContext(), bpline, false);
					if (bp == nullptr) {
						rootmenu->ConsolePrint("[SM] Invalid breakpoint address specification.");
					}
					else {
						rootmenu->ConsolePrint("[SM] Added breakpoint in file %s on line %d", bp->filename(), bp->line());
					}
					return;
				}
				else if (strcmp(arg, "clear") == 0) {
					if (argcount < 6) {
						rootmenu->ConsolePrint("[SM] Usage: sm debug bp <#|file> clear <#>");
						return;
					}

					const char *bpstr = command->Arg(5);
					int bpnum = strtoul(bpstr, NULL, 10);
					if (g_pConsoleDebugger->ClearBreakpoint(pl->GetBaseContext(), bpnum)) {
						rootmenu->ConsolePrint("[SM] Breakpoint cleared.");
					}
					else {
						rootmenu->ConsolePrint("[SM] Failed to clear breakpoint.");
					}
					return;
				}
			}

			/* Draw the sub menu */
			rootmenu->ConsolePrint("[SM] Usage: sm debug bp <#|file> <option>");
			rootmenu->DrawGenericOption("list", "List breakpoints");
			rootmenu->DrawGenericOption("add", "Add a breakpoint");
			rootmenu->DrawGenericOption("clear", "Remove a breakpoint");
			return;
		}
	}

	/* Draw the main menu */
	rootmenu->ConsolePrint("SourceMod Debug Menu:");
	rootmenu->DrawGenericOption("start", "Start debugging a plugin");
	rootmenu->DrawGenericOption("next", "Start debugging the plugin which is loaded next");
	rootmenu->DrawGenericOption("bp", "Handle breakpoints in a plugin");
}
