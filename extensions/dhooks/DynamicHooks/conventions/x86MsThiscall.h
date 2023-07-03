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

#ifndef _X86_MS_THISCALL_H
#define _X86_MS_THISCALL_H

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "../convention.h"
#include <am-vector.h>


// ============================================================================
// >> CLASSES
// ============================================================================
/*
Source: DynCall manual and Windows docs

Registers:
	- eax = return value
	- ecx = this pointer
	- edx = return value
	- esp = stack pointer
	- st0 = floating point return value

Parameter passing:
	- stack parameter order: right-to-left
	- callee cleans up the stack
	- all other arguments are pushed onto the stack
	- alignment: 4 bytes

Return values:
	- return values of pointer or intergral type (<= 32 bits) are returned via the eax register
	- integers > 32 bits are returned via the eax and edx registers
	- floating pointer types are returned via the st0 register
*/
class x86MsThiscall: public ICallingConvention
{	
public:
	x86MsThiscall(std::vector<DataTypeSized_t> &vecArgTypes, DataTypeSized_t returnType, int iAlignment=4);
	virtual ~x86MsThiscall();

	virtual std::vector<Register_t> GetRegisters();
	virtual int GetPopSize();
	virtual int GetArgStackSize();
	virtual void** GetStackArgumentPtr(CRegisters* pRegisters);
	virtual int GetArgRegisterSize();
	
	virtual void* GetArgumentPtr(unsigned int iIndex, CRegisters* pRegisters);
	virtual void ArgumentPtrChanged(unsigned int iIndex, CRegisters* pRegisters, void* pArgumentPtr);

	virtual void* GetReturnPtr(CRegisters* pRegisters);
	virtual void ReturnPtrChanged(CRegisters* pRegisters, void* pReturnPtr);

	virtual void SaveCallArguments(CRegisters* pRegisters);
	virtual void RestoreCallArguments(CRegisters* pRegisters);

private:
	void* m_pReturnBuffer;
};

#endif // _X86_MS_THISCALL_H