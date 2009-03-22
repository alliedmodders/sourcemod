#include <stdio.h>
#include <dlfcn.h>
#include "string.h"
#include "malloc.h"

extern "C" char * cplus_demangle(const char *, int);
#define DMGL_PARAMS	 (1 << 0)

struct VOffset
{
	int linux_offset;
	int windows_offset;
};

static bool FindVFunc(void *handle, void **vtable, const char* vtable_end, const char *class_function, const char *params, VOffset *offsets);

int main(int argc, char **argv)
{
	void *handle;
	void **pVtable;

	VOffset offsets;
	char *params = NULL;
	bool ret;

	if (argc < 4)
	{
		fprintf(stderr, "Usage: <binary> <vtable start symbol> <function> <optional: params> <optional: vtable end symbol>\n");
		return -1;
	}

	handle = dlopen(argv[1], RTLD_NOW);

	if (handle == NULL)
	{
		fprintf(stderr, "Failed to open server image, error [%s]\n", dlerror());
		return -1;
	}

	pVtable = (void **)dlsym(handle, argv[2]);

	if (pVtable == NULL)
	{
		char startSym[128];
		snprintf(startSym, sizeof(startSym), "_ZTV%d%s", strlen(argv[2]), argv[2]);
		pVtable = (void **)dlsym(handle, startSym);
	}

	if (pVtable == NULL)
	{
		fprintf(stderr, "Invalid vtable symbol \"%s\"\n", argv[2]);
		dlclose(handle);
		return -1;
	}

	if (argc >= 5)
	{
		params = argv[4];

		if (params[0] == '"')
		{
			params++;
		}

		int len = strlen(params)-1;

		if (params[len] == '"')
		{
			params[len] = '\0';
		}

		if (params[0] == 0)
		{
			params = NULL;
		}
	}

	if (argc == 6)
	{
		ret = FindVFunc(handle, pVtable, argv[5], argv[3], params, &offsets);
	}
	else
	{
		ret = FindVFunc(handle, pVtable, "_ZTI", argv[3], params, &offsets);
	}

	if (ret)
	{
		if (params != NULL)
		{
			printf("%s%s - Win: %i  Linux : %i\n", argv[3], params, offsets.windows_offset, offsets.linux_offset);
		}
		else
		{
			printf("%s - Win: %i  Linux : %i\n", argv[3], offsets.windows_offset, offsets.linux_offset);
		}
		
		dlclose(handle);
		return 0;
	}

	fprintf(stderr, "Failed to find function!");
	dlclose(handle);
	return -1;
}

static bool FindVFunc(void *handle, void **vtable, const char* vtable_end, const char *class_function, const char *params, VOffset *offsets)
{
	Dl_info d;

	int linux_offset = -1;
	int windows_offset = -1;

	int overloads = 0;
	int location = 0;
	int start_offset = -1;

	for (int i=0; i< 1000; i++)
	{
		void *FuncPtr = vtable[i];

		int status = dladdr(FuncPtr, &d);

		if (!status)
		{
			continue;
		}

		if (i > 1 && strncmp(d.dli_sname, vtable_end, strlen(vtable_end)) == 0)
		{
			break;
		}

		char *name = cplus_demangle(d.dli_sname, DMGL_PARAMS);

		if (name == NULL)
		{
			printf("Demangling failed\n");
			continue;
		}

		char *foundfunction = strpbrk(name, ":");

		if (foundfunction == NULL)
		{
			//printf("Couldnt split function\n");
			free(name);
			continue;
		}

		//Skip both ':' chars
		foundfunction += 2;


		if (strncmp(foundfunction, class_function, strlen(class_function)) == 0 && foundfunction[strlen(class_function)] == '(')
		{
			//printf("Symbol: %s Demangled: %s Offset: %i\n", d.dli_sname, name, i);

			//We have a pointer to a function, but it may be overloaded.
			overloads++;

			if (start_offset == -1)
			{
				start_offset = i-1;
			}

			char *foundparams = strpbrk(foundfunction, "(");

			if (foundparams == NULL)
			{
				printf("ARGH\n");
				free(name);
				continue;
			}

			if(params == NULL || strcmp(foundparams, params) == 0)
			{
				//This is actually our function - So this is the linux offset
				linux_offset = i;
				location = overloads;
			}
		}

		free(name);
	}

	windows_offset = (overloads-location) + start_offset;

	offsets->linux_offset = linux_offset - 2;
	offsets->windows_offset = windows_offset - 2;

	return true;
}
