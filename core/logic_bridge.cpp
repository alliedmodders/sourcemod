/**
 * vim: set ts=4 sw=4 tw=99 noet:
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
#include "sm_autonatives.h"
#include "logic/intercom.h"
#include "LibrarySys.h"
#include "sm_stringutil.h"
#include "Logger.h"
#include "sm_srvcmds.h"
#include "TimerSys.h"
#include "logic_bridge.h"
#include "PlayerManager.h"
#include "AdminCache.h"
#include "HalfLife2.h"
#include "CoreConfig.h"
#include "IDBDriver.h"
#if SOURCE_ENGINE == SE_DOTA
#include "convar_sm_dota.h"
#elif SOURCE_ENGINE >= SE_ALIENSWARM
#include "convar_sm_swarm.h"
#elif SOURCE_ENGINE >= SE_LEFT4DEAD
#include "convar_sm_l4d.h"
#elif SOURCE_ENGINE >= SE_ORANGEBOX
#include "convar_sm_ob.h"
#else
#include "convar_sm.h"
#endif

#if defined _WIN32
	#define MATCHMAKINGDS_SUFFIX	""
	#define MATCHMAKINGDS_EXT	"dll"
#elif defined __APPLE__
	#define MATCHMAKINGDS_SUFFIX	""
	#define MATCHMAKINGDS_EXT	"dylib"
#elif defined __linux__
#if SOURCE_ENGINE < SE_LEFT4DEAD2
	#define MATCHMAKINGDS_SUFFIX	"_i486"
#else
	#define MATCHMAKINGDS_SUFFIX	""
#endif
	#define MATCHMAKINGDS_EXT	"so"
#endif

static ILibrary *g_pLogic = NULL;
static LogicInitFunction logic_init_fn;

IThreader *g_pThreader;
ITextParsers *textparsers;
sm_logic_t logicore;
ITranslator *translator;
IScriptManager *scripts;
IShareSys *sharesys;
IExtensionSys *extsys;
IHandleSys *handlesys;
IForwardManager *forwardsys;

class VEngineServer_Logic : public IVEngineServer_Logic
{
public:
	virtual bool IsMapValid(const char *map)
	{
		return !!engine->IsMapValid(map);
	}

	virtual void ServerCommand(const char *cmd)
	{
		engine->ServerCommand(cmd);
	}
};

static VEngineServer_Logic logic_engine;


class VFileSystem_Logic : public IFileSystem_Logic
{
public:
	const char *FindFirstEx(const char *pWildCard, const char *pPathID, FileFindHandle_t *pHandle)
	{
		return filesystem->FindFirstEx(pWildCard, pPathID, pHandle);
	}
	const char *FindNext(FileFindHandle_t handle)
	{
		return filesystem->FindNext(handle);
	}
	void FindClose(FileFindHandle_t handle)
	{
		filesystem->FindClose(handle);
	}
	FileHandle_t Open(const char *pFileName, const char *pOptions, const char *pathID = 0)
	{
		return filesystem->Open(pFileName, pOptions, pathID);
	}
	void Close(FileHandle_t file)
	{
		filesystem->Close(file);
	}
	char *ReadLine(char *pOutput, int maxChars, FileHandle_t file)
	{
		return filesystem->ReadLine(pOutput, maxChars, file);
	}
	bool EndOfFile(FileHandle_t file)
	{
		return filesystem->EndOfFile(file);
	}
	bool FileExists(const char *pFileName, const char *pPathID = 0)
	{
		return filesystem->FileExists(pFileName, pPathID);
	}
};

static VFileSystem_Logic logic_filesystem;


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

static void log_fatal(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	g_Logger.LogFatalEx(fmt, ap);
	va_end(ap);
}

static void log_message(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	g_Logger.LogMessageEx(fmt, ap);
	va_end(ap);
}

static void log_to_file(FILE *fp, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	g_Logger.LogToOpenFileEx(fp, fmt, ap);
	va_end(ap);
}

static void log_to_game(const char *message)
{
	Engine_LogPrintWrapper(message);
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
			pGameInfo->deleteThis();
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
#elif SOURCE_ENGINE == SE_EYE
	return "eye";
#elif SOURCE_ENGINE == SE_CSS
	return "css";
#elif SOURCE_ENGINE == SE_HL2DM
	return "hl2dm";
#elif SOURCE_ENGINE == SE_DODS
	return "dods";
#elif SOURCE_ENGINE == SE_SDK2013
	return "sdk2013";
#elif SOURCE_ENGINE == SE_TF2
	return "tf2";
#elif SOURCE_ENGINE == SE_LEFT4DEAD
	return "left4dead";
#elif SOURCE_ENGINE == SE_NUCLEARDAWN
	return "nucleardawn";
#elif SOURCE_ENGINE == SE_LEFT4DEAD2
	return "left4dead2";
#elif SOURCE_ENGINE == SE_ALIENSWARM
	return "alienswarm";
#elif SOURCE_ENGINE == SE_PORTAL2
	return "portal2";
#elif SOURCE_ENGINE == SE_BLADE
	return "blade";
#elif SOURCE_ENGINE == SE_INSURGENCY
	return "insurgency";
#elif SOURCE_ENGINE == SE_CSGO
	return "csgo";
#elif SOURCE_ENGINE == SE_DOTA
	return "dota";
#endif
}

static bool symbols_are_hidden()
{
#if (SOURCE_ENGINE == SE_CSS)            \
	|| (SOURCE_ENGINE == SE_HL2DM)       \
	|| (SOURCE_ENGINE == SE_DODS)        \
	|| (SOURCE_ENGINE == SE_SDK2013)     \
	|| (SOURCE_ENGINE == SE_TF2)         \
	|| (SOURCE_ENGINE == SE_LEFT4DEAD)   \
	|| (SOURCE_ENGINE == SE_NUCLEARDAWN) \
	|| (SOURCE_ENGINE == SE_LEFT4DEAD2)  \
	|| (SOURCE_ENGINE == SE_INSURGENCY)  \
	|| (SOURCE_ENGINE == SE_BLADE)       \
	|| (SOURCE_ENGINE == SE_CSGO)        \
	|| (SOURCE_ENGINE == SE_DOTA)
	return true;
#else
	return false;
#endif
}

static const char* get_core_config_value(const char* key)
{
	return g_CoreConfig.GetCoreConfigValue(key);
}

static bool is_map_loading()
{
	return g_SourceMod.IsMapLoading();
}

static bool is_map_running()
{
	return g_SourceMod.IsMapRunning();
}

static DatabaseInfo keyvalues_to_dbinfo(KeyValues *kv)
{
	DatabaseInfo info;
	info.database = kv->GetString("database", "");
	info.driver = kv->GetString("driver", "default");
	info.host = kv->GetString("host", "");
	info.maxTimeout = kv->GetInt("timeout", 0);
	info.pass = kv->GetString("pass", "");
	info.port = kv->GetInt("port", 0);
	info.user = kv->GetString("user", "");
	return info;
}

int read_cmd_argc(const CCommand &args)
{
	return args.ArgC();
}

static const char *read_cmd_arg(const CCommand &args, int arg)
{
	return args.Arg(arg);
}

static int load_mms_plugin(const char *file, bool *ok, char *error, size_t maxlength)
{
	bool ignore_already;
	PluginId id = g_pMMPlugins->Load(file, g_PLID, ignore_already, error, maxlength);

	Pl_Status status;

#ifndef METAMOD_PLAPI_VERSION
	const char *filep;
	PluginId source;
#endif

	if (!id || (
#ifndef METAMOD_PLAPI_VERSION
		g_pMMPlugins->Query(id, filep, status, source)
#else
		g_pMMPlugins->Query(id, NULL, &status, NULL)
#endif
		&& status < Pl_Paused))
	{
		*ok = false;
	}
	else
	{
		*ok = true;
	}

	return id;
}

static void unload_mms_plugin(int id)
{
	char ignore[255];
	g_pMMPlugins->Unload(id, true, ignore, sizeof(ignore));
}

void do_global_plugin_loads()
{
	g_SourceMod.DoGlobalPluginLoads();
}

#if defined METAMOD_PLAPI_VERSION
#if SOURCE_ENGINE == SE_LEFT4DEAD
#define GAMEFIX "2.l4d"
#elif SOURCE_ENGINE == SE_LEFT4DEAD2
#define GAMEFIX "2.l4d2"
#elif SOURCE_ENGINE == SE_ALIENSWARM
#define GAMEFIX "2.swarm"
#elif SOURCE_ENGINE == SE_ORANGEBOX
#define GAMEFIX "2.ep2"
#elif SOURCE_ENGINE == SE_BLOODYGOODTIME
#define GAMEFIX "2.bgt"
#elif SOURCE_ENGINE == SE_EYE
#define GAMEFIX "2.eye"
#elif SOURCE_ENGINE == SE_CSS
#define GAMEFIX "2.css"
#elif SOURCE_ENGINE == SE_HL2DM
#define GAMEFIX "2.hl2dm"
#elif SOURCE_ENGINE == SE_DODS
#define GAMEFIX "2.dods"
#elif SOURCE_ENGINE == SE_SDK2013
#define GAMEFIX "2.sdk2013"
#elif SOURCE_ENGINE == SE_TF2
#define GAMEFIX "2.tf2"
#elif SOURCE_ENGINE == SE_DARKMESSIAH
#define GAMEFIX "2.darkm"
#elif SOURCE_ENGINE == SE_PORTAL2
#define GAMEFIX "2.portal2"
#elif SOURCE_ENGINE == SE_BLADE
#define GAMEFIX "2.blade"
#elif SOURCE_ENGINE == SE_INSURGENCY
#define GAMEFIX "2.insurgency"
#elif SOURCE_ENGINE == SE_CSGO
#define GAMEFIX "2.csgo"
#elif SOURCE_ENGINE == SE_DOTA
#define GAMEFIX "2.dota"
#else
#define GAMEFIX "2.ep1"
#endif //(SOURCE_ENGINE == SE_LEFT4DEAD) || (SOURCE_ENGINE == SE_LEFT4DEAD2)
#else  //METAMOD_PLAPI_VERSION
#define GAMEFIX "1.ep1"
#endif //METAMOD_PLAPI_VERSION

static ServerGlobals serverGlobals;

static sm_core_t core_bridge =
{
	/* Objects */
	&g_SourceMod,
	&g_LibSys,
	reinterpret_cast<IVEngineServer*>(&logic_engine),
	reinterpret_cast<IFileSystem*>(&logic_filesystem),
	&g_RootMenu,
	&g_Timers,
	&g_Players,
	&g_Admins,
	&g_HL2,
	&g_pSourcePawn,
	&g_pSourcePawn2,
	/* Functions */
	find_convar,
	strncopy,
	UTIL_TrimWhitespace,
	log_error,
	log_fatal,
	log_message,
	log_to_file,
	log_to_game,
	get_cvar_string,
	UTIL_Format,
	UTIL_FormatArgs,
	gnprintf,
	atcprintf,
	get_game_name,
	get_game_description,
	get_source_engine_name,
	symbols_are_hidden,
	get_core_config_value,
	is_map_loading,
	is_map_running,
	read_cmd_argc,
	read_cmd_arg,
	load_mms_plugin,
	unload_mms_plugin,
	do_global_plugin_loads,
	SM_AreConfigsExecuted,
	SM_ExecuteForPlugin,
	keyvalues_to_dbinfo,
	GAMEFIX,
	&serverGlobals
};

void InitLogicBridge()
{
	serverGlobals.universalTime = g_pUniversalTime;
	serverGlobals.frametime = &gpGlobals->frametime;
	serverGlobals.interval_per_tick = &gpGlobals->interval_per_tick;

	core_bridge.engineFactory = (void *)g_SMAPI->GetEngineFactory(false);
	core_bridge.serverFactory = (void *)g_SMAPI->GetServerFactory(false);
	core_bridge.listeners = SMGlobalClass::head;

	ILibrary *mmlib;
	char path[PLATFORM_MAX_PATH];

	g_LibSys.PathFormat(path, sizeof(path), "%s/bin/matchmaking_ds%s.%s", g_SMAPI->GetBaseDir(), MATCHMAKINGDS_SUFFIX, MATCHMAKINGDS_EXT);

	if ((mmlib = g_LibSys.OpenLibrary(path, NULL, 0)))
	{
		core_bridge.matchmakingDSFactory = mmlib->GetSymbolAddress("CreateInterface");
		mmlib->CloseLibrary();
	}
	
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
	scripts = logicore.scripts;
	sharesys = logicore.sharesys;
	extsys = logicore.extsys;
	g_pCoreIdent = logicore.core_ident;
	handlesys = logicore.handlesys;
	forwardsys = logicore.forwardsys;
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

