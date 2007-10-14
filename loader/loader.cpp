/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#include <ISmmPluginExt.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#if defined _MSC_VER
	#define DLL_EXPORT				extern "C" __declspec(dllexport)
	#define openlib(lib)			LoadLibrary(lib)
	#define closelib(lib)			FreeLibrary(lib)
	#define findsym(lib, sym)		GetProcAddress(lib, sym)
	#define PLATFORM_EXT			".dll"
	#define vsnprintf				_vsnprintf
	#define PATH_SEP_CHAR			"\\"
	inline bool IsPathSepChar(char c) 
	{
		return (c == '/' || c == '\\');
	}
	#include <Windows.h>
#else
	#define DLL_EXPORT				extern "C" __attribute__((visibility("default")))
	#define openlib(lib)			dlopen(lib, RTLD_NOW)
	#define closelib(lib)			dlclose(lib)
	#define findsym(lib, sym)		dlsym(lib, sym)
	#define PLATFORM_EXT			".so"
	typedef void *					HINSTANCE;
	#define PATH_SEP_CHAR			"/"
	inline bool IsPathSepChar(char c) 
	{
		return (c == '/');
	}
	#include <dlfcn.h>
#endif

#define METAMOD_API_MAJOR			2
#define FILENAME_1_4_EP1			"sourcemod.1.ep1" PLATFORM_EXT
#define FILENAME_1_6_EP2			"sourcemod.2.ep2" PLATFORM_EXT
#define FILENAME_1_6_EP1			"sourcemod.2.ep1" PLATFORM_EXT

HINSTANCE g_hCore = NULL;

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	size_t len = vsnprintf(buffer, maxlength, fmt, ap);
	va_end(ap);

	if (len >= maxlength)
	{
		len = maxlength - 1;
		buffer[len] = '\0';
	}

	return len;
}

METAMOD_PLUGIN *_GetPluginPtr(const char *path)
{
	METAMOD_FN_ORIG_LOAD fn;
	METAMOD_PLUGIN *pl;
	int ret;

	if (!(g_hCore=openlib(path)))
	{
		return NULL;
	}

	if (!(fn=(METAMOD_FN_ORIG_LOAD)findsym(g_hCore, "CreateInterface")))
	{
		goto error;
	}

	pl = (METAMOD_PLUGIN *)fn(METAMOD_PLAPI_NAME, &ret);
	if (!pl)
	{
		goto error;
	}

	return pl;
error:
	closelib(g_hCore);
	g_hCore = NULL;
	return NULL;
}

bool GetFileOfAddress(void *pAddr, char *buffer, size_t maxlength)
{
#if defined _MSC_VER
	MEMORY_BASIC_INFORMATION mem;
	if (!VirtualQuery(pAddr, &mem, sizeof(mem)))
		return false;
	if (mem.AllocationBase == NULL)
		return false;
	HMODULE dll = (HMODULE)mem.AllocationBase;
	GetModuleFileName(dll, (LPTSTR)buffer, maxlength);
#else
	Dl_info info;
	if (!dladdr(pAddr, &info))
		return false;
	if (!info.dli_fbase || !info.dli_fname)
		return false;
	const char *dllpath = info.dli_fname;
	snprintf(buffer, maxlength, "%s", dllpath);
#endif
	return true;
}

DLL_EXPORT METAMOD_PLUGIN *CreateInterface_MMS(const MetamodVersionInfo *mvi, const MetamodLoaderInfo *mli)
{
	char *filename;
	
	if (mvi->api_major > METAMOD_API_MAJOR)
	{
		return NULL;
	}

	switch (mvi->source_engine)
	{
	case SOURCE_ENGINE_ORANGEBOX:
		{
			filename = FILENAME_1_6_EP2;
			break;
		}
	case SOURCE_ENGINE_EPISODEONE:
	case SOURCE_ENGINE_ORIGINAL:
		{
			filename = FILENAME_1_6_EP1;
			break;
		}
	default:
		{
			return NULL;
		}
	}

	char abspath[256];
	UTIL_Format(abspath, sizeof(abspath), "%s" PATH_SEP_CHAR "%s", mli->pl_path, filename);

	return _GetPluginPtr(abspath);
}

DLL_EXPORT void UnloadInterface_MMS()
{
	if (g_hCore)
	{
		closelib(g_hCore);
		g_hCore = NULL;
	}
}

DLL_EXPORT void *CreateInterface(const char *iface, int *ret)
{
	if (strcmp(iface, METAMOD_PLAPI_NAME))
	{
		char thisfile[256];
		char targetfile[256];

		if (!GetFileOfAddress((void *)CreateInterface_MMS, thisfile, sizeof(thisfile)))
		{
			return NULL;
		}

		size_t len = strlen(thisfile);
		for (size_t iter=len-1; iter>=0 && iter<len; iter--)
		{
			if (IsPathSepChar(thisfile[iter]))
			{
				thisfile[iter] = '\0';
				break;
			}
		}

		UTIL_Format(targetfile, sizeof(targetfile), "%s" PATH_SEP_CHAR FILENAME_1_4_EP1, thisfile);

		return _GetPluginPtr(targetfile);
	}

	return NULL;
}

#if defined _MSC_VER
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_DETACH)
	{
		UnloadInterface_MMS();
	}
	return TRUE;
}
#else
__attribute__((destructor)) static void gcc_fini()
{
	UnloadInterface_MMS();
}
#endif
