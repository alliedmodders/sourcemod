#include <stdio.h>
#include <sp_vm_api.h>
#include "sp_vm_engine.h"
#include "sp_vm_basecontext.h"
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

using namespace SourcePawn;

typedef void (*GIVEENGINE)(ISourcePawnEngine *);
typedef IVirtualMachine *(*GETEXPORT)(unsigned int);

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
	if (err != SP_ERR_NONE)
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
	ctx = vm->CompileToContext(co, &err);

	IPluginContext *base = engine.CreateBaseContext(ctx);

	base->PushCell(1);
	base->PushCell(5);
	err = base->Execute(0, &result);
	printf("Result: %d Error: %d\n", result, err);

	engine.FreeBaseContext(base);

	fclose(fp);
	FreeLibrary(dll);
}
