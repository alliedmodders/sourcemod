/**
 * vim: set ts=4 :
 * =============================================================================
 * SourcePawn JIT SDK
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_JIT_X86_MACROS_H
#define _INCLUDE_JIT_X86_MACROS_H

// :XXX:
// :XXX: This file is deprecated. For new code, use assembler-x86.h.
// :XXX:

#include <limits.h>

//MOD R/M
#define MOD_MEM_REG	0
#define MOD_DISP8	1
#define MOD_DISP32	2
#define MOD_REG		3

//SIB
#define NOSCALE		0
#define	SCALE2		1
#define	SCALE4		2
#define SCALE8		3

//Register codes
const jit_uint8_t kREG_EAX      = 0;
const jit_uint8_t kREG_ECX      = 1;
const jit_uint8_t kREG_EDX      = 2;
const jit_uint8_t kREG_EBX      = 3;
const jit_uint8_t kREG_ESP      = 4;
const jit_uint8_t kREG_SIB      = 4;
const jit_uint8_t kREG_NOIDX    = 4;
const jit_uint8_t kREG_IMM_BASE = 5;
const jit_uint8_t kREG_EBP      = 5;
const jit_uint8_t kREG_ESI      = 6;
const jit_uint8_t kREG_EDI      = 7;

#define IA32_16BIT_PREFIX	0x66

//condition codes (for example, Jcc opcodes)
#define CC_B	0x2
#define CC_NAE	CC_B
#define CC_NB	0x3
#define CC_AE	CC_NB
#define CC_E	0x4
#define CC_Z	CC_E
#define CC_NE	0x5
#define CC_NZ	CC_NE
#define CC_NA	0x6
#define CC_BE	CC_NA
#define CC_A	0x7
#define CC_NBE	CC_A
#define CC_L	0xC
#define CC_NGE	CC_L
#define CC_NL	0xD
#define CC_GE	CC_NL
#define CC_NG	0xE
#define CC_LE	CC_NG
#define CC_G	0xF
#define CC_NLE	CC_G

#define IA32_MOVZX_R32_RM8_1	0x0F	// opcode part 1
#define IA32_MOVZX_R32_RM16_1	0x0F	// opcode part 1
#define IA32_PUSH_REG			0x50	// encoding is +r
#define IA32_POP_REG			0x58	// encoding is +r
#define IA32_PUSH_IMM32			0x68	// encoding is <imm32>
#define IA32_PUSH_IMM8			0x6A	// encoding is <imm8>
#define IA32_ADD_RM_IMM32		0x81	// encoding is /0
#define IA32_SUB_RM_IMM32		0x81	// encoding is /5 <imm32>
#define IA32_ADD_RM_IMM8		0x83	// encoding is /0
#define IA32_AND_RM_IMM8		0x83	// encoding is /4
#define IA32_SUB_RM_IMM8		0x83	// encoding is /5 <imm8>
#define IA32_MOV_RM8_REG8		0x88	// encoding is /r
#define	IA32_MOV_RM_REG			0x89	// encoding is /r
#define IA32_MOV_REG8_RM8		0x8A	// encoding is /r
#define	IA32_MOV_REG_RM			0x8B	// encoding is /r
#define IA32_MOV_RM_IMM32		0xC7	// encoding is /0
#define IA32_LEA_REG_MEM		0x8D	// encoding is /r
#define IA32_MOVSB				0xA4	// no extra encoding
#define IA32_MOVSD				0xA5	// no extra encoding
#define IA32_MOVZX_R32_RM8_2	0xB6	// encoding is /r
#define IA32_MOVZX_R32_RM16_2	0xB7	// encoding is /r
#define IA32_MOV_REG_IMM		0xB8	// encoding is +r <imm32>
#define IA32_FLD_MEM32			0xD9	// encoding is /0
#define IA32_FSTP_MEM32			0xD9	// encoding is /3
#define IA32_FLD_MEM64			0xDD	// encoding is /0
#define IA32_FSTP_MEM64			0xDD	// encoding is /3
#define IA32_CALL_IMM32			0xE8	// relative call, <imm32>
#define IA32_JMP_IMM32			0xE9	// encoding is imm32
#define IA32_RETN				0xC2	// encoding is <imm16> 
#define IA32_RET				0xC3	// no extra encoding
#define IA32_REP				0xF3	// no extra encoding
#define IA32_CLD				0xFC	// no extra encoding
#define IA32_CALL_RM			0xFF	// encoding is /2

inline jit_uint8_t ia32_modrm(jit_uint8_t mode, jit_uint8_t reg, jit_uint8_t rm)
{
	jit_uint8_t modrm = (mode << 6);

	modrm |= (reg << 3);
	modrm |= (rm);

	return modrm;
}

//mode is the scaling method - NOSCALE ... SCALE8
//index is the register that is scaled
//base is the base register
inline jit_uint8_t ia32_sib(jit_uint8_t mode, jit_uint8_t index, jit_uint8_t base)
{
	jit_uint8_t sib = (mode << 6);

	sib |= (index << 3);
	sib |= (base);

	return sib;
}

inline void IA32_Return(JitWriter *jit)
{
	jit->write_ubyte(IA32_RET);
}

inline void IA32_Mov_Reg_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_MOV_REG_RM);
	jit->write_ubyte(ia32_modrm(mode, dest, src));
}

inline void IA32_Movzx_Reg32_Rm8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_MOVZX_R32_RM8_1);
	jit->write_ubyte(IA32_MOVZX_R32_RM8_2);
	jit->write_ubyte(ia32_modrm(mode, dest, src));
}

inline void IA32_Movzx_Reg32_Rm8_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(IA32_MOVZX_R32_RM8_1);
	jit->write_ubyte(IA32_MOVZX_R32_RM8_2);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest, src));
	jit->write_byte(disp);
}

inline void IA32_Movzx_Reg32_Rm8_Disp32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	jit->write_ubyte(IA32_MOVZX_R32_RM8_1);
	jit->write_ubyte(IA32_MOVZX_R32_RM8_2);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, dest, src));
	jit->write_int32(disp);
}

inline void IA32_Movzx_Reg32_Rm16(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_MOVZX_R32_RM16_1);
	jit->write_ubyte(IA32_MOVZX_R32_RM16_2);
	jit->write_ubyte(ia32_modrm(mode, dest, src));
}

inline void IA32_Movzx_Reg32_Rm16_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(IA32_MOVZX_R32_RM16_1);
	jit->write_ubyte(IA32_MOVZX_R32_RM16_2);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest, src));
	jit->write_byte(disp);
}

inline void IA32_Movzx_Reg32_Rm16_Disp32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	jit->write_ubyte(IA32_MOVZX_R32_RM16_1);
	jit->write_ubyte(IA32_MOVZX_R32_RM16_2);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, dest, src));
	jit->write_int32(disp);
}

inline void IA32_Push_Reg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(IA32_PUSH_REG+reg);
}

inline void IA32_Pop_Reg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(IA32_POP_REG+reg);
}

inline void IA32_Lea_DispRegImm8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src_base, jit_int8_t val)
{
	jit->write_ubyte(IA32_LEA_REG_MEM);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest, src_base));
	jit->write_byte(val);
}

inline void IA32_Lea_DispRegImm32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src_base, jit_int32_t val)
{
	jit->write_ubyte(IA32_LEA_REG_MEM);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, dest, src_base));
	jit->write_int32(val);
}

inline void IA32_Mov_Reg_Rm_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(IA32_MOV_REG_RM);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest, src));
	jit->write_byte(disp);
}

inline void IA32_Mov_Reg_Rm_Disp32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	jit->write_ubyte(IA32_MOV_REG_RM);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, dest, src));
	jit->write_int32(disp);
}

inline void IA32_Sub_Rm_Imm8(JitWriter *jit, jit_uint8_t reg, jit_int8_t val, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_SUB_RM_IMM8);
	jit->write_ubyte(ia32_modrm(mode, 5, reg));
	jit->write_byte(val);
}

inline void IA32_Sub_Rm_Imm32(JitWriter *jit, jit_uint8_t reg, jit_int32_t val, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_SUB_RM_IMM32);
	jit->write_ubyte(ia32_modrm(mode, 5, reg));
	jit->write_int32(val);
}

inline void IA32_Cld(JitWriter *jit)
{
	jit->write_ubyte(IA32_CLD);
}

inline void IA32_Rep(JitWriter *jit)
{
	jit->write_ubyte(IA32_REP);
}

inline void IA32_Movsd(JitWriter *jit)
{
	jit->write_ubyte(IA32_MOVSD);
}

inline void IA32_Movsb(JitWriter *jit)
{
	jit->write_ubyte(IA32_MOVSB);
}

inline jitoffs_t IA32_Call_Imm32(JitWriter *jit, jit_int32_t disp)
{
	jitoffs_t ptr;
	jit->write_ubyte(IA32_CALL_IMM32);
	ptr = jit->get_outputpos();
	jit->write_int32(disp);
	return ptr;
}

inline void IA32_Mov_Rm8_Reg8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_MOV_RM8_REG8);
	jit->write_ubyte(ia32_modrm(mode, src, dest));
}

inline void IA32_Mov_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_MOV_RM_REG);
	jit->write_ubyte(ia32_modrm(mode, src, dest));
}

/**
 * Corrects a jump using an absolute offset, not a relative one.
 */
inline void IA32_Write_Jump32_Abs(JitWriter *jit, jitoffs_t jmp, void *target)
{
	//save old ptr
	jitcode_t oldptr = jit->outptr;
	//get relative difference
	long diff = ((long)target - ((long)jit->outbase + jmp + 4));
	//overwrite old value
	jit->outptr = jit->outbase + jmp;
	jit->write_int32(diff);
	//restore old ptr
	jit->outptr = oldptr;
}

inline void IA32_Fstp_Mem32(JitWriter *jit, jit_uint8_t dest)
{
	jit->write_ubyte(IA32_FSTP_MEM32);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, 3, dest));
}

inline void IA32_Fstp_Mem64(JitWriter *jit, jit_uint8_t dest)
{
	jit->write_ubyte(IA32_FSTP_MEM64);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, 3, dest));
}

inline void IA32_Mov_Rm_Reg_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(IA32_MOV_RM_REG);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, src, dest));
	jit->write_byte(disp);
}

inline void IA32_Call_Reg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(IA32_CALL_RM);
	jit->write_ubyte(ia32_modrm(MOD_REG, 2, reg));
}

inline jitoffs_t IA32_Mov_Reg_Imm32(JitWriter *jit, jit_uint8_t dest, jit_int32_t num)
{
	jitoffs_t offs;
	jit->write_ubyte(IA32_MOV_REG_IMM+dest);
	offs = jit->get_outputpos();
	jit->write_int32(num);
	return offs;
}

inline void IA32_Add_Rm_Imm8(JitWriter *jit, jit_uint8_t reg, jit_int8_t value, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_ADD_RM_IMM8);
	jit->write_ubyte(ia32_modrm(mode, 0, reg));
	jit->write_byte(value);
}

inline void IA32_Add_Rm_Imm32(JitWriter *jit, jit_uint8_t reg, jit_int32_t value, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_ADD_RM_IMM32);
	jit->write_ubyte(ia32_modrm(mode, 0, reg));
	jit->write_int32(value);
}

inline void IA32_And_Rm_Imm8(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode, jit_int8_t value)
{
	jit->write_ubyte(IA32_AND_RM_IMM8);
	jit->write_ubyte(ia32_modrm(mode, 4, reg));
	jit->write_byte(value);
}

inline void IA32_Lea_Reg_DispRegMultImm8(JitWriter *jit, 
							  jit_uint8_t dest, 
							  jit_uint8_t src_base, 
							  jit_uint8_t src_index, 
							  jit_uint8_t scale, 
							  jit_int8_t val)
{
	jit->write_ubyte(IA32_LEA_REG_MEM);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest, kREG_SIB));
	jit->write_ubyte(ia32_sib(scale, src_index, src_base));
	jit->write_byte(val);
}

inline void IA32_Fld_Mem32_Disp8(JitWriter *jit, jit_uint8_t src, jit_int8_t val)
{
	jit->write_ubyte(IA32_FLD_MEM32);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 0, src));
	jit->write_byte(val);
}

inline void IA32_Fld_Mem64_Disp8(JitWriter *jit, jit_uint8_t src, jit_int8_t val)
{
	jit->write_ubyte(IA32_FLD_MEM64);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 0, src));
	jit->write_byte(val);
}

inline void IA32_Fld_Mem32(JitWriter *jit, jit_uint8_t src)
{
	jit->write_ubyte(IA32_FLD_MEM32);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, 0, src));
}

inline void IA32_Fld_Mem64(JitWriter *jit, jit_uint8_t src)
{
	jit->write_ubyte(IA32_FLD_MEM64);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, 0, src));
}

inline void IA32_Fld_Mem32_Disp32(JitWriter *jit, jit_uint8_t src, jit_int32_t val)
{
	jit->write_ubyte(IA32_FLD_MEM32);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, 0, src));
	jit->write_int32(val);
}

inline void IA32_Fld_Mem64_Disp32(JitWriter *jit, jit_uint8_t src, jit_int32_t val)
{
	jit->write_ubyte(IA32_FLD_MEM64);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, 0, src));
	jit->write_int32(val);
}

inline void IA32_Fstp_Mem32_ESP(JitWriter *jit)
{
	jit->write_ubyte(IA32_FSTP_MEM32);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, 3, kREG_SIB));
	jit->write_ubyte(ia32_sib(NOSCALE, kREG_NOIDX, kREG_ESP));
}

inline void IA32_Fstp_Mem64_ESP(JitWriter *jit)
{
	jit->write_ubyte(IA32_FSTP_MEM64);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, 3, kREG_SIB));
	jit->write_ubyte(ia32_sib(NOSCALE, kREG_NOIDX, kREG_ESP));
}

inline void IA32_Return_Popstack(JitWriter *jit, unsigned short bytes)
{
	jit->write_ubyte(IA32_RETN);
	jit->write_ushort(bytes);
}

inline void IA32_Push_Imm8(JitWriter *jit, jit_int8_t val)
{
	jit->write_ubyte(IA32_PUSH_IMM8);
	jit->write_byte(val);
}

inline void IA32_Push_Imm32(JitWriter *jit, jit_int32_t val)
{
	jit->write_ubyte(IA32_PUSH_IMM32);
	jit->write_int32(val);
}

inline void IA32_Mov_Reg8_Rm8_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(IA32_MOV_REG8_RM8);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest, src));
	jit->write_byte(disp);
}

inline jitoffs_t IA32_Jump_Imm32(JitWriter *jit, jit_int32_t disp)
{
	jitoffs_t ptr;
	jit->write_ubyte(IA32_JMP_IMM32);
	ptr = jit->get_outputpos();
	jit->write_int32(disp);
	return ptr;
}

inline void IA32_Mov_ESP_Disp8_Imm32(JitWriter *jit, jit_int8_t disp8, jit_int32_t val)
{
	jit->write_ubyte(IA32_MOV_RM_IMM32);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 0, kREG_SIB));
	jit->write_ubyte(ia32_sib(NOSCALE, kREG_NOIDX, kREG_ESP));
	jit->write_byte(disp8);
	jit->write_int32(val);
}

#endif //_INCLUDE_JIT_X86_MACROS_H

