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
#define REG_EAX			0
#define REG_ECX			1
#define REG_EDX			2
#define REG_EBX			3
#define	REG_ESP			4
#define	REG_SIB			4
#define REG_NOIDX		4
#define REG_IMM_BASE	5
#define REG_EBP			5
#define REG_ESI			6
#define REG_EDI			7

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

//Opcodes with encoding information
#define IA32_XOR_RM_REG			0x31	// encoding is /r
#define IA32_XOR_REG_RM			0x33	// encoding is /r
#define IA32_XOR_EAX_IMM32		0x35	// encoding is /r
#define IA32_XOR_RM_IMM32		0x81	// encoding is /6
#define IA32_XOR_RM_IMM8		0x83	// encoding is /6
#define IA32_ADD_RM_REG			0x01	// encoding is /r
#define IA32_ADD_REG_RM			0x03	// encoding is /r
#define IA32_ADD_RM_IMM32		0x81	// encoding is /0
#define IA32_ADD_RM_IMM8		0x83	// encoding is /0
#define IA32_ADD_EAX_IMM32		0x05	// no extra encoding
#define IA32_SUB_RM_REG			0x29	// encoding is /r
#define IA32_SUB_REG_RM			0x2B	// encoding is /r
#define IA32_SUB_RM_IMM8		0x83	// encoding is /5 <imm8>
#define IA32_SUB_RM_IMM32		0x81	// encoding is /5 <imm32>
#define IA32_SBB_REG_RM			0x1B	// encoding is /r
#define IA32_SBB_RM_IMM8		0x83	// encoding is <imm32>
#define IA32_JMP_IMM32			0xE9	// encoding is imm32
#define IA32_JMP_IMM8			0xEB	// encoding is imm8
#define IA32_JMP_RM				0xFF	// encoding is /4
#define IA32_CALL_IMM32			0xE8	// relative call, <imm32>
#define IA32_CALL_RM			0xFF	// encoding is /2
#define IA32_MOV_REG_IMM		0xB8	// encoding is +r <imm32>
#define	IA32_MOV_RM8_REG		0x88	// encoding is /r
#define	IA32_MOV_RM_REG			0x89	// encoding is /r
#define	IA32_MOV_REG_RM			0x8B	// encoding is /r
#define IA32_MOV_REG8_RM8		0x8A	// encoding is /r
#define IA32_MOV_RM8_REG8		0x88	// encoding is /r
#define IA32_MOV_RM_IMM32		0xC7	// encoding is /0
#define IA32_MOV_EAX_MEM		0xA1	// encoding is <imm32>
#define IA32_CMP_RM_IMM32		0x81	// encoding is /7 <imm32>
#define IA32_CMP_RM_IMM8		0x83	// encoding is /7 <imm8>
#define IA32_CMP_AL_IMM32		0x3C	// no extra encoding
#define IA32_CMP_EAX_IMM32		0x3D	// no extra encoding
#define IA32_CMP_RM_REG			0x39	// encoding is /r
#define IA32_CMP_REG_RM			0x3B	// encoding is /r
#define IA32_CMPSB				0xA6	// no extra encoding
#define IA32_TEST_RM_REG		0x85	// encoding is /r
#define IA32_JCC_IMM			0x70	// encoding is +cc <imm8>
#define IA32_JCC_IMM32_1		0x0F	// opcode part 1
#define IA32_JCC_IMM32_2		0x80	// encoding is +cc <imm32>
#define IA32_RET				0xC3	// no extra encoding
#define IA32_RETN				0xC2	// encoding is <imm16> 
#define IA32_NEG_RM				0xF7	// encoding is /3
#define IA32_INC_REG			0x40	// encoding is +r
#define IA32_INC_RM				0xFF	// encoding is /0
#define IA32_DEC_REG			0x48	// encoding is +r
#define IA32_DEC_RM				0xFF	// encoding is /1
#define IA32_OR_REG_RM			0x0B	// encoding is /r
#define IA32_AND_REG_RM			0x23	// encoding is /r
#define IA32_AND_EAX_IMM32		0x25	// encoding is <imm32>
#define IA32_AND_RM_IMM32		0x81	// encoding is /4
#define IA32_AND_RM_IMM8		0x83	// encoding is /4
#define IA32_NOT_RM				0xF7	// encoding is /2
#define IA32_DIV_RM				0xF7	// encoding is /6
#define IA32_MUL_RM				0xF7	// encoding is /4
#define IA32_IDIV_RM			0xF7	// encoding is /7
#define IA32_IMUL_RM			0xF7	// encoding is /5
#define IA32_IMUL_REG_IMM32		0x69	// encoding is /r
#define IA32_IMUL_REG_IMM8		0x6B	// encoding is /r
#define IA32_IMUL_REG_RM_1		0x0F	// encoding is _2
#define IA32_IMUL_REG_RM_2		0xAF	// encoding is /r
#define IA32_SHR_RM_IMM8		0xC1	// encoding is /5 <ib>
#define IA32_SHR_RM_1			0xD1	// encoding is /5
#define IA32_SHL_RM_IMM8		0xC1	// encoding is /4 <ib>
#define IA32_SHL_RM_1			0xD1	// encoding is /4
#define IA32_SAR_RM_CL			0xD3	// encoding is /7
#define IA32_SAR_RM_1			0xD1	// encoding is /7
#define IA32_SHR_RM_CL			0xD3	// encoding is /5
#define IA32_SHL_RM_CL			0xD3	// encoding is /4
#define IA32_SAR_RM_IMM8		0xC1	// encoding is /7 <ib>
#define IA32_SETCC_RM8_1		0x0F	// opcode part 1
#define IA32_SETCC_RM8_2		0x90	// encoding is +cc /0 (8bits)
#define IA32_CMOVCC_RM_1		0x0F	// opcode part 1
#define IA32_CMOVCC_RM_2		0x40	// encoding is +cc /r
#define IA32_XCHG_EAX_REG		0x90	// encoding is +r
#define IA32_LEA_REG_MEM		0x8D	// encoding is /r
#define IA32_POP_REG			0x58	// encoding is +r
#define IA32_PUSH_REG			0x50	// encoding is +r
#define IA32_PUSH_RM			0xFF	// encoding is /6
#define IA32_PUSH_IMM32			0x68	// encoding is <imm32>
#define IA32_PUSH_IMM8			0x6A	// encoding is <imm8>
#define IA32_REP				0xF3	// no extra encoding
#define IA32_MOVSD				0xA5	// no extra encoding
#define IA32_MOVSB				0xA4	// no extra encoding
#define IA32_STOSD				0xAB	// no extra encoding
#define IA32_CLD				0xFC	// no extra encoding
#define IA32_PUSHAD				0x60	// no extra encoding
#define IA32_POPAD				0x61	// no extra encoding
#define IA32_NOP				0x90	// no extra encoding
#define IA32_INT3				0xCC	// no extra encoding
#define IA32_FSTP_MEM32			0xD9	// encoding is /3
#define IA32_FSTP_MEM64			0xDD	// encoding is /3
#define IA32_FLD_MEM32			0xD9	// encoding is /0
#define IA32_FLD_MEM64			0xDD	// encoding is /0
#define IA32_FILD_MEM32			0xDB	// encoding is /0
#define IA32_FADD_MEM32			0xD8	// encoding is /0
#define IA32_FADD_FPREG_ST0_1	0xDC	// opcode part 1
#define IA32_FADD_FPREG_ST0_2	0xC0	// encoding is +r
#define IA32_FSUB_MEM32			0xD8	// encoding is /4
#define IA32_FMUL_MEM32			0xD8	// encoding is /1
#define IA32_FDIV_MEM32			0xD8	// encoding is /6
#define IA32_FSTCW_MEM16_1		0x9B	// opcode part 1
#define IA32_FSTCW_MEM16_2		0xD9	// encoding is /7
#define IA32_FLDCW_MEM16		0xD9	// encoding is /5
#define IA32_FISTP_MEM32		0xDB	// encoding is /3
#define IA32_FUCOMIP_1			0xDF	// opcode part 1
#define IA32_FUCOMIP_2			0xE8	// encoding is +r
#define IA32_FSTP_FPREG_1		0xDD	// opcode part 1
#define IA32_FSTP_FPREG_2		0xD8	// encoding is +r
#define IA32_MOVZX_R32_RM8_1	0x0F	// opcode part 1
#define IA32_MOVZX_R32_RM8_2	0xB6	// encoding is /r
#define IA32_MOVZX_R32_RM16_1	0x0F	// opcode part 1
#define IA32_MOVZX_R32_RM16_2	0xB7	// encoding is /r

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

/***********************
 * INCREMENT/DECREMENT *
 ***********************/

inline void IA32_Inc_Reg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(IA32_INC_REG+reg);
}

inline void IA32_Inc_Rm_Disp8(JitWriter *jit, jit_uint8_t reg, jit_int8_t disp)
{
	jit->write_ubyte(IA32_INC_RM);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 0, reg));
	jit->write_byte(disp);
}

inline void IA32_Inc_Rm_Disp32(JitWriter *jit, jit_uint8_t reg, jit_int32_t disp)
{
	jit->write_ubyte(IA32_INC_RM);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, 0, reg));
	jit->write_int32(disp);
}

inline void IA32_Inc_Rm_Disp_Reg(JitWriter *jit, jit_uint8_t base, jit_uint8_t reg, jit_uint8_t scale)
{
	jit->write_ubyte(IA32_INC_RM);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, 0, REG_SIB));
	jit->write_ubyte(ia32_sib(scale, reg, base));
}

inline void IA32_Dec_Reg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(IA32_DEC_REG+reg);
}

inline void IA32_Dec_Rm_Disp8(JitWriter *jit, jit_uint8_t reg, jit_int8_t disp)
{
	jit->write_ubyte(IA32_DEC_RM);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 1, reg));
	jit->write_byte(disp);
}

inline void IA32_Dec_Rm_Disp32(JitWriter *jit, jit_uint8_t reg, jit_int32_t disp)
{
	jit->write_ubyte(IA32_DEC_RM);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, 1, reg));
	jit->write_int32(disp);
}

inline void IA32_Dec_Rm_Disp_Reg(JitWriter *jit, jit_uint8_t base, jit_uint8_t reg, jit_uint8_t scale)
{
	jit->write_ubyte(IA32_DEC_RM);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, 1, REG_SIB));
	jit->write_ubyte(ia32_sib(scale, reg, base));
}

/****************
 * BINARY LOGIC *
 ****************/

inline void IA32_Xor_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t dest_mode)
{
	jit->write_ubyte(IA32_XOR_RM_REG);
	jit->write_ubyte(ia32_modrm(dest_mode, src, dest));
}

inline void IA32_Xor_Reg_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t dest_mode)
{
	jit->write_ubyte(IA32_XOR_REG_RM);
	jit->write_ubyte(ia32_modrm(dest_mode, dest, src));
}

inline void IA32_Xor_Rm_Imm8(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode, jit_int8_t value)
{
	jit->write_ubyte(IA32_XOR_RM_IMM8);
	jit->write_ubyte(ia32_modrm(mode, 6, reg));
	jit->write_byte(value);
}

inline void IA32_Xor_Rm_Imm32(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode, jit_int32_t value)
{
	jit->write_ubyte(IA32_XOR_RM_IMM32);
	jit->write_ubyte(ia32_modrm(mode, 6, reg));
	jit->write_int32(value);
}

inline void IA32_Xor_Eax_Imm32(JitWriter *jit, jit_int32_t value)
{
	jit->write_ubyte(IA32_XOR_EAX_IMM32);
	jit->write_int32(value);
}

inline void IA32_Neg_Rm(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_NEG_RM);
	jit->write_ubyte(ia32_modrm(mode, 3, reg));
}

inline void IA32_Or_Reg_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_OR_REG_RM);
	jit->write_ubyte(ia32_modrm(mode, dest, src));
}

inline void IA32_And_Reg_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_AND_REG_RM);
	jit->write_ubyte(ia32_modrm(mode, dest, src));
}

inline void IA32_And_Rm_Imm32(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode, jit_int32_t value)
{
	jit->write_ubyte(IA32_AND_RM_IMM32);
	jit->write_ubyte(ia32_modrm(mode, 4, reg));
	jit->write_int32(value);
}

inline void IA32_And_Rm_Imm8(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode, jit_int8_t value)
{
	jit->write_ubyte(IA32_AND_RM_IMM8);
	jit->write_ubyte(ia32_modrm(mode, 4, reg));
	jit->write_byte(value);
}

inline void IA32_And_Eax_Imm32(JitWriter *jit, jit_int32_t value)
{
	jit->write_ubyte(IA32_AND_EAX_IMM32);
	jit->write_int32(value);
}

inline void IA32_Not_Rm(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_NOT_RM);
	jit->write_ubyte(ia32_modrm(mode, 2, reg));
}

inline void IA32_Shr_Rm_Imm8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t value, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_SHR_RM_IMM8);
	jit->write_ubyte(ia32_modrm(mode, 5, dest));
	jit->write_ubyte(value);
}

inline void IA32_Shr_Rm_1(JitWriter *jit, jit_uint8_t dest, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_SHR_RM_1);
	jit->write_ubyte(ia32_modrm(mode, 5, dest));
}

inline void IA32_Shl_Rm_Imm8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t value, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_SHL_RM_IMM8);
	jit->write_ubyte(ia32_modrm(mode, 4, dest));
	jit->write_ubyte(value);
}

inline void IA32_Shl_Rm_1(JitWriter *jit, jit_uint8_t dest, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_SHL_RM_1);
	jit->write_ubyte(ia32_modrm(mode, 4, dest));
}

inline void IA32_Sar_Rm_Imm8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t value, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_SAR_RM_IMM8);
	jit->write_ubyte(ia32_modrm(mode, 7, dest));
	jit->write_ubyte(value);
}

inline void IA32_Sar_Rm_CL(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_SAR_RM_CL);
	jit->write_ubyte(ia32_modrm(mode, 7, reg));
}

inline void IA32_Sar_Rm_1(JitWriter *jit, jit_uint8_t dest, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_SAR_RM_1);
	jit->write_ubyte(ia32_modrm(mode, 7, dest));
}

inline void IA32_Shr_Rm_CL(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_SHR_RM_CL);
	jit->write_ubyte(ia32_modrm(mode, 5, reg));
}

inline void IA32_Shl_Rm_CL(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_SHL_RM_CL);
	jit->write_ubyte(ia32_modrm(mode, 4, reg));
}

inline void IA32_Xchg_Eax_Reg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(IA32_XCHG_EAX_REG+reg);
}

/**********************
 * ARITHMETIC (BASIC) *
 **********************/

inline void IA32_Add_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_ADD_RM_REG);
	jit->write_ubyte(ia32_modrm(mode, src, dest));
}

inline void IA32_Add_Reg_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_ADD_REG_RM);
	jit->write_ubyte(ia32_modrm(mode, dest, src));
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

inline void IA32_Add_Eax_Imm32(JitWriter *jit, jit_int32_t value)
{
	jit->write_ubyte(IA32_ADD_EAX_IMM32);
	jit->write_int32(value);
}

inline void IA32_Sub_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_SUB_RM_REG);
	jit->write_ubyte(ia32_modrm(mode, src, dest));
}

inline void IA32_Sub_Reg_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_SUB_REG_RM);
	jit->write_ubyte(ia32_modrm(mode, dest, src));
}

inline void IA32_Sub_Reg_Rm_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp8)
{
	jit->write_ubyte(IA32_SUB_REG_RM);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest, src));
	jit->write_byte(disp8);
}

inline void IA32_Sub_Rm_Reg_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp8)
{
	jit->write_ubyte(IA32_SUB_RM_REG);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, src, dest));
	jit->write_byte(disp8);
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

inline void IA32_Sbb_Reg_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_SBB_REG_RM);
	jit->write_ubyte(ia32_modrm(mode, dest, src));
}

inline void IA32_Sbb_Rm_Imm8(JitWriter *jit, jit_uint8_t dest, jit_int8_t value, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_SBB_RM_IMM8);
	jit->write_ubyte(ia32_modrm(mode, 3, dest));
	jit->write_byte(value);
}

inline void IA32_Div_Rm(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_DIV_RM);
	jit->write_ubyte(ia32_modrm(mode, 6, reg));
}

inline void IA32_IDiv_Rm(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_IDIV_RM);
	jit->write_ubyte(ia32_modrm(mode, 7, reg));
}

inline void IA32_Mul_Rm(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_MUL_RM);
	jit->write_ubyte(ia32_modrm(mode, 4, reg));
}

inline void IA32_IMul_Rm(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_IMUL_RM);
	jit->write_ubyte(ia32_modrm(mode, 5, reg));
}

inline void IA32_IMul_Reg_Imm8(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode, jit_int8_t value)
{
	jit->write_ubyte(IA32_IMUL_REG_IMM8);
	jit->write_ubyte(ia32_modrm(mode, 0, reg));
	jit->write_byte(value);
}

inline void IA32_IMul_Reg_Imm32(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode, jit_int32_t value)
{
	jit->write_ubyte(IA32_IMUL_REG_IMM32);
	jit->write_ubyte(ia32_modrm(mode, 0, reg));
	jit->write_int32(value);
}

inline void IA32_IMul_Reg_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_IMUL_REG_RM_1);
	jit->write_ubyte(IA32_IMUL_REG_RM_2);
	jit->write_ubyte(ia32_modrm(mode, dest, src));
}

inline void IA32_Add_Rm_Reg_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(IA32_ADD_RM_REG);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, src, dest));
	jit->write_byte(disp);
}

inline void IA32_Add_Reg_Rm_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(IA32_ADD_REG_RM);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest, src));
	jit->write_byte(disp);
}

inline void IA32_Add_Rm_Imm8_Disp8(JitWriter *jit, 
							 jit_uint8_t dest, 
							 jit_int8_t val, 
							 jit_int8_t disp8)
{
	jit->write_ubyte(IA32_ADD_RM_IMM8);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 0, dest));
	jit->write_byte(disp8);
	jit->write_byte(val);
}

inline void IA32_Add_Rm_Imm32_Disp8(JitWriter *jit, 
							 jit_uint8_t dest, 
							 jit_int32_t val, 
							 jit_int8_t disp8)
{
	jit->write_ubyte(IA32_ADD_RM_IMM32);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 0, dest));
	jit->write_byte(disp8);
	jit->write_int32(val);
}

inline jitoffs_t IA32_Add_Rm_Imm32_Later(JitWriter *jit, 
									jit_uint8_t dest, 
									jit_uint8_t mode)
{
	jit->write_ubyte(IA32_ADD_RM_IMM32);
	jit->write_ubyte(ia32_modrm(mode, 0, dest));
	jitoffs_t ptr = jit->get_outputpos();
	jit->write_int32(0);
	return ptr;
}

inline void IA32_Add_Rm_Imm8_Disp32(JitWriter *jit, 
							jit_uint8_t dest, 
							jit_int8_t val, 
							jit_int32_t disp32)
{
	jit->write_ubyte(IA32_ADD_RM_IMM8);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, 0, dest));
	jit->write_int32(disp32);
	jit->write_byte(val);
}

inline void IA32_Add_RmEBP_Imm8_Disp_Reg(JitWriter *jit, 
								jit_uint8_t dest_base, 
								jit_uint8_t dest_index,
								jit_uint8_t dest_scale,
								jit_int8_t val)
{
	jit->write_ubyte(IA32_ADD_RM_IMM8);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 0, REG_SIB));
	jit->write_ubyte(ia32_sib(dest_scale, dest_index, dest_base));
	jit->write_byte(0);
	jit->write_byte(val);
}

inline void IA32_Sub_Rm_Imm8_Disp8(JitWriter *jit, 
							jit_uint8_t dest, 
							jit_int8_t val, 
							jit_int8_t disp8)
{
	jit->write_ubyte(IA32_SUB_RM_IMM8);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 5, dest));
	jit->write_byte(disp8);
	jit->write_byte(val);
}

inline void IA32_Sub_Rm_Imm8_Disp32(JitWriter *jit, 
							 jit_uint8_t dest, 
							 jit_int8_t val, 
							 jit_int32_t disp32)
{
	jit->write_ubyte(IA32_SUB_RM_IMM8);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, 5, dest));
	jit->write_int32(disp32);
	jit->write_byte(val);
}

inline void IA32_Sub_RmEBP_Imm8_Disp_Reg(JitWriter *jit, 
							   jit_uint8_t dest_base, 
							   jit_uint8_t dest_index,
							   jit_uint8_t dest_scale,
							   jit_int8_t val)
{
	jit->write_ubyte(IA32_SUB_RM_IMM8);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 5, REG_SIB));
	jit->write_ubyte(ia32_sib(dest_scale, dest_index, dest_base));
	jit->write_byte(0);
	jit->write_byte(val);
}

/**
* Memory Instructions
*/

inline void IA32_Lea_Reg_DispRegMult(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src_base, jit_uint8_t src_index, jit_uint8_t scale)
{
	jit->write_ubyte(IA32_LEA_REG_MEM);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, dest, REG_SIB));
	jit->write_ubyte(ia32_sib(scale, src_index, src_base));
}

inline void IA32_Lea_Reg_DispEBPRegMult(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src_base, jit_uint8_t src_index, jit_uint8_t scale)
{
	jit->write_ubyte(IA32_LEA_REG_MEM);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest, REG_SIB));
	jit->write_ubyte(ia32_sib(scale, src_index, src_base));
	jit->write_byte(0);
}

inline void IA32_Lea_Reg_DispRegMultImm8(JitWriter *jit, 
							  jit_uint8_t dest, 
							  jit_uint8_t src_base, 
							  jit_uint8_t src_index, 
							  jit_uint8_t scale, 
							  jit_int8_t val)
{
	jit->write_ubyte(IA32_LEA_REG_MEM);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest, REG_SIB));
	jit->write_ubyte(ia32_sib(scale, src_index, src_base));
	jit->write_byte(val);
}

inline void IA32_Lea_Reg_DispRegMultImm32(JitWriter *jit, 
										 jit_uint8_t dest, 
										 jit_uint8_t src_base, 
										 jit_uint8_t src_index, 
										 jit_uint8_t scale, 
										 jit_int32_t val)
{
	jit->write_ubyte(IA32_LEA_REG_MEM);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, dest, REG_SIB));
	jit->write_ubyte(ia32_sib(scale, src_index, src_base));
	jit->write_int32(val);
}

inline void IA32_Lea_Reg_RegMultImm32(JitWriter *jit, 
										jit_uint8_t dest, 
										jit_uint8_t src_index, 
										jit_uint8_t scale, 
										jit_int32_t val)
{
	jit->write_ubyte(IA32_LEA_REG_MEM);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, dest, REG_SIB));
	jit->write_ubyte(ia32_sib(scale, src_index, REG_IMM_BASE));
	jit->write_int32(val);
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

/**
* Stack Instructions
*/

inline void IA32_Pop_Reg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(IA32_POP_REG+reg);
}

inline void IA32_Push_Reg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(IA32_PUSH_REG+reg);
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

inline void IA32_Pushad(JitWriter *jit)
{
	jit->write_ubyte(IA32_PUSHAD);
}

inline void IA32_Popad(JitWriter *jit)
{
	jit->write_ubyte(IA32_POPAD);
}

inline void IA32_Push_Rm_Disp8(JitWriter *jit, jit_uint8_t reg, jit_int8_t disp8)
{
	jit->write_ubyte(IA32_PUSH_RM);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 6, reg));
	jit->write_byte(disp8);
}

inline void IA32_Push_Rm_Disp8_ESP(JitWriter *jit, jit_int8_t disp8)
{
	jit->write_ubyte(IA32_PUSH_RM);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 6, REG_SIB));
	jit->write_ubyte(ia32_sib(NOSCALE, REG_NOIDX, REG_ESP));
	jit->write_byte(disp8);
}

/**
 * Moving from REGISTER/MEMORY to REGISTER
 */

inline void IA32_Mov_Eax_Mem(JitWriter *jit, jit_uint32_t mem)
{
	jit->write_ubyte(IA32_MOV_EAX_MEM);
	jit->write_uint32(mem);
}

inline void IA32_Mov_Reg_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_MOV_REG_RM);
	jit->write_ubyte(ia32_modrm(mode, dest, src));
}

inline void IA32_Mov_Reg8_Rm8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_MOV_REG8_RM8);
	jit->write_ubyte(ia32_modrm(mode, dest, src));
}

inline void IA32_Mov_Reg_RmESP(JitWriter *jit, jit_uint8_t dest)
{
	jit->write_ubyte(IA32_MOV_REG_RM);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, dest, REG_ESP));
	jit->write_ubyte(ia32_sib(NOSCALE, REG_NOIDX, REG_ESP));
}

inline void IA32_Mov_Reg_Rm_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(IA32_MOV_REG_RM);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest, src));
	jit->write_byte(disp);
}

inline void IA32_Mov_Reg8_Rm8_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(IA32_MOV_REG8_RM8);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest, src));
	jit->write_byte(disp);
}

inline void IA32_Mov_Reg_Esp_Disp8(JitWriter *jit, jit_uint8_t dest, jit_int8_t disp)
{
	jit->write_ubyte(IA32_MOV_REG_RM);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest, REG_SIB));
	jit->write_ubyte(ia32_sib(NOSCALE, REG_NOIDX, REG_ESP));
	jit->write_byte(disp);
}

inline void IA32_Mov_Reg_Rm_Disp32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	jit->write_ubyte(IA32_MOV_REG_RM);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, dest, src));
	jit->write_int32(disp);
}

inline void IA32_Mov_Reg8_Rm8_Disp32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	jit->write_ubyte(IA32_MOV_REG8_RM8);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, dest, src));
	jit->write_int32(disp);
}

inline void IA32_Mov_Reg_Rm_Disp_Reg(JitWriter *jit, 
							jit_uint8_t dest, 
							jit_uint8_t src_base, 
							jit_uint8_t src_index,
							jit_uint8_t src_scale)
{
	jit->write_ubyte(IA32_MOV_REG_RM);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, dest, REG_SIB));
	jit->write_ubyte(ia32_sib(src_scale, src_index, src_base));
}

inline void IA32_Mov_Reg_Rm_Disp_Reg_Disp8(JitWriter *jit, 
									 jit_uint8_t dest, 
									 jit_uint8_t src_base, 
									 jit_uint8_t src_index,
									 jit_uint8_t src_scale,
									 jit_int8_t disp8)
{
	jit->write_ubyte(IA32_MOV_REG_RM);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest, REG_SIB));
	jit->write_ubyte(ia32_sib(src_scale, src_index, src_base));
	jit->write_byte(disp8);
}

inline void IA32_Mov_Reg_RmEBP_Disp_Reg(JitWriter *jit, 
									 jit_uint8_t dest, 
									 jit_uint8_t src_base, 
									 jit_uint8_t src_index,
									 jit_uint8_t src_scale)
{
	jit->write_ubyte(IA32_MOV_REG_RM);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, dest, REG_SIB));
	jit->write_ubyte(ia32_sib(src_scale, src_index, src_base));
	jit->write_byte(0);
}

/**
 * Moving from REGISTER to REGISTER/MEMORY
 */

inline void IA32_Mov_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_MOV_RM_REG);
	jit->write_ubyte(ia32_modrm(mode, src, dest));
}

inline void IA32_Mov_Rm8_Reg8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_MOV_RM8_REG8);
	jit->write_ubyte(ia32_modrm(mode, src, dest));
}

inline void IA32_Mov_RmESP_Reg(JitWriter *jit, jit_uint8_t src)
{
	jit->write_ubyte(IA32_MOV_RM_REG);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, src, REG_ESP));
	jit->write_ubyte(ia32_sib(NOSCALE, REG_NOIDX, REG_ESP));
}

inline void IA32_Mov_Rm_Reg_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(IA32_MOV_RM_REG);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, src, dest));
	jit->write_byte(disp);
}

inline void IA32_Mov_Rm_Reg_Disp32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	jit->write_ubyte(IA32_MOV_RM_REG);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, src, dest));
	jit->write_int32(disp);
}

inline void IA32_Mov_RmEBP_Reg_Disp_Reg(JitWriter *jit, 
							  jit_uint8_t dest_base, 
							  jit_uint8_t dest_index,
							  jit_uint8_t dest_scale,
							  jit_uint8_t src)
{
	jit->write_ubyte(IA32_MOV_RM_REG);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, src, REG_SIB));
	jit->write_ubyte(ia32_sib(dest_scale, dest_index, dest_base));
	jit->write_byte(0);
}

inline void IA32_Mov_Rm8EBP_Reg_Disp_Reg(JitWriter *jit, 
									 jit_uint8_t dest_base, 
									 jit_uint8_t dest_index,
									 jit_uint8_t dest_scale,
									 jit_uint8_t src)
{
	jit->write_ubyte(IA32_MOV_RM8_REG);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, src, REG_SIB));
	jit->write_ubyte(ia32_sib(dest_scale, dest_index, dest_base));
	jit->write_byte(0);
}

inline void IA32_Mov_Rm16EBP_Reg_Disp_Reg(JitWriter *jit, 
									  jit_uint8_t dest_base, 
									  jit_uint8_t dest_index,
									  jit_uint8_t dest_scale,
									  jit_uint8_t src)
{
	jit->write_ubyte(IA32_16BIT_PREFIX);
	jit->write_ubyte(IA32_MOV_RM_REG);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, src, REG_SIB));
	jit->write_ubyte(ia32_sib(dest_scale, dest_index, dest_base));
	jit->write_byte(0);
}

/**
 * Moving from IMMEDIATE to REGISTER
 */

inline jitoffs_t IA32_Mov_Reg_Imm32(JitWriter *jit, jit_uint8_t dest, jit_int32_t num)
{
	jitoffs_t offs;
	jit->write_ubyte(IA32_MOV_REG_IMM+dest);
	offs = jit->get_outputpos();
	jit->write_int32(num);
	return offs;
}

inline void IA32_Mov_Rm_Imm32(JitWriter *jit, jit_uint8_t dest, jit_int32_t val, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_MOV_RM_IMM32);
	jit->write_ubyte(ia32_modrm(mode, 0, dest));
	jit->write_int32(val);
}

inline void IA32_Mov_Rm_Imm32_Disp8(JitWriter *jit, 
							 jit_uint8_t dest, 
							 jit_int32_t val, 
							 jit_int8_t disp8)
{
	jit->write_ubyte(IA32_MOV_RM_IMM32);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 0, dest));
	jit->write_byte(disp8);
	jit->write_int32(val);
}

inline void IA32_Mov_Rm_Imm32_Disp32(JitWriter *jit, 
							 jit_uint8_t dest, 
							 jit_int32_t val, 
							 jit_int32_t disp32)
{
	jit->write_ubyte(IA32_MOV_RM_IMM32);
	jit->write_ubyte(ia32_modrm(MOD_DISP32, 0, dest));
	jit->write_int32(disp32);
	jit->write_int32(val);
}

inline void IA32_Mov_Rm_Imm32_SIB(JitWriter *jit, 
								  jit_uint8_t dest, 
								  jit_int32_t val, 
								  jit_int32_t disp,
								  jit_uint8_t index,
								  jit_uint8_t scale)
{
	jit->write_ubyte(IA32_MOV_RM_IMM32);

	if (disp >= SCHAR_MIN && disp <= SCHAR_MAX)
	{
		jit->write_ubyte(ia32_modrm(MOD_DISP8, 0, REG_SIB));
	}
	else
	{
		jit->write_ubyte(ia32_modrm(MOD_DISP32, 0, REG_SIB));
	}

	jit->write_ubyte(ia32_sib(scale, index, dest));

	if (disp >= SCHAR_MIN && disp <= SCHAR_MAX)
	{
		jit->write_byte((jit_int8_t)disp);
	}
	else
	{
		jit->write_int32(disp);
	}

	jit->write_int32(val);
}

inline void IA32_Mov_RmEBP_Imm32_Disp_Reg(JitWriter *jit, 
								jit_uint8_t dest_base, 
								jit_uint8_t dest_index, 
								jit_uint8_t dest_scale, 
								jit_int32_t val)
{
	jit->write_ubyte(IA32_MOV_RM_IMM32);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 0, REG_SIB));
	jit->write_ubyte(ia32_sib(dest_scale, dest_index, dest_base));
	jit->write_byte(0);
	jit->write_int32(val);
}

inline void IA32_Mov_ESP_Disp8_Imm32(JitWriter *jit, jit_int8_t disp8, jit_int32_t val)
{
	jit->write_ubyte(IA32_MOV_RM_IMM32);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 0, REG_SIB));
	jit->write_ubyte(ia32_sib(NOSCALE, REG_NOIDX, REG_ESP));
	jit->write_byte(disp8);
	jit->write_int32(val);
}

/**
* Floating Point Instructions
*/

inline void IA32_Fstcw_Mem16_ESP(JitWriter *jit)
{
	jit->write_ubyte(IA32_FSTCW_MEM16_1);
	jit->write_ubyte(IA32_FSTCW_MEM16_2);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, 7, REG_SIB));
	jit->write_ubyte(ia32_sib(NOSCALE, REG_NOIDX, REG_ESP));
}

inline void IA32_Fldcw_Mem16_ESP(JitWriter *jit)
{
	jit->write_ubyte(IA32_FLDCW_MEM16);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, 5, REG_SIB));
	jit->write_ubyte(ia32_sib(NOSCALE, REG_NOIDX, REG_ESP));
}

inline void IA32_Fldcw_Mem16_Disp8_ESP(JitWriter *jit, jit_int8_t disp8)
{
	jit->write_ubyte(IA32_FLDCW_MEM16);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 5, REG_SIB));
	jit->write_ubyte(ia32_sib(NOSCALE, REG_NOIDX, REG_ESP));
	jit->write_byte(disp8);
}

inline void IA32_Fistp_Mem32_ESP(JitWriter *jit)
{
	jit->write_ubyte(IA32_FISTP_MEM32);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, 3, REG_SIB));
	jit->write_ubyte(ia32_sib(NOSCALE, REG_NOIDX, REG_ESP));
}

inline void IA32_Fistp_Mem32_Disp8_Esp(JitWriter *jit, jit_int8_t disp8)
{
	jit->write_ubyte(IA32_FISTP_MEM32);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 3, REG_SIB));
	jit->write_ubyte(ia32_sib(NOSCALE, REG_NOIDX, REG_ESP));
	jit->write_byte(disp8);
}

inline void IA32_Fucomip_ST0_FPUreg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(IA32_FUCOMIP_1);
	jit->write_ubyte(IA32_FUCOMIP_2+reg);
}

inline void IA32_Fadd_FPUreg_ST0(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(IA32_FADD_FPREG_ST0_1);
	jit->write_ubyte(IA32_FADD_FPREG_ST0_2+reg);
}

inline void IA32_Fadd_Mem32_Disp8(JitWriter *jit, jit_uint8_t src, jit_int8_t val)
{
	jit->write_ubyte(IA32_FADD_MEM32);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 0, src));
	jit->write_byte(val);
}

inline void IA32_Fadd_Mem32_ESP(JitWriter *jit)
{
	jit->write_ubyte(IA32_FADD_MEM32);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, 0, REG_SIB));
	jit->write_ubyte(ia32_sib(NOSCALE, REG_NOIDX, REG_ESP));
}

inline void IA32_Fsub_Mem32_Disp8(JitWriter *jit, jit_uint8_t src, jit_int8_t val)
{
	jit->write_ubyte(IA32_FSUB_MEM32);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 4, src));
	jit->write_byte(val);
}

inline void IA32_Fmul_Mem32_Disp8(JitWriter *jit, jit_uint8_t src, jit_int8_t val)
{
	jit->write_ubyte(IA32_FMUL_MEM32);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 1, src));
	jit->write_byte(val);
}

inline void IA32_Fdiv_Mem32_Disp8(JitWriter *jit, jit_uint8_t src, jit_int8_t val)
{
	jit->write_ubyte(IA32_FDIV_MEM32);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 6, src));
	jit->write_byte(val);
}

inline void IA32_Fild_Mem32(JitWriter *jit, jit_uint8_t src)
{
	jit->write_ubyte(IA32_FILD_MEM32);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, 0, src));
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

inline void IA32_Fstp_Mem32_ESP(JitWriter *jit)
{
	jit->write_ubyte(IA32_FSTP_MEM32);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, 3, REG_SIB));
	jit->write_ubyte(ia32_sib(NOSCALE, REG_NOIDX, REG_ESP));
}

inline void IA32_Fstp_Mem64_ESP(JitWriter *jit)
{
	jit->write_ubyte(IA32_FSTP_MEM64);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, 3, REG_SIB));
	jit->write_ubyte(ia32_sib(NOSCALE, REG_NOIDX, REG_ESP));
}

inline void IA32_Fstp_FPUreg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(IA32_FSTP_FPREG_1);
	jit->write_ubyte(IA32_FSTP_FPREG_2+reg);
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

/**
* Move data with zero extend
*/

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

/**
 * Branching/Jumping
 */

inline jitoffs_t IA32_Jump_Cond_Imm8(JitWriter *jit, jit_uint8_t cond, jit_int8_t disp)
{
	jitoffs_t ptr;
	jit->write_ubyte(IA32_JCC_IMM+cond);
	ptr = jit->get_outputpos();
	jit->write_byte(disp);
	return ptr;
}

inline jitoffs_t IA32_Jump_Imm32(JitWriter *jit, jit_int32_t disp)
{
	jitoffs_t ptr;
	jit->write_ubyte(IA32_JMP_IMM32);
	ptr = jit->get_outputpos();
	jit->write_int32(disp);
	return ptr;
}

inline jitoffs_t IA32_Jump_Imm8(JitWriter *jit, jit_int8_t disp)
{
	jitoffs_t ptr;
	jit->write_ubyte(IA32_JMP_IMM8);
	ptr = jit->get_outputpos();
	jit->write_byte(disp);
	return ptr;
}

inline jitoffs_t IA32_Jump_Cond_Imm32(JitWriter *jit, jit_uint8_t cond, jit_int32_t disp)
{
	jitoffs_t ptr;
	jit->write_ubyte(IA32_JCC_IMM32_1);
	jit->write_ubyte(IA32_JCC_IMM32_2+cond);
	ptr = jit->get_outputpos();
	jit->write_int32(disp);
	return ptr;
}

inline void IA32_Jump_Reg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(IA32_JMP_RM);
	jit->write_ubyte(ia32_modrm(MOD_REG, 4, reg));
}

inline void IA32_Jump_Rm(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_JMP_RM);
	jit->write_ubyte(ia32_modrm(mode, 4, reg));
}

inline jitoffs_t IA32_Call_Imm32(JitWriter *jit, jit_int32_t disp)
{
	jitoffs_t ptr;
	jit->write_ubyte(IA32_CALL_IMM32);
	ptr = jit->get_outputpos();
	jit->write_int32(disp);
	return ptr;
}

inline void IA32_Call_Reg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(IA32_CALL_RM);
	jit->write_ubyte(ia32_modrm(MOD_REG, 2, reg));
}

inline void IA32_Write_Jump8(JitWriter *jit, jitoffs_t jmp, jitoffs_t target)
{
	//save old ptr
	jitcode_t oldptr = jit->outptr;
	//get relative difference
	jit_int8_t diff = (target - (jmp + 1));
	//overwrite old value
	jit->outptr = jit->outbase + jmp;
	jit->write_byte(diff);
	//restore old ptr
	jit->outptr = oldptr;
}

inline void IA32_Write_Jump32(JitWriter *jit, jitoffs_t jmp, jitoffs_t target)
{
	//save old ptr
	jitcode_t oldptr = jit->outptr;
	//get relative difference
	jit_int32_t diff = (target - (jmp + 4));
	//overwrite old value
	jit->outptr = jit->outbase + jmp;
	jit->write_int32(diff);
	//restore old ptr
	jit->outptr = oldptr;
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

/* For writing and auto-calculating a relative target */
inline void IA32_Jump_Imm32_Rel(JitWriter *jit, jitoffs_t target)
{
	jit->write_ubyte(IA32_JMP_IMM32);
	IA32_Write_Jump32(jit, jit->get_outputpos(), target);
	jit->outptr += 4;
}

/* For writing and auto-calculating an absolute target */
inline void IA32_Jump_Imm32_Abs(JitWriter *jit, void *target)
{
	jitoffs_t jmp;

	jmp = IA32_Jump_Imm32(jit, 0);
	IA32_Write_Jump32_Abs(jit, jmp, target);
}

inline void IA32_Jump_Cond_Imm32_Rel(JitWriter *jit, jit_uint8_t cond, jitoffs_t target)
{
	jit->write_ubyte(IA32_JCC_IMM32_1);
	jit->write_ubyte(IA32_JCC_IMM32_2+cond);
	IA32_Write_Jump32(jit, jit->get_outputpos(), target);
	jit->outptr += 4;
}

inline void IA32_Jump_Cond_Imm32_Abs(JitWriter *jit, jit_uint8_t cond, void *target)
{
	jit->write_ubyte(IA32_JCC_IMM32_1);
	jit->write_ubyte(IA32_JCC_IMM32_2+cond);
	IA32_Write_Jump32_Abs(jit, jit->get_outputpos(), target);
	jit->outptr += 4;
}

inline void IA32_Send_Jump8_Here(JitWriter *jit, jitoffs_t jmp)
{
	jitoffs_t curptr = jit->get_outputpos();
	IA32_Write_Jump8(jit, jmp, curptr);
}

inline void IA32_Send_Jump32_Here(JitWriter *jit, jitoffs_t jmp)
{
	jitoffs_t curptr = jit->get_outputpos();
	IA32_Write_Jump32(jit, jmp, curptr);
}

inline void IA32_Return(JitWriter *jit)
{
	jit->write_ubyte(IA32_RET);
}

inline void IA32_Return_Popstack(JitWriter *jit, unsigned short bytes)
{
	jit->write_ubyte(IA32_RETN);
	jit->write_ushort(bytes);
}

inline void IA32_Test_Rm_Reg(JitWriter *jit, jit_uint8_t reg1, jit_uint8_t reg2, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_TEST_RM_REG);
	jit->write_ubyte(ia32_modrm(mode, reg2, reg1));
}

inline void IA32_Cmp_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_CMP_RM_REG);
	jit->write_ubyte(ia32_modrm(mode, src, dest));
}

inline void IA32_Cmp_Reg_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(IA32_CMP_REG_RM);
	jit->write_ubyte(ia32_modrm(mode, dest, src));
}

inline void IA32_Cmp_Reg_Rm_ESP(JitWriter *jit, jit_uint8_t cmpreg)
{
	jit->write_ubyte(IA32_CMP_REG_RM);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, cmpreg, REG_SIB));
	jit->write_ubyte(ia32_sib(NOSCALE, REG_NOIDX, REG_ESP));
}

inline void IA32_Cmp_Reg_Rm_Disp8(JitWriter *jit, jit_uint8_t reg1, jit_uint8_t reg2, jit_int8_t disp8)
{
	jit->write_ubyte(IA32_CMP_REG_RM);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, reg1, reg2));
	jit->write_byte(disp8);
}

inline void IA32_Cmp_Rm_Imm8(JitWriter *jit, jit_uint8_t mode, jit_uint8_t rm, jit_int8_t imm8)
{
	jit->write_ubyte(IA32_CMP_RM_IMM8);
	jit->write_ubyte(ia32_modrm(mode, 7, rm));
	jit->write_byte(imm8);
}

inline void IA32_Cmp_Rm_Imm32(JitWriter *jit, jit_uint8_t mode, jit_uint8_t rm, jit_int32_t imm32)
{
	jit->write_ubyte(IA32_CMP_RM_IMM32);
	jit->write_ubyte(ia32_modrm(mode, 7, rm));
	jit->write_int32(imm32);
}

inline void IA32_Cmp_Rm_Imm32_Disp8(JitWriter *jit, jit_uint8_t reg, jit_int8_t disp8, jit_int32_t imm32)
{
	jit->write_ubyte(IA32_CMP_RM_IMM32);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 7, reg));
	jit->write_byte(disp8);
	jit->write_int32(imm32);
}

inline void IA32_Cmp_Rm_Disp8_Imm8(JitWriter *jit, jit_uint8_t reg, jit_int8_t disp, jit_int8_t imm8)
{
	jit->write_ubyte(IA32_CMP_RM_IMM8);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 7, reg));
	jit->write_byte(disp);
	jit->write_byte(imm8);
}

inline void IA32_Cmp_Al_Imm8(JitWriter *jit, jit_int8_t value)
{
	jit->write_ubyte(IA32_CMP_AL_IMM32);
	jit->write_byte(value);
}

inline void IA32_Cmp_Eax_Imm32(JitWriter *jit, jit_int32_t value)
{
	jit->write_ubyte(IA32_CMP_EAX_IMM32);
	jit->write_int32(value);
}

inline void IA32_SetCC_Rm8(JitWriter *jit, jit_uint8_t reg, jit_uint8_t cond)
{
	jit->write_ubyte(IA32_SETCC_RM8_1);
	jit->write_ubyte(IA32_SETCC_RM8_2+cond);
	jit->write_ubyte(ia32_modrm(MOD_REG, 0, reg));
}

inline void IA32_CmovCC_Rm(JitWriter *jit, jit_uint8_t src, jit_uint8_t cond)
{
	jit->write_ubyte(IA32_CMOVCC_RM_1);
	jit->write_ubyte(IA32_CMOVCC_RM_2+cond);
	jit->write_ubyte(ia32_modrm(MOD_MEM_REG, 0, src));
}

inline void IA32_CmovCC_Rm_Disp8(JitWriter *jit, jit_uint8_t src, jit_uint8_t cond, jit_int8_t disp)
{
	jit->write_ubyte(IA32_CMOVCC_RM_1);
	jit->write_ubyte(IA32_CMOVCC_RM_2+cond);
	jit->write_ubyte(ia32_modrm(MOD_DISP8, 0, src));
	jit->write_byte(disp);
}

inline void IA32_Cmpsb(JitWriter *jit)
{
	jit->write_ubyte(IA32_CMPSB);
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

inline void IA32_Stosd(JitWriter *jit)
{
	jit->write_ubyte(IA32_STOSD);
}

inline void IA32_Cld(JitWriter *jit)
{
	jit->write_ubyte(IA32_CLD);
}

#endif //_INCLUDE_JIT_X86_MACROS_H
