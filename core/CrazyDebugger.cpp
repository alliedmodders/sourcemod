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

#include "sm_globals.h"
#include "sourcemm_api.h"
#include "Tlhelp32.h"
#include "LibrarySys.h"
#include "minidump.h"
#include "sm_stringutil.h"

bool HookImportAddrTable(BYTE *base, const char *func, DWORD hookfunc, char *err, size_t maxlength)
{
	IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
	if (dos->e_magic != IMAGE_DOS_SIGNATURE)
	{
		UTIL_Format(err, maxlength, "%s", "Could not detect valid DOS signature");
		return false;
	}

	IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE)
	{
		UTIL_Format(err, maxlength, "%s", "Could not detect valid NT signature");
		return false;
	}

	IMAGE_IMPORT_DESCRIPTOR *desc = 
		(IMAGE_IMPORT_DESCRIPTOR *)
		(base + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	if (base == (BYTE *)desc)
	{
		UTIL_Format(err, maxlength, "%s", "Could not find IAT");
		return false;
	}

	bool entryFound = false;
	while (desc->Name)
	{
		if (desc->FirstThunk != 0)
		{
			IMAGE_THUNK_DATA *data = (IMAGE_THUNK_DATA *)(base + desc->OriginalFirstThunk);
			DWORD *iat = (DWORD *)(base + desc->FirstThunk);
			while (data->u1.Function)
			{
				if ((data->u1.Ordinal & IMAGE_ORDINAL_FLAG32) != IMAGE_ORDINAL_FLAG32)
				{
					IMAGE_IMPORT_BY_NAME *import = (IMAGE_IMPORT_BY_NAME *)(base + data->u1.AddressOfData);
					if (strcmp((char *)import->Name, func) == 0)
					{
						DWORD oldprot, oldprot2;
						VirtualProtect(iat, 4, PAGE_READWRITE, &oldprot);
						*iat = hookfunc;
						VirtualProtect(iat, 4, oldprot, &oldprot2);
						entryFound = true;
					}
				}
				data++;
				iat++;
			}
		}
		desc++;
	}

	if (!entryFound)
	{
		UTIL_Format(err, maxlength, "Could not find IAT entry for %s", func);
		return false;
	}

	return true;
}

BOOL
WINAPI
MiniDumpWriteDump2(
				  IN HANDLE hProcess,
				  IN DWORD ProcessId,
				  IN HANDLE hFile,
				  IN MINIDUMP_TYPE DumpType,
				  IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
				  IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
				  IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL
				  )
{
	DumpType = (MINIDUMP_TYPE)((int)DumpType|MiniDumpWithFullMemory|MiniDumpWithHandleData);
	return MiniDumpWriteDump(hProcess, ProcessId, hFile, DumpType, ExceptionParam, UserStreamParam, CallbackParam);
}

FARPROC WINAPI GetProcAddress2(HMODULE hModule, LPCSTR lpProcName)
{
	if (strcmp(lpProcName, "MiniDumpWriteDump") == 0)
	{
		return (FARPROC)MiniDumpWriteDump2;
	}

	return GetProcAddress(hModule, lpProcName);
}

HMODULE WINAPI LoadLibraryEx2(LPCTSTR lpFileName, HANDLE hFile, DWORD dwFlags)
{
	HMODULE lib = LoadLibraryEx(lpFileName, hFile, dwFlags);

	/* Steam.dll seems to get loaded using an abs path, thus the use of stristr() */
	if (stristr(lpFileName, "steam.dll"))
	{
		char err[64];
		if (!HookImportAddrTable((BYTE *)lib, "GetProcAddress", (DWORD)GetProcAddress2, err, sizeof(err)))
		{
			Error("[SM] %s in steam.dll\n", err);
			return NULL;
		}
	}

	return lib;
}

class CrazyWindowsDebugger : public SMGlobalClass
{
public:
	void OnSourceModAllInitialized()
	{
		HANDLE hModuleList = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
		MODULEENTRY32 me32;
		BYTE *engine = NULL;
		BYTE *steam = NULL;
		char err[64];

		me32.dwSize = sizeof(MODULEENTRY32);

		if (!Module32First(hModuleList, &me32))
		{
			Error("Could not initialize crazy debugger!");
		}

		do
		{
			if (strcasecmp(me32.szModule, "engine.dll") == 0)
			{
				engine = me32.modBaseAddr;
			}
			else if (strcasecmp(me32.szModule, "steam.dll") == 0)
			{
				steam = me32.modBaseAddr;
			}
		} while  (Module32Next(hModuleList, &me32));

		CloseHandle(hModuleList);

		/* This really should not happen, but ... */
		if (!engine)
		{
			Error("[SM] Could not find engine.dll\n");
		}

		/* Steam.dll isn't loaded yet */
		if (!steam)
		{
			if (!HookImportAddrTable(engine, "LoadLibraryExA", (DWORD)LoadLibraryEx2, err, sizeof(err)))
			{
				Error("[SM] %s in engine.dll)\n", err);
			}
		}
		else
		{
			if (!HookImportAddrTable(steam, "GetProcAddress", (DWORD)GetProcAddress2, err, sizeof(err)))
			{
				Error("[SM] %s in steam.dll)\n", err);
			}
		}
	}
} s_CrazyDebugger;
