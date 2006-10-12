#include <stdio.h>
#include <sp_vm_api.h>
#include <sp_vm_context.h>
#include "sp_vm_engine.h"
#include "sp_vm_basecontext.h"
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

using namespace SourcePawn;

typedef void (*GIVEENGINE)(ISourcePawnEngine *);
typedef IVirtualMachine *(*GETEXPORT)(unsigned int);

cell_t TestNative(sp_context_t *ctx, cell_t *params)
{
	IPluginContext *pl = ctx->context;
	cell_t *phys;
	int err;
	printf("params[0] = %d\n", params[0]);
	printf("params[1] = %d\n", params[1]);
	if ((err=pl->LocalToPhysAddr(params[2], &phys)) != SP_ERROR_NONE)
	{
		ctx->err = err;
		return 0;
	}
	printf("params[2] %c%c%c%c%c\n", phys[0], phys[1], phys[2], phys[3], phys[4]);
	if ((err=pl->LocalToPhysAddr(params[3], &phys)) != SP_ERROR_NONE)
	{
		ctx->err = err;
		return 0;
	}
	printf("params[3] = %d\n", *phys);
	*phys = 95;
	return 5;
}

int main()
{
	SourcePawnEngine engine;
	IVirtualMachine *vm;
	ICompilation *co;
	sp_plugin_t *plugin = NULL;
	sp_context_t *ctx = NULL;
	cell_t result;
	int err;

	/* Note: for now this is hardcoded for testing purposes!
	 * The ..\ should be one above msvc8.
	 */

	FILE *fp = fopen("..\\test.smx", "rb");
	if (!fp)
	{
		return 0;
	}
	plugin = engine.LoadFromFilePointer(fp, &err);
	if (err != SP_ERROR_NONE)
	{
		printf("Error loading: %d", err);
		return 0;
	}
	HMODULE dll = LoadLibrary("..\\jit\\x86\\msvc8\\Debug\\jit-x86.dll");
	if (dll == NULL)
	{
		printf("Error loading DLL: %d\n", GetLastError());
		return 0;
	}
	GIVEENGINE give_eng = (GIVEENGINE)GetProcAddress(dll, "GiveEnginePointer");
	if (!give_eng)
	{
		printf("Error getting \"GiveEnginePointer!\"!\n");
		return 0;
	}
	give_eng(&engine);
	GETEXPORT getex = (GETEXPORT)GetProcAddress(dll, "GetExport");
	if ((vm=getex(0)) == NULL)
	{
		printf("GetExport() returned no VM!\n");
		return 0;
	}

	co = vm->StartCompilation(plugin);
	if ((ctx = vm->CompileToContext(co, &err)) == NULL)
	{
		printf("CompileToContext() failed: %d", err);
		return 0;
	}

	IPluginContext *base = engine.CreateBaseContext(ctx);

	sp_nativeinfo_t mynative;
	mynative.name = "gaben";
	mynative.func = TestNative;

	if ((err=base->BindNative(&mynative, SP_NATIVE_OKAY)) != SP_ERROR_NONE)
	{
		printf("BindNative() failed: %d", err);
		return 0;
	}

	base->PushCell(1);
	base->PushCell(4);
	err = base->Execute(0, &result);
	printf("Result: %d Error: %d\n", result, err);

	engine.FreeBaseContext(base);

	fclose(fp);
	FreeLibrary(dll);
}
