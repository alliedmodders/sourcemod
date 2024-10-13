/**
* =============================================================================
* DynamicHooks-x86_64
* Copyright (C) 2024 Benoist "Kenzzer" André. All rights reserved.
* Copyright (C) 2024 AlliedModders LLC.  All rights reserved.
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
*/

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "x86_64MicrosoftDefault.h"
#include <smsdk_ext.h>

// ============================================================================
// >> CLASSES
// ============================================================================
x86_64MicrosoftDefault::x86_64MicrosoftDefault(std::vector<DataTypeSized_t> &vecArgTypes, DataTypeSized_t returnType, int iAlignment) :
	ICallingConvention(vecArgTypes, returnType, iAlignment),
	m_stackArgs(0)
{
	const Register_t params_reg[] = { RCX, RDX, R8, R9 };
	const Register_t params_floatreg[] = { XMM0, XMM1, XMM2, XMM3 };
	//const char* regNames[] = { "RCX or XMM0", "RDX or XMM1", "R8 or XMM2", "R9 or XMM3"};
	const std::uint8_t num_reg = sizeof(params_reg) / sizeof(Register_t);

	bool used_reg[] = { false, false, false, false };

	// Figure out if any register has been used
	auto retreg = m_returnType.custom_register;
	used_reg[0] = (retreg == RCX || retreg == XMM0);
	used_reg[1] = (retreg == RDX || retreg == XMM1);
	used_reg[2] = (retreg == R8 || retreg == XMM2);
	used_reg[3] = (retreg == R9 || retreg == XMM3);

	for (const auto& arg : m_vecArgTypes) {
		int reg_index = -1;
		if (arg.custom_register == RCX || arg.custom_register == XMM0) {
			reg_index = 0;
		} else if (arg.custom_register == RDX || arg.custom_register == XMM1) {
			reg_index = 1;
		} else if (arg.custom_register == R8 || arg.custom_register == XMM2) {
			reg_index = 2;
		} else if (arg.custom_register == R9 || arg.custom_register == XMM3) {
			reg_index = 3;
		}

		if (reg_index != -1) {
			if (used_reg[reg_index]) {
				puts("Argument register is used twice, or shared with return");
				return;
			}
			used_reg[reg_index] = true;
		}
	}

	// Special return type
	if (m_returnType.custom_register == None && m_returnType.type == DATA_TYPE_OBJECT &&
	// If size unknown, or doesn't fit on 1, 2, 4 or 8 bytes
	// special place must have been allocated for it
	(m_returnType.size == 0
	|| m_returnType.size == 3
	|| m_returnType.size == 5
	|| m_returnType.size == 6
	|| m_returnType.size == 7
	|| m_returnType.size > 8)) {
		for (std::uint8_t i = 0; i < num_reg && m_returnType.custom_register == None; i++) {
			if (!used_reg[i]) {
				m_returnType.custom_register = params_reg[i];
				used_reg[i] = true;
			}
			// Couldn't find a free register, this is a big problem
			if (m_returnType.custom_register == None) {
				puts("Missing free register for return pointer");
				return;
			}
		}
	}

	for (auto& arg : m_vecArgTypes) {
		if (arg.custom_register == None) {
			for (std::uint8_t i = 0; i < num_reg && arg.custom_register == None; i++) {
				// Register is unused assign it
				if (!used_reg[i]) {
					arg.custom_register = (arg.type == DATA_TYPE_FLOAT || arg.type == DATA_TYPE_DOUBLE) ? params_floatreg[i] : params_reg[i];
					used_reg[i] = true;
				}
			}
			// Couldn't find a free register, it's therefore a stack parameter
			if (arg.custom_register == None) {
				m_stackArgs++;
			}
		}
	}
}

std::vector<Register_t> x86_64MicrosoftDefault::GetRegisters()
{
	std::vector<Register_t> registers;
	
	registers.push_back(RSP);

	if (m_returnType.custom_register != None)
	{
		registers.push_back(m_returnType.custom_register);
	}
	else if (m_returnType.type == DATA_TYPE_FLOAT || m_returnType.type == DATA_TYPE_DOUBLE)
	{
		registers.push_back(XMM0);
	}
	else
	{
		registers.push_back(RAX);
	}

	for (size_t i = 0; i < m_vecArgTypes.size(); i++)
	{
		auto reg = m_vecArgTypes[i].custom_register;
		if (reg == None)
		{
			continue;
		}
		registers.push_back(m_vecArgTypes[i].custom_register);
	}

	return registers;
}

int x86_64MicrosoftDefault::GetPopSize()
{
	// Clean-up is caller handled
	return 0;
}

int x86_64MicrosoftDefault::GetArgStackSize()
{
	// Shadow space (32 bytes) + 8 bytes * amount of stack arguments
	return 32 + 8 * m_stackArgs;
}

void** x86_64MicrosoftDefault::GetStackArgumentPtr(CRegisters* registers)
{
	// Skip shadow space + return address
	return (void **)(registers->m_rsp->GetValue<uintptr_t>() + 8 + 32);
}

int x86_64MicrosoftDefault::GetArgRegisterSize()
{
	int argRegisterSize = 0;

	for (size_t i = 0; i < m_vecArgTypes.size(); i++)
	{
		if (m_vecArgTypes[i].custom_register != None)
		{
			// It doesn't matter, it's always 8 bytes or less
			argRegisterSize += 8;
		}
	}

	return argRegisterSize;
}

void* x86_64MicrosoftDefault::GetArgumentPtr(unsigned int index, CRegisters* registers)
{
	//g_pSM->LogMessage(myself, "Retrieving argument %d (max args %d) registers %p", index, m_vecArgTypes.size(), registers);
	if (index >= m_vecArgTypes.size())
	{
		//g_pSM->LogMessage(myself, "Not enough arguments");
		return nullptr;
	}
	
	// Check if this argument was passed in a register.
	if (m_vecArgTypes[index].custom_register != None)
	{
		CRegister* reg = registers->GetRegister(m_vecArgTypes[index].custom_register);
		if (!reg)
		{
			//g_pSM->LogMessage(myself, "Register does not exit");
			return nullptr;
		}
		//g_pSM->LogMessage(myself, "Register arg %d", m_vecArgTypes[index].custom_register);
		return reg->m_pAddress;
	}

	// Return address + shadow space
	size_t offset = 8 + 32;
	for (unsigned int i = 0; i < index; i++)
	{
		if (m_vecArgTypes[i].custom_register == None)
		{
			// No matter what, the stack is allocated in slices of 8 bytes
			offset += 8;
		}
	}
	return (void *) (registers->m_rsp->GetValue<uintptr_t>() + offset);
}

void x86_64MicrosoftDefault::ArgumentPtrChanged(unsigned int index, CRegisters* registers, void* argumentPtr)
{
}

void* x86_64MicrosoftDefault::GetReturnPtr(CRegisters* registers)
{
	// Custom return value register
	if (m_returnType.custom_register != None)
	{
		return registers->GetRegister(m_returnType.custom_register)->m_pAddress;
	}

	if (m_returnType.type == DATA_TYPE_FLOAT || m_returnType.type == DATA_TYPE_DOUBLE)
	{
		// Floating point register
		return registers->m_xmm0->m_pAddress;
	}
	return registers->m_rax->m_pAddress;
}

void x86_64MicrosoftDefault::ReturnPtrChanged(CRegisters* pRegisters, void* pReturnPtr)
{
}

void x86_64MicrosoftDefault::SaveReturnValue(CRegisters* registers)
{
	// It doesn't matter what the return value is, it will always be fitting on 8 bytes (or less)
	std::unique_ptr<uint8_t[]> savedReturn = std::make_unique<uint8_t[]>(8);
	memcpy(savedReturn.get(), GetReturnPtr(registers), 8);
	m_pSavedReturnBuffers.push_back(std::move(savedReturn));
}

void x86_64MicrosoftDefault::RestoreReturnValue(CRegisters* registers)
{
	// Like stated in SaveReturnValue, it will always fit within 8 bytes
	// the actual underlining type does not matter
	uint8_t* savedReturn = m_pSavedReturnBuffers.back().get();
	memcpy(GetReturnPtr(registers), savedReturn, 8);
	ReturnPtrChanged(registers, savedReturn);
	m_pSavedReturnBuffers.pop_back();
}

void x86_64MicrosoftDefault::SaveCallArguments(CRegisters* registers)
{
	int size = GetArgStackSize() + GetArgRegisterSize();
	std::unique_ptr<uint8_t[]> savedCallArguments = std::make_unique<uint8_t[]>(size);
	size_t offset = 0;
	for (unsigned int i = 0; i < m_vecArgTypes.size(); i++) {
		// Doesn't matter the type, it will always be within 8 bytes
		memcpy((void *)((uintptr_t)savedCallArguments.get() + offset), GetArgumentPtr(i, registers), 8);
		offset += 8;
	}
	m_pSavedCallArguments.push_back(std::move(savedCallArguments));
}

void x86_64MicrosoftDefault::RestoreCallArguments(CRegisters* registers)
{
	uint8_t *savedCallArguments = m_pSavedCallArguments.back().get();
	size_t offset = 0;
	for (size_t i = 0; i < m_vecArgTypes.size(); i++) {
		// Doesn't matter the type, it will always be within 8 bytes
		memcpy(GetArgumentPtr((unsigned int)i, registers), (void *)((uintptr_t)savedCallArguments + offset), 8);
		offset += 8;
	}
	m_pSavedCallArguments.pop_back();
}