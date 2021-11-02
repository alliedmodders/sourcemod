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

#ifndef _MANAGER_H
#define _MANAGER_H

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "hook.h"
#include "convention.h"
#include <vector>


// ============================================================================
// >> CHookManager
// ============================================================================
class CHookManager
{
public:
	/*
	Hooks the given function and returns a new CHook instance. If the
	function was already hooked, the existing CHook instance will be
	returned.
	*/
    CHook* HookFunction(void* pFunc, ICallingConvention* pConvention);
	
	/*
	Removes all callbacks and restores the original function.
	*/
    void UnhookFunction(void* pFunc);

	/*
	Returns either NULL or the found CHook instance.
	*/
	CHook* FindHook(void* pFunc);

	/*
	Removes all callbacks and restores all functions.
	*/
	void UnhookAllFunctions();

public:
	std::vector<CHook *> m_Hooks;
};


// ============================================================================
// >> GetHookManager
// ============================================================================
/*
Returns a pointer to a static CHookManager object.
*/
CHookManager* GetHookManager();

#endif // _MANAGER_H