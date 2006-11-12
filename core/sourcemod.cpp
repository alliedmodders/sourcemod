#include <stdio.h>
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "systems/LibrarySys.h"
#include "vm/sp_vm_engine.h"
#include <sh_string.h>
#include "PluginSys.h"
#include "ForwardSys.h"

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

bool SourceModBase::InitializeSourceMod(char *error, size_t err_max, bool late)
{
	//:TODO: we need a localinfo system!
	g_BaseDir.assign(g_SMAPI->GetBaseDir());

	/* Attempt to load the JIT! */
	char file[PLATFORM_MAX_PATH];
	char myerror[255];
	g_SMAPI->PathFormat(file, sizeof(file), "%s/addons/sourcemod/bin/sourcepawn.jit.x86.%s",
		g_BaseDir.c_str(),
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

	g_SMAPI->PathFormat(file, sizeof(file), "%s/addons/sourcemod/plugins/test.smx", g_BaseDir.c_str());
	IPlugin *pPlugin = g_PluginMngr.LoadPlugin(file, false, PluginType_Global, error, err_max);
	IPluginFunction *func = pPlugin->GetFunctionByName("Test");
	IPluginFunction *func2 = pPlugin->GetFunctionByName("Test2");
	cell_t result = 2;
	cell_t val = 6;
	ParamType types[] = {Param_Cell, Param_CellByRef};
	va_list ap = va_start(ap, late);
	CForward *fwd = CForward::CreateForward(NULL, ET_Custom, 2, types, ap);
	fwd->AddFunction(func);
	fwd->AddFunction(func2);
	fwd->PushCell(1);
	fwd->PushCellByRef(&val, 0);
	fwd->Execute(&result, NULL);
	g_PluginMngr.UnloadPlugin(pPlugin);

	return true;
}
