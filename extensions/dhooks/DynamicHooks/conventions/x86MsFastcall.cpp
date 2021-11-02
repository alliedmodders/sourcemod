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
#include "x86MsFastcall.h"


// ============================================================================
// >> x86MsFastcall
// ============================================================================
x86MsFastcall::x86MsFastcall(std::vector<DataTypeSized_t> &vecArgTypes, DataTypeSized_t returnType, int iAlignment) :
	x86MsStdcall(vecArgTypes, returnType, iAlignment)
{
	// First argument is passed in ecx.
	if (m_vecArgTypes.size() > 0) {
		DataTypeSized_t &type = m_vecArgTypes[0];
		// Don't force the register on the user.
		// Why choose Fastcall if you set your own argument registers though..
		if (type.custom_register == None)
			type.custom_register = ECX;
	}

	// Second argument is passed in edx.
	if (m_vecArgTypes.size() > 1) {
		DataTypeSized_t &type = m_vecArgTypes[1];
		if (type.custom_register == None)
			type.custom_register = EDX;
	}
}
