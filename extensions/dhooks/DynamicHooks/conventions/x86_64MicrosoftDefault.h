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

#ifndef _X86_64_MICROSOFT_DEFAULT_H
#define _X86_64_MICROSOFT_DEFAULT_H

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "../convention.h"

// ============================================================================
// >> CLASSES
// ============================================================================
class x86_64MicrosoftDefault : public ICallingConvention
{
public:
	x86_64MicrosoftDefault(std::vector<DataTypeSized_t>& vecArgTypes, DataTypeSized_t returnType, int iAlignment = 8);
	virtual ~x86_64MicrosoftDefault() = default;

	virtual std::vector<Register_t> GetRegisters() override;
	virtual int GetPopSize() override;

	virtual int GetArgStackSize() override;
	virtual void** GetStackArgumentPtr(CRegisters* registers) override;

	virtual int GetArgRegisterSize() override;

	virtual void* GetArgumentPtr(unsigned int index, CRegisters* registers) override;
	virtual void ArgumentPtrChanged(unsigned int index, CRegisters* registers, void* argumentPtr) override;

	virtual void* GetReturnPtr(CRegisters* registers) override;
	virtual void ReturnPtrChanged(CRegisters* registers, void* returnPtr) override;

	virtual void SaveReturnValue(CRegisters* registers) override;
	virtual void RestoreReturnValue(CRegisters* registers) override;

	virtual void SaveCallArguments(CRegisters* pRegisters) override;
	virtual void RestoreCallArguments(CRegisters* pRegisters) override;

protected:
	std::uint32_t m_stackArgs;
};

#endif // _X86_64_MICROSOFT_DEFAULT_H