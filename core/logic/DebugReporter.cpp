/**
 * vim: set ts=4 :
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
#include "DebugReporter.h"

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
	smcore.FormatArgs(buffer, sizeof(buffer), msg, ap);
	va_end(ap);

	smcore.Log("[SM] %s", buffer);
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
	smcore.FormatArgs(buffer, sizeof(buffer), message, ap);

	const char *plname = pluginsys->FindPluginByContext(ctx->GetContext())->GetFilename();
	const char *error = g_pSourcePawn2->GetErrorString(err);

	if (error)
	{
		smcore.LogError("[SM] Plugin \"%s\" encountered error %d: %s", plname, err, error);
	} else {
		smcore.LogError("[SM] Plugin \"%s\" encountered unknown error %d", plname, err);
	}

	smcore.LogError("[SM] %s", buffer);

	if (func_idx != -1)
	{
		if (func_idx & 1)
		{
			func_idx >>= 1;
			sp_public_t *function;
			if (ctx->GetRuntime()->GetPublicByIndex(func_idx, &function) == SP_ERROR_NONE)
			{
				smcore.LogError("[SM] Unable to call function \"%s\" due to above error(s).", function->name);
			}
		}
	}
}

void DebugReport::GenerateCodeError(IPluginContext *pContext, uint32_t code_addr, int err, const char *message, ...)
{
	va_list ap;
	char buffer[512];

	va_start(ap, message);
	smcore.FormatArgs(buffer, sizeof(buffer), message, ap);
	va_end(ap);

	const char *plname = pluginsys->FindPluginByContext(pContext->GetContext())->GetFilename();
	const char *error = g_pSourcePawn2->GetErrorString(err);

	if (error)
	{
		smcore.LogError("[SM] Plugin \"%s\" encountered error %d: %s", plname, err, error);
	} else {
		smcore.LogError("[SM] Plugin \"%s\" encountered unknown error %d", plname, err);
	}

	smcore.LogError("[SM] %s", buffer);

	IPluginDebugInfo *pDebug;
	if ((pDebug = pContext->GetRuntime()->GetDebugInfo()) == NULL)
	{
		smcore.LogError("[SM] Debug mode is not enabled for \"%s\"", plname);
		smcore.LogError("[SM] To enable debug mode, edit plugin_settings.cfg, or type: sm plugins debug %d on",
			_GetPluginIndex(pContext));
		return;
	}

	const char *name;
	if (pDebug->LookupFunction(code_addr, &name) == SP_ERROR_NONE)
	{
		smcore.LogError("[SM] Unable to call function \"%s\" due to above error(s).", name);
	} else {
		smcore.LogError("[SM] Unable to call function (name unknown, address \"%x\").", code_addr);
	}
}

void DebugReport::OnContextExecuteError(IPluginContext *ctx, IContextTrace *error)
{
	const char *lastname;
	const char *plname = pluginsys->FindPluginByContext(ctx->GetContext())->GetFilename();
	int n_err = error->GetErrorCode();

	if (n_err != SP_ERROR_NATIVE)
	{
		smcore.LogError("[SM] Plugin encountered error %d: %s",
			n_err,
			error->GetErrorString());
	}

	if ((lastname=error->GetLastNative(NULL)) != NULL)
	{
		const char *custerr;
		if ((custerr=error->GetCustomErrorString()) != NULL)
		{
			smcore.LogError("[SM] Native \"%s\" reported: %s", lastname, custerr);
		} else {
			smcore.LogError("[SM] Native \"%s\" encountered a generic error.", lastname);
		}
	}

	if (!error->DebugInfoAvailable())
	{
		smcore.LogError("[SM] Debug mode is not enabled for \"%s\"", plname);
		smcore.LogError("[SM] To enable debug mode, edit plugin_settings.cfg, or type: sm plugins debug %d on",
			_GetPluginIndex(ctx));
		return;
	}

	CallStackInfo stk_info;
	int i = 0;
	smcore.LogError("[SM] Displaying call stack trace for plugin \"%s\":", plname);
	while (error->GetTraceInfo(&stk_info))
	{
		smcore.LogError("[SM]   [%d]  Line %d, %s::%s()",
			i++,
			stk_info.line,
			stk_info.filename,
			stk_info.function);
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

