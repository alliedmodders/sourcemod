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
#include "hook.h"
#include "extension.h"

#ifdef DYNAMICHOOKS_x86_64

#else
#include <macro-assembler-x86.h>
#include <jit/jit_helpers.h>
#include <CDetour/detourhelpers.h>
using namespace sp;
#endif

// ============================================================================
// >> DEFINITIONS
// ============================================================================
#define JMP_SIZE 6


// ============================================================================
// >> CHook
// ============================================================================
CHook::CHook(void* pFunc, ICallingConvention* pConvention)
{
	m_pFunc = pFunc;
	m_pRegisters = new CRegisters(pConvention->GetRegisters());
	m_pCallingConvention = pConvention;

	if (!m_hookHandler.init())
		return;

	if (!m_RetAddr.init())
		return;

	CreateBridge();
	if (!m_pBridge)
		return;

	auto result = safetyhook::InlineHook::create(pFunc, m_pBridge, safetyhook::InlineHook::Flags::StartDisabled);
	if (!result) {
		return;
	}

	m_Hook = std::move(result.value());
	m_pTrampoline = m_Hook.original<void*>();

	m_Hook.enable();
}

CHook::~CHook()
{
	if (m_Hook.enabled()) {
		m_Hook.disable();
	}

	// x64 will free these in the m_bridge/m_postCallback destructors.
#ifndef DYNAMICHOOKS_x86_64
	if (m_pBridge) {
		smutils->GetScriptingEngine()->FreePageMemory(m_pBridge);
		smutils->GetScriptingEngine()->FreePageMemory(m_pNewRetAddr);
	}
#endif

	delete m_pRegisters;
	delete m_pCallingConvention;
}

void CHook::AddCallback(HookType_t eHookType, HookHandlerFn* pCallback)
{
	if (!pCallback)
		return;

	HookTypeMap::Insert i = m_hookHandler.findForAdd(eHookType);
	if (!i.found()) {
		HookHandlerSet set;
		set.init();
		m_hookHandler.add(i, eHookType, std::move(set));
	}

	i->value.add(pCallback);
}

void CHook::RemoveCallback(HookType_t eHookType, HookHandlerFn* pCallback)
{
	HookTypeMap::Result r = m_hookHandler.find(eHookType);
	if (!r.found())
		return;

	r->value.removeIfExists(pCallback);
}

bool CHook::IsCallbackRegistered(HookType_t eHookType, HookHandlerFn* pCallback)
{
	HookTypeMap::Result r = m_hookHandler.find(eHookType);
	if (!r.found())
		return false;

	return r->value.has(pCallback);
}

bool CHook::AreCallbacksRegistered()
{
	HookTypeMap::Result r = m_hookHandler.find(HOOKTYPE_PRE);
	if (r.found() && r->value.elements() > 0)
		return true;

	r = m_hookHandler.find(HOOKTYPE_POST);
	if (r.found() && r->value.elements() > 0)
		return true;

	return false;
}

ReturnAction_t CHook::HookHandler(HookType_t eHookType)
{
	if (eHookType == HOOKTYPE_POST)
	{
		ReturnAction_t lastPreReturnAction = m_LastPreReturnAction.back();
		m_LastPreReturnAction.pop_back();
		if (lastPreReturnAction >= ReturnAction_Override)
			m_pCallingConvention->RestoreReturnValue(m_pRegisters);
		if (lastPreReturnAction < ReturnAction_Supercede)
			m_pCallingConvention->RestoreCallArguments(m_pRegisters);
	}

	ReturnAction_t returnAction = ReturnAction_Ignored;
	HookTypeMap::Result r = m_hookHandler.find(eHookType);
	if (!r.found())
	{
		// Still save the arguments for the post hook even if there
		// is no pre-handler registered.
		if (eHookType == HOOKTYPE_PRE)
		{
			m_LastPreReturnAction.push_back(returnAction);
			m_pCallingConvention->SaveCallArguments(m_pRegisters);
		}
		return returnAction;
	}

	HookHandlerSet &callbacks = r->value;
	for(HookHandlerSet::iterator it=callbacks.iter(); !it.empty(); it.next())
	{
		ReturnAction_t result = ((HookHandlerFn) *it)(eHookType, this);
		if (result > returnAction)
			returnAction = result;
	}

	if (eHookType == HOOKTYPE_PRE)
	{
		m_LastPreReturnAction.push_back(returnAction);
		if (returnAction >= ReturnAction_Override)
			m_pCallingConvention->SaveReturnValue(m_pRegisters);
		if (returnAction < ReturnAction_Supercede)
			m_pCallingConvention->SaveCallArguments(m_pRegisters);
	}

	return returnAction;
}

void* __cdecl CHook::GetReturnAddress(void* pESP)
{
	ReturnAddressMap::Result r = m_RetAddr.find(pESP);
	assert(r.found());
	if (!r.found())
	{
		smutils->LogError(myself, "FATAL: Failed to find return address of original function. Check the arguments and return type of your detour setup.");
		return NULL;
	}

	void *pRetAddr = r->value.back();
	r->value.pop_back();

	// Clear the stack address from the cache now that we ran the last post hook.
	if (r->value.empty())
		m_RetAddr.remove(r);

	return pRetAddr;
}

void __cdecl CHook::SetReturnAddress(void* pRetAddr, void* pESP)
{
	ReturnAddressMap::Insert i = m_RetAddr.findForAdd(pESP);
	if (!i.found())
		m_RetAddr.add(i, pESP, std::vector<void *>());

	i->value.push_back(pRetAddr);
}

#ifdef DYNAMICHOOKS_x86_64
using namespace SourceHook::Asm;
SourceHook::CPageAlloc SourceHook::Asm::GenBuffer::ms_Allocator(16);

void PrintFunc(const char* message) {
	g_pSM->LogMessage(myself, message);
}

void PrintDebug(x64JitWriter& jit, const char* message) {
	// LogMessage has variadic parameters, this shouldn't be problem on x86_64
	// but paranoia calls for safety
	/*
	union {
		void (*PrintFunc)(const char* message);
		std::uint64_t address;
	} func;

	func.PrintFunc = &PrintFunc;

	// Shadow space
	MSVC_ONLY(jit.sub(rsp, 40));

	MSVC_ONLY(jit.mov(rcx, reinterpret_cast<std::uint64_t>(message)));
	GCC_ONLY(jit.mov(rdi, reinterpret_cast<std::uint64_t>(message)));

	jit.mov(rax, func.address);
	jit.call(rax);

	// Free shadow space
	MSVC_ONLY(jit.add(rsp, 40));*/
}

void _PrintRegs(std::uint64_t* rsp, int numregs) {
	g_pSM->LogMessage(myself, "RSP - %p", rsp);

	for (int i = 0; i < numregs; i++) {
		g_pSM->LogMessage(myself, "RSP[%d] - %llu", i, rsp[i]);
	}
}

void PrintRegisters(x64JitWriter& jit) {
	/*
	union {
		void (*PrintRegs)(std::uint64_t* rsp, int numregs);
		std::uint64_t address;
	} func;
	func.PrintRegs = &_PrintRegs;
	// Rax is pushed twice to keep the stack aligned
	jit.push(rax);
	jit.push(rax);
	jit.push(rcx);
	jit.push(rdx);
	jit.push(r8);
	jit.push(r9);

	jit.mov(rcx, rsp);
	jit.mov(rdx, 5);
	jit.sub(rsp, 40);
	jit.mov(rax, func.address);
	jit.call(rax);
	jit.add(rsp, 40);

	jit.pop(r9);
	jit.pop(r8);
	jit.pop(rdx);
	jit.pop(rcx);
	jit.pop(rax);
	jit.pop(rax);*/
}

void CHook::CreateBridge()
{
	auto& jit = m_bridge;

	//jit.breakpoint();
	PrintRegisters(jit);

	// Save registers right away
	Write_SaveRegisters(jit, HOOKTYPE_PRE);

	PrintDebug(jit, "Hook Called");

	Write_ModifyReturnAddress(jit);

	// Call the pre-hook handler and jump to label_supercede if ReturnAction_Supercede was returned
	Write_CallHandler(jit, HOOKTYPE_PRE);

	jit.cmp(rax, ReturnAction_Supercede);
	
	// Restore the previously saved registers, so any changes will be applied
	Write_RestoreRegisters(jit, HOOKTYPE_PRE);

	jit.je(0x0);
	std::int32_t jumpOff = jit.get_outputpos();

	// Dump regs
	PrintRegisters(jit);

	// Jump to the trampoline
	jit.sub(rsp, 8);
	jit.push(rax);
	jit.mov(rax, reinterpret_cast<std::uint64_t>(&m_pTrampoline));
	jit.mov(rax, rax());
	jit.mov(rsp(8), rax);
	jit.pop(rax);
	jit.retn();

	// This code will be executed if a pre-hook returns ReturnAction_Supercede
	jit.rewrite<std::int32_t>(jumpOff - sizeof(std::int32_t), jit.get_outputpos() - jumpOff);

	PrintDebug(jit, "Hook leave");

	// Finally, return to the caller
	// This will still call post hooks, but will skip the original function.
	jit.retn();

	// It's very important to only finalise the jit code creation here
	// setting the memory to ReadExecute too early can create crashes
	// as it changes the permissions of the whole memory page
	m_pBridge = m_bridge.GetData();
	m_pNewRetAddr = m_postCallback.GetData();

	m_bridge.SetRE();
	m_postCallback.SetRE();
}

void CHook::Write_ModifyReturnAddress(x64JitWriter& jit)
{
	// Store the return address in rax
	jit.mov(rax, rsp());
	
	// Save the original return address by using the current esp as the key.
	// This should be unique until we have returned to the original caller.
	union
	{
		void (__cdecl CHook::*SetReturnAddress)(void*, void*);
		std::uint64_t address;
	} func;
	func.SetReturnAddress = &CHook::SetReturnAddress;

	// Shadow space 32 bytes + 8 bytes to keep it aligned on 16 bytes
	MSVC_ONLY(jit.sub(rsp, 40));

	// 1st param (this)
	GCC_ONLY(jit.mov(rdi, reinterpret_cast<std::uint64_t>(this)));
	MSVC_ONLY(jit.mov(rcx, reinterpret_cast<std::uint64_t>(this)));

	// 2nd parameter (return address)
	GCC_ONLY(jit.mov(rsi, rax));
	MSVC_ONLY(jit.mov(rdx, rax));

	// 3rd parameter (rsp)
	GCC_ONLY(jit.lea(rdx, rsp()));
	MSVC_ONLY(jit.lea(r8, rsp(40)));

	// Call SetReturnAddress
	jit.mov(rax, func.address);
	jit.call(rax);

	// Free shadow space
	MSVC_ONLY(jit.add(rsp, 40));
	
	// Override the return address. This is a redirect to our post-hook code
	CreatePostCallback();
	jit.mov(rax, reinterpret_cast<std::uint64_t>(&m_pNewRetAddr));
	jit.mov(rax, rax());
	jit.mov(rsp(), rax);
}

void CHook::CreatePostCallback()
{
	auto& jit = m_postCallback;

	jit.sub(rsp, 8);
	PrintRegisters(jit);

	// Save registers right away
	Write_SaveRegisters(jit, HOOKTYPE_POST);

	PrintDebug(jit, "Hook post");
	PrintRegisters(jit);

	// Call the post-hook handler
	Write_CallHandler(jit, HOOKTYPE_POST);

	// Restore the previously saved registers, so any changes will be applied
	Write_RestoreRegisters(jit, HOOKTYPE_POST);

	// Get return address
	union
	{
		void* (__cdecl CHook::*GetReturnAddress)(void*);
		std::uint64_t address;
	} func;
	func.GetReturnAddress = &CHook::GetReturnAddress;

	// Shadow space 32 bytes + 8 bytes to keep it aligned on 16 bytes
	MSVC_ONLY(jit.sub(rsp, 40));

	// 1st param (this)
	GCC_ONLY(jit.mov(rdi, reinterpret_cast<std::uint64_t>(this)));
	MSVC_ONLY(jit.mov(rcx, reinterpret_cast<std::uint64_t>(this)));

	// 2n parameter (rsp)
	GCC_ONLY(jit.lea(rsi, rsp()));
	MSVC_ONLY(jit.lea(rdx, rsp(40)));

	// Call GetReturnAddress
	jit.mov(rax, func.address);
	jit.call(rax);

	// Free shadow space
	MSVC_ONLY(jit.add(rsp, 40));

	// Jump to the original return address
	jit.add(rsp, 8);
	jit.jump(rax);
}

void CHook::Write_CallHandler(x64JitWriter& jit, HookType_t type)
{
	union
	{
		ReturnAction_t (__cdecl CHook::*HookHandler)(HookType_t);
		std::uint64_t address;
	} func;

	func.HookHandler = &CHook::HookHandler;

	// Shadow space 32 bytes + 8 bytes to keep it aligned on 16 bytes
	MSVC_ONLY(jit.sub(rsp, 40));

	// Call the global hook handler

	// 1st param (this)
	GCC_ONLY(jit.mov(rdi, reinterpret_cast<std::uint64_t>(this)));
	MSVC_ONLY(jit.mov(rcx, reinterpret_cast<std::uint64_t>(this)));

	// 2nd parameter (type)
	GCC_ONLY(jit.mov(rsi, type));
	MSVC_ONLY(jit.mov(rdx, type));

	jit.mov(rax, func.address);
	jit.call(rax);
	
	// Free shadow space
	MSVC_ONLY(jit.add(rsp, 40));
}

void CHook::Write_SaveRegisters(x64JitWriter& jit, HookType_t type)
{
	// Save RAX
	jit.push(rax);
	bool saveRAX = false;

	std::vector<Register_t> vecRegistersToSave = m_pCallingConvention->GetRegisters();
	for(size_t i = 0; i < vecRegistersToSave.size(); i++)
	{
		switch(vecRegistersToSave[i])
		{
		// ========================================================================
		// >> 64-bit General purpose registers
		// ========================================================================
		// RAX is saved by default (see above)
		case Register_t::RAX: saveRAX = true; break;
		case Register_t::RCX: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_rcx->m_pAddress)); jit.mov(rax(), rcx); break;
		case Register_t::RDX: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_rdx->m_pAddress)); jit.mov(rax(), rdx); break;
		case Register_t::RBX: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_rbx->m_pAddress)); jit.mov(rax(), rbx); break;
		// BEWARE BECAUSE WE PUSHED TO STACK ABOVE, RSP NEEDS TO BE HANDLED DIFFERENTLY
		case Register_t::RSP:
			jit.push(r8);
			// Add +push(rax) & +push(r8)
			jit.lea(r8, rsp(16));
			jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_rsp->m_pAddress)); jit.mov(rax(), r8);
			jit.pop(r8);
			break;
		case Register_t::RBP: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_rbp->m_pAddress)); jit.mov(rax(), rbp); break;
		case Register_t::RSI: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_rsi->m_pAddress)); jit.mov(rax(), rsi); break;
		case Register_t::RDI: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_rdi->m_pAddress)); jit.mov(rax(), rdi); break;

		case Register_t::R8: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_r8->m_pAddress)); jit.mov(rax(), r8); break;
		case Register_t::R9: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_r9->m_pAddress)); jit.mov(rax(), r9); break;
		case Register_t::R10: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_r10->m_pAddress)); jit.mov(rax(), r10); break;
		case Register_t::R11: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_r11->m_pAddress)); jit.mov(rax(), r11); break;
		case Register_t::R12: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_r12->m_pAddress)); jit.mov(rax(), r12); break;
		case Register_t::R13: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_r13->m_pAddress)); jit.mov(rax(), r13); break;
		case Register_t::R14: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_r14->m_pAddress)); jit.mov(rax(), r14); break;
		case Register_t::R15: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_r15->m_pAddress)); jit.mov(rax(), r15); break;

		// ========================================================================
		// >> 128-bit XMM registers
		// ========================================================================
		case Register_t::XMM0: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm0->m_pAddress)); jit.movsd(rax(), xmm0); break;
		case Register_t::XMM1: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm1->m_pAddress)); jit.movsd(rax(), xmm1); break;
		case Register_t::XMM2: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm2->m_pAddress)); jit.movsd(rax(), xmm2); break;
		case Register_t::XMM3: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm3->m_pAddress)); jit.movsd(rax(), xmm3); break;
		case Register_t::XMM4: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm4->m_pAddress)); jit.movsd(rax(), xmm4); break;
		case Register_t::XMM5: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm5->m_pAddress)); jit.movsd(rax(), xmm5); break;
		case Register_t::XMM6: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm6->m_pAddress)); jit.movsd(rax(), xmm6); break;
		case Register_t::XMM7: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm7->m_pAddress)); jit.movsd(rax(), xmm7); break;

		case Register_t::XMM8: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm8->m_pAddress)); jit.movsd(rax(), xmm8); break;
		case Register_t::XMM9: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm9->m_pAddress)); jit.movsd(rax(), xmm9); break;
		case Register_t::XMM10: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm10->m_pAddress)); jit.movsd(rax(), xmm10); break;
		case Register_t::XMM11: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm11->m_pAddress)); jit.movsd(rax(), xmm11); break;
		case Register_t::XMM12: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm12->m_pAddress)); jit.movsd(rax(), xmm12); break;
		case Register_t::XMM13: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm13->m_pAddress)); jit.movsd(rax(), xmm13); break;
		case Register_t::XMM14: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm14->m_pAddress)); jit.movsd(rax(), xmm14); break;
		case Register_t::XMM15: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm15->m_pAddress)); jit.movsd(rax(), xmm15); break;

		default: puts("Unsupported register.");
		}
	}

	jit.pop(rax);
	if (saveRAX) {
		jit.push(r8);
		jit.mov(r8, reinterpret_cast<std::uint64_t>(m_pRegisters->m_rax->m_pAddress)); jit.mov(r8(), rax);
		jit.pop(r8);
	}
}

void CHook::Write_RestoreRegisters(x64JitWriter& jit, HookType_t type)
{
	// RAX & RSP will be restored last
	bool restoreRAX = false, restoreRSP = false;
	
	const auto& vecRegistersToRestore = m_pCallingConvention->GetRegisters();
	for(size_t i = 0; i < vecRegistersToRestore.size(); i++)
	{
		switch(vecRegistersToRestore[i])
		{
		// ========================================================================
		// >> 64-bit General purpose registers
		// ========================================================================
		// RAX is handled differently (see below)
		case Register_t::RAX: restoreRAX = true; break;
		case Register_t::RCX: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_rcx->m_pAddress)); jit.mov(rcx, rax()); break;
		case Register_t::RDX: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_rdx->m_pAddress)); jit.mov(rdx, rax()); break;
		case Register_t::RBX: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_rbx->m_pAddress)); jit.mov(rbx, rax()); break;
		// This could be very troublesome, but oh well...
		case Register_t::RSP: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_rsp->m_pAddress)); jit.mov(rsp, rax()); break;
		case Register_t::RBP: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_rbp->m_pAddress)); jit.mov(rbp, rax()); break;
		case Register_t::RSI: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_rsi->m_pAddress)); jit.mov(rsi, rax()); break;
		case Register_t::RDI: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_rdi->m_pAddress)); jit.mov(rdi, rax()); break;

		case Register_t::R8: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_r8->m_pAddress)); jit.mov(r8, rax()); break;
		case Register_t::R9: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_r9->m_pAddress)); jit.mov(r9, rax()); break;
		case Register_t::R10: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_r10->m_pAddress)); jit.mov(r10, rax()); break;
		case Register_t::R11: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_r11->m_pAddress)); jit.mov(r11, rax()); break;
		case Register_t::R12: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_r12->m_pAddress)); jit.mov(r12, rax()); break;
		case Register_t::R13: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_r13->m_pAddress)); jit.mov(r13, rax()); break;
		case Register_t::R14: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_r14->m_pAddress)); jit.mov(r14, rax()); break;
		case Register_t::R15: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_r15->m_pAddress)); jit.mov(r15, rax()); break;

		// ========================================================================
		// >> 128-bit XMM registers
		// ========================================================================
		case Register_t::XMM0: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm0->m_pAddress)); jit.movsd(xmm0, rax()); break;
		case Register_t::XMM1: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm1->m_pAddress)); jit.movsd(xmm1, rax()); break;
		case Register_t::XMM2: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm2->m_pAddress)); jit.movsd(xmm2, rax()); break;
		case Register_t::XMM3: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm3->m_pAddress)); jit.movsd(xmm3, rax()); break;
		case Register_t::XMM4: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm4->m_pAddress)); jit.movsd(xmm4, rax()); break;
		case Register_t::XMM5: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm5->m_pAddress)); jit.movsd(xmm5, rax()); break;
		case Register_t::XMM6: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm6->m_pAddress)); jit.movsd(xmm6, rax()); break;
		case Register_t::XMM7: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm7->m_pAddress)); jit.movsd(xmm7, rax()); break;

		case Register_t::XMM8: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm8->m_pAddress)); jit.movsd(xmm8, rax()); break;
		case Register_t::XMM9: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm9->m_pAddress)); jit.movsd(xmm9, rax()); break;
		case Register_t::XMM10: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm10->m_pAddress)); jit.movsd(xmm10, rax()); break;
		case Register_t::XMM11: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm11->m_pAddress)); jit.movsd(xmm11, rax()); break;
		case Register_t::XMM12: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm12->m_pAddress)); jit.movsd(xmm12, rax()); break;
		case Register_t::XMM13: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm13->m_pAddress)); jit.movsd(xmm13, rax()); break;
		case Register_t::XMM14: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm14->m_pAddress)); jit.movsd(xmm14, rax()); break;
		case Register_t::XMM15: jit.mov(rax, reinterpret_cast<std::uint64_t>(m_pRegisters->m_xmm15->m_pAddress)); jit.movsd(xmm15, rax()); break;

		default: puts("Unsupported register.");
		}
	}

	if (restoreRAX) {
		jit.push(r8);
		jit.mov(r8, reinterpret_cast<std::uint64_t>(m_pRegisters->m_rax->m_pAddress));
		jit.mov(rax, r8());
		jit.pop(r8);
	}
}

#else
void CHook::CreateBridge()
{
	sp::MacroAssembler masm;
	Label label_supercede;

	// Write a redirect to the post-hook code
	Write_ModifyReturnAddress(masm);

	// Call the pre-hook handler and jump to label_supercede if ReturnAction_Supercede was returned
	Write_CallHandler(masm, HOOKTYPE_PRE);
	masm.cmpb(r8_al, ReturnAction_Supercede);
	
	// Restore the previously saved registers, so any changes will be applied
	Write_RestoreRegisters(masm, HOOKTYPE_PRE);

	masm.j(equal, &label_supercede);

	// Jump to the trampoline
	masm.subl(esp, 4);
	masm.push(eax);
	masm.movl(eax, Operand(ExternalAddress(&m_pTrampoline)));
	masm.movl(Operand(esp, 4), eax);
	masm.pop(eax);
	masm.ret();

	// This code will be executed if a pre-hook returns ReturnAction_Supercede
	masm.bind(&label_supercede);

	// Finally, return to the caller
	// This will still call post hooks, but will skip the original function.
	masm.ret(m_pCallingConvention->GetPopSize());

	void* base = smutils->GetScriptingEngine()->AllocatePageMemory(masm.length());
	masm.emitToExecutableMemory(base);
	m_pBridge = base;
}

void CHook::Write_ModifyReturnAddress(sp::MacroAssembler& masm)
{
	// Save scratch registers that are used by SetReturnAddress
	static void* pEAX = NULL;
	static void* pECX = NULL;
	static void* pEDX = NULL;

	masm.movl(Operand(ExternalAddress(&pEAX)), eax);
	masm.movl(Operand(ExternalAddress(&pECX)), ecx);
	masm.movl(Operand(ExternalAddress(&pEDX)), edx);

	// Store the return address in eax
	masm.movl(eax, Operand(esp, 0));
	
	// Save the original return address by using the current esp as the key.
	// This should be unique until we have returned to the original caller.
	void (__cdecl CHook::*SetReturnAddress)(void*, void*) = &CHook::SetReturnAddress;
	masm.push(esp);
	masm.push(eax);
	masm.push(intptr_t(this));
	masm.call(ExternalAddress((void *&)SetReturnAddress));
	masm.addl(esp, 12);
	
	// Restore scratch registers
	masm.movl(eax, Operand(ExternalAddress(&pEAX)));
	masm.movl(ecx, Operand(ExternalAddress(&pECX)));
	masm.movl(edx, Operand(ExternalAddress(&pEDX)));

	// Override the return address. This is a redirect to our post-hook code
	CreatePostCallback();
	masm.movl(Operand(esp, 0), intptr_t(m_pNewRetAddr));
}

void CHook::CreatePostCallback()
{
	sp::MacroAssembler masm;

	int iPopSize = m_pCallingConvention->GetPopSize();

	// Subtract the previously added bytes (stack size + return address), so
	// that we can access the arguments again
	masm.subl(esp, iPopSize+4);

	// Call the post-hook handler
	Write_CallHandler(masm, HOOKTYPE_POST);

	// Restore the previously saved registers, so any changes will be applied
	Write_RestoreRegisters(masm, HOOKTYPE_POST);

	// Save scratch registers that are used by GetReturnAddress
	static void* pEAX = NULL;
	static void* pECX = NULL;
	static void* pEDX = NULL;
	masm.movl(Operand(ExternalAddress(&pEAX)), eax);
	masm.movl(Operand(ExternalAddress(&pECX)), ecx);
	masm.movl(Operand(ExternalAddress(&pEDX)), edx);
	
	// Get the original return address
	void* (__cdecl CHook::*GetReturnAddress)(void*) = &CHook::GetReturnAddress;
	masm.push(esp);
	masm.push(intptr_t(this));
	masm.call(ExternalAddress((void *&)GetReturnAddress));
	masm.addl(esp, 8);

	// Save the original return address
	static void* pRetAddr = NULL;
	masm.movl(Operand(ExternalAddress(&pRetAddr)), eax);
	
	// Restore scratch registers
	masm.movl(eax, Operand(ExternalAddress(&pEAX)));
	masm.movl(ecx, Operand(ExternalAddress(&pECX)));
	masm.movl(edx, Operand(ExternalAddress(&pEDX)));

	// Add the bytes again to the stack (stack size + return address), so we
	// don't corrupt the stack.
	masm.addl(esp, iPopSize+4);

	// Jump to the original return address
	masm.jmp(Operand(ExternalAddress(&pRetAddr)));

	// Generate the code
	void* base = smutils->GetScriptingEngine()->AllocatePageMemory(masm.length());
	masm.emitToExecutableMemory(base);
	m_pNewRetAddr = base;
}

void CHook::Write_CallHandler(sp::MacroAssembler& masm, HookType_t type)
{
	ReturnAction_t (__cdecl CHook::*HookHandler)(HookType_t) = &CHook::HookHandler;

	// Save the registers so that we can access them in our handlers
	Write_SaveRegisters(masm, type);

	// Align the stack to 16 bytes.
	masm.subl(esp, 4);

	// Call the global hook handler
	masm.push(type);
	masm.push(intptr_t(this));
	masm.call(ExternalAddress((void *&)HookHandler));
	masm.addl(esp, 12);
}

void CHook::Write_SaveRegisters(sp::MacroAssembler& masm, HookType_t type)
{
	std::vector<Register_t> vecRegistersToSave = m_pCallingConvention->GetRegisters();
	for(size_t i = 0; i < vecRegistersToSave.size(); i++)
	{
		switch(vecRegistersToSave[i])
		{
		// ========================================================================
		// >> 8-bit General purpose registers
		// ========================================================================
		case AL: masm.movl(Operand(ExternalAddress(m_pRegisters->m_al->m_pAddress)), r8_al); break;
		case CL: masm.movl(Operand(ExternalAddress(m_pRegisters->m_cl->m_pAddress)), r8_cl); break;
		case DL: masm.movl(Operand(ExternalAddress(m_pRegisters->m_dl->m_pAddress)), r8_dl); break;
		case BL: masm.movl(Operand(ExternalAddress(m_pRegisters->m_bl->m_pAddress)), r8_bl); break;
		case AH: masm.movl(Operand(ExternalAddress(m_pRegisters->m_ah->m_pAddress)), r8_ah); break;
		case CH: masm.movl(Operand(ExternalAddress(m_pRegisters->m_ch->m_pAddress)), r8_ch); break;
		case DH: masm.movl(Operand(ExternalAddress(m_pRegisters->m_dh->m_pAddress)), r8_dh); break;
		case BH: masm.movl(Operand(ExternalAddress(m_pRegisters->m_bh->m_pAddress)), r8_bh); break;

		// ========================================================================
		// >> 32-bit General purpose registers
		// ========================================================================
		case EAX: masm.movl(Operand(ExternalAddress(m_pRegisters->m_eax->m_pAddress)), eax); break;
		case ECX: masm.movl(Operand(ExternalAddress(m_pRegisters->m_ecx->m_pAddress)), ecx); break;
		case EDX: masm.movl(Operand(ExternalAddress(m_pRegisters->m_edx->m_pAddress)), edx); break;
		case EBX: masm.movl(Operand(ExternalAddress(m_pRegisters->m_ebx->m_pAddress)), ebx); break;
		case ESP: masm.movl(Operand(ExternalAddress(m_pRegisters->m_esp->m_pAddress)), esp); break;
		case EBP: masm.movl(Operand(ExternalAddress(m_pRegisters->m_ebp->m_pAddress)), ebp); break;
		case ESI: masm.movl(Operand(ExternalAddress(m_pRegisters->m_esi->m_pAddress)), esi); break;
		case EDI: masm.movl(Operand(ExternalAddress(m_pRegisters->m_edi->m_pAddress)), edi); break;

		// ========================================================================
		// >> 128-bit XMM registers
		// ========================================================================
		// TODO: Also provide movups?
		case XMM0: masm.movaps(Operand(ExternalAddress(m_pRegisters->m_xmm0->m_pAddress)), xmm0); break;
		case XMM1: masm.movaps(Operand(ExternalAddress(m_pRegisters->m_xmm1->m_pAddress)), xmm1); break;
		case XMM2: masm.movaps(Operand(ExternalAddress(m_pRegisters->m_xmm2->m_pAddress)), xmm2); break;
		case XMM3: masm.movaps(Operand(ExternalAddress(m_pRegisters->m_xmm3->m_pAddress)), xmm3); break;
		case XMM4: masm.movaps(Operand(ExternalAddress(m_pRegisters->m_xmm4->m_pAddress)), xmm4); break;
		case XMM5: masm.movaps(Operand(ExternalAddress(m_pRegisters->m_xmm5->m_pAddress)), xmm5); break;
		case XMM6: masm.movaps(Operand(ExternalAddress(m_pRegisters->m_xmm6->m_pAddress)), xmm6); break;
		case XMM7: masm.movaps(Operand(ExternalAddress(m_pRegisters->m_xmm7->m_pAddress)), xmm7); break;


		// ========================================================================
		// >> 80-bit FPU registers
		// ========================================================================
		case ST0:
			// Don't mess with the FPU stack in a pre-hook. The float return is returned in st0,
			// so only load it in a post hook to avoid writing back NaN.
			if (type == HOOKTYPE_POST)
				masm.fst32(Operand(ExternalAddress(m_pRegisters->m_st0->m_pAddress)));
			break;
		//case ST1: masm.movl(tword_ptr_abs(Ptr(m_pRegisters->m_st1->m_pAddress)), st1); break;
		//case ST2: masm.movl(tword_ptr_abs(Ptr(m_pRegisters->m_st2->m_pAddress)), st2); break;
		//case ST3: masm.movl(tword_ptr_abs(Ptr(m_pRegisters->m_st3->m_pAddress)), st3); break;
		//case ST4: masm.movl(tword_ptr_abs(Ptr(m_pRegisters->m_st4->m_pAddress)), st4); break;
		//case ST5: masm.movl(tword_ptr_abs(Ptr(m_pRegisters->m_st5->m_pAddress)), st5); break;
		//case ST6: masm.movl(tword_ptr_abs(Ptr(m_pRegisters->m_st6->m_pAddress)), st6); break;
		//case ST7: masm.movl(tword_ptr_abs(Ptr(m_pRegisters->m_st7->m_pAddress)), st7); break;

		default: puts("Unsupported register.");
		}
	}
}

void CHook::Write_RestoreRegisters(sp::MacroAssembler& masm, HookType_t type)
{
	std::vector<Register_t> vecRegistersToSave = m_pCallingConvention->GetRegisters();
	for (size_t i = 0; i < vecRegistersToSave.size(); i++)
	{
		switch (vecRegistersToSave[i])
		{
		// ========================================================================
		// >> 8-bit General purpose registers
		// ========================================================================
		case AL: masm.movl(r8_al, Operand(ExternalAddress(m_pRegisters->m_al->m_pAddress))); break;
		case CL: masm.movl(r8_cl, Operand(ExternalAddress(m_pRegisters->m_cl->m_pAddress))); break;
		case DL: masm.movl(r8_dl, Operand(ExternalAddress(m_pRegisters->m_dl->m_pAddress))); break;
		case BL: masm.movl(r8_bl, Operand(ExternalAddress(m_pRegisters->m_bl->m_pAddress))); break;
		case AH: masm.movl(r8_ah, Operand(ExternalAddress(m_pRegisters->m_ah->m_pAddress))); break;
		case CH: masm.movl(r8_ch, Operand(ExternalAddress(m_pRegisters->m_ch->m_pAddress))); break;
		case DH: masm.movl(r8_dh, Operand(ExternalAddress(m_pRegisters->m_dh->m_pAddress))); break;
		case BH: masm.movl(r8_bh, Operand(ExternalAddress(m_pRegisters->m_bh->m_pAddress))); break;

		// ========================================================================
		// >> 32-bit General purpose registers
		// ========================================================================
		case EAX: masm.movl(eax, Operand(ExternalAddress(m_pRegisters->m_eax->m_pAddress))); break;
		case ECX: masm.movl(ecx, Operand(ExternalAddress(m_pRegisters->m_ecx->m_pAddress))); break;
		case EDX: masm.movl(edx, Operand(ExternalAddress(m_pRegisters->m_edx->m_pAddress))); break;
		case EBX: masm.movl(ebx, Operand(ExternalAddress(m_pRegisters->m_ebx->m_pAddress))); break;
		case ESP: masm.movl(esp, Operand(ExternalAddress(m_pRegisters->m_esp->m_pAddress))); break;
		case EBP: masm.movl(ebp, Operand(ExternalAddress(m_pRegisters->m_ebp->m_pAddress))); break;
		case ESI: masm.movl(esi, Operand(ExternalAddress(m_pRegisters->m_esi->m_pAddress))); break;
		case EDI: masm.movl(edi, Operand(ExternalAddress(m_pRegisters->m_edi->m_pAddress))); break;

		// ========================================================================
		// >> 128-bit XMM registers
		// ========================================================================
		// TODO: Also provide movups?
		case XMM0: masm.movaps(xmm0, Operand(ExternalAddress(m_pRegisters->m_xmm0->m_pAddress))); break;
		case XMM1: masm.movaps(xmm1, Operand(ExternalAddress(m_pRegisters->m_xmm1->m_pAddress))); break;
		case XMM2: masm.movaps(xmm2, Operand(ExternalAddress(m_pRegisters->m_xmm2->m_pAddress))); break;
		case XMM3: masm.movaps(xmm3, Operand(ExternalAddress(m_pRegisters->m_xmm3->m_pAddress))); break;
		case XMM4: masm.movaps(xmm4, Operand(ExternalAddress(m_pRegisters->m_xmm4->m_pAddress))); break;
		case XMM5: masm.movaps(xmm5, Operand(ExternalAddress(m_pRegisters->m_xmm5->m_pAddress))); break;
		case XMM6: masm.movaps(xmm6, Operand(ExternalAddress(m_pRegisters->m_xmm6->m_pAddress))); break;
		case XMM7: masm.movaps(xmm7, Operand(ExternalAddress(m_pRegisters->m_xmm7->m_pAddress))); break;


		// ========================================================================
		// >> 80-bit FPU registers
		// ========================================================================
		case ST0:
			if (type == HOOKTYPE_POST) {
				// Replace the top of the FPU stack.
				// Copy st0 to st0 and pop -> just pop the FPU stack.
				masm.fstp(st0);
				// Push a value to the FPU stack.
				// TODO: Only write back when changed? Save full 80bits for that case.
				//       Avoid truncation of the data if it's unchanged.
				masm.fld32(Operand(ExternalAddress(m_pRegisters->m_st0->m_pAddress)));
			}
			break;
		//case ST1: masm.movl(st1, tword_ptr_abs(Ptr(m_pRegisters->m_st1->m_pAddress))); break;
		//case ST2: masm.movl(st2, tword_ptr_abs(Ptr(m_pRegisters->m_st2->m_pAddress))); break;
		//case ST3: masm.movl(st3, tword_ptr_abs(Ptr(m_pRegisters->m_st3->m_pAddress))); break;
		//case ST4: masm.movl(st4, tword_ptr_abs(Ptr(m_pRegisters->m_st4->m_pAddress))); break;
		//case ST5: masm.movl(st5, tword_ptr_abs(Ptr(m_pRegisters->m_st5->m_pAddress))); break;
		//case ST6: masm.movl(st6, tword_ptr_abs(Ptr(m_pRegisters->m_st6->m_pAddress))); break;
		//case ST7: masm.movl(st7, tword_ptr_abs(Ptr(m_pRegisters->m_st7->m_pAddress))); break;

		default: puts("Unsupported register.");
		}
	}
}
#endif