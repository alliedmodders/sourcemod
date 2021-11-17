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

#ifndef _REGISTERS_H
#define _REGISTERS_H


// ============================================================================
// >> INCLUDES
// ============================================================================
#include <vector>
#include <am-platform.h>
#include <cinttypes>
#include <cstdlib>


// ============================================================================
// >> Register_t
// ============================================================================
enum Register_t
{
	// No register at all.
	None,

	// ========================================================================
	// >> 8-bit General purpose registers
	// ========================================================================
	AL,
	CL,
	DL,
	BL,

	// 64-bit mode only
	/*
	SPL,
	BPL,
	SIL,
	DIL,
	R8B,
	R9B,
	R10B,
	R11B,
	R12B,
	R13B,
	R14B,
	R15B,
	*/

	AH,
	CH,
	DH,
	BH,

	// ========================================================================
	// >> 16-bit General purpose registers
	// ========================================================================
	AX,
	CX,
	DX,
	BX,
	SP,
	BP,
	SI,
	DI,

	// 64-bit mode only
	/*
	R8W,
	R9W,
	R10W,
	R11W,
	R12W,
	R13W,
	R14W,
	R15W,
	*/

	// ========================================================================
	// >> 32-bit General purpose registers
	// ========================================================================
	EAX,
	ECX,
	EDX,
	EBX,
	ESP,
	EBP,
	ESI,
	EDI,
	
	// 64-bit mode only
	/*
	R8D,
	R9D,
	R10D,
	R11D,
	R12D,
	R13D,
	R14D,
	R15D,
	*/
	
	// ========================================================================
	// >> 64-bit General purpose registers
	// ========================================================================
	// 64-bit mode only
	/*
	RAX,
	RCX,
	RDX,
	RBX,
	RSP,
	RBP,
	RSI,
	RDI,
	*/

	// 64-bit mode only
	/*
	R8,
	R9,
	R10,
	R11,
	R12,
	R13,
	R14,
	R15,
	*/

	// ========================================================================
	// >> 64-bit MM (MMX) registers
	// ========================================================================
	MM0,
	MM1,
	MM2,
	MM3,
	MM4,
	MM5,
	MM6,
	MM7,

	// ========================================================================
	// >> 128-bit XMM registers
	// ========================================================================
	XMM0,
	XMM1,
	XMM2,
	XMM3,
	XMM4,
	XMM5,
	XMM6,
	XMM7,

	// 64-bit mode only
	/*
	XMM8,
	XMM9,
	XMM10,
	XMM11,
	XMM12,
	XMM13,
	XMM14,
	XMM15,
	*/

	// ========================================================================
	// >> 16-bit Segment registers
	// ========================================================================
	CS,
	SS,
	DS,
	ES,
	FS,
	GS,

	// ========================================================================
	// >> 80-bit FPU registers
	// ========================================================================
	ST0,
	ST1,
	ST2,
	ST3,
	ST4,
	ST5,
	ST6,
	ST7,
};


// ============================================================================
// >> CRegister
// ============================================================================
class CRegister
{
public:
	CRegister(uint16_t iSize, uint16_t iAlignment = 0)
	{
		m_iSize = iSize;
		m_iAlignment = iAlignment;
		if (iAlignment > 0)
#ifdef KE_WINDOWS
			m_pAddress = _aligned_malloc(iSize, iAlignment);
#else
			m_pAddress = aligned_alloc(iAlignment, iSize);
#endif
		else
			m_pAddress = malloc(iSize);
	}

	~CRegister()
	{

#ifdef KE_WINDOWS
		if (m_iAlignment > 0)
			_aligned_free(m_pAddress);
		else
			free(m_pAddress);
#else
		free(m_pAddress);
#endif
	}

	template<class T>
	T GetValue()
	{
		return *(T *) m_pAddress;
	}

	template<class T>
	T GetPointerValue(int iOffset=0)
	{
		return *(T *) (GetValue<unsigned long>() + iOffset);
	}

	template<class T>
	void SetValue(T value)
	{
		*(T *) m_pAddress = value;
	}

	template<class T>
	void SetPointerValue(T value, int iOffset=0)
	{
		*(T *) (GetValue<unsigned long>() + iOffset) = value;
	}

public:
	uint16_t m_iSize;
	uint16_t m_iAlignment;
	void* m_pAddress;
};


// ============================================================================
// >> CRegisters
// ============================================================================
class CRegisters
{
public:
	CRegisters(std::vector<Register_t> registers);
	~CRegisters();

	CRegister* GetRegister(Register_t reg);

private:
	CRegister* CreateRegister(std::vector<Register_t>& registers, Register_t reg, uint16_t iSize, uint16_t iAlignment = 0);
	void DeleteRegister(CRegister* pRegister);

public:
	// ========================================================================
	// >> 8-bit General purpose registers
	// ========================================================================
	CRegister* m_al;
	CRegister* m_cl;
	CRegister* m_dl;
	CRegister* m_bl;

	// 64-bit mode only
	/*
	CRegister* m_spl;
	CRegister* m_bpl;
	CRegister* m_sil;
	CRegister* m_dil;
	CRegister* m_r8b;
	CRegister* m_r9b;
	CRegister* m_r10b;
	CRegister* m_r11b;
	CRegister* m_r12b;
	CRegister* m_r13b;
	CRegister* m_r14b;
	CRegister* m_r15b;
	*/

	CRegister* m_ah;
	CRegister* m_ch;
	CRegister* m_dh;
	CRegister* m_bh;

	// ========================================================================
	// >> 16-bit General purpose registers
	// ========================================================================
	CRegister* m_ax;
	CRegister* m_cx;
	CRegister* m_dx;
	CRegister* m_bx;
	CRegister* m_sp;
	CRegister* m_bp;
	CRegister* m_si;
	CRegister* m_di;

	// 64-bit mode only
	/*
	CRegister* m_r8w;
	CRegister* m_r9w;
	CRegister* m_r10w;
	CRegister* m_r11w;
	CRegister* m_r12w;
	CRegister* m_r13w;
	CRegister* m_r14w;
	CRegister* m_r15w;
	*/

	// ========================================================================
	// >> 32-bit General purpose registers
	// ========================================================================
	CRegister* m_eax;
	CRegister* m_ecx;
	CRegister* m_edx;
	CRegister* m_ebx;
	CRegister* m_esp;
	CRegister* m_ebp;
	CRegister* m_esi;
	CRegister* m_edi;

	// 64-bit mode only
	/*
	CRegister* m_r8d;
	CRegister* m_r9d;
	CRegister* m_r10d;
	CRegister* m_r11d;
	CRegister* m_r12d;
	CRegister* m_r13d;
	CRegister* m_r14d;
	CRegister* m_r15d;
	*/

	// ========================================================================
	// >> 64-bit General purpose registers
	// ========================================================================
	// 64-bit mode only
	/*
	CRegister* m_rax;
	CRegister* m_rcx;
	CRegister* m_rdx;
	CRegister* m_rbx;
	CRegister* m_rsp;
	CRegister* m_rbp;
	CRegister* m_rsi;
	CRegister* m_rdi;
	*/
	
	// 64-bit mode only
	/*
	CRegister* m_r8;
	CRegister* m_r9;
	CRegister* m_r10;
	CRegister* m_r11;
	CRegister* m_r12;
	CRegister* m_r13;
	CRegister* m_r14;
	CRegister* m_r15;
	*/

	// ========================================================================
	// >> 64-bit MM (MMX) registers
	// ========================================================================
	CRegister* m_mm0;
	CRegister* m_mm1;
	CRegister* m_mm2;
	CRegister* m_mm3;
	CRegister* m_mm4;
	CRegister* m_mm5;
	CRegister* m_mm6;
	CRegister* m_mm7;

	// ========================================================================
	// >> 128-bit XMM registers
	// ========================================================================
	CRegister* m_xmm0;
	CRegister* m_xmm1;
	CRegister* m_xmm2;
	CRegister* m_xmm3;
	CRegister* m_xmm4;
	CRegister* m_xmm5;
	CRegister* m_xmm6;
	CRegister* m_xmm7;

	// 64-bit mode only
	/*
	CRegister* m_xmm8;
	CRegister* m_xmm9;
	CRegister* m_xmm10;
	CRegister* m_xmm11;
	CRegister* m_xmm12;
	CRegister* m_xmm13;
	CRegister* m_xmm14;
	CRegister* m_xmm15;
	*/

	// ========================================================================
	// >> 16-bit Segment registers
	// ========================================================================
	CRegister* m_cs;
	CRegister* m_ss;
	CRegister* m_ds;
	CRegister* m_es;
	CRegister* m_fs;
	CRegister* m_gs;

	// ========================================================================
	// >> 80-bit FPU registers
	// ========================================================================
	CRegister* m_st0;
	CRegister* m_st1;
	CRegister* m_st2;
	CRegister* m_st3;
	CRegister* m_st4;
	CRegister* m_st5;
	CRegister* m_st6;
	CRegister* m_st7;
};

#endif // _REGISTERS_H