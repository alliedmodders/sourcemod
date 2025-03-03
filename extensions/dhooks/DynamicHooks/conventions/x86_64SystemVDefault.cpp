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
#include "x86_64SystemVDefault.h"
#include <smsdk_ext.h>

// ============================================================================
// >> CLASSES
// ============================================================================
x86_64SystemVDefault::x86_64SystemVDefault(std::vector<DataTypeSized_t> &vecArgTypes, DataTypeSized_t returnType, int iAlignment) :
	ICallingConvention(vecArgTypes, returnType, iAlignment),
	m_stackArgs(0)
{
	const Register_t params_reg[] = { RDI, RSI, RDX, RCX, R8, R9 };
	const Register_t params_floatreg[] = { XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7 };
	//const char* regNames[] = { "RCX or XMM0", "RDX or XMM1", "R8 or XMM2", "R9 or XMM3"};
	const std::uint8_t num_reg = sizeof(params_reg) / sizeof(Register_t);
	const std::uint8_t num_floatreg = sizeof(params_floatreg) / sizeof(Register_t);

	bool used_reg[] = { false, false, false, false, false, false };
	bool used_floatreg[] = { false, false, false, false, false, false, false, false };

	// Figure out if any register has been used
	auto retreg = m_returnType.custom_register;
	for (int i = 0; i < num_reg; ++i) {
		if (retreg == params_reg[i]) {
			used_reg[i] = true;
			break;
		}
	}
	for (int i = 0; i < num_floatreg; ++i) {
		if (retreg == params_floatreg[i]) {
			used_floatreg[i] = true;
			break;
		}
	}

	for (const auto& arg : m_vecArgTypes) {
		bool found_as_int_or_ptr = false;

		for (int i = 0; i < num_reg; ++i) {
			if (arg.custom_register == params_reg[i]) {
				if (used_reg[i]) {
					puts("Argument (int/ptr) register is used twice, or shared with return");
					return;
				}
				used_reg[i] = true;
				found_as_int_or_ptr = true;
				break;
			}
		}

		if (found_as_int_or_ptr) continue;

		for (int i = 0; i < num_floatreg; ++i) {
			if (arg.custom_register == params_floatreg[i]) {
				if (used_floatreg[i]) {
					puts("Argument (float/double) register is used twice, or shared with return");
					return;
				}
				used_floatreg[i] = true;
				break;
			}
		}
	}

	// TODO: Figure out if we need to do something different for Linux...
	// Special return type
	if (m_returnType.custom_register == None && m_returnType.type == DATA_TYPE_OBJECT &&
	// If size unknown, or doesn't fit on 1, 2, 4 or 8 bytes
	// special place must have been allocated for it
	(m_returnType.size != 1
	&& m_returnType.size != 2
	&& m_returnType.size != 4
	&& m_returnType.size != 8)) {
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
		if (arg.custom_register != None) continue;

		if (arg.type == DATA_TYPE_FLOAT || arg.type == DATA_TYPE_DOUBLE) {
			for (int i = 0; i < num_reg && arg.custom_register == None; ++i) {
				// Register is unused. Assign it.
				if (!used_floatreg[i]) {
					arg.custom_register = params_floatreg[i];
					used_floatreg[i] = true;
				}
			}
		} else {
			for (int i = 0; i < num_reg && arg.custom_register == None; ++i) {
				// Register is unused. Assign it.
				if (!used_reg[i]) {
					arg.custom_register = params_reg[i];
					used_reg[i] = true;
				}
			}
		}

		// Couldn't find a free register, it's therefore a stack parameter
		if (arg.custom_register == None) {
			m_stackArgs++;
		}
	}
}

std::vector<Register_t> x86_64SystemVDefault::GetRegisters()
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

int x86_64SystemVDefault::GetPopSize()
{
	// Clean-up is caller handled
	return 0;
}

int x86_64SystemVDefault::GetArgStackSize()
{
	// 8 bytes * amount of stack arguments
	return 8 * m_stackArgs;
}

void** x86_64SystemVDefault::GetStackArgumentPtr(CRegisters* registers)
{
	// Skip return address
	return (void **)(registers->m_rsp->GetValue<uintptr_t>() + 8);
}

int x86_64SystemVDefault::GetArgRegisterSize()
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

void* x86_64SystemVDefault::GetArgumentPtr(unsigned int index, CRegisters* registers)
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

	// Return address
	size_t offset = 8;
	for (unsigned int i = 0; i < index; i++)
	{
		if (m_vecArgTypes[i].custom_register == None)
		{
			// "Regular" types will have a size of 8.
			// An object's alignment depends on its contents, which we don't know, so we have to assume the user passed in a size that includes the alignment.
			// We at least align object sizes to 8 here though.
			offset += Align(m_vecArgTypes[i].size, m_iAlignment);
		}
	}
	return (void *) (registers->m_rsp->GetValue<uintptr_t>() + offset);
}

void x86_64SystemVDefault::ArgumentPtrChanged(unsigned int index, CRegisters* registers, void* argumentPtr)
{
}

void* x86_64SystemVDefault::GetReturnPtr(CRegisters* registers)
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

void x86_64SystemVDefault::ReturnPtrChanged(CRegisters* pRegisters, void* pReturnPtr)
{
}

void x86_64SystemVDefault::SaveReturnValue(CRegisters* registers)
{
	// It doesn't matter what the return value is, it will always be fitting on 8 bytes (or less)
	std::unique_ptr<uint8_t[]> savedReturn = std::make_unique<uint8_t[]>(8);
	memcpy(savedReturn.get(), GetReturnPtr(registers), 8);
	m_pSavedReturnBuffers.push_back(std::move(savedReturn));
}

void x86_64SystemVDefault::RestoreReturnValue(CRegisters* registers)
{
	// Like stated in SaveReturnValue, it will always fit within 8 bytes
	// the actual underlining type does not matter
	uint8_t* savedReturn = m_pSavedReturnBuffers.back().get();
	memcpy(GetReturnPtr(registers), savedReturn, 8);
	ReturnPtrChanged(registers, savedReturn);
	m_pSavedReturnBuffers.pop_back();
}

void x86_64SystemVDefault::SaveCallArguments(CRegisters* registers)
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

void x86_64SystemVDefault::RestoreCallArguments(CRegisters* registers)
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
