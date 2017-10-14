/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2017 AlliedModders LLC.  All rights reserved.
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

#include "PseudoAddrManager.h"
#ifdef PLATFORM_APPLE
#include <mach/mach.h>
#include <mach/vm_region.h>
#endif
#ifdef PLATFORM_LINUX
#include <inttypes.h>
#endif

PseudoAddressManager::PseudoAddressManager() : m_NumEntries(0)
{
}

// A pseudo address consists of a table index in the upper 6 bits and an offset in the
// lower 26 bits. The table consists of memory allocation base addresses.
void *PseudoAddressManager::FromPseudoAddress(uint32_t paddr)
{
#ifdef PLATFORM_X64
	uint8_t index = paddr >> PSEUDO_OFFSET_BITS;
	uint32_t offset = paddr & ((1 << PSEUDO_OFFSET_BITS) - 1);

	if (index >= m_NumEntries)
		return nullptr;

	return reinterpret_cast<void *>(uintptr_t(m_AllocBases[index]) + offset);
#else
	return nullptr;
#endif
}

uint32_t PseudoAddressManager::ToPseudoAddress(void *addr)
{
#ifdef PLATFORM_X64
	uint8_t index = 0;
	uint32_t offset = 0;
	bool hasEntry = false;
	void *base = GetAllocationBase(addr);

	if (base) {
		for (int i = 0; i < m_NumEntries; i++) {
			if (m_AllocBases[i] == base) {
				index = i;
				hasEntry = true;
				break;
			}
		}
	} else {
		return 0;
	}

	if (!hasEntry) {
		index = m_NumEntries;
		if (m_NumEntries < SM_ARRAYSIZE(m_AllocBases))
			m_AllocBases[m_NumEntries++] = base;
		else
			return 0;	// Table is full
	}

	ptrdiff_t diff = uintptr_t(addr) - uintptr_t(base);

	// Ensure difference fits in 26 bits
	if (diff > (UINT32_MAX >> PSEUDO_INDEX_BITS))
		return 0;

	return (index << PSEUDO_OFFSET_BITS) | diff;
#else
	return 0;
#endif
}

void *PseudoAddressManager::GetAllocationBase(void *ptr)
{
#if defined PLATFORM_WINDOWS

	MEMORY_BASIC_INFORMATION info;
	if (!VirtualQuery(ptr, &info, sizeof(MEMORY_BASIC_INFORMATION)))
		return nullptr;
	return info.AllocationBase;

#elif defined PLATFORM_APPLE

#ifdef PLATFORM_X86
	typedef vm_region_info_t mach_vm_region_info_t;
	typedef vm_region_basic_info_data_t mach_vm_region_basic_info_data_t;
	const vm_region_flavor_t MACH_VM_REGION_BASIC_INFO = VM_REGION_BASIC_INFO;
	const mach_msg_type_number_t MACH_VM_REGION_BASIC_INFO_COUNT = VM_REGION_BASIC_INFO_COUNT;
	#define mach_vm_region vm_region
#elif defined PLATFORM_X64
	typedef vm_region_info_64_t mach_vm_region_info_t ;
	typedef vm_region_basic_info_data_64_t mach_vm_region_basic_info_data_t;
	const vm_region_flavor_t MACH_VM_REGION_BASIC_INFO = VM_REGION_BASIC_INFO_64;
	const mach_msg_type_number_t MACH_VM_REGION_BASIC_INFO_COUNT = VM_REGION_BASIC_INFO_COUNT_64;
	#define mach_vm_region vm_region_64
#endif
	vm_size_t size;
	vm_address_t vmaddr = reinterpret_cast<vm_address_t>(ptr);
	mach_vm_region_basic_info_data_t info;
	memory_object_name_t obj;
	vm_region_flavor_t flavor = MACH_VM_REGION_BASIC_INFO;
	mach_msg_type_number_t count = MACH_VM_REGION_BASIC_INFO_COUNT;
	
	kern_return_t kr = mach_vm_region(mach_task_self(), &vmaddr, &size, flavor,
	                                  reinterpret_cast<mach_vm_region_info_t>(&info), 
	                                  &count, &obj);

	if (kr != KERN_SUCCESS)
		return nullptr;

	return reinterpret_cast<void *>(vmaddr);

#elif defined PLATFORM_LINUX

	uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

	// Format:
	// lower    upper    prot     stuff                 path
	// 08048000-0804c000 r-xp 00000000 03:03 1010107    /bin/cat
	FILE *fp = fopen("/proc/self/maps", "r");
	if (fp) {
		uintptr_t lower, upper;
		while (fscanf(fp, "%" PRIxPTR "-%" PRIxPTR, &lower, &upper) != EOF) {
			if (addr >= lower && addr <= upper) {
				fclose(fp);
				return reinterpret_cast<void *>(lower);
			}

			// Read to end of line
			int c;
			while ((c = fgetc(fp)) != '\n') {
				if (c == EOF)
					break;
			}
			if (c == EOF)
				break;
		}
		fclose(fp);
	}
	return nullptr;
#endif
}
