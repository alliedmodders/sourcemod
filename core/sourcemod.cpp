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

#include <stdio.h>
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "systems/LibrarySys.h"
#include <sh_string.h>
#include "PluginSys.h"
#include "ShareSys.h"
#include "CoreConfig.h"
#include "Logger.h"
#include "ExtensionSys.h"
#include "AdminCache.h"
#include "sm_stringutil.h"
#include "PlayerManager.h"
#include "Translator.h"
#include "ForwardSys.h"
#include "TimerSys.h"
#include "GameConfigs.h"
#include "DebugReporter.h"
#include "Profiler.h"

SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, false, bool, const char *, const char *, const char *, const char *, bool, bool);
SH_DECL_HOOK0_void(IServerGameDLL, LevelShutdown, SH_NOATTRIB, false);
SH_DECL_HOOK1_void(IServerGameDLL, GameFrame, SH_NOATTRIB, false, bool);
SH_DECL_HOOK1_void(IVEngineServer, ServerCommand, SH_NOATTRIB, false, const char *);

SourceModBase g_SourceMod;

ILibrary *g_pJIT = NULL;
SourceHook::String g_BaseDir;
ISourcePawnEngine *g_pSourcePawn = NULL;
ISourcePawnEngine2 *g_pSourcePawn2 = NULL;
IdentityToken_t *g_pCoreIdent = NULL;
IForward *g_pOnMapEnd = NULL;
bool g_Loaded = false;

typedef ISourcePawnEngine *(*GET_SP_V1)();
typedef ISourcePawnEngine2 *(*GET_SP_V2)();
typedef void (*NOTIFYSHUTDOWN)();

void ShutdownJIT()
{
	NOTIFYSHUTDOWN notify = (NOTIFYSHUTDOWN)g_pJIT->GetSymbolAddress("NotifyShutdown");
	if (notify)
	{
		notify();
	}

	if (g_pSourcePawn2 != NULL)
	{
		g_pSourcePawn2->Shutdown();
	}

	g_pJIT->CloseLibrary();
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

	return ConfigResult_Ignore;
}

bool SourceModBase::InitializeSourceMod(char *error, size_t maxlength, bool late)
{
	const char *gamepath = g_SMAPI->GetBaseDir();

	/* Store full path to game */
	g_BaseDir.assign(gamepath);

	/* Store name of game directory by itself */
	size_t len = strlen(gamepath);
	for (size_t i = len - 1; i >= 0; i--)
	{
		if (gamepath[i] == PLATFORM_SEP_CHAR)
		{
			strncopy(m_ModDir, &gamepath[++i], sizeof(m_ModDir));
			break;
		}
	}

	/* Initialize CoreConfig so we can get SourceMod base path properly - this basically parses core.cfg */
	g_CoreConfig.Initialize();

	/* This shouldn't happen, but can't hurt to be safe */
	if (!g_LibSys.PathExists(m_SMBaseDir) || !m_GotBasePath)
	{
		g_LibSys.PathFormat(m_SMBaseDir, sizeof(m_SMBaseDir), "%s/addons/sourcemod", g_BaseDir.c_str());
		g_LibSys.PathFormat(m_SMRelDir, sizeof(m_SMRelDir), "addons/sourcemod");
		m_GotBasePath = true;
	}

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
			snprintf(error, maxlength, "%s (failed to load bin/sourcepawn.jit.x86.%s)", 
				myerror,
				PLATFORM_LIB_EXT);
		}
		return false;
	}

	GET_SP_V1 getv1 = (GET_SP_V1)g_pJIT->GetSymbolAddress("GetSourcePawnEngine1");
	GET_SP_V2 getv2 = (GET_SP_V2)g_pJIT->GetSymbolAddress("GetSourcePawnEngine2");

	if (getv1 == NULL)
	{
		if (error && maxlength)
		{
			snprintf(error, maxlength, "JIT is too old; upgrade SourceMod");
		}
		ShutdownJIT();
		return false;
	}
	else if (getv2 == NULL)
	{
		if (error && maxlength)
		{
			snprintf(error, maxlength, "JIT is too old; upgrade SourceMod");
		}
		ShutdownJIT();
		return false;
	}

	g_pSourcePawn = getv1();
	g_pSourcePawn2 = getv2();

	if (g_pSourcePawn2->GetAPIVersion() < 2)
	{
		g_pSourcePawn2 = NULL;
		if (error && maxlength)
		{
			snprintf(error, maxlength, "JIT version is out of date");
		}
		return false;
	}

	if (!g_pSourcePawn2->Initialize())
	{
		g_pSourcePawn2 = NULL;
		if (error && maxlength)
		{
			snprintf(error, maxlength, "JIT could not be initialized");
		}
		return false;
	}

	g_pSourcePawn2->SetDebugListener(&g_DbgReporter);
	g_pSourcePawn2->SetProfiler(&g_Profiler);

	/* Hook this now so we can detect startup without calling StartSourceMod() */
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, gamedll, this, &SourceModBase::LevelInit, false);

	/* Only load if we're not late */
	if (!late)
	{
		StartSourceMod(false);
	}

	return true;
}

void SourceModBase::StartSourceMod(bool late)
{
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, gamedll, this, &SourceModBase::LevelShutdown, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, gamedll, &g_Timers, &TimerSystem::GameFrame, false);

	enginePatch = SH_GET_CALLCLASS(engine);
	gamedllPatch = SH_GET_CALLCLASS(gamedll);

	g_ShareSys.Initialize();

	/* Make the global core identity */
	g_pCoreIdent = g_ShareSys.CreateCoreIdentity();

	/* Notify! */
	SMGlobalClass *pBase = SMGlobalClass::head;
	while (pBase)
	{
		pBase->OnSourceModStartup(false);
		pBase = pBase->m_pGlobalClassNext;
	}

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
	g_ShareSys.AddInterface(NULL, this);

	/* We're loaded! */
	g_Loaded = true;

	/* Initialize VSP stuff */
	if (vsp_interface != NULL)
	{
		g_SourceMod_Core.OnVSPListening(vsp_interface);
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
		g_pOnMapEnd = g_Forwards.CreateForward("OnMapEnd", ET_Ignore, 0, NULL);
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

		g_Timers.RemoveMapChangeTimers();

		g_LevelEndBarrier = false;
	}

	g_OnMapStarted = false;

	if (m_ExecPluginReload)
	{
		g_PluginSys.ReloadOrUnloadPlugins();
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
	g_Extensions.TryAutoload();

	/* Fire the extensions ready message */
	g_SMAPI->MetaFactory(SOURCEMOD_NOTICE_EXTENSIONS, NULL, NULL);	

	/* Load any game extension */
	const char *game_ext;
	if ((game_ext = g_pGameConf->GetKeyValue("GameExtension")) != NULL)
	{
		char path[PLATFORM_MAX_PATH];
		UTIL_Format(path, sizeof(path), "%s.ext." PLATFORM_LIB_EXT, game_ext);
		g_Extensions.LoadAutoExtension(path);
	}

	/* Run the first pass */
	g_PluginSys.LoadAll_FirstPass(config_path, plugins_path);

	/* Mark any extensions as loaded */
	g_Extensions.MarkAllLoaded();

	/* No modules yet, it's safe to call this from here */
	g_PluginSys.LoadAll_SecondPass();

	/* Re-mark any extensions as loaded */
	g_Extensions.MarkAllLoaded();

	/* Call OnAllPluginsLoaded */
	g_PluginSys.AllPluginsLoaded();
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
	/* Force a level end */
	LevelShutdown();

	/* Unload plugins */
	g_PluginSys.Shutdown();

	/* Unload extensions */
	g_Extensions.Shutdown();

	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, gamedll, this, &SourceModBase::LevelInit, false);

	if (g_Loaded)
	{
		if (g_pOnMapEnd)
		{
			g_Forwards.ReleaseForward(g_pOnMapEnd);
		}
	
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

		SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, gamedll, this, &SourceModBase::LevelShutdown, false);
		SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameFrame, gamedll, &g_Timers, &TimerSystem::GameFrame, false);
	}

	/* Rest In Peace */
	ShutdownJIT();
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
		g_Logger.LogMessage("[%s] %s", tag, buffer);
	} else {
		g_Logger.LogMessage("%s", buffer);
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
		g_Logger.LogError("[%s] %s", tag, buffer);
	} else {
		g_Logger.LogError("%s", buffer);
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

SMGlobalClass *SMGlobalClass::head = NULL;

SMGlobalClass::SMGlobalClass()
{
	m_pGlobalClassNext = SMGlobalClass::head;
	SMGlobalClass::head = this;
}
