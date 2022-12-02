/**
* =============================================================================
* DynamicHooks
* Copyright (C) 2015 Robin Gohmert. All rights reserved.
* Copyright (C) 2018-2021 AlliedModders LLC.  All rights reserved.
* =============================================================================
*
* This software is provided 'as-is', without any express or implied warranty.
* In no event will the authors be held liable for any damages arising from 
* the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose, 
* including commercial applications, and to alter it and redistribute it 
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not 
* claim that you wrote the original software. If you use this software in a 
* product, an acknowledgment in the product documentation would be 
* appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
*
* asm.h/cpp from devmaster.net (thanks cybermind) edited by pRED* to handle gcc
* -fPIC thunks correctly
*
* Idea and trampoline code taken from DynDetours (thanks your-name-here).
*
* Adopted to provide similar features to SourceHook by AlliedModders LLC.
*/

#ifndef _X86_MS_FASTCALL_H
#define _X86_MS_FASTCALL_H

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "x86MsStdcall.h"


// ============================================================================
// >> CLASSES
// ============================================================================
/*
Source: DynCall manual and Windows docs

Registers:
	- eax = return value
	- edx = return value
	- esp = stack pointer
	- st0 = floating point return value

Parameter passing:
  - first parameter in ecx, second parameter in edx, rest on the stack
	- stack parameter order: right-to-left
	- callee cleans up the stack
	- alignment: 4 bytes

Return values:
	- return values of pointer or intergral type (<= 32 bits) are returned via the eax register
	- integers > 32 bits are returned via the eax and edx registers
	- floating pointer types are returned via the st0 register
*/
class x86MsFastcall: public x86MsStdcall
{	
public:
	x86MsFastcall(std::vector<DataTypeSized_t> &vecArgTypes, DataTypeSized_t returnType, int iAlignment=4);
};

#endif // _X86_MS_FASTCALL_H