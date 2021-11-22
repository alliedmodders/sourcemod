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

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "x86MsCdecl.h"
#include <string.h>


// ============================================================================
// >> x86MsCdecl
// ============================================================================
x86MsCdecl::x86MsCdecl(std::vector<DataTypeSized_t> &vecArgTypes, DataTypeSized_t returnType, int iAlignment) :
	ICallingConvention(vecArgTypes, returnType, iAlignment)
{
	if (m_returnType.size > 4)
	{
		m_pReturnBuffer = malloc(m_returnType.size);
	}
	else
	{
		m_pReturnBuffer = NULL;
	}
}

x86MsCdecl::~x86MsCdecl()
{
	if (m_pReturnBuffer)
	{
		free(m_pReturnBuffer);
	}
}

std::vector<Register_t> x86MsCdecl::GetRegisters()
{
	std::vector<Register_t> registers;

	registers.push_back(ESP);

	if (m_returnType.type == DATA_TYPE_FLOAT || m_returnType.type == DATA_TYPE_DOUBLE)
	{
		registers.push_back(ST0);
	}
	else
	{
		registers.push_back(EAX);
		if (m_pReturnBuffer)
		{
			registers.push_back(EDX);
		}
	}

	// Save all the custom calling convention registers as well.
	for (size_t i = 0; i < m_vecArgTypes.size(); i++)
	{
		if (m_vecArgTypes[i].custom_register == None)
			continue;

		// TODO: Make sure the list is unique? Set?
		registers.push_back(m_vecArgTypes[i].custom_register);
	}

	return registers;
}

int x86MsCdecl::GetPopSize()
{
	return 0;
}

int x86MsCdecl::GetArgStackSize()
{
	int iArgStackSize = 0;

	for (size_t i = 0; i < m_vecArgTypes.size(); i++)
	{
		if (m_vecArgTypes[i].custom_register == None)
			iArgStackSize += m_vecArgTypes[i].size;
	}

	return iArgStackSize;
}

void** x86MsCdecl::GetStackArgumentPtr(CRegisters* pRegisters)
{
	return (void **)(pRegisters->m_esp->GetValue<unsigned long>() + 4);
}

int x86MsCdecl::GetArgRegisterSize()
{
	int iArgRegisterSize = 0;

	for (size_t i = 0; i < m_vecArgTypes.size(); i++)
	{
		if (m_vecArgTypes[i].custom_register != None)
			iArgRegisterSize += m_vecArgTypes[i].size;
	}

	return iArgRegisterSize;
}

void* x86MsCdecl::GetArgumentPtr(unsigned int iIndex, CRegisters* pRegisters)
{
	if (iIndex >= m_vecArgTypes.size())
		return NULL;

	// Check if this argument was passed in a register.
	if (m_vecArgTypes[iIndex].custom_register != None)
	{
		CRegister *pRegister = pRegisters->GetRegister(m_vecArgTypes[iIndex].custom_register);
		if (!pRegister)
			return NULL;

		return pRegister->m_pAddress;
	}

	// Skip return address.
	size_t iOffset = 4;
	for(unsigned int i=0; i < iIndex; i++)
	{
		if (m_vecArgTypes[i].custom_register == None)
			iOffset += m_vecArgTypes[i].size;
	}

	return (void *) (pRegisters->m_esp->GetValue<unsigned long>() + iOffset);
}

void x86MsCdecl::ArgumentPtrChanged(unsigned int iIndex, CRegisters* pRegisters, void* pArgumentPtr)
{
}

void* x86MsCdecl::GetReturnPtr(CRegisters* pRegisters)
{
	if (m_returnType.type == DATA_TYPE_FLOAT || m_returnType.type == DATA_TYPE_DOUBLE)
		return pRegisters->m_st0->m_pAddress;

	if (m_pReturnBuffer)
	{
		// First half in eax, second half in edx
		memcpy(m_pReturnBuffer, pRegisters->m_eax, 4);
		memcpy((void *) ((unsigned long) m_pReturnBuffer + 4), pRegisters->m_edx, 4);
		return m_pReturnBuffer;
	}

	return pRegisters->m_eax->m_pAddress;
}

void x86MsCdecl::ReturnPtrChanged(CRegisters* pRegisters, void* pReturnPtr)
{
	if (m_pReturnBuffer)
	{
		// First half in eax, second half in edx
		memcpy(pRegisters->m_eax, m_pReturnBuffer, 4);
		memcpy(pRegisters->m_edx, (void *) ((unsigned long) m_pReturnBuffer + 4), 4);
	}
}