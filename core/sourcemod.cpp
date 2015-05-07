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
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "LibrarySys.h"
#include <sh_string.h>
#include "CoreConfig.h"
#include "Logger.h"
#include "sm_stringutil.h"
#include "PlayerManager.h"
#include "TimerSys.h"
#include <IGameConfigs.h>
#include "frame_hooks.h"
#include "logic_bridge.h"

SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, false, bool, const char *, const char *, const char *, const char *, bool, bool);
SH_DECL_HOOK0_void(IServerGameDLL, LevelShutdown, SH_NOATTRIB, false);
SH_DECL_HOOK1_void(IServerGameDLL, GameFrame, SH_NOATTRIB, false, bool);
SH_DECL_HOOK1_void(IVEngineServer, ServerCommand, SH_NOATTRIB, false, const char *);

SourceModBase g_SourceMod;

ILibrary *g_pJIT = NULL;
SourceHook::String g_BaseDir;
ISourcePawnEngine *g_pSourcePawn = NULL;
ISourcePawnEngine2 *g_pSourcePawn2 = NULL;
ISourcePawnEnvironment *g_pPawnEnv = NULL;
IdentityToken_t *g_pCoreIdent = NULL;
IForward *g_pOnMapEnd = NULL;
IGameConfig *g_pGameConf = NULL;
bool g_Loaded = false;
bool sm_show_debug_spew = false;
bool sm_disable_jit = false;

#ifdef PLATFORM_WINDOWS
ConVar sm_basepath("sm_basepath", "addons\\sourcemod", 0, "SourceMod base path (set via command line)");
#elif defined PLATFORM_LINUX || defined PLATFORM_APPLE
ConVar sm_basepath("sm_basepath", "addons/sourcemod", 0, "SourceMod base path (set via command line)");
#endif

void ShutdownJIT()
{
	if (g_pPawnEnv) {
		g_pPawnEnv->Shutdown();
		delete g_pPawnEnv;

		g_pPawnEnv = NULL;
		g_pSourcePawn2 = NULL;
		g_pSourcePawn = NULL;
	}

	g_pJIT->CloseLibrary();
	g_pJIT = NULL;
}

SourceModBase::SourceModBase()
{
	m_IsMapLoading = false;
	m_ExecPluginReload = false;
	m_GotBasePath = false;
}

ConfigResult SourceModBase::OnSourceModConfigChanged(const char *key, 
									  const char *value, 
									  ConfigSource source, 
									  char *error, 
									  size_t maxlength)
{
	if (strcasecmp(key, "BasePath") == 0)
	{
		if (source == ConfigSource_Console)
		{
			UTIL_Format(error, maxlength, "Cannot be set at runtime");
			return ConfigResult_Reject;
		}

		if (!m_GotBasePath)
		{
			g_LibSys.PathFormat(m_SMBaseDir, sizeof(m_SMBaseDir), "%s/%s", g_BaseDir.c_str(), value);
			g_LibSys.PathFormat(m_SMRelDir, sizeof(m_SMRelDir), value);

			m_GotBasePath = true;
		}

		return ConfigResult_Accept;
	}
	else if (strcasecmp(key, "DebugSpew") == 0)
	{
		sm_show_debug_spew = (strcasecmp(value, "yes") == 0) ? true : false;

		return ConfigResult_Accept;
	}
	else if (strcasecmp(key, "DisableJIT") == 0)
	{
		sm_disable_jit = (strcasecmp(value, "yes") == 0) ? true : false;
		if (g_pSourcePawn2)
			g_pSourcePawn2->SetJitEnabled(!sm_disable_jit);

		return ConfigResult_Accept;
	}

	return ConfigResult_Ignore;
}

static bool sSourceModInitialized = false;

bool SourceModBase::InitializeSourceMod(char *error, size_t maxlength, bool late)
{
	const char *gamepath = g_SMAPI->GetBaseDir();

	/* Store full path to game */
	g_BaseDir.assign(gamepath);

	/* Store name of game directory by itself */
	size_t len = strlen(gamepath);
	for (size_t i = len - 1; i < len; i--)
	{
		if (gamepath[i] == PLATFORM_SEP_CHAR)
		{
			strncopy(m_ModDir, &gamepath[++i], sizeof(m_ModDir));
			break;
		}
	}

	const char *basepath = icvar->GetCommandLineValue("sm_basepath");
	/* Set a custom base path if there is one. */
	if (basepath != NULL && basepath[0] != '\0')
	{
		m_GotBasePath = true;
	}
	/* Otherwise, use a default and keep the m_GotBasePath unlocked. */
	else
	{
		basepath = sm_basepath.GetDefault();
	}

	g_LibSys.PathFormat(m_SMBaseDir, sizeof(m_SMBaseDir), "%s/%s", g_BaseDir.c_str(), basepath);
	g_LibSys.PathFormat(m_SMRelDir, sizeof(m_SMRelDir), "%s", basepath);

	if (!StartLogicBridge(error, maxlength))
	{
		return false;
	}

	/* There will always be a path by this point, since it was force-set above. */
	m_GotBasePath = true;

	/* Attempt to load the JIT! */
	char file[PLATFORM_MAX_PATH];
	char myerror[255];
	g_SMAPI->PathFormat(file, sizeof(file), "%s/bin/sourcepawn.jit.x86.%s",
		GetSourceModPath(),
		PLATFORM_LIB_EXT
		);

	g_pJIT = g_LibSys.OpenLibrary(file, myerror, sizeof(myerror));
	if (!g_pJIT)
	{
		if (error && maxlength)
		{
			UTIL_Format(error, maxlength, "%s (failed to load bin/sourcepawn.jit.x86.%s)", 
				myerror,
				PLATFORM_LIB_EXT);
		}
		return false;
	}

	GetSourcePawnFactoryFn factoryFn =
		(GetSourcePawnFactoryFn)g_pJIT->GetSymbolAddress("GetSourcePawnFactory");

	if (!factoryFn) {
		if (error && maxlength)
			snprintf(error, maxlength, "SourcePawn library is out of date");
		ShutdownJIT();
		return false;
	}

	ISourcePawnFactory *factory = factoryFn(SOURCEPAWN_API_VERSION);
	if (!factory) {
		if (error && maxlength)
			snprintf(error, maxlength, "SourcePawn library is out of date");
		ShutdownJIT();
		return false;
	}

	g_pPawnEnv = factory->NewEnvironment();
	if (!g_pPawnEnv) {
		if (error && maxlength)
			snprintf(error, maxlength, "Could not create a SourcePawn environment!");
		ShutdownJIT();
		return false;
	}

	g_pSourcePawn = g_pPawnEnv->APIv1();
	g_pSourcePawn2 = g_pPawnEnv->APIv2();

	g_pSourcePawn2->SetDebugListener(logicore.debugger);

	if (sm_disable_jit)
		g_pSourcePawn2->SetJitEnabled(!sm_disable_jit);

	sSourceModInitialized = true;

	/* Hook this now so we can detect startup without calling StartSourceMod() */
	SH_ADD_HOOK(IServerGameDLL, LevelInit, gamedll, SH_MEMBER(this, &SourceModBase::LevelInit), false);

	/* Only load if we're not late */
	if (!late)
	{
		StartSourceMod(false);
	}

	return true;
}

void SourceModBase::StartSourceMod(bool late)
{
	SH_ADD_HOOK(IServerGameDLL, LevelShutdown, gamedll, SH_MEMBER(this, &SourceModBase::LevelShutdown), false);
	SH_ADD_HOOK(IServerGameDLL, GameFrame, gamedll, SH_MEMBER(&g_Timers, &TimerSystem::GameFrame), false);

	enginePatch = SH_GET_CALLCLASS(engine);
	gamedllPatch = SH_GET_CALLCLASS(gamedll);

	InitLogicBridge();

	/* Initialize CoreConfig to get the SourceMod base path properly - this parses core.cfg */
	g_CoreConfig.Initialize();

	/* Notify! */
	SMGlobalClass *pBase = SMGlobalClass::head;
	while (pBase)
	{
		pBase->OnSourceModStartup(false);
		pBase = pBase->m_pGlobalClassNext;
	}
	g_pGameConf = logicore.GetCoreGameConfig();

	/* Notify! */
	pBase = SMGlobalClass::head;
	while (pBase)
	{
		pBase->OnSourceModAllInitialized();
		pBase = pBase->m_pGlobalClassNext;
	}

	/* Notify! */
	pBase = SMGlobalClass::head;
	while (pBase)
	{
		pBase->OnSourceModAllInitialized_Post();
		pBase = pBase->m_pGlobalClassNext;
	}

	/* Add us now... */
	sharesys->AddInterface(NULL, this);

	/* We're loaded! */
	g_Loaded = true;

	/* Initialize VSP stuff */
	if (vsp_interface != NULL)
	{
		g_SourceMod_Core.OnVSPListening(vsp_interface);
	}

	if (late)
	{
		/* We missed doing anythin gin this if we late-loaded. Sneak it in now. */
		AllPluginsLoaded();
	}

	/* If we want to autoload, do that now */
	const char *disabled = GetCoreConfigValue("DisableAutoUpdate");
	if (disabled == NULL || strcasecmp(disabled, "yes") != 0)
	{
		extsys->LoadAutoExtension("updater.ext." PLATFORM_LIB_EXT);
	}

	const char *timeout = GetCoreConfigValue("SlowScriptTimeout");
	if (timeout == NULL)
	{
		timeout = "8";
	}
	if (atoi(timeout) != 0)
	{
		g_pSourcePawn2->InstallWatchdogTimer(atoi(timeout) * 1000);
	}
}

static bool g_LevelEndBarrier = false;
bool SourceModBase::LevelInit(char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background)
{
	/* If we're not loaded... */
	if (!g_Loaded)
	{
		/* Do all global initialization now */
		StartSourceMod(true);
	}

	m_IsMapLoading = true;
	m_ExecPluginReload = true;

	/* Notify! */
	SMGlobalClass *pBase = SMGlobalClass::head;
	while (pBase)
	{
		pBase->OnSourceModLevelChange(pMapName);
		pBase = pBase->m_pGlobalClassNext;
	}

	DoGlobalPluginLoads();

	m_IsMapLoading = false;

	/* Notify! */
	pBase = SMGlobalClass::head;
	while (pBase)
	{
		pBase->OnSourceModPluginsLoaded();
		pBase = pBase->m_pGlobalClassNext;
	}

	if (!g_pOnMapEnd)
	{
		g_pOnMapEnd = forwardsys->CreateForward("OnMapEnd", ET_Ignore, 0, NULL);
	}

	g_LevelEndBarrier = true;

	RETURN_META_VALUE(MRES_IGNORED, true);
}

void SourceModBase::LevelShutdown()
{
	if (g_LevelEndBarrier)
	{
		SMGlobalClass *next = SMGlobalClass::head;
		while (next)
		{
			next->OnSourceModLevelEnd();
			next = next->m_pGlobalClassNext;
		}
		
		if (g_pOnMapEnd != NULL)
		{
			g_pOnMapEnd->Execute(NULL);
		}
		extsys->CallOnCoreMapEnd();

		g_Timers.RemoveMapChangeTimers();

		g_LevelEndBarrier = false;
	}

	g_OnMapStarted = false;

	if (m_ExecPluginReload)
	{
		scripts->RefreshAll();
		m_ExecPluginReload = false;
	}
}

bool SourceModBase::IsMapLoading() const
{
	return m_IsMapLoading;
}

void SourceModBase::DoGlobalPluginLoads()
{
	char config_path[PLATFORM_MAX_PATH];
	char plugins_path[PLATFORM_MAX_PATH];

	BuildPath(Path_SM, config_path, 
		sizeof(config_path),
		"configs/plugin_settings.cfg");
	BuildPath(Path_SM, plugins_path,
		sizeof(plugins_path),
		"plugins");

	/* Load any auto extensions */
	extsys->TryAutoload();

	/* Fire the extensions ready message */
	g_SMAPI->MetaFactory(SOURCEMOD_NOTICE_EXTENSIONS, NULL, NULL);

	/* Load any game extension */
	const char *game_ext;
	if ((game_ext = g_pGameConf->GetKeyValue("GameExtension")) != NULL)
	{
		char path[PLATFORM_MAX_PATH];
		UTIL_Format(path, sizeof(path), "%s.ext." PLATFORM_LIB_EXT, game_ext);
		extsys->LoadAutoExtension(path);
	}

	scripts->LoadAll(config_path, plugins_path);
}

size_t SourceModBase::BuildPath(PathType type, char *buffer, size_t maxlength, const char *format, ...)
{
	char _buffer[PLATFORM_MAX_PATH];
	va_list ap;

	va_start(ap, format);
	vsnprintf(_buffer, PLATFORM_MAX_PATH, format, ap);
	va_end(ap);

	/* If we get a "file://" notation, strip off the file:// part so we're left 
	 * with an absolute path.  Note that the absolute path gets returned, so 
	 * usage with relative paths here is completely invalid.
	 */
	if (type != Path_SM_Rel && strncmp(_buffer, "file://", 7) == 0)
	{
		return g_LibSys.PathFormat(buffer, maxlength, "%s", &_buffer[7]);
	}

	const char *base = NULL;
	if (type == Path_Game)
	{
		base = GetGamePath();
	}
	else if (type == Path_SM)
	{
		base = GetSourceModPath();
	}
	else if (type == Path_SM_Rel)
	{
		base = m_SMRelDir;
	}

	if (base)
	{
		return g_LibSys.PathFormat(buffer, maxlength, "%s/%s", base, _buffer);
	}
	else
	{
		return g_LibSys.PathFormat(buffer, maxlength, "%s", _buffer);
	}
}

void SourceModBase::CloseSourceMod()
{
	if (!sSourceModInitialized)
		return;

	SH_REMOVE_HOOK(IServerGameDLL, LevelInit, gamedll, SH_MEMBER(this, &SourceModBase::LevelInit), false);

	if (g_Loaded)
	{
		/* Force a level end */
		LevelShutdown();
		ShutdownServices();
	}

	/* Rest In Peace */
	ShutdownLogicBridge();
	ShutdownJIT();
}

void SourceModBase::ShutdownServices()
{
	/* Unload plugins */
	scripts->Shutdown();

	/* Unload extensions */
	extsys->Shutdown();

	if (g_pOnMapEnd)
		forwardsys->ReleaseForward(g_pOnMapEnd);

	/* Notify! */
	SMGlobalClass *pBase = SMGlobalClass::head;
	while (pBase)
	{
		pBase->OnSourceModShutdown();
		pBase = pBase->m_pGlobalClassNext;
	}

	/* Delete all data packs */
	CStack<CDataPack *>::iterator iter;
	CDataPack *pd;
	for (iter=m_freepacks.begin(); iter!=m_freepacks.end(); iter++)
	{
		pd = (*iter);
		delete pd;
	}
	m_freepacks.popall();

	/* Notify! */
	pBase = SMGlobalClass::head;
	while (pBase)
	{
		pBase->OnSourceModAllShutdown();
		pBase = pBase->m_pGlobalClassNext;
	}

	if (enginePatch)
	{
		SH_RELEASE_CALLCLASS(enginePatch);
		enginePatch = NULL;
	}

	if (gamedllPatch)
	{
		SH_RELEASE_CALLCLASS(gamedllPatch);
		gamedllPatch = NULL;
	}

	SH_REMOVE_HOOK(IServerGameDLL, LevelShutdown, gamedll, SH_MEMBER(this, &SourceModBase::LevelShutdown), false);
	SH_REMOVE_HOOK(IServerGameDLL, GameFrame, gamedll, SH_MEMBER(&g_Timers, &TimerSystem::GameFrame), false);
}

void SourceModBase::LogMessage(IExtension *pExt, const char *format, ...)
{
	IExtensionInterface *pAPI = pExt->GetAPI();
	const char *tag = pAPI->GetExtensionTag();
	char buffer[2048];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);

	if (tag)
	{
		logger->LogMessage("[%s] %s", tag, buffer);
	} else {
		logger->LogMessage("%s", buffer);
	}
}

void SourceModBase::LogError(IExtension *pExt, const char *format, ...)
{
	IExtensionInterface *pAPI = pExt->GetAPI();
	const char *tag = pAPI->GetExtensionTag();
	char buffer[2048];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);

	if (tag)
	{
		logger->LogError("[%s] %s", tag, buffer);
	} else {
		logger->LogError("%s", buffer);
	}
}

size_t SourceModBase::FormatString(char *buffer, size_t maxlength, IPluginContext *pContext, const cell_t *params, unsigned int param)
{
	char *fmt;

	pContext->LocalToString(params[param], &fmt);

	int lparam = ++param;

	return atcprintf(buffer, maxlength, fmt, pContext, params, &lparam);
}

const char *SourceModBase::GetSourceModPath() const
{
	return m_SMBaseDir;
}

const char *SourceModBase::GetGamePath() const
{
	return g_BaseDir.c_str();
}

unsigned int SourceModBase::SetGlobalTarget(unsigned int index)
{
	unsigned int old = m_target;
	m_target = index;
	return old;
}

unsigned int SourceModBase::GetGlobalTarget() const
{
	return m_target;
}

IDataPack *SourceModBase::CreateDataPack()
{
	CDataPack *pack;
	if (m_freepacks.empty())
	{
		pack = new CDataPack;
	} else {
		pack = m_freepacks.front();
		m_freepacks.pop();
		pack->Initialize();
	}
	return pack;
}

void SourceModBase::FreeDataPack(IDataPack *pack)
{
	m_freepacks.push(static_cast<CDataPack *>(pack));
}

Handle_t SourceModBase::GetDataPackHandleType(bool readonly)
{
	//:TODO:
	return 0;
}

const char *SourceModBase::GetGameFolderName() const
{
	return m_ModDir;
}

ISourcePawnEngine *SourceModBase::GetScriptingEngine()
{
	return g_pSourcePawn;
}

IVirtualMachine *SourceModBase::GetScriptingVM()
{
	return NULL;
}

void SourceModBase::AllPluginsLoaded()
{
	if (!g_Loaded)
	{
		return;
	}

	SMGlobalClass *base = SMGlobalClass::head;
	while (base)
	{
		base->OnSourceModGameInitialized();
		base = base->m_pGlobalClassNext;
	}
}

time_t SourceModBase::GetAdjustedTime()
{
	return ::GetAdjustedTime();
}

void SourceModBase::AddGameFrameHook(GAME_FRAME_HOOK hook)
{
	m_frame_hooks.push_back(hook);
}

void SourceModBase::RemoveGameFrameHook(GAME_FRAME_HOOK hook)
{
	for (size_t i = 0; i < m_frame_hooks.size(); i++)
	{
		if (m_frame_hooks[i] == hook)
		{
			m_frame_hooks.erase(m_frame_hooks.iterAt(i));
			return;
		}
	}
}

void SourceModBase::ProcessGameFrameHooks(bool simulating)
{
	if (m_frame_hooks.size() == 0)
	{
		return;
	}

	for (size_t i = 0; i < m_frame_hooks.size(); i++)
	{
		m_frame_hooks[i](simulating);
	}
}

size_t SourceModBase::Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
	size_t len;
	va_list ap;

	va_start(ap, fmt);
	len = FormatArgs(buffer, maxlength, fmt, ap);
	va_end(ap);

	return len;
}

size_t SourceModBase::FormatArgs(char *buffer,
								 size_t maxlength,
								 const char *fmt,
								 va_list ap)
{
	return UTIL_FormatArgs(buffer, maxlength, fmt, ap);
}

void SourceModBase::AddFrameAction(FRAMEACTION fn, void *data)
{
	::AddFrameAction(FrameAction(fn, data));
}

const char *SourceModBase::GetCoreConfigValue(const char *key)
{
	return g_CoreConfig.GetCoreConfigValue(key);
}

int SourceModBase::GetPluginId()
{
	return g_PLID;
}

int SourceModBase::GetShApiVersion()
{
	int api, impl;
	g_SMAPI->GetShVersions(api, impl);

	return api;
}

bool SourceModBase::IsMapRunning()
{
	return g_OnMapStarted;
}

SMGlobalClass *SMGlobalClass::head = NULL;

