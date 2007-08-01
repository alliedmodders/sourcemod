/**
 * vim: set ts=4 :
 * ================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
 * ================================================================
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License, 
 * version 3.0, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to 
 * link the code of this program (as well as its derivative works) to 
 * "Half-Life 2," the "Source Engine," the "SourcePawn JIT," and any 
 * Game MODs that run on software by the Valve Corporation.  You must 
 * obey the GNU General Public License in all respects for all other 
 * code used. Additionally, AlliedModders LLC grants this exception 
 * to all derivative works. AlliedModders LLC defines further 
 * exceptions, found in LICENSE.txt (as of this writing, version 
 * JULY-31-2007), or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#if defined CRAZY_DEBUG
#include "sm_globals.h"
#include "sourcemm_api.h"
#include "Tlhelp32.h"
#include "LibrarySys.h"
#include "minidump.h"

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

class CrazyWindowsDebugger : public SMGlobalClass
{
public:
	void OnSourceModAllInitialized()
	{
		HANDLE hModuleList = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
		MODULEENTRY32 me32;

		me32.dwSize = sizeof(MODULEENTRY32);

		if (!Module32First(hModuleList, &me32))
		{
			Error("Could not initialize crazy debugger!");
		}

		bool found = false;

		do
		{
			if (strcasecmp(me32.szModule, "steam.dll") == 0)
			{
				IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)me32.modBaseAddr;
				if (dos->e_magic != IMAGE_DOS_SIGNATURE)
				{
					Error("[SM] Could not detect steam.dll with valid DOS signature");
				}
				char *base = (char *)dos;
				IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
				if (nt->Signature != IMAGE_NT_SIGNATURE)
				{
					Error("[SM] Could not detect steam.dll with valid NT signature");
				}
				IMAGE_IMPORT_DESCRIPTOR *desc = 
					(IMAGE_IMPORT_DESCRIPTOR *)
					(base + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
				if (base == (char *)desc)
				{
					Error("[SM] Could not find the steam.dll IAT");
				}
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
								if (strcmp((char *)import->Name, "GetProcAddress") == 0)
								{
									DWORD oldprot, oldprot2;
									VirtualProtect(iat, 4, PAGE_READWRITE, &oldprot);
									*iat = (DWORD)GetProcAddress2;
									VirtualProtect(iat, 4, oldprot, &oldprot2);
									found = true;
									goto _end;
								}
							}
							data++;
							iat++;
						}
					}
					desc++;
				}
				break;
			}
		} while  (Module32Next(hModuleList, &me32));

_end:

		if (!found)
		{
			Error("Could not find steam.dll's GetProcAddress IAT entry");
		}

		CloseHandle(hModuleList);
	}
} s_CrazyDebugger;
#endif
