/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2024 AlliedModders LLC.  All rights reserved.
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

#pragma once

#include <sourcehook.h>
#include <sh_memory.h>
#include <cstdint>
#include <sp_vm_types.h>

namespace SourceMod
{
	class IMemoryPointer
	{
	public:
		virtual ~IMemoryPointer() = default;

		/**
		 * @brief Retrieves the underlying pointer.
		 *
		 * @return			The underlying pointer.
		 */
		virtual void* Get() = 0;

		/**
		 * @brief Approximate size in bytes of the memory block pointed by the pointer.
		 *
		 * @return			The pointed memory size in bytes, 0 if size is unknown.
		 */
		virtual cell_t GetSize() = 0;

		/**
		 * @brief Stores data at the given offset.
		 *
		 * @param data                  The data to store.
		 * @param byteSize              Size of the data in bytes.
		 * @param offset                Offset in bytes to store the data at.
		 * @param updateMemAccess       Whether or not to update the memory access before writing.
		 */
		virtual void Store(cell_t data, unsigned int byteSize, cell_t offset, bool updateMemAccess);

		/**
		 * @brief Loads data at the given offset.
		 *
		 * @param byteSize              Size of the data in bytes.
		 * @param offset                Offset in bytes to read the data at.
		 * @return                      The data stored at the given offset.
		 */
		virtual cell_t Load(unsigned int byteSize, cell_t offset);
	};

	inline void IMemoryPointer::Store(cell_t data, unsigned int byteSize, cell_t offset, bool updateMemAccess)
	{
		auto ptr = &(((std::int8_t*)this->Get())[offset]);
		if (updateMemAccess)
		{
			SourceHook::SetMemAccess(ptr, byteSize, SH_MEM_READ|SH_MEM_WRITE|SH_MEM_EXEC);
		}

		memcpy(ptr, &data, byteSize);
	}

	inline cell_t IMemoryPointer::Load(unsigned int byteSize, cell_t offset)
	{
		auto ptr = &(((std::int8_t*)this->Get())[offset]);

		switch(byteSize)
		{
			case 1:
				return *(std::int8_t*)(ptr);
			case 2:
				return *(std::int16_t*)(ptr);
			case 4:
				return *(std::int32_t*)(ptr);
			default:
				return 0;
		}
	}
}