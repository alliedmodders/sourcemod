/**
 * vim: set ts=4 sw=4 tw=99 noet :
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
 */

#include <ISmmPluginExt.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
	#if defined __linux__
	#define PLATFORM_EXT			".so"
	#elif defined __APPLE__
	#define PLATFORM_EXT			".dylib"
	#endif
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
#define FILENAME_1_6_L4D			"sourcemod.2.l4d" PLATFORM_EXT
#define FILENAME_1_6_DARKM			"sourcemod.2.darkm" PLATFORM_EXT
#define FILENAME_1_6_L4D2			"sourcemod.2.l4d2" PLATFORM_EXT
#define FILENAME_1_6_SWARM			"sourcemod.2.swarm" PLATFORM_EXT
#define FILENAME_1_6_BGT			"sourcemod.2.bgt" PLATFORM_EXT
#define FILENAME_1_6_EYE			"sourcemod.2.eye" PLATFORM_EXT
#define FILENAME_1_6_PORTAL2		"sourcemod.2.portal2" PLATFORM_EXT
#define FILENAME_1_6_CSGO			"sourcemod.2.csgo" PLATFORM_EXT
#define FILENAME_1_6_CSS			"sourcemod.2.css" PLATFORM_EXT
#define FILENAME_1_6_HL2DM			"sourcemod.2.hl2dm" PLATFORM_EXT
#define FILENAME_1_6_DODS			"sourcemod.2.dods" PLATFORM_EXT
#define FILENAME_1_6_SDK2013		"sourcemod.2.sdk2013" PLATFORM_EXT
#define FILENAME_1_6_TF2			"sourcemod.2.tf2" PLATFORM_EXT
#define FILENAME_1_6_ND				"sourcemod.2.nd" PLATFORM_EXT
#define FILENAME_1_6_BLADE			"sourcemod.2.blade" PLATFORM_EXT
#define FILENAME_1_6_INSURGENCY		"sourcemod.2.insurgency" PLATFORM_EXT
#define FILENAME_1_6_CONTAGION		"sourcemod.2.contagion" PLATFORM_EXT
#define FILENAME_1_6_BMS			"sourcemod.2.bms" PLATFORM_EXT

HINSTANCE g_hCore = NULL;
bool load_attempted = false;

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...);

class FailPlugin : public SourceMM::ISmmFailPlugin
{
public:
	int GetApiVersion()
	{
		return fail_version;
	}

	bool Load(SourceMM::PluginId id, SourceMM::ISmmAPI *ismm, char *error, size_t maxlength, bool late)
	{
		if (error != NULL && maxlength != 0)
		{
			UTIL_Format(error, maxlength, "%s", error_buffer);
		}
		return false;
	}

	int fail_version;
	char error_buffer[512];
} s_FailPlugin;

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

METAMOD_PLUGIN *_GetPluginPtr(const char *path, int fail_api)
{
	METAMOD_FN_ORIG_LOAD fn;
	METAMOD_PLUGIN *pl;
	int ret;

	if (!(g_hCore=openlib(path)))
	{
#if defined __linux__ || defined __APPLE__
		UTIL_Format(s_FailPlugin.error_buffer, 
			sizeof(s_FailPlugin.error_buffer),
			"%s",
			dlerror());
#else
		DWORD err = GetLastError();

		if (FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				err,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				s_FailPlugin.error_buffer,
				sizeof(s_FailPlugin.error_buffer),
				NULL)
			== 0)
		{
			UTIL_Format(s_FailPlugin.error_buffer, 
				sizeof(s_FailPlugin.error_buffer),
				"unknown error %x",
				err);
		}
#endif

		s_FailPlugin.fail_version = fail_api;

		return (METAMOD_PLUGIN *)&s_FailPlugin;
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
	const char *filename;

	load_attempted = true;
	
	if (mvi->api_major > METAMOD_API_MAJOR)
	{
		return NULL;
	}

	switch (mvi->source_engine)
	{
	case SOURCE_ENGINE_ORIGINAL:
	case SOURCE_ENGINE_EPISODEONE:
		{
			filename = FILENAME_1_6_EP1;
			break;
		}
	case SOURCE_ENGINE_ORANGEBOX:
		{
			filename = FILENAME_1_6_EP2;
			break;
		}
	case SOURCE_ENGINE_LEFT4DEAD:
		{
			filename = FILENAME_1_6_L4D;
			break;
		}
	case SOURCE_ENGINE_DARKMESSIAH:
		{
			filename = FILENAME_1_6_DARKM;
			break;
		}
	case SOURCE_ENGINE_LEFT4DEAD2:
		{
			const char *gamedir = mvi->GetGameDir();
			if (strcmp(gamedir, "nucleardawn") == 0)
			{
				filename = FILENAME_1_6_ND;
			}
			else
			{
				filename = FILENAME_1_6_L4D2;
			}
			break;
		}
	case SOURCE_ENGINE_NUCLEARDAWN:
		{
			filename = FILENAME_1_6_ND;
			break;
		}
	case SOURCE_ENGINE_CONTAGION:
		{
			filename = FILENAME_1_6_CONTAGION;
			break;
		}
	case SOURCE_ENGINE_ALIENSWARM:
		{
			filename = FILENAME_1_6_SWARM;
			break;
		}
	case SOURCE_ENGINE_BLOODYGOODTIME:
		{
			filename = FILENAME_1_6_BGT;
			break;
		}
	case SOURCE_ENGINE_EYE:
		{
			filename = FILENAME_1_6_EYE;
			break;
		}
	case SOURCE_ENGINE_PORTAL2:
		{
			filename = FILENAME_1_6_PORTAL2;
			break;
		}
	case SOURCE_ENGINE_CSGO:
		{
			filename = FILENAME_1_6_CSGO;
			break;
		}
	case SOURCE_ENGINE_CSS:
		{
			filename = FILENAME_1_6_CSS;
			break;
		}
	case SOURCE_ENGINE_HL2DM:
		{
			filename = FILENAME_1_6_HL2DM;
			break;
		}
	case SOURCE_ENGINE_DODS:
		{
			filename = FILENAME_1_6_DODS;
			break;
		}
	case SOURCE_ENGINE_SDK2013:
		{
			filename = FILENAME_1_6_SDK2013;
			break;
		}
	case SOURCE_ENGINE_BMS:
		{
			filename = FILENAME_1_6_BMS;
			break;
		}
	case SOURCE_ENGINE_TF2:
		{
			filename = FILENAME_1_6_TF2;
			break;
		}
	case SOURCE_ENGINE_ORANGEBOXVALVE_DEPRECATED:
		{
			const char *gamedir = mvi->GetGameDir();
			if (strcmp(gamedir, "tf") == 0)
			{
				filename = FILENAME_1_6_TF2;
			}
			else if (strcmp(gamedir, "dod") == 0)
			{
				filename = FILENAME_1_6_DODS;
			}
			else if (strcmp(gamedir, "hl2mp") == 0)
			{
				filename = FILENAME_1_6_HL2DM;
			}
			else
			{
				return NULL;
			}
			break;
		}
	case SOURCE_ENGINE_BLADE:
		{
			filename = FILENAME_1_6_BLADE;
			break;
		}
	case SOURCE_ENGINE_INSURGENCY:
		{
			filename = FILENAME_1_6_INSURGENCY;
			break;
		}
	default:
		{
			return NULL;
		}
	}

	char abspath[256];
	UTIL_Format(abspath, sizeof(abspath), "%s" PATH_SEP_CHAR "%s", mli->pl_path, filename);

	return _GetPluginPtr(abspath, METAMOD_FAIL_API_V2);
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
	/**
	 * If a load has already been attempted, bail out immediately.
	 */
	if (load_attempted)
	{
		return NULL;
	}

	if (strcmp(iface, METAMOD_PLAPI_NAME) == 0)
	{
		char thisfile[256];
		char targetfile[256];

		if (!GetFileOfAddress((void *)CreateInterface_MMS, thisfile, sizeof(thisfile)))
		{
			return NULL;
		}

		size_t len = strlen(thisfile);
		for (size_t iter=len-1; iter<len; iter--)
		{
			if (IsPathSepChar(thisfile[iter]))
			{
				thisfile[iter] = '\0';
				break;
			}
		}

		UTIL_Format(targetfile, sizeof(targetfile), "%s" PATH_SEP_CHAR FILENAME_1_4_EP1, thisfile);

		return _GetPluginPtr(targetfile, METAMOD_FAIL_API_V1);
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

/* Overload a few things to prevent libstdc++ linking */
#if defined __linux__ || defined __APPLE__
extern "C" void __cxa_pure_virtual(void)
{
}

void *operator new(size_t size)
{
	return malloc(size);
}

void *operator new[](size_t size)
{
	return malloc(size);
}

void operator delete(void *ptr)
{
	free(ptr);
}

void operator delete[](void * ptr)
{
	free(ptr);
}
#endif


