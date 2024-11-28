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
#include <bridge/include/CoreProvider.h>
#ifdef PLATFORM_APPLE
#include <mach/mach.h>
#include <mach/vm_region.h>
#endif
#ifdef PLATFORM_LINUX
#include <inttypes.h>
#endif
#ifdef PLATFORM_WINDOWS
#include <Psapi.h>
#endif

PseudoAddressManager::PseudoAddressManager() : m_dictionary(am::IPlatform::GetDefault())
{
}

void PseudoAddressManager::Initialize() {
#ifdef PLATFORM_WINDOWS
	auto process = GetCurrentProcess();
	auto get_module_details = [process](const char* name, void*& baseAddress, size_t& moduleSize) {
		if (process == NULL) {
			return false;
		}
		auto hndl = GetModuleHandle(name);
		if (hndl == NULL) {
			return false;
		}
		MODULEINFO info;
		if (!GetModuleInformation(process, hndl, &info, sizeof(info))) {
			return false;
		}
		moduleSize = info.SizeOfImage;
		baseAddress = info.lpBaseOfDll;
		return true;
	};
#endif
	// Early map commonly used modules, it's okay if not all of them are here
	// Everything else will be caught by "ToPseudoAddress" but you risk running out of ranges by then
	const char* libs[] = { "engine", "server", "tier0", "vstdlib" };

	char formattedName[64];
	for (int i = 0; i < sizeof(libs) / sizeof(const char*); i++) {
		bridge->FormatSourceBinaryName(libs[i], formattedName, sizeof(formattedName));
		void* base_addr = nullptr;
		size_t module_size = 0;
		if (get_module_details(formattedName, base_addr, module_size)) {
			// Create the mapping (hopefully)
			m_dictionary.Make32bitAddress(base_addr, module_size);
		}
	}
}

void *PseudoAddressManager::FromPseudoAddress(uint32_t paddr)
{
	if (paddr == 0) {
		return nullptr;
	}
	return m_dictionary.RecoverAddress(paddr).value_or(nullptr);
}

uint32_t PseudoAddressManager::ToPseudoAddress(void *addr)
{
	if (addr == nullptr) {
		return 0;
	}
	return m_dictionary.Make32bitAddress(addr).value_or(0);
}
