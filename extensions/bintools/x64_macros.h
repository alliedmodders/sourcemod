/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod BinTools Extension
 * Copyright (C) 2004-2017 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 */
 
#include <jit_helpers.h>
#include <x86_macros.h>

// 64-bit registers
const jit_uint8_t kREG_RAX      = kREG_EAX;
const jit_uint8_t kREG_RCX      = kREG_ECX;
const jit_uint8_t kREG_RDX      = kREG_EDX;
const jit_uint8_t kREG_RBX      = kREG_EBX;
const jit_uint8_t kREG_RSP      = kREG_ESP;
const jit_uint8_t kREG_RBP      = kREG_EBP;
const jit_uint8_t kREG_RSI      = kREG_ESI;
const jit_uint8_t kREG_RDI      = kREG_EDI;
const jit_uint8_t kREG_R8       = 8;
const jit_uint8_t kREG_R9       = 9;
const jit_uint8_t kREG_R10      = 10;
const jit_uint8_t kREG_R11      = 11;
const jit_uint8_t kREG_R12      = 12;
const jit_uint8_t kREG_R13      = 13;
const jit_uint8_t kREG_R14      = 14;
const jit_uint8_t kREG_R15      = 15;

const jit_uint8_t kREG_XMM0		= 0;
const jit_uint8_t kREG_XMM1		= 1;
const jit_uint8_t kREG_XMM2		= 2;
const jit_uint8_t kREG_XMM3		= 3;
const jit_uint8_t kREG_XMM4		= 4;
const jit_uint8_t kREG_XMM5		= 5;
const jit_uint8_t kREG_XMM6		= 6;
const jit_uint8_t kREG_XMM7		= 7;
const jit_uint8_t kREG_XMM8		= 8;
const jit_uint8_t kREG_XMM9		= 9;
const jit_uint8_t kREG_XMM10	= 10;
const jit_uint8_t kREG_XMM11	= 11;
const jit_uint8_t kREG_XMM12	= 12;
const jit_uint8_t kREG_XMM13	= 13;
const jit_uint8_t kREG_XMM14	= 14;
const jit_uint8_t kREG_XMM15	= 15;

#define X64_REX_PREFIX	0x40

#define IA32_MOV_REG8_IMM8 0xB0

inline jit_uint8_t x64_rex(bool w, jit_uint8_t r, jit_uint8_t x, jit_uint8_t b)
{
	return (X64_REX_PREFIX | ((jit_uint8_t)w << 3) | ((r>>3) << 2) | ((x>>3) << 1) | (b >> 3));
}

inline void X64_Push_Reg(JitWriter *jit, jit_uint8_t reg)
{
	if (reg >= kREG_R8)
		jit->write_ubyte(x64_rex(false, 0, 0, reg));
	IA32_Push_Reg(jit, reg & 7);
}

inline void X64_Mov_Reg_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(x64_rex(true, dest, 0, src));
	IA32_Mov_Reg_Rm(jit, dest & 7, src & 7, mode);
}

inline void X64_Mov_Reg_Rm_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(x64_rex(true, dest, 0, src));
	IA32_Mov_Reg_Rm_Disp8(jit, dest & 7, src & 7, disp);
}

inline void X64_Mov_Reg_Rm_Disp32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	jit->write_ubyte(x64_rex(true, dest, 0, src));
	IA32_Mov_Reg_Rm_Disp32(jit, dest & 7, src & 7, disp);
}

inline void X64_Mov_Reg32_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	if (src>= kREG_R8 || dest >= kREG_R8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	IA32_Mov_Reg_Rm(jit, dest & 7, src & 7, mode);
}

inline void X64_Mov_Reg32_Rm_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	if (src>= kREG_R8 || dest >= kREG_R8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	IA32_Mov_Reg_Rm_Disp8(jit, dest & 7, src & 7, disp);
}

inline void X64_Mov_Reg32_Rm_Disp32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	if (src>= kREG_R8 || dest >= kREG_R8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	IA32_Mov_Reg_Rm_Disp32(jit, dest & 7, src & 7, disp);
}

inline void X64_And_Rm_Imm8(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode, jit_int8_t value)
{
	jit->write_ubyte(x64_rex(true, 0, 0, reg));
	IA32_And_Rm_Imm8(jit, reg & 7, mode, value);
}

inline void X64_Sub_Rm_Imm8(JitWriter *jit, jit_uint8_t reg, jit_int8_t val, jit_uint8_t mode)
{
	jit->write_ubyte(x64_rex(true, 5, 0, reg));
	IA32_Sub_Rm_Imm8(jit, reg & 7, val, mode);
}

inline void X64_Sub_Rm_Imm32(JitWriter *jit, jit_uint8_t reg, jit_int32_t val, jit_uint8_t mode)
{
	jit->write_ubyte(x64_rex(true, 5, 0, reg));
	IA32_Sub_Rm_Imm32(jit, reg & 7, val, mode);
}

inline void X64_Pop_Reg(JitWriter *jit, jit_uint8_t reg)
{
	if (reg >= kREG_R8)
		jit->write_ubyte(x64_rex(false, 0, 0, reg));
	IA32_Pop_Reg(jit, reg & 7);
}

inline void X64_Return(JitWriter *jit)
{
	IA32_Return(jit);
}

inline void X64_Movzx_Reg64_Rm8_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(x64_rex(true, dest, 0, src));
	IA32_Movzx_Reg32_Rm8_Disp8(jit, dest & 7, src & 7, disp);
}

inline void X64_Movzx_Reg64_Rm8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(x64_rex(true, dest, 0, src));
	IA32_Movzx_Reg32_Rm8(jit, dest & 7, src & 7, mode);
}

inline void X64_Movzx_Reg64_Rm8_Disp32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	jit->write_ubyte(x64_rex(true, dest, 0, src));
	IA32_Movzx_Reg32_Rm8_Disp32(jit, dest & 7, src & 7, disp);
}

inline void X64_Mov_RmRSP_Reg(JitWriter *jit, jit_uint8_t src)
{
	jit->write_ubyte(x64_rex(true, kREG_RSP, 0, src));
	jit->write_ubyte(IA32_MOV_RM_REG);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, src & 7, kREG_RSP));
	jit->write_ubyte(ia32_sib(NOSCALE, kREG_NOIDX, kREG_RSP));
}

inline void X64_Mov_RmRSP_Disp8_Reg(JitWriter *jit, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(x64_rex(true, src, 0, kREG_RSP));
	jit->write_ubyte(IA32_MOV_RM_REG);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, src & 7, kREG_RSP));
	jit->write_ubyte(ia32_sib(NOSCALE, kREG_NOIDX, kREG_RSP));
	jit->write_byte(disp);
}

inline void X64_Mov_RmRSP_Disp32_Reg(JitWriter *jit, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(x64_rex(true, src, 0, kREG_RSP));
	jit->write_ubyte(IA32_MOV_RM_REG);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, src & 7, kREG_RSP));
	jit->write_ubyte(ia32_sib(NOSCALE, kREG_NOIDX, kREG_RSP));
	jit->write_byte(disp);
}

inline void X64_Movzx_Reg64_Rm16(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(x64_rex(true, dest, 0, src));
	IA32_Movzx_Reg32_Rm16(jit, dest & 7, src & 7, mode);
}

inline void X64_Movzx_Reg64_Rm16_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(x64_rex(true, dest, 0, src));
	IA32_Movzx_Reg32_Rm16_Disp8(jit, dest & 7, src & 7, disp);
}

inline void X64_Movzx_Reg64_Rm16_Disp32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	jit->write_ubyte(x64_rex(true, dest, 0, src));
	IA32_Movzx_Reg32_Rm16_Disp32(jit, dest & 7, src & 7, disp);
}

inline void X64_Lea_DispRegImm8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src_base, jit_int8_t val)
{
	jit->write_ubyte(x64_rex(true, dest, 0, src_base));
	IA32_Lea_DispRegImm8(jit, dest & 7, src_base & 7, val);
}

inline void X64_Lea_DispRegImm32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src_base, jit_int32_t val)
{
	jit->write_ubyte(x64_rex(true, dest, 0, src_base));
	IA32_Lea_DispRegImm32(jit, dest & 7, src_base & 7, val);
}

inline void X64_Add_Rm_Imm8(JitWriter *jit, jit_uint8_t reg, jit_int8_t value, jit_uint8_t mode)
{
	jit->write_ubyte(x64_rex(true, 0, 0, reg));
	IA32_Add_Rm_Imm8(jit, reg & 7, value, mode);
}

inline void X64_Add_Rm_Imm32(JitWriter *jit, jit_uint8_t reg, jit_int32_t value, jit_uint8_t mode)
{
	jit->write_ubyte(x64_rex(true, 0, 0, reg));
	IA32_Add_Rm_Imm32(jit, reg & 7, value, mode);
}

inline void X64_Movaps_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x28);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, dest & 7, src & 7));
}

inline void X64_Movups_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x10);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, dest & 7, src & 7));
}

inline void X64_Movaps_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, src, 0, dest));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x29);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, src & 7, dest & 7));
	jit->write_ubyte(ia32_sib(NOSCALE, kREG_NOIDX, dest & 7));
}

inline void X64_Movapd_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src)
{
	jit->write_ubyte(0x66);
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, src, 0, dest));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x29);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, src & 7, dest & 7));
	jit->write_ubyte(ia32_sib(NOSCALE, kREG_NOIDX, dest & 7));
}

inline void X64_Movups_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, src, 0, dest));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x11);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, src & 7, dest & 7));
}

inline void X64_Movupd_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src)
{
	jit->write_ubyte(0x66);
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, src, 0, dest));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x11);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, src & 7, dest & 7));
}

inline void X64_Movupd_Rm_Disp8_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(0x66);
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, src, 0, dest));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x11);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, src & 7, dest & 7));
	jit->write_byte(disp);
}

inline void X64_Movaps_Rm_Disp8_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, src, 0, dest));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x29);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, src & 7, dest & 7));
	jit->write_byte(disp);
}

inline void X64_Movups_Rm_Disp8_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, src, 0, dest));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x11);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, src & 7, dest & 7));
	jit->write_byte(disp);
}

inline void X64_Movaps_Rm_Disp32_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, src, 0, dest));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x29);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, src & 7, dest & 7));
	jit->write_byte(disp);
}

inline void X64_Movups_Rm_Disp32_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, src, 0, dest));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x11);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, src & 7, dest & 7));
	jit->write_byte(disp);
}

inline void X64_Movlps_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x12);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, dest & 7, src & 7));
}

inline void X64_Movhps_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x16);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, dest & 7, src & 7));
}

inline void X64_Movaps_Rm_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x28);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest & 7, src & 7));
	jit->write_byte(disp);
}

inline void X64_Movups_Rm_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x10);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest & 7, src & 7));
	jit->write_byte(disp);
}

inline void X64_Movlps_Rm_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x12);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest & 7, src & 7));
	jit->write_byte(disp);
}

inline void X64_Movhps_Rm_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x16);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest & 7, src & 7));
	jit->write_byte(disp);
}

inline void X64_Movaps_Rm_Disp32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x28);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, dest & 7, src & 7));
	jit->write_byte(disp);
}

inline void X64_Movups_Rm_Disp32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x10);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, dest & 7, src & 7));
	jit->write_byte(disp);
}

inline void X64_Movapd_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	jit->write_ubyte(0x66);
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x28);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, dest & 7, src & 7));
}

inline void X64_Movupd_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	jit->write_ubyte(0x66);
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x10);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, dest & 7, src & 7));
}

inline void X64_Movapd_Rm_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	jit->write_ubyte(0x66);
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x28);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest & 7, src & 7));
	jit->write_byte(disp);
}

inline void X64_Movupd_Rm_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	jit->write_ubyte(0x66);
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x10);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest & 7, src & 7));
	jit->write_byte(disp);
}

inline void X64_Movapd_Rm_Disp32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	jit->write_ubyte(0x66);
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x28);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, dest & 7, src & 7));
	jit->write_byte(disp);
}

inline void X64_Movupd_Rm_Disp32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	if (dest >= kREG_XMM8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, dest, 0, src));
	jit->write_ubyte(0x66);
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x10);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, dest & 7, src & 7));
	jit->write_byte(disp);
}

inline void X64_Cld(JitWriter *jit)
{
	IA32_Cld(jit);
}

inline void X64_Rep(JitWriter *jit)
{
	IA32_Rep(jit);
}

inline void X64_Movsq(JitWriter *jit)
{
	jit->write_ubyte(x64_rex(true, 0, 0, 0));
	jit->write_ubyte(IA32_MOVSD);
}

inline void X64_Movsb(JitWriter *jit)
{
	jit->write_ubyte(IA32_MOVSB);
}

inline void X64_Lea_Reg_DispRegMultImm8(JitWriter *jit, 
							  jit_uint8_t dest, 
							  jit_uint8_t src_base, 
							  jit_uint8_t src_index, 
							  jit_uint8_t scale, 
							  jit_int8_t val)
{
	jit->write_ubyte(x64_rex(true, dest, 0, src_index));
	IA32_Lea_Reg_DispRegMultImm8(jit, dest & 7, src_base, src_index & 7, scale, val);
}

inline void X64_Lea_Reg_DispRegMultImm32(JitWriter *jit, 
							  jit_uint8_t dest, 
							  jit_uint8_t src_base, 
							  jit_uint8_t src_index, 
							  jit_uint8_t scale, 
							  jit_int32_t val)
{
	jit->write_ubyte(x64_rex(true, dest, 0, src_index));
	jit->write_ubyte(IA32_LEA_REG_MEM);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, dest & 7, kREG_SIB));
	jit->write_ubyte(ia32_sib(scale, src_index & 7, src_base));
	jit->write_int32(val);
}

inline jitoffs_t X64_Mov_Reg_Imm32(JitWriter *jit, jit_uint8_t dest, jit_int32_t num)
{
	jitoffs_t offs;
	jit->write_ubyte(x64_rex(true, 0, 0, dest));
	jit->write_ubyte(0xC7);
	jit->write_ubyte(ia32_modrm(MOD_REG, 0, dest & 7));
	offs = jit->get_outputpos();
	jit->write_int32(num);
	return offs;
}

inline jitoffs_t X64_Mov_Reg_Imm64(JitWriter *jit, jit_uint8_t dest, jit_int64_t num)
{
	jitoffs_t offs;
	jit->write_ubyte(x64_rex(true, 0, 0, dest));
	jit->write_ubyte(IA32_MOV_REG_IMM+(dest & 7));
	offs = jit->get_outputpos();
	jit->write_int64(num);
	return offs;
}

inline void X64_Mov_Rm8_Reg8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	if (dest >= kREG_R8 || src > kREG_R8)
		jit->write_ubyte(x64_rex(false, src, 0, dest));
	jit->write_ubyte(IA32_MOV_RM8_REG8);
	if ((dest & 7) == kREG_RBP)	// Using rbp/r13 requires a displacement
		mode = MOD_DISP8;
	jit->write_ubyte(ia32_modrm(mode, src & 7, dest & 7));
	if ((dest & 7) == kREG_RSP)	// rsp/r12 needs SIB byte
		jit->write_ubyte(ia32_sib(NOSCALE, kREG_NOIDX, dest & 7));
	else if ((dest & 7) == kREG_RBP)
		jit->write_ubyte(0); 	// Displacement of 0 needed to use rbp/r13
}

inline void X64_Mov_Rm32_Reg32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	if (dest >= kREG_R8 || src > kREG_R8)
		jit->write_ubyte(x64_rex(false, src, 0, dest));
	jit->write_ubyte(IA32_MOV_RM_REG);
	if ((dest & 7) == kREG_RBP)	// Using rbp/r13 requires a displacement
		mode = MOD_DISP8;
	jit->write_ubyte(ia32_modrm(mode, src & 7, dest & 7));
	if ((dest & 7) == kREG_RSP)	// rsp/r12 needs SIB byte
		jit->write_ubyte(ia32_sib(NOSCALE, kREG_NOIDX, dest & 7));
	else if ((dest & 7) == kREG_RBP)
		jit->write_ubyte(0); 	// Displacement of 0 needed to use rbp/r13
}

inline void X64_Mov_Rm16_Reg16(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_16BIT_PREFIX);
	X64_Mov_Rm32_Reg32(jit, dest, src, mode);
}


inline void X64_Mov_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(x64_rex(true, src, 0, dest));
	jit->write_ubyte(IA32_MOV_RM_REG);
	if ((dest & 7) == kREG_RBP)	// Using rbp/r13 requires a displacement
		mode = MOD_DISP8;
	jit->write_ubyte(ia32_modrm(mode, src & 7, dest & 7));
	if ((dest & 7) == kREG_RSP)	// rsp/r12 needs SIB byte
		jit->write_ubyte(ia32_sib(NOSCALE, kREG_NOIDX, dest & 7));
	else if ((dest & 7) == kREG_RBP)
		jit->write_ubyte(0); 	// Displacement of 0 needed to use rbp/r13
}

inline void X64_Mov_Rm_Reg_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(x64_rex(true, src, 0, dest));
	IA32_Mov_Rm_Reg_Disp8(jit, dest & 7, src & 7, disp);
}

inline jitoffs_t X64_Call_Imm32(JitWriter *jit, jit_int32_t disp)
{
	return IA32_Call_Imm32(jit, disp);
}

inline void X64_Write_Jump32_Abs(JitWriter *jit, jitoffs_t jmp, void *target)
{
	IA32_Write_Jump32_Abs(jit, jmp, target);
}

inline void X64_Call_Reg(JitWriter *jit, jit_uint8_t reg)
{
	if (reg >= kREG_R8)
		jit->write_ubyte(x64_rex(false, 0, 0, reg));
	IA32_Call_Reg(jit, reg & 7);
}

inline jitoffs_t X64_Mov_Reg8_Imm8(JitWriter *jit, jit_uint8_t dest, jit_int8_t value)
{
	jitoffs_t offs;
	if (dest >= kREG_R8)
		jit->write_ubyte(x64_rex(false, 0, 0, dest));
	jit->write_ubyte(IA32_MOV_REG8_IMM8+(dest & 7));
	offs = jit->get_outputpos();
	jit->write_byte(value);
	return offs;
}

inline void X64_Movd_Reg32_Xmm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src)
{
	jit->write_ubyte(0x66);
	if (dest >= kREG_R8 || src >= kREG_XMM8)
		jit->write_ubyte(x64_rex(false, src, 0, dest));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x7E);
	jit->write_ubyte(ia32_modrm(MOD_REG, src & 7, dest & 7));
}

inline void X64_Movq_Reg_Xmm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src)
{
	jit->write_ubyte(0x66);
	jit->write_ubyte(x64_rex(true, src, 0, dest));
	jit->write_ubyte(0x0F);
	jit->write_ubyte(0x7E);
	jit->write_ubyte(ia32_modrm(MOD_REG, src & 7, dest & 7));
}