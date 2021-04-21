/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod BinTools Extension
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

#ifndef _INCLUDE_SOURCEMOD_JIT_COMPILE_H_
#define _INCLUDE_SOURCEMOD_JIT_COMPILE_H_

#include <jit_helpers.h>
#include <x86_macros.h>
#include <sm_platform.h>
#include "CallWrapper.h"

void *JIT_CallCompile(CallWrapper *pWrapper, FuncAddrMethod method);

#if 0
void *JIT_HookCompile(HookWrapper *pWrapper);
void JIT_FreeHook(void *addr);
#endif

/********************
 * Assembly Helpers *
 ********************/

inline jit_uint8_t _DecodeRegister3(jit_uint32_t val)
{
	switch (val % 3)
	{
	case 0:
		{
			return kREG_EAX;
		}
	case 1:
		{
			return kREG_EDX;
		}
	case 2:
		{
			return kREG_ECX;
		}
	}

	/* Should never happen */
	assert(false);
	return 0xFF;
}

extern jit_uint32_t g_RegDecoder;

#endif //_INCLUDE_SOURCEMOD_JIT_COMPILE_H_
