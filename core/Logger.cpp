/**
 * vim: set ts=4 sw=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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

#include <time.h>
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "sm_stringutil.h"
#include "Logger.h"
#include "TimerSys.h"
#include "logic_bridge.h"
#include <sourcemod_version.h>
#include <bridge/include/IProviderCallbacks.h>

bool g_in_game_log_hook = false;

static LoggerCore g_LoggerCore;

static KHook::Return<void> HookLogPrint(IVEngineServer*, const char *message)
{
	g_in_game_log_hook = true;
	bool stopped = logicore.callbacks->OnLogPrint(message);
	g_in_game_log_hook = false;

	if (stopped)
	{
		return { KHook::Action::Supersede };
	}
	return { KHook::Action::Ignore };
}

KHook::Virtual LogPrintHook(&IVEngineServer::LogPrint, &HookLogPrint, nullptr);

void LoggerCore::OnSourceModStartup(bool late)
{
	LogPrintHook.Add(engine);
}

void LoggerCore::OnSourceModAllShutdown()
{
	LogPrintHook.Remove(engine);
}

void Engine_LogPrintWrapper(const char *msg)
{
	if (g_in_game_log_hook)
	{
		LogPrintHook.CallOriginal(engine, msg);
	}
	else
	{
		engine->LogPrint(msg);
	}
}
