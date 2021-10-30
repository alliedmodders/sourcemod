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
#include "provider.h"
#include "sm_convar.h"
#include <amtl/os/am-shared-library.h>
#include <amtl/os/am-path.h>
#include <bridge/include/IVEngineServerBridge.h>
#include <bridge/include/IPlayerInfoBridge.h>
#include <bridge/include/IFileSystemBridge.h>
#if PROTOBUF_PROXY_ENABLE
# include "pb_handle.h"
#endif

sm_logic_t logicore;

IThreader *g_pThreader = nullptr;
ITextParsers *textparsers = nullptr;
ITranslator *translator = nullptr;
IScriptManager *scripts = nullptr;
IShareSys *sharesys = nullptr;
IExtensionSys *extsys = nullptr;
IHandleSys *handlesys = nullptr;
IForwardManager *forwardsys = nullptr;
IAdminSystem *adminsys = nullptr;
ILogger *logger = nullptr;
IRootConsole *rootmenu = nullptr;

class VEngineServer_Logic : public IVEngineServerBridge
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
		engine->ClientCommand(pEdict, "%s", szCommand);
	}
	virtual void FakeClientCommand(edict_t *pEdict, const char *szCommand)
	{
		serverpluginhelpers->ClientCommand(pEdict, szCommand);
	}
} engine_wrapper;

class VFileSystem_Logic : public IFileSystemBridge
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
} fs_wrapper;

class VPlayerInfo_Logic : public IPlayerInfoBridge
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
} playerinfo_wrapper;

static ConVar sm_show_activity("sm_show_activity", "13", FCVAR_SPONLY, "Activity display setting (see sourcemod.cfg)");
static ConVar sm_immunity_mode("sm_immunity_mode", "1", FCVAR_SPONLY, "Mode for deciding immunity protection");
static ConVar sm_datetime_format("sm_datetime_format", "%m/%d/%Y - %H:%M:%S", 0, "Default formatting time rules");

static const char* get_core_config_value(const char* key)
{
	return g_CoreConfig.GetCoreConfigValue(key);
}

static void keyvalues_to_dbinfo(KeyValues *kv, DatabaseInfo *out)
{
	out->database = kv->GetString("database", "");
	out->driver = kv->GetString("driver", "default");
	out->host = kv->GetString("host", "");
	out->maxTimeout = kv->GetInt("timeout", 0);
	out->pass = kv->GetString("pass", "");
	out->port = kv->GetInt("port", 0);
	out->user = kv->GetString("user", "");
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

void do_global_plugin_loads()
{
	g_SourceMod.DoGlobalPluginLoads();
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
#elif SOURCE_ENGINE == SE_DOI
#define GAMEFIX "2.doi"
#elif SOURCE_ENGINE == SE_CSGO
#define GAMEFIX "2.csgo"
#elif SOURCE_ENGINE == SE_CONTAGION
#define GAMEFIX "2.contagion"
#else
#define GAMEFIX "2.ep1"
#endif

static ServerGlobals serverGlobals;

CoreProviderImpl sCoreProviderImpl;

CoreProviderImpl::CoreProviderImpl()
{
	this->sm = &g_SourceMod;
	this->engine = &engine_wrapper;
	this->filesystem = &fs_wrapper;
	this->playerInfo = &playerinfo_wrapper;
	this->timersys = &g_Timers;
	this->playerhelpers = &g_Players;
	this->gamehelpers = &g_HL2;
	this->menus = &g_Menus;
	this->spe1 = &g_pSourcePawn;
	this->spe2 = &g_pSourcePawn2;
	this->GetCoreConfigValue = get_core_config_value;
	this->DoGlobalPluginLoads = do_global_plugin_loads;
	this->AreConfigsExecuted = SM_AreConfigsExecuted;
	this->ExecuteConfigs = SM_ExecuteForPlugin;
	this->GetDBInfoFromKeyValues = keyvalues_to_dbinfo;
	this->GetActivityFlags = get_activity_flags;
	this->GetImmunityMode = get_immunity_mode;
	this->UpdateAdminCmdFlags = update_admin_cmd_flags;
	this->LookForCommandAdminFlags = look_for_cmd_admin_flags;
	this->GetGlobalTarget = get_global_target;
	this->gamesuffix = GAMEFIX;
	this->serverGlobals = &::serverGlobals;
	this->serverFactory = nullptr;
	this->engineFactory = nullptr;
	this->matchmakingDSFactory = nullptr;
	this->listeners = nullptr;
}

ConVar *CoreProviderImpl::FindConVar(const char *name)
{
	return icvar->FindVar(name);
}

const char *CoreProviderImpl::GetCvarString(ConVar* cvar)
{
	return cvar->GetString();
}

bool CoreProviderImpl::GetCvarBool(ConVar* cvar)
{
	return cvar->GetBool();
}

bool CoreProviderImpl::GetGameName(char *buffer, size_t maxlength)
{
	KeyValues *pGameInfo = new KeyValues("GameInfo");
	if (g_HL2.KVLoadFromFile(pGameInfo, basefilesystem, "gameinfo.txt"))
	{
		const char *str;
		if ((str = pGameInfo->GetString("game", NULL)) != NULL)
		{
			ke::SafeStrcpy(buffer, maxlength, str);
			pGameInfo->deleteThis();
			return true;
		}
	}
	pGameInfo->deleteThis();
	return false;
}

const char *CoreProviderImpl::GetGameDescription()
{
	return SERVER_CALL(GetGameDescription)();
}

const char *CoreProviderImpl::GetSourceEngineName()
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
#elif SOURCE_ENGINE == SE_DOI
	return "doi";
#elif SOURCE_ENGINE == SE_CSGO
	return "csgo";
#elif SOURCE_ENGINE == SE_MOCK
	return "mock";
#endif
}

bool CoreProviderImpl::SymbolsAreHidden()
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
	|| (SOURCE_ENGINE == SE_DOI)  \
	|| (SOURCE_ENGINE == SE_BLADE)       \
	|| (SOURCE_ENGINE == SE_CSGO)
	return true;
#else
	return false;
#endif
}

void CoreProviderImpl::LogToGame(const char *message)
{
	Engine_LogPrintWrapper(message);
}

void CoreProviderImpl::ConPrint(const char *message)
{
	META_CONPRINT(message);
}

void CoreProviderImpl::ConsolePrint(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	UTIL_ConsolePrintVa(fmt, ap);
	va_end(ap);
}

void CoreProviderImpl::ConsolePrintVa(const char *message, va_list ap)
{
	UTIL_ConsolePrintVa(message, ap);
}

bool CoreProviderImpl::IsMapLoading()
{
	return g_SourceMod.IsMapLoading();
}

bool CoreProviderImpl::IsMapRunning()
{
	return g_SourceMod.IsMapRunning();
}

int CoreProviderImpl::MaxClients()
{
	return g_Players.MaxClients();
}

bool CoreProviderImpl::DescribePlayer(int index, const char **namep, const char **authp, int *useridp)
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
		*useridp = ::engine->GetPlayerUserId(player->GetEdict());
	return true;
}

int CoreProviderImpl::LoadMMSPlugin(const char *file, bool *ok, char *error, size_t maxlength)
{
	bool ignore_already;
	PluginId id = g_pMMPlugins->Load(file, g_PLID, ignore_already, error, maxlength);

	Pl_Status status;

	if (!id || (g_pMMPlugins->Query(id, NULL, &status, NULL) && status < Pl_Paused))
	{
		*ok = false;
	}
	else
	{
		*ok = true;
	}

	return id;
}

void CoreProviderImpl::UnloadMMSPlugin(int id)
{
	char ignore[255];
	g_pMMPlugins->Unload(id, true, ignore, sizeof(ignore));
}

bool CoreProviderImpl::IsClientConVarQueryingSupported()
{
	return hooks_.GetClientCvarQueryMode() != ClientCvarQueryMode::Unavailable;
}

int CoreProviderImpl::QueryClientConVar(int client, const char *cvar)
{
#if SOURCE_ENGINE != SE_DARKMESSIAH
	switch (hooks_.GetClientCvarQueryMode()) {
	case ClientCvarQueryMode::DLL:
		return ::engine->StartQueryCvarValue(PEntityOfEntIndex(client), cvar);
	case ClientCvarQueryMode::VSP:
		return serverpluginhelpers->StartQueryCvarValue(PEntityOfEntIndex(client), cvar);
	default:
		return InvalidQueryCvarCookie;
	}
#endif
	return -1;
}

void CoreProviderImpl::InitializeBridge()
{
	::serverGlobals.universalTime = g_pUniversalTime;
	::serverGlobals.frametime = &gpGlobals->frametime;
	::serverGlobals.interval_per_tick = &gpGlobals->interval_per_tick;

	this->engineFactory = (void *)g_SMAPI->GetEngineFactory(false);
	this->serverFactory = (void *)g_SMAPI->GetServerFactory(false);
	this->listeners = SMGlobalClass::head;

	if (auto mmlib = ::filesystem->LoadModule("matchmaking_ds" SOURCE_BIN_SUFFIX, "GAMEBIN")) {
		this->matchmakingDSFactory = (void*)Sys_GetFactory(mmlib);
	}

	logic_init_(this, &logicore);

	// Join logic's SMGlobalClass instances.
	SMGlobalClass* glob = SMGlobalClass::head;
	while (glob->m_pGlobalClassNext)
		glob = glob->m_pGlobalClassNext;
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

bool CoreProviderImpl::LoadProtobufProxy(char *error, size_t maxlength)
{
#if !defined(PROTOBUF_PROXY_ENABLE)
	return false;
#else
	char file[PLATFORM_MAX_PATH];

#if !defined(PROTOBUF_PROXY_BINARY_NAME)
# error "No engine suffix defined"
#endif

	/* Now it's time to load the logic binary */
	g_SMAPI->PathFormat(file,
		sizeof(file),
		"%s/bin/" PLATFORM_ARCH_FOLDER PROTOBUF_PROXY_BINARY_NAME PLATFORM_LIB_EXT,
		g_SourceMod.GetSourceModPath());

	char myerror[255];
	pbproxy_ = ke::SharedLib::Open(file, myerror, sizeof(myerror));
	if (!pbproxy_) {
		ke::SafeSprintf(error, maxlength, "failed to load %s: %s", file, myerror);
		return false;
	}

	auto fn = pbproxy_->get<GetProtobufProxyFn>("GetProtobufProxy");
	if (!fn) {
		ke::SafeStrcpy(error, maxlength, "could not find GetProtobufProxy function");
		return false;
	}

	gProtobufProxy = fn();
	return true;
#endif
}

bool CoreProviderImpl::LoadBridge(char *error, size_t maxlength)
{
	char file[PLATFORM_MAX_PATH];

	/* Now it's time to load the logic binary */
	g_SMAPI->PathFormat(file,
		sizeof(file),
		"%s/bin/" PLATFORM_ARCH_FOLDER "sourcemod.logic." PLATFORM_LIB_EXT,
		g_SourceMod.GetSourceModPath());

	char myerror[255];
	logic_ = ke::SharedLib::Open(file, myerror, sizeof(myerror));
	if (!logic_) {
		ke::SafeSprintf(error, maxlength, "failed to load %s: %s", file, myerror);
		return false;
	}

	LogicLoadFunction llf = logic_->get<decltype(llf)>("logic_load");
	if (!llf) {
		logic_ = nullptr;
		ke::SafeStrcpy(error, maxlength, "could not find logic_load function");
		return false;
	}

	GetITextParsers getitxt = logic_->get<decltype(getitxt)>("get_textparsers");
	textparsers = getitxt();

	logic_init_ = llf(SM_LOGIC_MAGIC);
	if (!logic_init_) {
		ke::SafeStrcpy(error, maxlength, "component version mismatch");
		return false;
	}
	return true;
}

ke::RefPtr<CommandHook>
CoreProviderImpl::AddCommandHook(ConCommand *cmd, const CommandHook::Callback &callback)
{
	return hooks_.AddCommandHook(cmd, callback);
}

ke::RefPtr<CommandHook>
CoreProviderImpl::AddPostCommandHook(ConCommand *cmd, const CommandHook::Callback &callback)
{
	return hooks_.AddPostCommandHook(cmd, callback);
}

CoreProviderImpl::CommandImpl::CommandImpl(ConCommand *cmd, CommandHook *hook)
: cmd_(cmd),
  hook_(hook)
{
}

CoreProviderImpl::CommandImpl::~CommandImpl()
{
	hook_ = nullptr;

	g_SMAPI->UnregisterConCommandBase(g_PLAPI, cmd_);
	delete [] const_cast<char *>(cmd_->GetHelpText());
	delete [] const_cast<char *>(cmd_->GetName());
	delete cmd_;
}

void
CoreProviderImpl::DefineCommand(const char *name, const char *help, const CommandFunc &callback)
{
	char *new_name = sm_strdup(name);
	char *new_help = sm_strdup(help);
	int flags = 0;

	auto ignore_callback = [] (DISPATCH_ARGS) -> void {
	};

	ConCommand *cmd = new ConCommand(new_name, ignore_callback, new_help, flags);
	ke::RefPtr<CommandHook> hook = AddCommandHook(cmd, callback);

	ke::RefPtr<CommandImpl> impl = new CommandImpl(cmd, hook);
	commands_.push_back(impl);
}

void CoreProviderImpl::FormatSourceBinaryName(const char *basename, char *buffer, size_t maxlength)
{
	bool use_prefix = (!strcasecmp(basename, "tier0") || !strcasecmp(basename, "vstdlib"));
	ke::SafeSprintf(buffer, maxlength, "%s%s%s%s", use_prefix ? SOURCE_BIN_PREFIX : "", basename, SOURCE_BIN_SUFFIX, SOURCE_BIN_EXT);
}

void CoreProviderImpl::InitializeHooks()
{
	hooks_.Start();
}

void CoreProviderImpl::OnVSPReceived()
{
	hooks_.OnVSPReceived();
}

void CoreProviderImpl::ShutdownHooks()
{
	commands_.clear();
	hooks_.Shutdown();
}

void CoreProviderImpl::ShutdownBridge()
{
	logic_ = nullptr;
}

void InitLogicBridge()
{
	sCoreProviderImpl.InitializeBridge();
}

bool StartLogicBridge(char *error, size_t maxlength)
{
	return sCoreProviderImpl.LoadBridge(error, maxlength);
}

void ShutdownLogicBridge()
{
	sCoreProviderImpl.ShutdownBridge();
}
