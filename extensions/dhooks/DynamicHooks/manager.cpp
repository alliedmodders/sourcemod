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
#include "manager.h"


// ============================================================================
// >> CHookManager
// ============================================================================
CHook* CHookManager::HookFunction(void* pFunc, ICallingConvention* pConvention)
{
	if (!pFunc)
		return NULL;

	CHook* pHook = FindHook(pFunc);
	if (pHook)
	{
		delete pConvention;
		return pHook;
	}
	
	pHook = new CHook(pFunc, pConvention);
	m_Hooks.push_back(pHook);
	return pHook;
}

void CHookManager::UnhookFunction(void* pFunc)
{
	if (!pFunc)
		return;

	for (size_t i = 0; i < m_Hooks.size(); i++)
	{
		CHook* pHook = m_Hooks[i];
		if (pHook->m_pFunc == pFunc)
		{
			m_Hooks.erase(m_Hooks.begin() + i);
			delete pHook;
			return;
		}
	}
}

CHook* CHookManager::FindHook(void* pFunc)
{
	if (!pFunc)
		return NULL;

	for(size_t i = 0; i < m_Hooks.size(); i++)
	{
		CHook* pHook = m_Hooks[i];
		if (pHook->m_pFunc == pFunc)
			return pHook;
	}
	return NULL;
}

void CHookManager::UnhookAllFunctions()
{
	for(size_t i = 0; i < m_Hooks.size(); i++)
		delete m_Hooks[i];

	m_Hooks.clear();
}


// ============================================================================
// >> GetHookManager
// ============================================================================
CHookManager* GetHookManager()
{
	static CHookManager* s_pManager = new CHookManager;
	return s_pManager;
}