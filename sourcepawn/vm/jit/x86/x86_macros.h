#ifndef _INCLUDE_JIT_X86_MACROS_H
#define _INCLUDE_JIT_X86_MACROS_H

#include "..\jit_helpers.h"

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
#define REG_EAX		0
#define REG_ECX		1
#define REG_EDX		2
#define REG_EBX		3
#define	REG_ESP		4
#define	REG_SIB		4
#define REG_NOIDX	4
#define REG_EBP		5
#define REG_ESI		6
#define REG_EDI		7

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
#define IA32_XOR_EAX_IMM32		0x35	// encoding is /r
#define IA32_XOR_RM_IMM32		0x81	// encoding is /6
#define IA32_XOR_RM_IMM8		0x83	// encoding is /6
#define IA32_ADD_RM_REG			0x01	// encoding is /r
#define IA32_ADD_RM_IMM32		0x81	// encoding is /0
#define IA32_ADD_RM_IMM8		0x83	// encoding is /0
#define IA32_ADD_EAX_IMM32		0x05	// no extra encoding
#define IA32_SUB_RM_REG			0x29	// encoding is /r
#define IA32_SUB_RM_IMM8		0x83	// encoding is /5 <imm8>
#define IA32_SUB_RM_IMM32		0x81	// encoding is /5 <imm32>
#define IA32_JMP_IMM32			0xE9	// encoding is imm32
#define IA32_CALL_IMM32			0xE8	// relative call, <imm32>
#define IA32_MOV_REG_IMM		0xB8	// encoding is +r <imm32>
#define	IA32_MOV_RM_REG			0x89	// encoding is /r
#define	IA32_MOV_REG_MEM		0x8B	// encoding is /r
#define IA32_MOV_RM_IMM32		0xC7	// encoding is /0
#define IA32_CMP_RM_IMM32		0x81	// encoding is /7 <imm32>
#define IA32_CMP_RM_IMM8		0x83	// encoding is /7 <imm8>
#define IA32_CMP_EAX_IMM32		0x3D	// no extra encoding
#define IA32_TEST_RM_REG		0x85	// encoding is /r
#define IA32_JCC_IMM			0x70	// encoding is +cc <imm8>
#define IA32_JCC_IMM32_1		0x0F	// opcode part 1
#define IA32_JCC_IMM32_2		0x80	// encoding is +cc <imm32>
#define IA32_RET				0xC3	// no extra encoding
#define IA32_NEG_RM				0xF7	// encoding is /3
#define IA32_INC_REG			0x40	// encoding is +r
#define IA32_INC_RM				0xFF	// encoding is /0
#define IA32_DEC_REG			0x48	// encoding is +r
#define IA32_DEC_RM				0xFF	// encoding is /1
#define IA32_OR_RM_REG			0x09	// encoding is /r
#define IA32_AND_RM_REG			0x21	// encoding is /r
#define IA32_NOT_RM				0xF7	// encoding is /2
#define IA32_DIV_RM				0xF7	// encoding is /6
#define IA32_MUL_RM				0xF7	// encoding is /4
#define IA32_IMUL_RM			0xF7	// encoding is /5
#define IA32_IDIV_RM			0xF7	// encoding is /7
#define IA32_IMUL_REG_IMM32		0x69	// encoding is /r
#define IA32_IMUL_REG_IMM8		0x6B	// encoding is /r
#define IA32_SHR_RM_IMM8		0xC1	// encoding is /5 <ib>
#define IA32_SHL_RM_IMM8		0xC1	// encoding is /4 <ib>
#define IA32_SAR_RM_CL			0xD3	// encoding is /7
#define IA32_SHR_RM_CL			0xD3	// encoding is /5
#define IA32_SHL_RM_CL			0xD3	// encoding is /4
#define IA32_SAR_RM_IMM8		0xC1	// encoding is /7 <ib>
#define IA32_CMP_RM_REG			0x39	// encoding is /r
#define IA32_SETCC_RM8_1		0x0F	// opcode part 1
#define IA32_SETCC_RM8_2		0x90	// encoding is +cc /0 (8bits)
#define IA32_XCHG_EAX_REG		0x90	// encoding is +r
#define IA32_XCHG_RM_REG		0x87	// encoding is /r
#define IA32_LEA_REG_MEM		0x8D	// encoding is /r
#define IA32_POP_REG			0x58	// encoding is +r
#define IA32_PUSH_REG			0x50	// encoding is +r
#define IA32_REP				0xF3	// no extra encoding
#define IA32_MOVSD				0xA5	// no extra encoding
#define IA32_MOVSB				0xA4	// no extra encoding
#define IA32_STOSD				0xAB	// no extra encoding
#define IA32_CLD				0xFC	// no extra encoding

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

inline void IA32_Inc_Rm_Disp32(JitWriter *jit, jit_uint8_t reg, jit_int32_t disp)
{
	jit->write_ubyte(jit, IA32_INC_RM);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP32, 0, reg));
	jit->write_int32(jit, disp);
}

inline void IA32_Inc_Rm_Disp8(JitWriter *jit, jit_uint8_t reg, jit_int8_t disp)
{
	jit->write_ubyte(jit, IA32_INC_RM);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP8, 0, reg));
	jit->write_byte(jit, disp);
}

inline void IA32_Inc_Rm_Disp_Reg(JitWriter *jit, jit_uint8_t base, jit_uint8_t reg, jit_uint8_t scale)
{
	jit->write_ubyte(jit, IA32_INC_RM);
	jit->write_ubyte(jit, ia32_modrm(MOD_MEM_REG, 0, REG_SIB));
	jit->write_ubyte(jit, ia32_sib(scale, reg, base));
}

inline void IA32_Inc_Reg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(jit, IA32_INC_REG+reg);
}

inline void IA32_Dec_Rm_Disp32(JitWriter *jit, jit_uint8_t reg, jit_int32_t disp)
{
	jit->write_ubyte(jit, IA32_DEC_RM);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP32, 1, reg));
	jit->write_int32(jit, disp);
}

inline void IA32_Dec_Rm_Disp8(JitWriter *jit, jit_uint8_t reg, jit_int8_t disp)
{
	jit->write_ubyte(jit, IA32_DEC_RM);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP8, 1, reg));
	jit->write_byte(jit, disp);
}

inline void IA32_Dec_Rm_Disp_Reg(JitWriter *jit, jit_uint8_t base, jit_uint8_t reg, jit_uint8_t scale)
{
	jit->write_ubyte(jit, IA32_DEC_RM);
	jit->write_ubyte(jit, ia32_modrm(MOD_MEM_REG, 1, REG_SIB));
	jit->write_ubyte(jit, ia32_sib(scale, reg, base));
}

inline void IA32_Dec_Reg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(jit, IA32_DEC_REG+reg);
}

/****************
 * BINARY LOGIC *
 ****************/

inline void IA32_Xor_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t dest_mode)
{
	jit->write_ubyte(jit, IA32_XOR_RM_REG);
	jit->write_ubyte(jit, ia32_modrm(dest_mode, src, dest));
}

inline void IA32_Xor_Eax_Imm32(JitWriter *jit, jit_int32_t value)
{
	jit->write_ubyte(jit, IA32_XOR_EAX_IMM32);
	jit->write_int32(jit, value);
}

inline void IA32_Xor_Rm_Imm32(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode, jit_int32_t value)
{
	jit->write_ubyte(jit, IA32_XOR_RM_IMM32);
	jit->write_ubyte(jit, ia32_modrm(mode, 6, reg));
	jit->write_int32(jit, value);
}

inline void IA32_Xor_Rm_Imm8(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode, jit_int8_t value)
{
	jit->write_ubyte(jit, IA32_XOR_RM_IMM8);
	jit->write_ubyte(jit, ia32_modrm(mode, 6, reg));
	jit->write_byte(jit, value);
}

inline void IA32_Neg_Rm(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_NEG_RM);
	jit->write_ubyte(jit, ia32_modrm(mode, 3, reg));
}

inline void IA32_Or_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_OR_RM_REG);
	jit->write_ubyte(jit, ia32_modrm(mode, src, dest));
}

inline void IA32_And_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_AND_RM_REG);
	jit->write_ubyte(jit, ia32_modrm(mode, src, dest));
}

inline void IA32_Not_Rm(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_NOT_RM);
	jit->write_ubyte(jit, ia32_modrm(mode, 2, reg));
}

inline void IA32_Shr_Rm_Imm8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t value, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_SHR_RM_IMM8);
	jit->write_ubyte(jit, ia32_modrm(mode, 5, dest));
	jit->write_ubyte(jit, value);
}

inline void IA32_Shl_Rm_Imm8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t value, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_SHL_RM_IMM8);
	jit->write_ubyte(jit, ia32_modrm(mode, 4, dest));
	jit->write_ubyte(jit, value);
}

inline void IA32_Sar_Rm_Imm8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t value, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_SAR_RM_IMM8);
	jit->write_ubyte(jit, ia32_modrm(mode, 7, dest));
	jit->write_ubyte(jit, value);
}

inline void IA32_Sar_Rm_CL(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_SAR_RM_CL);
	jit->write_ubyte(jit, ia32_modrm(mode, 7, reg));
}

inline void IA32_Shr_Rm_CL(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_SHR_RM_CL);
	jit->write_ubyte(jit, ia32_modrm(mode, 5, reg));
}

inline void IA32_Shl_Rm_CL(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_SHL_RM_CL);
	jit->write_ubyte(jit, ia32_modrm(mode, 4, reg));
}

inline void IA32_Xchg_Eax_Reg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(jit, IA32_XCHG_EAX_REG+reg);
}

inline void IA32_Xchg_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_XCHG_RM_REG);
	jit->write_ubyte(jit, ia32_modrm(mode, src, dest));
}

/**********************
 * ARITHMETIC (BASIC) *
 **********************/

inline void IA32_Add_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_ADD_RM_REG);
	jit->write_ubyte(jit, ia32_modrm(mode, src, dest));
}

inline void IA32_Add_Rm_Imm8(JitWriter *jit, jit_uint8_t reg, jit_int8_t value, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_ADD_RM_IMM8);
	jit->write_ubyte(jit, ia32_modrm(mode, 0, reg));
	jit->write_byte(jit, value);
}

inline void IA32_Add_Rm_Imm32(JitWriter *jit, jit_uint8_t reg, jit_int32_t value, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_ADD_RM_IMM32);
	jit->write_ubyte(jit, ia32_modrm(mode, 0, reg));
	jit->write_int32(jit, value);
}

inline void IA32_Add_Eax_Imm32(JitWriter *jit, jit_int32_t value)
{
	jit->write_ubyte(jit, IA32_ADD_EAX_IMM32);
	jit->write_int32(jit, value);
}

inline void IA32_Sub_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_SUB_RM_REG);
	jit->write_ubyte(jit, ia32_modrm(mode, src, dest));
}

inline void IA32_Sub_Rm_Imm8(JitWriter *jit, jit_uint8_t reg, jit_int8_t val, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_SUB_RM_IMM8);
	jit->write_ubyte(jit, ia32_modrm(mode, 5, reg));
	jit->write_byte(jit, val);
}

inline void IA32_Sub_Rm_Imm32(JitWriter *jit, jit_uint8_t reg, jit_int32_t val, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_SUB_RM_IMM32);
	jit->write_ubyte(jit, ia32_modrm(mode, 5, reg));
	jit->write_int32(jit, val);
}

inline void IA32_Div_Rm(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_DIV_RM);
	jit->write_ubyte(jit, ia32_modrm(mode, 6, reg));
}

inline void IA32_IDiv_Rm(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_IDIV_RM);
	jit->write_ubyte(jit, ia32_modrm(mode, 7, reg));
}

inline void IA32_Mul_Rm(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_MUL_RM);
	jit->write_ubyte(jit, ia32_modrm(mode, 4, reg));
}

inline void IA32_IMul_Rm(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_IMUL_RM);
	jit->write_ubyte(jit, ia32_modrm(mode, 5, reg));
}

inline void IA32_IMul_Reg_Imm8(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode, jit_int8_t value)
{
	jit->write_ubyte(jit, IA32_IMUL_REG_IMM8);
	jit->write_ubyte(jit, ia32_modrm(mode, 0, reg));
	jit->write_byte(jit, value);
}

inline void IA32_IMul_Reg_Imm32(JitWriter *jit, jit_uint8_t reg, jit_uint8_t mode, jit_int32_t value)
{
	jit->write_ubyte(jit, IA32_IMUL_REG_IMM32);
	jit->write_ubyte(jit, ia32_modrm(mode, 0, reg));
	jit->write_int32(jit, value);
}

inline void IA32_Add_Rm_Reg_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(jit, IA32_ADD_RM_REG);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP8, src, dest));
	jit->write_byte(jit, disp);
}

inline void IA32_Add_Rm_Imm8_Disp8(JitWriter *jit, 
							 jit_uint8_t dest, 
							 jit_int8_t val, 
							 jit_int8_t disp8)
{
	jit->write_ubyte(jit, IA32_ADD_RM_IMM8);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP8, 0, dest));
	jit->write_byte(jit, disp8);
	jit->write_byte(jit, val);
}

inline void IA32_Add_Rm_Imm32_Disp8(JitWriter *jit, 
							 jit_uint8_t dest, 
							 jit_int32_t val, 
							 jit_int8_t disp8)
{
	jit->write_ubyte(jit, IA32_ADD_RM_IMM32);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP8, 0, dest));
	jit->write_byte(jit, disp8);
	jit->write_int32(jit, val);
}

inline void IA32_Add_Rm_Imm8_Disp32(JitWriter *jit, 
							jit_uint8_t dest, 
							jit_int8_t val, 
							jit_int32_t disp32)
{
	jit->write_ubyte(jit, IA32_ADD_RM_IMM8);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP32, 0, dest));
	jit->write_int32(jit, disp32);
	jit->write_byte(jit, val);
}

inline void IA32_Add_Rm_Imm8_Disp_Reg(JitWriter *jit, 
								jit_uint8_t dest_base, 
								jit_uint8_t dest_index,
								jit_uint8_t dest_scale,
								jit_int8_t val)
{
	jit->write_ubyte(jit, IA32_ADD_RM_IMM8);
	jit->write_ubyte(jit, ia32_modrm(MOD_MEM_REG, 0, REG_SIB));
	jit->write_ubyte(jit, ia32_sib(dest_scale, dest_index, dest_base));
	jit->write_byte(jit, val);
}

inline void IA32_Sub_Rm_Imm8_Disp8(JitWriter *jit, 
							jit_uint8_t dest, 
							jit_int8_t val, 
							jit_int8_t disp8)
{
	jit->write_ubyte(jit, IA32_SUB_RM_IMM8);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP8, 5, dest));
	jit->write_byte(jit, disp8);
	jit->write_byte(jit, val);
}

inline void IA32_Sub_Rm_Imm8_Disp32(JitWriter *jit, 
							 jit_uint8_t dest, 
							 jit_int8_t val, 
							 jit_int32_t disp32)
{
	jit->write_ubyte(jit, IA32_SUB_RM_IMM8);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP32, 5, dest));
	jit->write_int32(jit, disp32);
	jit->write_byte(jit, val);
}

inline void IA32_Sub_Rm_Imm8_Disp_Reg(JitWriter *jit, 
							   jit_uint8_t dest_base, 
							   jit_uint8_t dest_index,
							   jit_uint8_t dest_scale,
							   jit_int8_t val)
{
	jit->write_ubyte(jit, IA32_SUB_RM_IMM8);
	jit->write_ubyte(jit, ia32_modrm(MOD_MEM_REG, 5, REG_SIB));
	jit->write_ubyte(jit, ia32_sib(dest_scale, dest_index, dest_base));
	jit->write_byte(jit, val);
}

/**
* Memory Instructions
*/

inline void IA32_Lea_Reg_DispRegMult(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src_base, jit_uint8_t src_index, jit_uint8_t scale)
{
	jit->write_ubyte(jit, IA32_LEA_REG_MEM);
	jit->write_ubyte(jit, ia32_modrm(MOD_MEM_REG, dest, REG_SIB));
	jit->write_ubyte(jit, ia32_sib(scale, src_index, src_base));
}

inline void IA32_Lea_Reg_DispRegMultImm8(JitWriter *jit, 
							  jit_uint8_t dest, 
							  jit_uint8_t src_base, 
							  jit_uint8_t src_index, 
							  jit_uint8_t scale, 
							  jit_int8_t val)
{
	jit->write_ubyte(jit, IA32_LEA_REG_MEM);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP8, dest, REG_SIB));
	jit->write_ubyte(jit, ia32_sib(scale, src_index, src_base));
	jit->write_byte(jit, val);
}

inline void IA32_Lea_DispRegImm8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src_base, jit_int8_t val)
{
	jit->write_ubyte(jit, IA32_LEA_REG_MEM);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP8, dest, MOD_MEM_REG));
	jit->write_byte(jit, val);
}

inline void IA32_Lea_DispRegImm32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src_base, jit_int32_t val)
{
	jit->write_ubyte(jit, IA32_LEA_REG_MEM);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP32, dest, MOD_MEM_REG));
	jit->write_int32(jit, val);
}

/**
* Stack Instructions
*/

inline void IA32_Pop_Reg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(jit, IA32_POP_REG+reg);
}

inline void IA32_Push_Reg(JitWriter *jit, jit_uint8_t reg)
{
	jit->write_ubyte(jit, IA32_PUSH_REG+reg);
}

/**
 * Moving from REGISTER/MEMORY to REGISTER
 */

inline void IA32_Mov_Reg_Rm(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_MOV_REG_MEM);
	jit->write_ubyte(jit, ia32_modrm(mode, dest, src));
}

inline void IA32_Mov_Reg_Rm_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(jit, IA32_MOV_REG_MEM);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP8, dest, src));
	jit->write_byte(jit, disp);
}

inline void IA32_Mov_Reg_Rm_Disp32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	jit->write_ubyte(jit, IA32_MOV_REG_MEM);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP32, dest, src));
	jit->write_int32(jit, disp);
}

inline void IA32_Mov_Reg_Rm_Disp_Reg(JitWriter *jit, 
							jit_uint8_t dest, 
							jit_uint8_t src_base, 
							jit_uint8_t src_index,
							jit_uint8_t src_scale)
{
	jit->write_ubyte(jit, IA32_MOV_REG_MEM);
	jit->write_ubyte(jit, ia32_modrm(MOD_MEM_REG, dest, REG_SIB));
	jit->write_ubyte(jit, ia32_sib(src_scale, src_index, src_base));
}

/**
 * Moving from REGISTER to REGISTER/MEMORY
 */

inline void IA32_Mov_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_MOV_RM_REG);
	jit->write_ubyte(jit, ia32_modrm(mode, src, dest));
}

inline void IA32_Mov_Rm_Reg_Disp8(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int8_t disp)
{
	jit->write_ubyte(jit, IA32_MOV_RM_REG);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP8, src, dest));
	jit->write_byte(jit, disp);
}

inline void IA32_Mov_Rm_Reg_Disp32(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_int32_t disp)
{
	jit->write_ubyte(jit, IA32_MOV_RM_REG);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP32, src, dest));
	jit->write_int32(jit, disp);
}

inline void IA32_Mov_Rm_Reg_Disp_Reg(JitWriter *jit, 
							  jit_uint8_t dest_base, 
							  jit_uint8_t dest_index,
							  jit_uint8_t dest_scale,
							  jit_uint8_t src)
{
	jit->write_ubyte(jit, IA32_MOV_RM_REG);
	jit->write_ubyte(jit, ia32_modrm(MOD_MEM_REG, src, REG_SIB));
	jit->write_ubyte(jit, ia32_sib(dest_scale, dest_index, dest_base));
}

/**
 * Moving from IMMEDIATE to REGISTER
 */

inline void IA32_Mov_Reg_Imm32(JitWriter *jit, jit_uint8_t dest, jit_int32_t num)
{
	jit->write_ubyte(jit, IA32_MOV_REG_IMM+dest);
	jit->write_int32(jit, num);
}

inline void IA32_Mov_Rm_Imm32_Disp8(JitWriter *jit, 
							 jit_uint8_t dest, 
							 jit_int32_t val, 
							 jit_int8_t disp8)
{
	jit->write_ubyte(jit, IA32_MOV_RM_IMM32);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP8, 0, dest));
	jit->write_byte(jit, disp8);
	jit->write_int32(jit, val);
}

inline void IA32_Mov_Rm_Imm32_Disp32(JitWriter *jit, 
							 jit_uint8_t dest, 
							 jit_int32_t val, 
							 jit_int32_t disp32)
{
	jit->write_ubyte(jit, IA32_MOV_RM_IMM32);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP32, 0, dest));
	jit->write_int32(jit, disp32);
	jit->write_int32(jit, val);
}

inline void IA32_Mov_Rm_Imm32_Disp_Reg(JitWriter *jit, 
								jit_uint8_t dest_base, 
								jit_uint8_t dest_index, 
								jit_uint8_t dest_scale, 
								jit_int32_t val)
{
	jit->write_ubyte(jit, IA32_MOV_RM_IMM32);
	jit->write_ubyte(jit, ia32_modrm(MOD_MEM_REG, 0, REG_SIB));
	jit->write_ubyte(jit, ia32_sib(dest_scale, dest_index, dest_base));
	jit->write_int32(jit, val);
}

/**
 * Branching/Jumping
 */

inline jitoffs_t IA32_Jump_Cond_Imm8(JitWriter *jit, jit_uint8_t cond, jit_int8_t disp)
{
	jitoffs_t ptr;
	jit->write_ubyte(jit, IA32_JCC_IMM+cond);
	ptr = jit->jit_curpos();
	jit->write_byte(jit, disp);
	return ptr;
}

inline jitoffs_t IA32_Jump_Imm32(JitWriter *jit, jit_int32_t disp)
{
	jitoffs_t ptr;
	jit->write_ubyte(jit, IA32_JMP_IMM32);
	ptr = jit->jit_curpos();
	jit->write_int32(jit, disp);
	return ptr;
}

inline jitoffs_t IA32_Jump_Cond_Imm32(JitWriter *jit, jit_uint8_t cond, jit_int32_t disp)
{
	jitoffs_t ptr;
	jit->write_ubyte(jit, IA32_JCC_IMM32_1);
	jit->write_ubyte(jit, IA32_JCC_IMM32_2+cond);
	ptr = jit->jit_curpos();
	jit->write_int32(jit, disp);
	return ptr;
}

inline jitoffs_t IA32_Call_Imm32(JitWriter *jit, jit_int32_t disp)
{
	jitoffs_t ptr;
	jit->write_ubyte(jit, IA32_CALL_IMM32);
	ptr = jit->jit_curpos();
	jit->write_int32(jit, disp);
	return ptr;
}

inline void IA32_Write_Jump8(JitWriter *jit, jitoffs_t jmp, jitoffs_t target)
{
	//save old ptr
	jitcode_t oldptr = jit->outptr;
	//get relative difference
	jit_int8_t diff = (target - (jmp + 1));
	//overwrite old value
	jit->outptr = jit->outbase + jmp;
	jit->write_byte(jit, diff);
	//restore old ptr
	jit->outptr = oldptr;
}

inline void IA32_Send_Jump8_Here(JitWriter *jit, jitoffs_t jmp)
{
	jitoffs_t curptr = jit->jit_curpos(jit);
	IA32_Write_Jump8(jit, jmp, curptr);
}

inline void IA32_Return(JitWriter *jit)
{
	jit->write_ubyte(jit, IA32_RET);
}

inline void IA32_Test_Rm_Reg(JitWriter *jit, jit_uint8_t reg1, jit_uint8_t reg2, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_TEST_RM_REG);
	jit->write_ubyte(jit, ia32_modrm(mode, reg2, reg1));
}

inline void IA32_Cmp_Rm_Reg(JitWriter *jit, jit_uint8_t dest, jit_uint8_t src, jit_uint8_t mode)
{
	jit->write_ubyte(jit, IA32_CMP_RM_REG);
	jit->write_ubyte(jit, ia32_modrm(mode, src, dest));
}

inline void IA32_Cmp_Rm_Imm8(JitWriter *jit, jit_uint8_t mode, jit_uint8_t rm, jit_int8_t imm8)
{
	jit->write_ubyte(jit, IA32_CMP_RM_IMM8);
	jit->write_ubyte(jit, ia32_modrm(mode, 7, rm));
	jit->write_byte(jit, imm8);
}

inline void IA32_Cmp_Rm_Imm32(JitWriter *jit, jit_uint8_t mode, jit_uint8_t rm, jit_int32_t imm32)
{
	jit->write_ubyte(jit, IA32_CMP_RM_IMM32);
	jit->write_ubyte(jit, ia32_modrm(mode, 7, rm));
	jit->write_int32(jit, imm32);
}

inline void IA32_Cmp_Rm_Disp8_Imm8(JitWriter *jit, jit_uint8_t reg, jit_int8_t disp, jit_int8_t imm8)
{
	jit->write_ubyte(jit, IA32_CMP_RM_IMM8);
	jit->write_ubyte(jit, ia32_modrm(MOD_DISP8, 7, reg));
	jit->write_byte(jit, disp);
	jit->write_byte(jit, imm8);
}

inline void IA32_Cmp_Eax_Imm32(JitWriter *jit, jit_int32_t value)
{
	jit->write_ubyte(jit, IA32_CMP_EAX_IMM32);
	jit->write_int32(jit, value);
}

inline void IA32_SetCC_Rm8(JitWriter *jit, jit_uint8_t reg, jit_uint8_t cond)
{
	jit->write_ubyte(jit, IA32_SETCC_RM8_1);
	jit->write_ubyte(jit, IA32_SETCC_RM8_2+cond);
	jit->write_ubyte(jit, ia32_modrm(MOD_REG, 0, reg));
}

inline void IA32_Rep(JitWriter *jit)
{
	jit->write_ubyte(jit, IA32_REP);
}

inline void IA32_Movsd(JitWriter *jit)
{
	jit->write_ubyte(jit, IA32_MOVSD);
}

inline void IA32_Movsb(JitWriter *jit)
{
	jit->write_ubyte(jit, IA32_MOVSB);
}

inline void IA32_Stosd(JitWriter *jit)
{
	jit->write_ubyte(jit, IA32_STOSD);
}

inline void IA32_Cld(JitWriter *jit)
{
	jit->write_ubyte(jit, IA32_CLD);
}

#endif //_INCLUDE_JIT_X86_MACROS_H
