/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_MEMORYUTILS_H_
#define _INCLUDE_SOURCEMOD_MEMORYUTILS_H_

#include <IMemoryUtils.h>

using namespace SourceMod;

struct DynLibInfo
{
	void *baseAddress;
	size_t memorySize;
};

class MemoryUtils : public IMemoryUtils
{
public:
	void *FindPattern(const void *libPtr, const char *pattern, size_t len);
private:
	bool GetLibraryInfo(const void *libPtr, DynLibInfo &lib);
};

extern MemoryUtils g_MemUtils;

#endif // _INCLUDE_SOURCEMOD_MEMORYUTILS_H_
