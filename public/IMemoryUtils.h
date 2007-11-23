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

#ifndef _INCLUDE_SOURCEMOD_INTERFACE_BINARYUTILS_H_
#define _INCLUDE_SOURCEMOD_INTERFACE_BINARYUTILS_H_

#include <IShareSys.h>

#define SMINTERFACE_MEMORYUTILS_NAME		"IMemoryUtils"
#define SMINTERFACE_MEMORYUTILS_VERSION		1

/**
 * @file IMemoryUtils.h
 * @brief Interface for finding patterns in memory.
 */

namespace SourceMod
{
	class IMemoryUtils : public SMInterface
	{
	public:
		const char *GetInterfaceName()
		{
			return SMINTERFACE_MEMORYUTILS_NAME;
		}
		unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_MEMORYUTILS_VERSION;
		}
	public:
		/**
		 * @brief Searches for a pattern of bytes within the memory of a dynamic library.
		 *
		 * @param libPtr	Pointer to any chunk of memory that resides in the dynamic library.
		 * @param pattern	Pattern of bytes to search for. 0x2A can be used as a wildcard.
		 * @param len		Size of the pattern in bytes.
		 * @return			Pointer to pattern found in memory, NULL if not found.
		 */
		virtual void *FindPattern(const void *libPtr, const char *pattern, size_t len) =0;
	};
}

#endif // _INCLUDE_SOURCEMOD_INTERFACE_MEMORYUTILS_H_
