/**
 * vim: set ts=4 sw=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
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
#include <stdio.h>
#include <assert.h>
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "sm_globals.h"
#include "NativeOwner.h"
#include "sm_autonatives.h"
#include "logic/intercom.h"
#include "LibrarySys.h"
#include "HandleSys.h"
#include "sm_stringutil.h"
#include "Logger.h"
#include "ShareSys.h"
#include "sm_srvcmds.h"
#include "ForwardSys.h"
#include "TimerSys.h"
#include "logic_bridge.h"
#include "DebugReporter.h"
#include "PlayerManager.h"
#include "AdminCache.h"
#include "HalfLife2.h"

static ILibrary *g_pLogic = NULL;
static LogicInitFunction logic_init_fn;

IThreader *g_pThreader;
ITextParsers *textparsers;
sm_logic_t logicore;
ITranslator *translator;

class VEngineServer_Logic : public IVEngineServer_Logic
{
public:
	virtual bool IsMapValid(const char *map)
	{
		return engine->IsMapValid(map);
	}

	virtual void ServerCommand(const char *cmd)
	{
		engine->ServerCommand(cmd);
	}
};

static VEngineServer_Logic logic_engine;

static void add_natives(sp_nativeinfo_t *natives)
{
	g_pCoreNatives->AddNatives(natives);
}

static ConVar *find_convar(const char *name)
{
	return icvar->FindVar(name);
}

static void log_error(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	g_Logger.LogErrorEx(fmt, ap);
	va_end(ap);
}

static void generate_error(IPluginContext *ctx, cell_t func_idx, int err, const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	g_DbgReporter.GenerateErrorVA(ctx, func_idx, err, fmt, ap);
	va_end(ap);
}

static const char *get_cvar_string(ConVar* cvar)
{
	return cvar->GetString();
}

static bool get_game_name(char *buffer, size_t maxlength)
{
	KeyValues *pGameInfo = new KeyValues("GameInfo");
	if (g_HL2.KVLoadFromFile(pGameInfo, basefilesystem, "gameinfo.txt"))
	{
		const char *str;
		if ((str = pGameInfo->GetString("game", NULL)) != NULL)
		{
			strncopy(buffer, str, maxlength);
			return true;
		}
	}
	pGameInfo->deleteThis();
	return false;
}

static const char *get_game_description()
{
	return SERVER_CALL(GetGameDescription)();
}

static const char *get_source_engine_name()
{
#if !defined SOURCE_ENGINE
# error "Unknown engine type"
#endif
#if SOURCE_ENGINE == SE_EPISODEONE
	return "original";
#elif SOURCE_ENGINE == SE_DARKMESSIAH
	return "darkmessiah";
#elif SOURCE_ENGINE == SE_ORANGEBOX
	return "orangebox";
#elif SOURCE_ENGINE == SE_BLOODYGOODTIME
	return "bloodygoodtime";
#elif SOURCE_ENGINE == SE_ORANGEBOXVALVE
	return "orangebox_valve";
#elif SOURCE_ENGINE == SE_LEFT4DEAD
	return "left4dead";
#elif SOURCE_ENGINE == SE_LEFT4DEAD2
	return "left4dead2";
#elif SOURCE_ENGINE == SE_ALIENSWARM
	return "alienswarm";
#endif
}

static bool symbols_are_hidden()
{
#if (SOURCE_ENGINE == SE_ORANGEBOXVALVE) || (SOURCE_ENGINE == SE_LEFT4DEAD) || (SOURCE_ENGINE == SE_LEFT4DEAD2)
	return true;
#else
	return false;
#endif
}

static ServerGlobals serverGlobals;

static sm_core_t core_bridge =
{
	/* Objects */
	&g_HandleSys,
	NULL,
	&g_SourceMod,
	&g_LibSys,
	reinterpret_cast<IVEngineServer*>(&logic_engine),
	&g_ShareSys,
	&g_RootMenu,
	&g_PluginSys,
	&g_Forwards,
	&g_Timers,
	&g_Players,
	&g_Admins,
	&g_HL2,
	/* Functions */
	add_natives,
	find_convar,
	strncopy,
	UTIL_TrimWhitespace,
	log_error,
	get_cvar_string,
	UTIL_Format,
	generate_error,
	gnprintf,
	atcprintf,
	get_game_name,
	get_game_description,
	get_source_engine_name,
	symbols_are_hidden,
	&serverGlobals
};

void InitLogicBridge()
{
	serverGlobals.universalTime = g_pUniversalTime;
	serverGlobals.frametime = &gpGlobals->frametime;
	serverGlobals.interval_per_tick = &gpGlobals->interval_per_tick;

	core_bridge.core_ident = g_pCoreIdent;
	core_bridge.engineFactory = (void *)g_SMAPI->GetEngineFactory(false);
	core_bridge.serverFactory = (void *)g_SMAPI->GetServerFactory(false);
	logic_init_fn(&core_bridge, &logicore);

	/* Add SMGlobalClass instances */
	SMGlobalClass* glob = SMGlobalClass::head;
	while (glob->m_pGlobalClassNext != NULL)
	{
		glob = glob->m_pGlobalClassNext;
	}
	assert(glob->m_pGlobalClassNext == NULL);
	glob->m_pGlobalClassNext = logicore.head;

	g_pThreader = logicore.threader;
	g_pSourcePawn2->SetProfiler(logicore.profiler);
	translator = logicore.translator;
}

bool StartLogicBridge(char *error, size_t maxlength)
{
	char file[PLATFORM_MAX_PATH];

	/* Now it's time to load the logic binary */
	g_SMAPI->PathFormat(file,
		sizeof(file),
		"%s/bin/sourcemod.logic." PLATFORM_LIB_EXT,
		g_SourceMod.GetSourceModPath());

	char myerror[255];
	g_pLogic = g_LibSys.OpenLibrary(file, myerror, sizeof(myerror));

	if (!g_pLogic)
	{
		if (error && maxlength)
		{
			UTIL_Format(error, maxlength, "failed to load %s: %s", file, myerror);
		}
		return false;
	}

	LogicLoadFunction llf = (LogicLoadFunction)g_pLogic->GetSymbolAddress("logic_load");
	if (llf == NULL)
	{
		g_pLogic->CloseLibrary();
		if (error && maxlength)
		{
			UTIL_Format(error, maxlength, "could not find logic_load function");
		}
		return false;
	}

	GetITextParsers getitxt = (GetITextParsers)g_pLogic->GetSymbolAddress("get_textparsers");
	textparsers = getitxt();

	logic_init_fn = llf(SM_LOGIC_MAGIC);

	return true;
}

void ShutdownLogicBridge()
{
	g_pLogic->CloseLibrary();
}

