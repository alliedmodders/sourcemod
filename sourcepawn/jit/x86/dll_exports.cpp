/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#include <sp_vm_api.h>
#include <malloc.h>
#include "jit_x86.h"
#include "dll_exports.h"

SourcePawn::ISourcePawnEngine *engine = NULL;
JITX86 g_jit;

EXPORTFUNC int GiveEnginePointer2(SourcePawn::ISourcePawnEngine *engine_p, unsigned int api_version)
{
	engine = engine_p;

	if (api_version > SOURCEPAWN_ENGINE_API_VERSION || api_version < 2)
	{
		return SP_ERROR_PARAM;
	}

	return SP_ERROR_NONE;
}

EXPORTFUNC unsigned int GetExportCount()
{
	return 1;
}

EXPORTFUNC SourcePawn::IVirtualMachine *GetExport(unsigned int exportnum)
{
	/* Don't return anything if we're not initialized yet */
	if (!engine)
	{
		return NULL;
	}

	/* We only have one export - 0 */
	if (exportnum)
	{
		return NULL;
	}

	return &g_jit;
}

#if defined __linux__
extern "C" void __cxa_pure_virtual(void)
{
}

void *operator new(size_t size)
{
	return malloc(size);
}

void *operator new[](size_t size) 
{
	return malloc(size);
}

void operator delete(void *ptr) 
{
	free(ptr);
}

void operator delete[](void * ptr)
{
	free(ptr);
}
#endif

