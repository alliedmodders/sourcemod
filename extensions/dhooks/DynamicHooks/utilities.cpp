/**
* =============================================================================
* DynamicHooks
* Copyright (C) 2015 Robin Gohmert. All rights reserved.
* Copyright (C) 2018-2021 AlliedModders LLC.  All rights reserved.
* =============================================================================
*
* This software is provided 'as-is', without any express or implied warranty.
* In no event will the authors be held liable for any damages arising from 
* the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose, 
* including commercial applications, and to alter it and redistribute it 
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not 
* claim that you wrote the original software. If you use this software in a 
* product, an acknowledgment in the product documentation would be 
* appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
*
* asm.h/cpp from devmaster.net (thanks cybermind) edited by pRED* to handle gcc
* -fPIC thunks correctly
*
* Idea and trampoline code taken from DynDetours (thanks your-name-here).
*/

// ============================================================================
// >> INCLUDES
// ============================================================================
#ifdef _WIN32
	#include <windows.h>
#endif

#ifdef __linux__
	#include <sys/mman.h>
	#include <unistd.h>
	#include <stdio.h>
	#define PAGE_SIZE 4096
	#define ALIGN(ar) ((long)ar & ~(PAGE_SIZE-1))
	#define PAGE_EXECUTE_READWRITE PROT_READ|PROT_WRITE|PROT_EXEC
#endif

#include <asm/asm.h>


// ============================================================================
// >> ParseParams
// ============================================================================
void SetMemPatchable(void* pAddr, size_t size)
{
#if defined __linux__
	if (mprotect((void *) ALIGN(pAddr), sysconf(_SC_PAGESIZE), PAGE_EXECUTE_READWRITE) == -1)
		perror("mprotect");
#elif defined _WIN32
	DWORD old_prot;
	VirtualProtect(pAddr, size, PAGE_EXECUTE_READWRITE, &old_prot);
#endif
}


// ============================================================================
// >> WriteJMP
// ============================================================================
void WriteJMP(unsigned char* src, void* dest)
{
	SetMemPatchable(src, 20);
	inject_jmp((void *)src, dest);
}
