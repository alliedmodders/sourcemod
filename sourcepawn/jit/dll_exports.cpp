/**
 * vim: set ts=4 :
 * =============================================================================
 * SourcePawn JIT
 * Copyright (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is not open source and may not be copied without explicit wriiten
 * permission of AlliedModders LLC.  This file may not be redistributed in whole
 * or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#include <sp_vm_api.h>
#include <malloc.h>
#include "x86/jit_x86.h"
#include "dll_exports.h"
#include "sp_vm_engine.h"

JITX86 g_v1_jit;
SourcePawnEngine g_engine;
SourcePawn::ISourcePawnEngine *engine = &g_engine;

EXPORTFUNC void GetSourcePawn(SourcePawn::IVirtualMachine **pVm,
							  SourcePawn::ISourcePawnEngine **pEngine)
{
	*pVm = &g_v1_jit;
	*pEngine = &g_engine;
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

