#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sp_vm_types.h>
#include <sp_vm_api.h>
#include "natives.h"

#if defined WIN32
#include <windows.h>
#else
#include <dlfcn.h>
typedef void* HMODULE;
#define LoadLibrary(x) dlopen(x, RTLD_NOW)
#define GetProcAddress(x, y) dlsym(x, y)
#define FreeLibrary(x) dlclose(x)
#endif

using namespace SourcePawn;

typedef void (*GETSOURCEPAWN)(ISourcePawnEngine **eng);

int main(int argc, char **argv)
{
	int err;
	FILE *fp;
	long size;
	void *data;
	cell_t res;
	bool bDebug;
	HMODULE pJitLib;
	const char *file;
	const char *jitlib;
	GETSOURCEPAWN jit_load;
	IPluginFunction *pMain;
	IPluginContext *pContext;
	ISourcePawnEngine *engine;

	file = NULL;
	bDebug = false;
	jitlib = "sourcepawn.jit.x86.dll";

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-debug") == 0)
		{
			bDebug = true;
		}
		else if (strcmp(argv[i], "-jitlib") == 0 && (i + 1 < argc))
		{
			jitlib = argv[++i];
		}
		else
		{
			file = argv[i];
		}
	}

	if (argc == 1 || file == NULL)
	{
		fprintf(stdout, "Usage: spshell [options] <file>\n");
		fprintf(stdout, " -debug                enables debug mode.\n");
		fprintf(stdout, " -jitlib               JIT library file path.\n");
		fprintf(stdout, "\n");
		
		return 0;
	}

	fp = fopen(file, "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "Could not open file for reading: %s\n", file);
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);

	data = malloc(size);
	fread(data, size, 1, fp);
	fclose(fp);

	pJitLib = LoadLibrary(jitlib);
	if (pJitLib == NULL)
	{
		fprintf(stderr, "Could not load JIT library: %s\n", jitlib);
#if defined __GNUC__
		fprintf(stderr, "Error: %s", dlerror());
#endif
		free(data);
		exit(1);
	}

	jit_load = (GETSOURCEPAWN)GetProcAddress(pJitLib, "GetSourcePawn");
	if (jit_load == NULL)
	{
		fprintf(stderr, "Could not find JIT function: \"GetSourcePawnEngine\"\n");
		FreeLibrary(pJitLib);
		free(data);
		exit(1);
	}

	jit_load(&engine);

	pContext = engine->LoadPluginFromMemory(data, &err);

	free(data);

	if (pContext == NULL)
	{
		fprintf(stderr, "Error loading plugin: (%d) %s\n", err, engine->GetErrorString(err));
		FreeLibrary(pJitLib);
		exit(1);
	}

	pMain = pContext->GetFunctionByName("main2");
	if (pMain == NULL)
	{
		fprintf(stderr, "Could not find a main function in plugin.");
		engine->DestroyContext(pContext);
		exit(1);
	}

	/* Check natives */
	{
		uint32_t i = 0;
		uint32_t idx;

		while (g_Natives[i].name != NULL)
		{
			if (pContext->FindNativeByName(g_Natives[i].name, &idx) == SP_ERROR_NONE)
			{
				pContext->BindNativeToIndex(idx, g_Natives[i].func);
			}
			i++;
		}
	}

	pMain->PushCell(1);
	pMain->PushCell(2);
	pMain->PushCell(3);
	err = pMain->Execute(&res);
	if (err != SP_ERROR_NONE)
	{
		fprintf(stderr, "Plugin returned error: (%d) %s\n", err, engine->GetErrorString(err));
	}
	else
	{
		fprintf(stdout, "Plugin returned value: %d\n", res);
	}

	getchar();

	engine->DestroyContext(pContext);
	FreeLibrary(pJitLib);

	return 0;
}
