#include <stdio.h>
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "systems/LibrarySys.h"
#include "vm/sp_vm_engine.h"
#include <sh_string.h>
#include "PluginSys.h"
#include "ShareSys.h"
#include "CLogger.h"
#include "ExtensionSys.h"

SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, false, bool, const char *, const char *, const char *, const char *, bool, bool);

SourcePawnEngine g_SourcePawn;
SourceModBase g_SourceMod;

ILibrary *g_pJIT = NULL;
SourceHook::String g_BaseDir;
ISourcePawnEngine *g_pSourcePawn = &g_SourcePawn;
IVirtualMachine *g_pVM;
IdentityToken_t *g_pCoreIdent = NULL;

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
		GetSourceModPath(),
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

	/* Notify! */
	SMGlobalClass *pBase = SMGlobalClass::head;
	while (pBase)
	{
		pBase->OnSourceModStartup(false);
		pBase = pBase->m_pGlobalClassNext;
	}

	/* Make the global core identity */
	g_pCoreIdent = g_ShareSys.CreateCoreIdentity();

	/* Notify! */
	pBase = SMGlobalClass::head;
	while (pBase)
	{
		pBase->OnSourceModAllInitialized();
		pBase = pBase->m_pGlobalClassNext;
	}

	/* Add us now... */
	g_ShareSys.AddInterface(NULL, this);
}

bool SourceModBase::LevelInit(char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background)
{
	m_IsMapLoading = true;

	g_Logger.MapChange(pMapName);

	DoGlobalPluginLoads();

	m_IsMapLoading = false;

	RETURN_META_VALUE(MRES_IGNORED, true);
}

bool SourceModBase::IsMapLoading()
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

	/* Run the first pass */
	g_PluginSys.LoadAll_FirstPass(config_path, plugins_path);

	/* Mark any extensions as loaded */
	g_Extensions.MarkAllLoaded();

	/* No modules yet, it's safe to call this from here */
	g_PluginSys.LoadAll_SecondPass();

	/* Re-mark any extensions as loaded */
	g_Extensions.MarkAllLoaded();
}

size_t SourceModBase::BuildPath(PathType type, char *buffer, size_t maxlength, char *format, ...)
{
	char _buffer[PLATFORM_MAX_PATH+1];
	va_list ap;

	va_start(ap, format);
	vsnprintf(_buffer, PLATFORM_MAX_PATH, format, ap);
	va_end(ap);

	const char *base = NULL;
	if (type == Path_Game)
	{
		base = GetModPath();
	} else if (type == Path_SM) {
		base = GetSourceModPath();
	}

	if (base)
	{
		return g_LibSys.PathFormat(buffer, maxlength, "%s/%s", base, _buffer);
	} else {
		return g_LibSys.PathFormat(buffer, maxlength, "%s", _buffer);
	}
}

void SourceModBase::CloseSourceMod()
{
	/* Notify! */
	SMGlobalClass *pBase = SMGlobalClass::head;
	while (pBase)
	{
		pBase->OnSourceModShutdown();
		pBase = pBase->m_pGlobalClassNext;
	}

	/* Notify! */
	pBase = SMGlobalClass::head;
	while (pBase)
	{
		pBase->OnSourceModAllShutdown();
		pBase = pBase->m_pGlobalClassNext;
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

const char *SourceModBase::GetSourceModPath()
{
	return m_SMBaseDir;
}

const char *SourceModBase::GetModPath()
{
	return g_BaseDir.c_str();
}

SMGlobalClass *SMGlobalClass::head = NULL;

SMGlobalClass::SMGlobalClass()
{
	m_pGlobalClassNext = SMGlobalClass::head;
	SMGlobalClass::head = this;
}
