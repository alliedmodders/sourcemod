#include <stdio.h>
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "systems/LibrarySys.h"
#include "vm/sp_vm_engine.h"
#include <sh_string.h>
#include "PluginSys.h"
#include "ForwardSys.h"

SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, false, bool, const char *, const char *, const char *, const char *, bool, bool);

SourcePawnEngine g_SourcePawn;
SourceModBase g_SourceMod;

ILibrary *g_pJIT = NULL;
SourceHook::String g_BaseDir;
ISourcePawnEngine *g_pSourcePawn = &g_SourcePawn;
IVirtualMachine *g_pVM;

typedef int (*GIVEENGINEPOINTER)(ISourcePawnEngine *);
typedef unsigned int (*GETEXPORTCOUNT)();
typedef IVirtualMachine *(*GETEXPORT)(unsigned int);
typedef void (*NOTIFYSHUTDOWN)();

void ShutdownJIT()
{
	NOTIFYSHUTDOWN notify = (NOTIFYSHUTDOWN)g_pJIT->GetSymbolAddress("NotifyShutdown");
	if (notify)
	{
		notify();
	}

	g_pJIT->CloseLibrary();
}

SourceModBase::SourceModBase()
{
	m_IsMapLoading = false;
}

bool SourceModBase::InitializeSourceMod(char *error, size_t err_max, bool late)
{
	g_BaseDir.assign(g_SMAPI->GetBaseDir());
	g_LibSys.PathFormat(m_SMBaseDir, sizeof(m_SMBaseDir), "%s/addons/sourcemod", g_BaseDir.c_str());

	/* Attempt to load the JIT! */
	char file[PLATFORM_MAX_PATH];
	char myerror[255];
	g_SMAPI->PathFormat(file, sizeof(file), "%s/bin/sourcepawn.jit.x86.%s",
		GetSMBaseDir(),
		PLATFORM_LIB_EXT
		);

	g_pJIT = g_LibSys.OpenLibrary(file, myerror, sizeof(myerror));
	if (!g_pJIT)
	{
		if (error && err_max)
		{
			snprintf(error, err_max, "%s (failed to load bin/sourcepawn.jit.x86.%s)", 
				myerror,
				PLATFORM_LIB_EXT);
		}
		return false;
	}

	int err;
	GIVEENGINEPOINTER jit_init = (GIVEENGINEPOINTER)g_pJIT->GetSymbolAddress("GiveEnginePointer");
	if (!jit_init)
	{
		ShutdownJIT();
		if (error && err_max)
		{
			snprintf(error, err_max, "Failed to find GiveEnginePointer in JIT!");
		}
		return false;
	}

	if ((err=jit_init(g_pSourcePawn)) != 0)
	{
		ShutdownJIT();
		if (error && err_max)
		{
			snprintf(error, err_max, "GiveEnginePointer returned %d in the JIT", err);
		}
		return false;
	}

	GETEXPORTCOUNT jit_getnum = (GETEXPORTCOUNT)g_pJIT->GetSymbolAddress("GetExportCount");
	GETEXPORT jit_get = (GETEXPORT)g_pJIT->GetSymbolAddress("GetExport");
	if (!jit_get)
	{
		ShutdownJIT();
		if (error && err_max)
		{
			snprintf(error, err_max, "JIT is missing a necessary export!");
		}
		return false;
	}

	unsigned int num = jit_getnum();
	if (!num || ((g_pVM=jit_get(0)) == NULL))
	{
		ShutdownJIT();
		if (error && err_max)
		{
			snprintf(error, err_max, "JIT did not export any virtual machines!");
		}
		return false;
	}

	unsigned int api = g_pVM->GetAPIVersion();
	if (api != SOURCEPAWN_VM_API_VERSION)
	{
		ShutdownJIT();
		if (error && err_max)
		{
			snprintf(error, err_max, "JIT is not a compatible version");
		}
		return false;
	}

	StartSourceMod(late);

	return true;
}

void SourceModBase::StartSourceMod(bool late)
{
	/* First initialize the global hooks we need */
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, gamedll, this, &SourceModBase::LevelInit, false);

	/* If we're late, automatically load plugins now */
	DoGlobalPluginLoads();
}

bool SourceModBase::LevelInit(char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background)
{
	m_IsMapLoading = true;

	DoGlobalPluginLoads();

	m_IsMapLoading = false;

	RETURN_META_VALUE(MRES_IGNORED, true);
}

void SourceModBase::DoGlobalPluginLoads()
{
	char config_path[PLATFORM_MAX_PATH];
	char plugins_path[PLATFORM_MAX_PATH];

	g_SMAPI->PathFormat(config_path, 
		sizeof(config_path),
		"%s/configs/plugin_settings.cfg",
		GetSMBaseDir());
	g_SMAPI->PathFormat(plugins_path,
		sizeof(plugins_path),
		"%s/plugins",
		GetSMBaseDir());

	g_PluginMngr.RefreshOrLoadPlugins(config_path, plugins_path);
}

const char *SourceModBase::GetSMBaseDir()
{
	return m_SMBaseDir;
}
