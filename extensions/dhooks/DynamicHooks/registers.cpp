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

#include "registers.h"

CRegisters::CRegisters(std::vector<Register_t> registers)
{	
	// ========================================================================
	// >> 8-bit General purpose registers
	// ========================================================================
	m_al = CreateRegister(registers, AL, 1);
	m_cl = CreateRegister(registers, CL, 1);
	m_dl = CreateRegister(registers, DL, 1);
	m_bl = CreateRegister(registers, BL, 1);

	// 64-bit mode only
	/*
	m_spl = CreateRegister(registers, SPL, 1);
	m_bpl = CreateRegister(registers, BPL, 1);
	m_sil = CreateRegister(registers, SIL, 1);
	m_dil = CreateRegister(registers, DIL, 1);
	m_r8b = CreateRegister(registers, R8B, 1);
	m_r9b = CreateRegister(registers, R9B, 1);
	m_r10b = CreateRegister(registers, R10B, 1);
	m_r11b = CreateRegister(registers, R11B, 1);
	m_r12b = CreateRegister(registers, R12B, 1);
	m_r13b = CreateRegister(registers, R13B, 1);
	m_r14b = CreateRegister(registers, R14B, 1);
	m_r15b = CreateRegister(registers, R15B, 1);
	*/

	m_ah = CreateRegister(registers, AH, 1);
	m_ch = CreateRegister(registers, CH, 1);
	m_dh = CreateRegister(registers, DH, 1);
	m_bh = CreateRegister(registers, BH, 1);
	
	// ========================================================================
	// >> 16-bit General purpose registers
	// ========================================================================
	m_ax = CreateRegister(registers, AX, 2);
	m_cx = CreateRegister(registers, CX, 2);
	m_dx = CreateRegister(registers, DX, 2);
	m_bx = CreateRegister(registers, BX, 2);
	m_sp = CreateRegister(registers, SP, 2);
	m_bp = CreateRegister(registers, BP, 2);
	m_si = CreateRegister(registers, SI, 2);
	m_di = CreateRegister(registers, DI, 2);

	// 64-bit mode only
	/*
	m_r8w = CreateRegister(registers, R8W, 2);
	m_r9w = CreateRegister(registers, R9W, 2);
	m_r10w = CreateRegister(registers, R10W, 2);
	m_r11w = CreateRegister(registers, R11W, 2);
	m_r12w = CreateRegister(registers, R12W, 2);
	m_r13w = CreateRegister(registers, R13W, 2);
	m_r14w = CreateRegister(registers, R14W, 2);
	m_r15w = CreateRegister(registers, R14W, 2);
	*/

	// ========================================================================
	// >> 32-bit General purpose registers
	// ========================================================================
	m_eax = CreateRegister(registers, EAX, 4);
	m_ecx = CreateRegister(registers, ECX, 4);
	m_edx = CreateRegister(registers, EDX, 4);
	m_ebx = CreateRegister(registers, EBX, 4);
	m_esp = CreateRegister(registers, ESP, 4);
	m_ebp = CreateRegister(registers, EBP, 4);
	m_esi = CreateRegister(registers, ESI, 4);
	m_edi = CreateRegister(registers, EDI, 4);

	// 64-bit mode only
	/*
	m_r8d = CreateRegister(registers, R8D, 4);
	m_r9d = CreateRegister(registers, R9D, 4);
	m_r10d = CreateRegister(registers, R10D, 4);
	m_r11d = CreateRegister(registers, R11D, 4);
	m_r12d = CreateRegister(registers, R12D, 4);
	m_r13d = CreateRegister(registers, R13D, 4);
	m_r14d = CreateRegister(registers, R14D, 4);
	m_r15d = CreateRegister(registers, R15D, 4);
	*/

	// ========================================================================
	// >> 64-bit General purpose registers
	// ========================================================================
	// 64-bit mode only
	/*
	m_rax = CreateRegister(registers, RAX, 8);
	m_rcx = CreateRegister(registers, RCX, 8);
	m_rdx = CreateRegister(registers, RDX, 8);
	m_rbx = CreateRegister(registers, RBX, 8);
	m_rsp = CreateRegister(registers, RSP, 8);
	m_rbp = CreateRegister(registers, RBP, 8);
	m_rsi = CreateRegister(registers, RSI, 8);
	m_rdi = CreateRegister(registers, RDI, 8);
	*/
	
	// 64-bit mode only
	/*
	m_r8 = CreateRegister(registers, R8, 8);
	m_r9 = CreateRegister(registers, R9, 8);
	m_r10 = CreateRegister(registers, R10, 8);
	m_r11 = CreateRegister(registers, R11, 8);
	m_r12 = CreateRegister(registers, R12, 8);
	m_r13 = CreateRegister(registers, R13, 8);
	m_r14 = CreateRegister(registers, R14, 8);
	m_r15 = CreateRegister(registers, R15, 8);
	*/

	// ========================================================================
	// >> 64-bit MM (MMX) registers
	// ========================================================================
	m_mm0 = CreateRegister(registers, MM0, 8);
	m_mm1 = CreateRegister(registers, MM1, 8);
	m_mm2 = CreateRegister(registers, MM2, 8);
	m_mm3 = CreateRegister(registers, MM3, 8);
	m_mm4 = CreateRegister(registers, MM4, 8);
	m_mm5 = CreateRegister(registers, MM5, 8);
	m_mm6 = CreateRegister(registers, MM6, 8);
	m_mm7 = CreateRegister(registers, MM7, 8);

	// ========================================================================
	// >> 128-bit XMM registers
	// ========================================================================
	// Copying data from xmm0-7 requires the memory address to be 16-byte aligned.
	m_xmm0 = CreateRegister(registers, XMM0, 16, 16);
	m_xmm1 = CreateRegister(registers, XMM1, 16, 16);
	m_xmm2 = CreateRegister(registers, XMM2, 16, 16);
	m_xmm3 = CreateRegister(registers, XMM3, 16, 16);
	m_xmm4 = CreateRegister(registers, XMM4, 16, 16);
	m_xmm5 = CreateRegister(registers, XMM5, 16, 16);
	m_xmm6 = CreateRegister(registers, XMM6, 16, 16);
	m_xmm7 = CreateRegister(registers, XMM7, 16, 16);

	// 64-bit mode only
	/*
	m_xmm8 = CreateRegister(registers, XMM8, 16);
	m_xmm9 = CreateRegister(registers, XMM9, 16);
	m_xmm10 = CreateRegister(registers, XMM10, 16);
	m_xmm11 = CreateRegister(registers, XMM11, 16);
	m_xmm12 = CreateRegister(registers, XMM12, 16);
	m_xmm13 = CreateRegister(registers, XMM13, 16);
	m_xmm14 = CreateRegister(registers, XMM14, 16);
	m_xmm15 = CreateRegister(registers, XMM15, 16);
	*/

	// ========================================================================
	// >> 16-bit Segment registers
	// ========================================================================
	m_cs = CreateRegister(registers, CS, 2);
	m_ss = CreateRegister(registers, SS, 2);
	m_ds = CreateRegister(registers, DS, 2);
	m_es = CreateRegister(registers, ES, 2);
	m_fs = CreateRegister(registers, FS, 2);
	m_gs = CreateRegister(registers, GS, 2);
	
	// ========================================================================
	// >> 80-bit FPU registers
	// ========================================================================
	m_st0 = CreateRegister(registers, ST0, 10);
	m_st1 = CreateRegister(registers, ST1, 10);
	m_st2 = CreateRegister(registers, ST2, 10);
	m_st3 = CreateRegister(registers, ST3, 10);
	m_st4 = CreateRegister(registers, ST4, 10);
	m_st5 = CreateRegister(registers, ST5, 10);
	m_st6 = CreateRegister(registers, ST6, 10);
	m_st7 = CreateRegister(registers, ST7, 10);
}

CRegisters::~CRegisters()
{
	// ========================================================================
	// >> 8-bit General purpose registers
	// ========================================================================
	DeleteRegister(m_al);
	DeleteRegister(m_cl);
	DeleteRegister(m_dl);
	DeleteRegister(m_bl);

	// 64-bit mode only
	/*
	DeleteRegister(m_spl);
	DeleteRegister(m_bpl);
	DeleteRegister(m_sil);
	DeleteRegister(m_dil);
	DeleteRegister(m_r8b);
	DeleteRegister(m_r9b);
	DeleteRegister(m_r10b);
	DeleteRegister(m_r11b);
	DeleteRegister(m_r12b);
	DeleteRegister(m_r13b);
	DeleteRegister(m_r14b);
	DeleteRegister(m_r15b);
	*/

	DeleteRegister(m_ah);
	DeleteRegister(m_ch);
	DeleteRegister(m_dh);
	DeleteRegister(m_bh);
	
	// ========================================================================
	// >> 16-bit General purpose registers
	// ========================================================================
	DeleteRegister(m_ax);
	DeleteRegister(m_cx);
	DeleteRegister(m_dx);
	DeleteRegister(m_bx);
	DeleteRegister(m_sp);
	DeleteRegister(m_bp);
	DeleteRegister(m_si);
	DeleteRegister(m_di);

	// 64-bit mode only
	/*
	DeleteRegister(m_r8w);
	DeleteRegister(m_r9w);
	DeleteRegister(m_r10w);
	DeleteRegister(m_r11w);
	DeleteRegister(m_r12w);
	DeleteRegister(m_r13w);
	DeleteRegister(m_r14w);
	DeleteRegister(m_r15w);
	*/

	// ========================================================================
	// >> 32-bit General purpose registers
	// ========================================================================
	DeleteRegister(m_eax);
	DeleteRegister(m_ecx);
	DeleteRegister(m_edx);
	DeleteRegister(m_ebx);
	DeleteRegister(m_esp);
	DeleteRegister(m_ebp);
	DeleteRegister(m_esi);
	DeleteRegister(m_edi);

	// 64-bit mode only
	/*
	DeleteRegister(m_r8d);
	DeleteRegister(m_r9d);
	DeleteRegister(m_r10d);
	DeleteRegister(m_r11d);
	DeleteRegister(m_r12d);
	DeleteRegister(m_r13d);
	DeleteRegister(m_r14d);
	DeleteRegister(m_r15d);
	*/

	// ========================================================================
	// >> 64-bit General purpose registers
	// ========================================================================
	// 64-bit mode only
	/*
	DeleteRegister(m_rax);
	DeleteRegister(m_rcx);
	DeleteRegister(m_rdx);
	DeleteRegister(m_rbx);
	DeleteRegister(m_rsp);
	DeleteRegister(m_rbp);
	DeleteRegister(m_rsi);
	DeleteRegister(m_rdi);
	*/
	
	// 64-bit mode only
	/*
	DeleteRegister(m_r8);
	DeleteRegister(m_r9);
	DeleteRegister(m_r10);
	DeleteRegister(m_r11);
	DeleteRegister(m_r12);
	DeleteRegister(m_r13);
	DeleteRegister(m_r14);
	DeleteRegister(m_r15);
	*/

	// ========================================================================
	// >> 64-bit MM (MMX) registers
	// ========================================================================
	DeleteRegister(m_mm0);
	DeleteRegister(m_mm1);
	DeleteRegister(m_mm2);
	DeleteRegister(m_mm3);
	DeleteRegister(m_mm4);
	DeleteRegister(m_mm5);
	DeleteRegister(m_mm6);
	DeleteRegister(m_mm7);

	// ========================================================================
	// >> 128-bit XMM registers
	// ========================================================================
	DeleteRegister(m_xmm0);
	DeleteRegister(m_xmm1);
	DeleteRegister(m_xmm2);
	DeleteRegister(m_xmm3);
	DeleteRegister(m_xmm4);
	DeleteRegister(m_xmm5);
	DeleteRegister(m_xmm6);
	DeleteRegister(m_xmm7);

	// 64-bit mode only
	/*
	DeleteRegister(m_xmm8);
	DeleteRegister(m_xmm9);
	DeleteRegister(m_xmm10);
	DeleteRegister(m_xmm11);
	DeleteRegister(m_xmm12);
	DeleteRegister(m_xmm13);
	DeleteRegister(m_xmm14);
	DeleteRegister(m_xmm15);
	*/

	// ========================================================================
	// >> 2-bit Segment registers
	// ========================================================================
	DeleteRegister(m_cs);
	DeleteRegister(m_ss);
	DeleteRegister(m_ds);
	DeleteRegister(m_es);
	DeleteRegister(m_fs);
	DeleteRegister(m_gs);
	
	// ========================================================================
	// >> 80-bit FPU registers
	// ========================================================================
	DeleteRegister(m_st0);
	DeleteRegister(m_st1);
	DeleteRegister(m_st2);
	DeleteRegister(m_st3);
	DeleteRegister(m_st4);
	DeleteRegister(m_st5);
	DeleteRegister(m_st6);
	DeleteRegister(m_st7);
}

CRegister* CRegisters::CreateRegister(std::vector<Register_t>& registers, Register_t reg, uint16_t iSize, uint16_t iAlignment)
{
	for(size_t i = 0; i < registers.size(); i++)
	{
		if (registers[i] == reg)
		{
			return new CRegister(iSize, iAlignment);
		}
	}
	return NULL;
}

void CRegisters::DeleteRegister(CRegister* pRegister)
{
	if (pRegister)
	{
		delete pRegister;
	}
}

CRegister* CRegisters::GetRegister(Register_t reg)
{
	switch (reg)
	{
	case AL:
		return m_al;
	case CL:
		return m_cl;
	case DL:
		return m_dl;
	case BL:
		return m_bl;
	case AH:
		return m_ah;
	case CH:
		return m_ch;
	case DH:
		return m_dh;
	case BH:
		return m_bh;

	case AX:
		return m_ax;
	case CX:
		return m_cx;
	case DX:
		return m_dx;
	case BX:
		return m_bx;
	case SP:
		return m_sp;
	case BP:
		return m_bp;
	case SI:
		return m_si;
	case DI:
		return m_di;

	case EAX:
		return m_eax;
	case ECX:
		return m_ecx;
	case EDX:
		return m_edx;
	case EBX:
		return m_ebx;
	case ESP:
		return m_esp;
	case EBP:
		return m_ebp;
	case ESI:
		return m_esi;
	case EDI:
		return m_edi;

	case MM0:
		return m_mm0;
	case MM1:
		return m_mm1;
	case MM2:
		return m_mm2;
	case MM3:
		return m_mm3;
	case MM4:
		return m_mm4;
	case MM5:
		return m_mm5;
	case MM6:
		return m_mm6;
	case MM7:
		return m_mm7;

	case XMM0:
		return m_xmm0;
	case XMM1:
		return m_xmm1;
	case XMM2:
		return m_xmm2;
	case XMM3:
		return m_xmm3;
	case XMM4:
		return m_xmm4;
	case XMM5:
		return m_xmm5;
	case XMM6:
		return m_xmm6;
	case XMM7:
		return m_xmm7;

	case CS:
		return m_cs;
	case SS:
		return m_ss;
	case DS:
		return m_ds;
	case ES:
		return m_es;
	case FS:
		return m_fs;
	case GS:
		return m_gs;

	case ST0:
		return m_st0;
	case ST1:
		return m_st1;
	case ST2:
		return m_st2;
	case ST3:
		return m_st3;
	case ST4:
		return m_st4;
	case ST5:
		return m_st5;
	case ST6:
		return m_st6;
	case ST7:
		return m_st7;

	default:
		return NULL;
	}
}