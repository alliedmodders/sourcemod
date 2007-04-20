/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod, Copyright (C) 2004-2007 AlliedModders LLC. 
 * All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be 
 * used or modified under the Terms and Conditions of its License Agreement, 
 * which is found in public/licenses/LICENSE.txt.  As of this notice, derivative 
 * works must be licensed under the GNU General Public License (version 2 or 
 * greater).  A copy of the GPL is included under public/licenses/GPL.txt.
 * 
 * To view the latest information, see: http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_INTERFACE_BINARYUTILS_H_
#define _INCLUDE_SOURCEMOD_INTERFACE_BINARYUTILS_H_

#include <IShareSys.h>

#define SMINTERFACE_MEMORYUTILS_NAME		"IMemoryUtils"
#define SMINTERFACE_MEMORYUTILS_VERSION		1

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
