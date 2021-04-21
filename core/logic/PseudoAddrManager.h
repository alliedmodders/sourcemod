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

#ifndef _INCLUDE_SOURCEMOD_PSEUDOADDRESSMANAGER_H_
#define _INCLUDE_SOURCEMOD_PSEUDOADDRESSMANAGER_H_

#include "common_logic.h"

class PseudoAddressManager
{
public:
	PseudoAddressManager();
public:
	void *FromPseudoAddress(uint32_t paddr);
	uint32_t ToPseudoAddress(void *addr);
private:
	void *GetAllocationBase(void *ptr);
private:
	static constexpr uint8_t PSEUDO_OFFSET_BITS = 26;
	static constexpr uint8_t PSEUDO_INDEX_BITS = sizeof(uint32_t) * 8 - PSEUDO_OFFSET_BITS;
	void *m_AllocBases[1 << PSEUDO_INDEX_BITS];
	uint8_t m_NumEntries;
};

#endif // _INCLUDE_SOURCEMOD_PSEUDOADDRESSMANAGER_H_
