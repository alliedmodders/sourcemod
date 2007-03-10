/**
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

#include "DebugReporter.h"
#include "Logger.h"
#include "PluginSys.h"

CDbgReporter g_DbgReporter;

void CDbgReporter::OnSourceModAllInitialized()
{
	g_pSourcePawn->SetDebugListener(this);
}

void CDbgReporter::OnContextExecuteError(IPluginContext *ctx, IContextTrace *error)
{
	const char *lastname;
	const char *plname = g_PluginSys.FindPluginByContext(ctx->GetContext())->GetFilename();
	int n_err = error->GetErrorCode();

	if (n_err != SP_ERROR_NATIVE)
	{
		g_Logger.LogError("[SM] Plugin \"%s\" encountered error %d: %s",
			plname,
			n_err,
			error->GetErrorString());
	}

	if ((lastname=error->GetLastNative(NULL)) != NULL)
	{
		const char *custerr;
		if ((custerr=error->GetCustomErrorString()) != NULL)
		{
			g_Logger.LogError("[SM] Native \"%s\" reported: %s", lastname, custerr);
		} else {
			g_Logger.LogError("[SM] Native \"%s\" encountered a generic error.", lastname);
		}
	}

	if (!error->DebugInfoAvailable())
	{
		g_Logger.LogError("[SM] Debug mode is not enabled for this plugin.");
		g_Logger.LogError("[SM] To enable debug mode, edit plugin_settings.cfg, or type: sm plugins debug %d on",
			_GetPluginIndex(ctx));
		return;
	}

	CallStackInfo stk_info;
	int i = 0;
	g_Logger.LogError("[SM] Displaying call stack trace:");
	while (error->GetTraceInfo(&stk_info))
	{
		g_Logger.LogError("[SM]   [%d]  Line %d, %s::%s()",
			i++,
			stk_info.line,
			stk_info.filename,
			stk_info.function);
	}
}

int CDbgReporter::_GetPluginIndex(IPluginContext *ctx)
{
	int id = 1;
	IPluginIterator *iter = g_PluginSys.GetPluginIterator();

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
	return -1;
}

