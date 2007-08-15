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

#include "MemoryUtils.h"
#include "ShareSys.h"
#ifdef PLATFORM_LINUX
#include <string.h>
#include <elf.h>
#endif

MemoryUtils g_MemUtils;

#if 0
MemoryUtils::MemoryUtils()
{
#ifdef PLATFORM_WINDOWS

	SYSTEM_INFO info;
	GetSystemInfo(&info);

	m_PageSize = info.dwPageSize;

#elif defined PLATFORM_POSIX

	m_PageSize = sysconf(_SC_PAGE_SIZE);

#endif
}
#endif

void MemoryUtils::OnSourceModAllInitialized()
{
	g_ShareSys.AddInterface(NULL, this);
}

void *MemoryUtils::FindPattern(const void *libPtr, const char *pattern, size_t len)
{
	DynLibInfo lib;
	bool found;
	char *ptr, *end;

	memset(&lib, 0, sizeof(DynLibInfo));

	if (!GetLibraryInfo(libPtr, lib))
	{
		return NULL;
	}

	ptr = reinterpret_cast<char *>(lib.baseAddress);
	end = ptr + lib.memorySize;

	while (ptr < end)
	{
		found = true;
		for (register size_t i = 0; i < len; i++)
		{
			if (pattern[i] != '\x2A' && pattern[i] != ptr[i])
			{
				found = false;
				break;
			}
		}

		if (found)
			return ptr;

		ptr++;
	}

	return NULL;
}

bool MemoryUtils::GetLibraryInfo(const void *libPtr, DynLibInfo &lib)
{
	unsigned long baseAddr;

	if (libPtr == NULL)
	{
		return false;
	}

#ifdef PLATFORM_WINDOWS

	MEMORY_BASIC_INFORMATION info;
	IMAGE_DOS_HEADER *dos;
	IMAGE_NT_HEADERS *pe;
	IMAGE_FILE_HEADER *file;
	IMAGE_OPTIONAL_HEADER *opt;

	if (!VirtualQuery(libPtr, &info, sizeof(MEMORY_BASIC_INFORMATION)))
	{
		return false;
	}

	baseAddr = reinterpret_cast<unsigned long>(info.AllocationBase);

	/* All this is for our insane sanity checks :o */
	dos = reinterpret_cast<IMAGE_DOS_HEADER *>(baseAddr);
	pe = reinterpret_cast<IMAGE_NT_HEADERS *>(baseAddr + dos->e_lfanew);
	file = &pe->FileHeader;
	opt = &pe->OptionalHeader;

	/* Check PE magic and signature */
	if (dos->e_magic != IMAGE_DOS_SIGNATURE || pe->Signature != IMAGE_NT_SIGNATURE || opt->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		return false;
	}

	/* Check architecture, which is 32-bit/x86 right now
	 * Should change this for 64-bit if Valve gets their act together
	 */
	if (file->Machine != IMAGE_FILE_MACHINE_I386)
	{
		return false;
	}

	/* For our purposes, this must be a dynamic library */
	if ((file->Characteristics & IMAGE_FILE_DLL) == 0)
	{
		return false;
	}

	/* Finally, we can do this */
	lib.memorySize = opt->SizeOfImage;

#elif defined PLATFORM_LINUX

	Dl_info info;
	Elf32_Ehdr *file;
	Elf32_Phdr *phdr;
	uint16_t phdrCount;

	if (!dladdr(libPtr, &info))
	{
		return false;
	}

	if (!info.dli_fbase || !info.dli_fname)
	{
		return false;
	}

	/* This is for our insane sanity checks :o */
	baseAddr = reinterpret_cast<unsigned long>(info.dli_fbase);
	file = reinterpret_cast<Elf32_Ehdr *>(baseAddr);

	/* Check ELF magic */
	if (memcmp(ELFMAG, file->e_ident, SELFMAG) != 0)
	{
		return false;
	}

	/* Check ELF version */
	if (file->e_ident[EI_VERSION] != EV_CURRENT)
	{
		return false;
	}

	/* Check ELF architecture, which is 32-bit/x86 right now
	 * Should change this for 64-bit if Valve gets their act together
	 */
	if (file->e_ident[EI_CLASS] != ELFCLASS32 || file->e_machine != EM_386 || file->e_ident[EI_DATA] != ELFDATA2LSB)
	{
		return false;
	}

	/* For our purposes, this must be a dynamic library/shared object */
	if (file->e_type != ET_DYN)
	{
		return false;
	}

	phdrCount = file->e_phnum;
	phdr = reinterpret_cast<Elf32_Phdr *>(baseAddr + file->e_phoff);
	
	/* Add up the memory sizes of segments marked as PT_LOAD as those are the only ones that should be in memory */
	for (uint16_t i = 0; i < phdrCount; i++)
	{
		Elf32_Phdr &hdr = phdr[i];

		if (hdr.p_type == PT_LOAD)
		{
			lib.memorySize += hdr.p_memsz;
		}
	}

#endif

	lib.baseAddress = reinterpret_cast<void *>(baseAddr);

	return true;
}
