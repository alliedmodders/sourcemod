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
#include <asm/asm.h>
#include <macro-assembler-x86.h>
#include "extension.h"
#include <jit/jit_helpers.h>
#include <CDetour/detourhelpers.h>

using namespace sp;

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

	unsigned char* pTarget = (unsigned char *) pFunc;

	// Determine the number of bytes we need to copy
	int iBytesToCopy = copy_bytes(pTarget, NULL, JMP_SIZE);

	// Create a buffer for the bytes to copy + a jump to the rest of the
	// function.
	unsigned char* pCopiedBytes = (unsigned char *) smutils->GetScriptingEngine()->AllocatePageMemory(iBytesToCopy + JMP_SIZE);

	// Fill the array with NOP instructions
	memset(pCopiedBytes, 0x90, iBytesToCopy + JMP_SIZE);

	// Copy the required bytes to our array
	copy_bytes(pTarget, pCopiedBytes, JMP_SIZE);

	// Write a jump after the copied bytes to the function/bridge + number of bytes to copy
	DoGatePatch(pCopiedBytes + iBytesToCopy, pTarget + iBytesToCopy);

	// Save the trampoline
	m_pTrampoline = (void *) pCopiedBytes;

	// Create the bridge function
	m_pBridge = CreateBridge();

	// Write a jump to the bridge
	DoGatePatch((unsigned char *) pFunc, m_pBridge);
}

CHook::~CHook()
{
	// Copy back the previously copied bytes
	copy_bytes((unsigned char *) m_pTrampoline, (unsigned char *) m_pFunc, JMP_SIZE);

	// Free the trampoline buffer
	smutils->GetScriptingEngine()->FreePageMemory(m_pTrampoline);

	// Free the asm bridge and new return address
	smutils->GetScriptingEngine()->FreePageMemory(m_pBridge);
	smutils->GetScriptingEngine()->FreePageMemory(m_pNewRetAddr);

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
		if (lastPreReturnAction == ReturnAction_Override)
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
		if (returnAction == ReturnAction_Override)
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

void* CHook::CreateBridge()
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
	masm.jmp(ExternalAddress(m_pTrampoline));

	// This code will be executed if a pre-hook returns ReturnAction_Supercede
	masm.bind(&label_supercede);

	// Finally, return to the caller
	// This will still call post hooks, but will skip the original function.
	masm.ret(m_pCallingConvention->GetPopSize());

	void *base = smutils->GetScriptingEngine()->AllocatePageMemory(masm.length());
	masm.emitToExecutableMemory(base);
	return base;
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
	m_pNewRetAddr = CreatePostCallback();
	masm.movl(Operand(esp, 0), intptr_t(m_pNewRetAddr));
}

void* CHook::CreatePostCallback()
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
	void *base = smutils->GetScriptingEngine()->AllocatePageMemory(masm.length());
	masm.emitToExecutableMemory(base);
	return base;
}

void CHook::Write_CallHandler(sp::MacroAssembler& masm, HookType_t type)
{
	ReturnAction_t (__cdecl CHook::*HookHandler)(HookType_t) = &CHook::HookHandler;

	// Save the registers so that we can access them in our handlers
	Write_SaveRegisters(masm, type);

	// Align the stack to 16 bytes.
	masm.subl(esp, 8);

	// Call the global hook handler
	masm.push(type);
	masm.push(intptr_t(this));
	masm.call(ExternalAddress((void *&)HookHandler));
	masm.addl(esp, 16);
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