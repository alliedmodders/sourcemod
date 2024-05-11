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

#ifndef _HOOK_H
#define _HOOK_H

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "registers.h"
#include "convention.h"
#include <am-hashmap.h>
#include <am-hashset.h>

// ============================================================================
// >> HookType_t
// ============================================================================
enum HookType_t
{
	// Callback will be executed before the original function.
	HOOKTYPE_PRE,

	// Callback will be executed after the original function.
	HOOKTYPE_POST
};

enum ReturnAction_t
{
	ReturnAction_Ignored,				// handler didn't take any action
	ReturnAction_Handled,				// plugin did something, but real function should still be called
	ReturnAction_Override,				// call real function, but use my return value
	ReturnAction_Supercede				// skip real function; use my return value
};


// ============================================================================
// >> TYPEDEFS
// ============================================================================
class CHook;
typedef ReturnAction_t (*HookHandlerFn)(HookType_t, CHook*);

#ifdef __linux__
#define __cdecl
#endif

struct IntegerPolicy
{
	static inline uint32_t hash(size_t i) {
		return ke::HashInteger<sizeof(size_t)>(i);
	}
	static inline bool matches(size_t i1, size_t i2) {
		return i1 == i2;
	}
};

typedef ke::HashSet<HookHandlerFn*, ke::PointerPolicy<HookHandlerFn>> HookHandlerSet;
typedef ke::HashMap<HookType_t, HookHandlerSet, IntegerPolicy> HookTypeMap;
typedef ke::HashMap<void*, std::vector<void*>, ke::PointerPolicy<void>> ReturnAddressMap;

namespace sp
{
	class MacroAssembler;
}


// ============================================================================
// >> CLASSES
// ============================================================================

class CHook
{
private:
	friend class CHookManager;

	/*
	Creates a new function hook.

	@param <pFunc>:
	The address of the function to hook

	@param <pConvention>:
	The calling convention of <pFunc>.
	*/
	CHook(void* pFunc, ICallingConvention* pConvention);
	~CHook();

public:
	/*
	Adds a hook handler to the hook.

	@param type The hook type.
	@param pFunc The hook handler that should be added.
	*/
	void AddCallback(HookType_t type, HookHandlerFn* pFunc);
	
	/*
	Removes a hook handler to the hook.

	@param type The hook type.
	@param pFunc The hook handler that should be removed.
	*/
	void RemoveCallback(HookType_t type, HookHandlerFn* pFunc);
	
	/*
	Checks if a hook handler is already added.

	@param type The hook type.
	@param pFunc The hook handler that should be checked.
	*/
	bool IsCallbackRegistered(HookType_t type, HookHandlerFn* pFunc);

	/*
	Checks if there are any hook handlers added to this hook.
	*/
	bool AreCallbacksRegistered();

	template<class T>
	T GetArgument(int iIndex)
	{
		return *(T *) m_pCallingConvention->GetArgumentPtr(iIndex, m_pRegisters);
	}

	template<class T>
	void SetArgument(int iIndex, T value)
	{
		void* pPtr = m_pCallingConvention->GetArgumentPtr(iIndex, m_pRegisters);
		*(T *) pPtr = value;
		m_pCallingConvention->ArgumentPtrChanged(iIndex, m_pRegisters, pPtr);
	}

	template<class T>
	T GetReturnValue()
	{
		return *(T *)  m_pCallingConvention->GetReturnPtr(m_pRegisters);
	}

	template<class T>
	void SetReturnValue(T value)
	{
		void* pPtr = m_pCallingConvention->GetReturnPtr(m_pRegisters);
		*(T *)  pPtr = value;
		m_pCallingConvention->ReturnPtrChanged(m_pRegisters, pPtr);
	}

private:
	void* CreateBridge();

	void Write_ModifyReturnAddress(sp::MacroAssembler& masm);
	void Write_CallHandler(sp::MacroAssembler& masm, HookType_t type);
	void Write_SaveRegisters(sp::MacroAssembler& masm, HookType_t type);
	void Write_RestoreRegisters(sp::MacroAssembler& masm, HookType_t type);

	void* CreatePostCallback();

	ReturnAction_t __cdecl HookHandler(HookType_t type);

	void* __cdecl GetReturnAddress(void* pESP);
	void __cdecl SetReturnAddress(void* pRetAddr, void* pESP);

public:

	HookTypeMap m_hookHandler;

	// Address of the original function
	void* m_pFunc;

	ICallingConvention* m_pCallingConvention;

	// Address of the bridge
	void* m_pBridge;

	// Address of the trampoline
	void* m_pTrampoline;

	// Register storage
	CRegisters* m_pRegisters;

	// New return address
	void* m_pNewRetAddr;

	ReturnAddressMap m_RetAddr;

	// Save the last return action of the pre HookHandler for use in the post handler.
	std::vector<ReturnAction_t> m_LastPreReturnAction;
};

#endif // _HOOK_H