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

#include <ISourceMod.h>
#include <IPluginSys.h>
#include <stdarg.h>
#include "DebugReporter.h"
#include "Logger.h"

DebugReport g_DbgReporter;

void DebugReport::OnSourceModAllInitialized()
{
	g_pSourcePawn->SetDebugListener(this);
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

	if (err >= 0) {
		const char *error = g_pSourcePawn2->GetErrorString(err);
		if (error)
			g_Logger.LogError("[SM] Plugin \"%s\" encountered error %d: %s", plname, err, error);
		else
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

	std::vector<std::string> arr = GetStackTrace(&iter);
	for (size_t i = 0; i < arr.size(); i++)
	{
		g_Logger.LogError("%s", arr[i].c_str());
	}
}

std::vector<std::string> DebugReport::GetStackTrace(IFrameIterator *iter)
{
	char temp[3072];
	std::vector<std::string> trace;
	iter->Reset();
	
	if (!iter->Done())
	{
		trace.push_back("[SM] Call stack trace:");

		for (int index = 0; !iter->Done(); iter->Next(), index++) 
		{
			const char *fn = iter->FunctionName();
			if (!fn)
			{
				fn = "<unknown function>";
			}
			if (iter->IsNativeFrame()) 
			{
				g_pSM->Format(temp, sizeof(temp), "[SM]   [%d] %s", index, fn);
				trace.push_back(temp);
				continue;
			}
			if (iter->IsScriptedFrame()) 
			{
				const char *file = iter->FilePath();
				if (!file)
				{
					file = "<unknown>";
				}
				g_pSM->Format(temp, sizeof(temp), "[SM]   [%d] Line %d, %s::%s",
						index,
						iter->LineNumber(),
						file,
						fn);
				
				trace.push_back(temp);
			}
		}
	}
	
	return trace;
}
