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
#include "sm_stringutil.h"
#include "Logger.h"
#include "TimerSys.h"
#include "logic_bridge.h"
#include "PlayerManager.h"
#include "HalfLife2.h"
#include "MenuManager.h"
#include "CoreConfig.h"
#include "ConCmdManager.h"
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
#include <amtl/os/am-shared-library.h>
#include <amtl/os/am-path.h>

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

static ke::Ref<ke::SharedLib> g_Logic;
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
IAdminSystem *adminsys;
ILogger *logger;
IRootConsole *rootmenu;

class VEngineServer_Logic : public IVEngineServer_Logic
{
public:
	virtual bool IsDedicatedServer()
	{
		return engine->IsDedicatedServer();
	}
	virtual void InsertServerCommand(const char *cmd)
	{
		engine->InsertServerCommand(cmd);
	}
	virtual void ServerCommand(const char *cmd)
	{
		engine->ServerCommand(cmd);
	}
	virtual void ServerExecute()
	{
		engine->ServerExecute();
	}
	virtual const char *GetClientConVarValue(int clientIndex, const char *name)
	{
		return engine->GetClientConVarValue(clientIndex, name);
	}
	virtual void ClientCommand(edict_t *pEdict, const char *szCommand)
	{
#if SOURCE_ENGINE == SE_DOTA
		engine->ClientCommand(IndexOfEdict(pEdict), "%s", szCommand);
#else
		engine->ClientCommand(pEdict, "%s", szCommand);
#endif
	}
	virtual void FakeClientCommand(edict_t *pEdict, const char *szCommand)
	{
#if SOURCE_ENGINE == SE_DOTA
		engine->ClientCommand(IndexOfEdict(pEdict), "%s", szCommand);
#else
		serverpluginhelpers->ClientCommand(pEdict, szCommand);
#endif
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
	bool FindIsDirectory(FileFindHandle_t handle)
	{
		return filesystem->FindIsDirectory(handle);
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
	unsigned int Size(const char *pFileName, const char *pPathID = 0)
	{
		return filesystem->Size(pFileName, pPathID);
	}
	int Read(void* pOutput, int size, FileHandle_t file)
	{
		return filesystem->Read(pOutput, size, file);
	}
	int Write(void const* pInput, int size, FileHandle_t file)
	{
		return filesystem->Write(pInput, size, file);
	}
	void Seek(FileHandle_t file, int pos, int seekType)
	{
		filesystem->Seek(file, pos, (FileSystemSeek_t) seekType);
	}
	unsigned int Tell(FileHandle_t file)
	{
		return filesystem->Tell(file);
	}
	int FPrint(FileHandle_t file, const char *pData)
	{
		return filesystem->FPrintf(file, "%s", pData);
	}
	void Flush(FileHandle_t file)
	{
		filesystem->Flush(file);
	}
	bool IsOk(FileHandle_t file)
	{
		return filesystem->IsOk(file);
	}
	void RemoveFile(const char *pRelativePath, const char *pathID)
	{
		filesystem->RemoveFile(pRelativePath, pathID);
	}
	void RenameFile(char const *pOldPath, char const *pNewPath, const char *pathID)
	{
		filesystem->RenameFile(pOldPath, pNewPath, pathID);
	}
	bool IsDirectory(const char *pFileName, const char *pathID)
	{
		return filesystem->IsDirectory(pFileName, pathID);
	}
	void CreateDirHierarchy(const char *path, const char *pathID)
	{
		filesystem->CreateDirHierarchy(path, pathID);
	}
};

static VFileSystem_Logic logic_filesystem;

class VPlayerInfo_Logic : public IPlayerInfo_Logic
{
public:
	bool IsObserver(IPlayerInfo *pInfo)
	{
		return pInfo->IsObserver();
	}
	int GetTeamIndex(IPlayerInfo *pInfo)
	{
		return pInfo->GetTeamIndex();
	}
	int GetFragCount(IPlayerInfo *pInfo)
	{
		return pInfo->GetFragCount();
	}
	int GetDeathCount(IPlayerInfo *pInfo)
	{
		return pInfo->GetDeathCount();
	}
	int GetArmorValue(IPlayerInfo *pInfo)
	{
		return pInfo->GetArmorValue();
	}
	void GetAbsOrigin(IPlayerInfo *pInfo, float *x, float *y, float *z)
	{
		Vector vec = pInfo->GetAbsOrigin();
		*x = vec.x;
		*y = vec.y;
		*z = vec.z;
	}
	void GetAbsAngles(IPlayerInfo *pInfo, float *x, float *y, float *z)
	{
		QAngle ang = pInfo->GetAbsAngles();
		*x = ang.x;
		*y = ang.y;
		*z = ang.z;
	}
	void GetPlayerMins(IPlayerInfo *pInfo, float *x, float *y, float *z)
	{
		Vector vec = pInfo->GetPlayerMins();
		*x = vec.x;
		*y = vec.y;
		*z = vec.z;
	}
	void GetPlayerMaxs(IPlayerInfo *pInfo, float *x, float *y, float *z)
	{
		Vector vec = pInfo->GetPlayerMaxs();
		*x = vec.x;
		*y = vec.y;
		*z = vec.z;
	}
	const char *GetWeaponName(IPlayerInfo *pInfo)
	{
		return pInfo->GetWeaponName();
	}
	const char *GetModelName(IPlayerInfo *pInfo)
	{
		return pInfo->GetModelName();
	}
	int GetHealth(IPlayerInfo *pInfo)
	{
		return pInfo->GetHealth();
	}
	void ChangeTeam(IPlayerInfo *pInfo, int iTeamNum)
	{
		pInfo->ChangeTeam(iTeamNum);
	}
};

static VPlayerInfo_Logic logic_playerinfo;

static ConVar sm_show_activity("sm_show_activity", "13", FCVAR_SPONLY, "Activity display setting (see sourcemod.cfg)");
static ConVar sm_immunity_mode("sm_immunity_mode", "1", FCVAR_SPONLY, "Mode for deciding immunity protection");
static ConVar sm_datetime_format("sm_datetime_format", "%m/%d/%Y - %H:%M:%S", 0, "Default formatting time rules");

static ConVar *find_convar(const char *name)
{
	return icvar->FindVar(name);
}

static void log_to_game(const char *message)
{
	Engine_LogPrintWrapper(message);
}

static void conprint(const char *message)
{
	META_CONPRINT(message);
}

static const char *get_cvar_string(ConVar* cvar)
{
	return cvar->GetString();
}

static bool get_cvar_bool(ConVar* cvar)
{
	return cvar->GetBool();
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
#elif SOURCE_ENGINE == SE_BMS
	return "bms";
#elif SOURCE_ENGINE == SE_TF2
	return "tf2";
#elif SOURCE_ENGINE == SE_LEFT4DEAD
	return "left4dead";
#elif SOURCE_ENGINE == SE_NUCLEARDAWN
	return "nucleardawn";
#elif SOURCE_ENGINE == SE_CONTAGION
	return "contagion";
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
	|| (SOURCE_ENGINE == SE_BMS)         \
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

static int get_activity_flags()
{
	return sm_show_activity.GetInt();
}

static int get_immunity_mode()
{
	return sm_immunity_mode.GetInt();
}

static void update_admin_cmd_flags(const char *cmd, OverrideType type, FlagBits bits, bool remove)
{
	g_ConCmds.UpdateAdminCmdFlags(cmd, type, bits, remove);
}

static bool look_for_cmd_admin_flags(const char *cmd, FlagBits *pFlags)
{
	return g_ConCmds.LookForCommandAdminFlags(cmd, pFlags);
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

static bool describe_player(int index, const char **namep, const char **authp, int *useridp)
{
	CPlayer *player = g_Players.GetPlayerByIndex(index);
	if (!player || !player->IsConnected())
		return false;

	if (namep)
		*namep = player->GetName();
	if (authp) {
		const char *auth = player->GetAuthString();
		*authp = (auth && *auth) ? auth : "STEAM_ID_PENDING";
	}
	if (useridp)
		*useridp = GetPlayerUserId(player->GetEdict());
	return true;
}

static int get_max_clients()
{
	return g_Players.MaxClients();
}

static int get_global_target()
{
	return g_SourceMod.GetGlobalTarget();
}

void UTIL_ConsolePrintVa(const char *fmt, va_list ap)
{
	char buffer[512];
	size_t len = ke::SafeVsprintf(buffer, sizeof(buffer), fmt, ap);

	if (len >= sizeof(buffer) - 1)
	{
		buffer[510] = '\n';
		buffer[511] = '\0';
	} else {
		buffer[len++] = '\n';
		buffer[len] = '\0';
	}
	META_CONPRINT(buffer);
}

void UTIL_ConsolePrint(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	UTIL_ConsolePrintVa(fmt, ap);
	va_end(ap);
}

#if defined METAMOD_PLAPI_VERSION
#if SOURCE_ENGINE == SE_LEFT4DEAD
#define GAMEFIX "2.l4d"
#elif SOURCE_ENGINE == SE_LEFT4DEAD2
#define GAMEFIX "2.l4d2"
#elif SOURCE_ENGINE == SE_NUCLEARDAWN
#define GAMEFIX "2.nd"
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
#elif SOURCE_ENGINE == SE_BMS
#define GAMEFIX "2.bms"
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
#elif SOURCE_ENGINE == SE_CONTAGION
#define GAMEFIX "2.contagion"
#else
#define GAMEFIX "2.ep1"
#endif //(SOURCE_ENGINE == SE_LEFT4DEAD) || (SOURCE_ENGINE == SE_LEFT4DEAD2)
#else  //METAMOD_PLAPI_VERSION
#define GAMEFIX "1.ep1"
#endif //METAMOD_PLAPI_VERSION

static ServerGlobals serverGlobals;

static CoreProvider core_bridge =
{
	/* Objects */
	&g_SourceMod,
	reinterpret_cast<IVEngineServer*>(&logic_engine),
	reinterpret_cast<IFileSystem*>(&logic_filesystem),
	&logic_playerinfo,
	&g_Timers,
	&g_Players,
	&g_HL2,
	&g_Menus,
	&g_pSourcePawn,
	&g_pSourcePawn2,
	/* Functions */
	find_convar,
	log_to_game,
	conprint,
	get_cvar_string,
	get_cvar_bool,
	get_game_name,
	get_game_description,
	get_source_engine_name,
	symbols_are_hidden,
	get_core_config_value,
	is_map_loading,
	is_map_running,
	load_mms_plugin,
	unload_mms_plugin,
	do_global_plugin_loads,
	SM_AreConfigsExecuted,
	SM_ExecuteForPlugin,
	keyvalues_to_dbinfo,
	get_activity_flags,
	get_immunity_mode,
	update_admin_cmd_flags,
	look_for_cmd_admin_flags,
	describe_player,
	get_max_clients,
	get_global_target,
	UTIL_ConsolePrintVa,
	GAMEFIX,
	&serverGlobals,
};

void InitLogicBridge()
{
	serverGlobals.universalTime = g_pUniversalTime;
	serverGlobals.frametime = &gpGlobals->frametime;
	serverGlobals.interval_per_tick = &gpGlobals->interval_per_tick;

	core_bridge.engineFactory = (void *)g_SMAPI->GetEngineFactory(false);
	core_bridge.serverFactory = (void *)g_SMAPI->GetServerFactory(false);
	core_bridge.listeners = SMGlobalClass::head;

	char path[PLATFORM_MAX_PATH];

	ke::path::Format(path, sizeof(path), "%s/bin/matchmaking_ds%s.%s", g_SMAPI->GetBaseDir(), MATCHMAKINGDS_SUFFIX, MATCHMAKINGDS_EXT);

	if (ke::Ref<ke::SharedLib> mmlib = ke::SharedLib::Open(path, NULL, 0))
	{
		core_bridge.matchmakingDSFactory =
		  mmlib->get<decltype(core_bridge.matchmakingDSFactory)>("CreateInterface");
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
	translator = logicore.translator;
	scripts = logicore.scripts;
	sharesys = logicore.sharesys;
	extsys = logicore.extsys;
	g_pCoreIdent = logicore.core_ident;
	handlesys = logicore.handlesys;
	forwardsys = logicore.forwardsys;
	adminsys = logicore.adminsys;
	logger = logicore.logger;
	rootmenu = logicore.rootmenu;
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
	g_Logic = ke::SharedLib::Open(file, myerror, sizeof(myerror));
	if (!g_Logic)
	{
		if (error && maxlength)
		{
			UTIL_Format(error, maxlength, "failed to load %s: %s", file, myerror);
		}
		return false;
	}

	LogicLoadFunction llf = g_Logic->get<decltype(llf)>("logic_load");
	if (llf == NULL)
	{
		g_Logic = nullptr;
		if (error && maxlength)
		{
			UTIL_Format(error, maxlength, "could not find logic_load function");
		}
		return false;
	}

	GetITextParsers getitxt = g_Logic->get<decltype(getitxt)>("get_textparsers");
	textparsers = getitxt();

	logic_init_fn = llf(SM_LOGIC_MAGIC);

	return true;
}

void ShutdownLogicBridge()
{
	g_Logic = nullptr;
}

