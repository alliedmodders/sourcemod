#include <string.h>
#include <stdlib.h>
#include "jit_x86.h"
#include "opcode_helpers.h"
#include "x86_macros.h"

inline void WriteOp_Move_Pri(JitWriter *jit)
{
	//mov eax, edx
	IA32_Mov_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_Move_Alt(JitWriter *jit)
{
	//mov edx, eax
	IA32_Mov_Rm_Reg(jit, AMX_REG_ALT, AMX_REG_PRI, MOD_REG);
}

inline void WriteOp_Xchg(JitWriter *jit)
{
	//xchg eax, edx
	IA32_Xchg_Eax_Reg(jit, AMX_REG_ALT);
}

inline void WriteOp_Push(JitWriter *jit)
{
	//push stack, DAT offset based
	//sub ebp, 4
	//mov ecx, [edi+<val>]
	//mov [ebp], ecx
	cell_t val = jit->read_cell();
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
	//optimize encoding a bit...
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_DAT, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_DAT, val);
	IA32_Mov_Rm_Reg(jit, AMX_REG_STK, AMX_REG_TMP, MOD_MEM_REG);
}

inline void WriteOp_Push_C(JitWriter *jit)
{
	//push stack
	//mov [ebp-4], <val>
	//sub ebp, 4
	cell_t val = jit->read_cell();
	IA32_Mov_Rm_Imm32_Disp8(jit, AMX_REG_STK, val, -4);
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
}

inline void WriteOp_Zero(JitWriter *jit)
{
	//mov [edi+<val>], 0
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Rm_Imm32_Disp8(jit, AMX_REG_DAT, 0, (jit_int8_t)val);
	else
		IA32_Mov_Rm_Imm32_Disp32(jit, AMX_REG_DAT, 0, val);
}

inline void WriteOp_Zero_S(JitWriter *jit)
{
	//mov [ebx+<val>], 0
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Rm_Imm32_Disp8(jit, AMX_REG_FRM, 0, (jit_int8_t)val);
	else
		IA32_Mov_Rm_Imm32_Disp32(jit, AMX_REG_FRM, 0, val);
}

inline void WriteOp_Push_S(JitWriter *jit)
{
	//push stack, FRM offset based
	//sub ebp, 4
	//mov ecx, [ebx+<val>]
	//mov [ebp], ecx
	cell_t val = jit->read_cell();
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
	//optimize encoding a bit...
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_FRM, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_FRM, val);
	IA32_Mov_Rm_Reg(jit, AMX_REG_STK, AMX_REG_TMP, MOD_MEM_REG);
}

inline void WriteOp_Push_Pri(JitWriter *jit)
{
	//mov [ebp-4], eax
	//sub ebp, 4
	IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_STK, AMX_REG_PRI, -4);
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
}

inline void WriteOp_Push_Alt(JitWriter *jit)
{
	//mov [ebp-4], edx
	//sub ebp, 4
	IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_STK, AMX_REG_ALT, -4);
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
}

inline void WriteOp_Push2_C(JitWriter *jit)
{
	Macro_PushN_C(jit, 2);
}

inline void WriteOp_Push3_C(JitWriter *jit)
{
	Macro_PushN_C(jit, 3);
}

inline void WriteOp_Push4_C(JitWriter *jit)
{
	Macro_PushN_C(jit, 4);
}

inline void WriteOp_Push5_C(JitWriter *jit)
{
	Macro_PushN_C(jit, 5);
}

inline void WriteOp_Push2_Adr(JitWriter *jit)
{
	Macro_PushN_Addr(jit, 2);
}

inline void WriteOp_Push3_Adr(JitWriter *jit)
{
	Macro_PushN_Addr(jit, 3);
}

inline void WriteOp_Push4_Adr(JitWriter *jit)
{
	Macro_PushN_Addr(jit, 4);
}

inline void WriteOp_Push5_Adr(JitWriter *jit)
{
	Macro_PushN_Addr(jit, 5);
}

inline void WriteOp_Push2_S(JitWriter *jit)
{
	Macro_PushN_S(jit, 2);
}

inline void WriteOp_Push3_S(JitWriter *jit)
{
	Macro_PushN_S(jit, 3);
}

inline void WriteOp_Push4_S(JitWriter *jit)
{
	Macro_PushN_S(jit, 4);
}

inline void WriteOp_Push5_S(JitWriter *jit)
{
	Macro_PushN_S(jit, 5);
}

inline void WriteOp_Push5(JitWriter *jit)
{
	Macro_PushN(jit, 5);
}

inline void WriteOp_Push4(JitWriter *jit)
{
	Macro_PushN(jit, 4);
}

inline void WriteOp_Push3(JitWriter *jit)
{
	Macro_PushN(jit, 3);
}

inline void WriteOp_Push2(JitWriter *jit)
{
	Macro_PushN(jit, 2);
}

inline void WriteOp_Zero_Pri(JitWriter *jit)
{
	//xor eax, eax
	IA32_Xor_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_PRI, MOD_REG);
}

inline void WriteOp_Zero_Alt(JitWriter *jit)
{
	//xor edx, edx
	IA32_Xor_Rm_Reg(jit, AMX_REG_ALT, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_Add(JitWriter *jit)
{
	//add eax, edx
	IA32_Add_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_Sub(JitWriter *jit)
{
	//sub eax, edx
	IA32_Sub_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_Sub_Alt(JitWriter *jit)
{
	//neg eax
	//add eax, edx
	IA32_Neg_Rm(jit, AMX_REG_PRI, MOD_REG);
	IA32_Add_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_Proc(JitWriter *jit)
{
	//push old frame on stack:
	//sub ebp, 4
	//mov ecx, [frm]
	//mov [ebp], ecx
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_INFO_FRM, MOD_MEM_REG);
	IA32_Mov_Rm_Reg(jit, AMX_REG_STK, AMX_REG_TMP, MOD_MEM_REG);
	//save frame:
	//mov [frm], ebp	- get new frame
	//mov ebx, ebp		- store frame back
	//sub [frm], edi	- relocate local frame
	IA32_Mov_Rm_Reg(jit, AMX_INFO_FRM, AMX_REG_STK, MOD_MEM_REG);
	IA32_Mov_Rm_Reg(jit, AMX_REG_FRM, AMX_REG_STK, MOD_REG);
	IA32_Sub_Rm_Reg(jit, AMX_INFO_FRM, AMX_REG_DAT, MOD_MEM_REG);
}

inline void WriteOp_Shl(JitWriter *jit)
{
	//mov ecx, edx
	//shl eax, cl
	IA32_Mov_Rm_Reg(jit, AMX_REG_TMP, AMX_REG_ALT, MOD_REG);
	IA32_Shl_Rm_CL(jit, AMX_REG_PRI, MOD_REG);
}

inline void WriteOp_Shr(JitWriter *jit)
{
	//mov ecx, edx
	//shr eax, cl
	IA32_Mov_Rm_Reg(jit, AMX_REG_TMP, AMX_REG_ALT, MOD_REG);
	IA32_Shr_Rm_CL(jit, AMX_REG_PRI, MOD_REG);
}

inline void WriteOp_Sshr(JitWriter *jit)
{
	//mov ecx, edx
	//sar eax, cl
	IA32_Mov_Rm_Reg(jit, AMX_REG_TMP, AMX_REG_ALT, MOD_REG);
	IA32_Sar_Rm_CL(jit, AMX_REG_PRI, MOD_REG);
}

inline void WriteOp_Shl_C_Pri(JitWriter *jit)
{
	//shl eax, <val>
	jit_uint8_t val = (jit_uint8_t)jit->read_cell();
	IA32_Shl_Rm_Imm8(jit, AMX_REG_PRI, val, MOD_REG);
}

inline void WriteOp_Shl_C_Alt(JitWriter *jit)
{
	//shl edx, <val>
	jit_uint8_t val = (jit_uint8_t)jit->read_cell();
	IA32_Shl_Rm_Imm8(jit, AMX_REG_ALT, val, MOD_REG);
}

inline void WriteOp_Shr_C_Pri(JitWriter *jit)
{
	//shr eax, <val>
	jit_uint8_t val = (jit_uint8_t)jit->read_cell();
	IA32_Shr_Rm_Imm8(jit, AMX_REG_PRI, val, MOD_REG);
}

inline void WriteOp_Shr_C_Alt(JitWriter *jit)
{
	//shr edx, <val>
	jit_uint8_t val = (jit_uint8_t)jit->read_cell();
	IA32_Shr_Rm_Imm8(jit, AMX_REG_ALT, val, MOD_REG);
}

inline void WriteOp_SMul(JitWriter *jit)
{
	//mov ecx, edx
	//imul edx
	//mov edx, ecx
	IA32_Mov_Rm_Reg(jit, AMX_REG_TMP, AMX_REG_ALT, MOD_REG);
	IA32_IMul_Rm(jit, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Rm_Reg(jit, AMX_REG_ALT, AMX_REG_TMP, MOD_REG);
}

inline void WriteOp_UMul(JitWriter *jit)
{
	//mov ecx, edx
	//mul edx
	//mov edx, ecx
	IA32_Mov_Rm_Reg(jit, AMX_REG_TMP, AMX_REG_ALT, MOD_REG);
	IA32_Mul_Rm(jit, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Rm_Reg(jit, AMX_REG_ALT, AMX_REG_TMP, MOD_REG);
}

inline void WriteOp_Not(JitWriter *jit)
{
	//test eax, eax
	//mov eax, 0
	//sete al
	IA32_Test_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_PRI, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_E);
}

inline void WriteOp_Neg(JitWriter *jit)
{
	//neg eax
	IA32_Neg_Rm(jit, AMX_REG_PRI, MOD_REG);
}

inline void WriteOp_Xor(JitWriter *jit)
{
	//xor eax, edx
	IA32_Xor_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_Or(JitWriter *jit)
{
	//or eax, edx
	IA32_Or_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_And(JitWriter *jit)
{
	//and eax, edx
	IA32_And_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_Invert(JitWriter *jit)
{
	//not eax
	IA32_Not_Rm(jit, AMX_REG_PRI, MOD_REG);
}

inline void WriteOp_Add_C(JitWriter *jit)
{
	//add eax, <val>
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Add_Rm_Imm8(jit, AMX_REG_PRI, (jit_int8_t)val, MOD_REG);
	else
		IA32_Add_Eax_Imm32(jit, val);
}

inline void WriteOp_SMul_C(JitWriter *jit)
{
	//imul eax, <val>
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_IMul_Reg_Imm8(jit, AMX_REG_PRI, MOD_REG, (jit_int8_t)val);
	else
		IA32_IMul_Reg_Imm32(jit, AMX_REG_PRI, MOD_REG, val);
}

inline void WriteOp_Sign_Pri(JitWriter *jit)
{
	//shl eax, 24
	//sar eax, 24
	IA32_Shl_Rm_Imm8(jit, AMX_REG_PRI, 24, MOD_REG);
	IA32_Sar_Rm_Imm8(jit, AMX_REG_PRI, 24, MOD_REG);
}

inline void WriteOp_Sign_Alt(JitWriter *jit)
{
	//shl edx, 24
	//sar edx, 24
	IA32_Shl_Rm_Imm8(jit, AMX_REG_ALT, 24, MOD_REG);
	IA32_Sar_Rm_Imm8(jit, AMX_REG_ALT, 24, MOD_REG);
}

inline void WriteOp_Eq(JitWriter *jit)
{
	//cmp eax, edx	; PRI == ALT ?
	//mov eax, 0
	//sete al
	IA32_Cmp_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_E);
}

inline void WriteOp_Neq(JitWriter *jit)
{
	//cmp eax, edx	; PRI != ALT ?
	//mov eax, 0
	//setne al
	IA32_Cmp_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_NE);
}

inline void WriteOp_Less(JitWriter *jit)
{
	//cmp eax, edx	; PRI < ALT ? (unsigned)
	//mov eax, 0
	//setb al
	IA32_Cmp_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_B);
}

inline void WriteOp_Leq(JitWriter *jit)
{
	//cmp eax, edx	; PRI <= ALT ? (unsigned)
	//mov eax, 0
	//setbe al
	IA32_Cmp_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_BE);
}

inline void WriteOp_Grtr(JitWriter *jit)
{
	//cmp eax, edx	; PRI > ALT ? (unsigned)
	//mov eax, 0
	//seta al
	IA32_Cmp_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_A);
}

inline void WriteOp_Geq(JitWriter *jit)
{
	//cmp eax, edx	; PRI >= ALT ? (unsigned)
	//mov eax, 0
	//setae al
	IA32_Cmp_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_AE);
}

inline void WriteOp_Sless(JitWriter *jit)
{
	//cmp eax, edx	; PRI < ALT ? (signed)
	//mov eax, 0
	//setl al
	IA32_Cmp_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_L);
}

inline void WriteOp_Sleq(JitWriter *jit)
{
	//cmp eax, edx	; PRI <= ALT ? (signed)
	//mov eax, 0
	//setle al
	IA32_Cmp_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_LE);
}

inline void WriteOp_Sgrtr(JitWriter *jit)
{
	//cmp eax, edx	; PRI > ALT ? (signed)
	//mov eax, 0
	//setg al
	IA32_Cmp_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_G);
}

inline void WriteOp_Sgeq(JitWriter *jit)
{
	//cmp eax, edx	; PRI >= ALT ? (signed)
	//mov eax, 0
	//setge al
	IA32_Cmp_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_GE);
}

inline void WriteOp_Eq_C_Pri(JitWriter *jit)
{
	//cmp eax, <val>   ; PRI == value ?
	//mov eax, 0
	//sete al
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Cmp_Rm_Imm8(jit, MOD_REG, AMX_REG_PRI, (jit_int8_t)val);
	else
		IA32_Cmp_Eax_Imm32(jit, val);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_E);
}

inline void WriteOp_Eq_C_Alt(JitWriter *jit)
{
	//xor eax, eax
	//cmp edx, <val>   ; ALT == value ?
	//sete al
	cell_t val = jit->read_cell();
	IA32_Xor_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_PRI, MOD_REG);
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Cmp_Rm_Imm8(jit, MOD_REG, AMX_REG_ALT, (jit_int8_t)val);
	else
		IA32_Cmp_Rm_Imm32(jit, MOD_REG, AMX_REG_ALT, val);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_E);
}

inline void WriteOp_Inc_Pri(JitWriter *jit)
{
	//add eax, 1
	IA32_Add_Rm_Imm8(jit, AMX_REG_PRI, 1, MOD_REG);
}

inline void WriteOp_Inc_Alt(JitWriter *jit)
{
	//add edx, 1
	IA32_Add_Rm_Imm8(jit, AMX_REG_ALT, 1, MOD_REG);
}

inline void WriteOp_Inc(JitWriter *jit)
{
	//add [edi+<val>], 1
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Add_Rm_Imm8_Disp8(jit, AMX_REG_DAT, 1, (jit_int8_t)val);
	else
		IA32_Add_Rm_Imm8_Disp32(jit, AMX_REG_DAT, 1, val);
}

inline void WriteOp_Inc_S(JitWriter *jit)
{
	//add [ebx+<val>], 1
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Add_Rm_Imm8_Disp8(jit, AMX_REG_FRM, 1, (jit_int8_t)val);
	else
		IA32_Add_Rm_Imm8_Disp32(jit, AMX_REG_FRM, 1, val);
}

inline void WriteOp_Inc_I(JitWriter *jit)
{
	//add [edi+eax], 1
	IA32_Add_Rm_Imm8_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_PRI, NOSCALE, 1);
}

inline void WriteOp_Dec_Pri(JitWriter *jit)
{
	//sub eax, 1
	IA32_Sub_Rm_Imm8(jit, AMX_REG_PRI, 1, MOD_REG);
}

inline void WriteOp_Dec_Alt(JitWriter *jit)
{
	//sub edx, 1
	IA32_Sub_Rm_Imm8(jit, AMX_REG_ALT, 1, MOD_REG);
}

inline void WriteOp_Dec(JitWriter *jit)
{
	//sub [edi+<val>], 1
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Sub_Rm_Imm8_Disp8(jit, AMX_REG_DAT, 1, (jit_int8_t)val);
	else
		IA32_Sub_Rm_Imm8_Disp32(jit, AMX_REG_DAT, 1, val);
}

inline void WriteOp_Dec_S(JitWriter *jit)
{
	//sub [ebx+<val>], 1
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Sub_Rm_Imm8_Disp8(jit, AMX_REG_FRM, 1, (jit_int8_t)val);
	else
		IA32_Sub_Rm_Imm8_Disp32(jit, AMX_REG_FRM, 1, val);
}

inline void WriteOp_Dec_I(JitWriter *jit)
{
	//sub [edi+eax], 1
	IA32_Sub_Rm_Imm8_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_PRI, NOSCALE, 1);
}

inline void WriteOp_Load_Pri(JitWriter *jit)
{
	//mov eax, [edi+<val>]
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, AMX_REG_DAT, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_PRI, AMX_REG_DAT, val);
}

inline void WriteOp_Load_Alt(JitWriter *jit)
{
	//mov edx, [edi+<val>]
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, AMX_REG_DAT, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_ALT, AMX_REG_DAT, val);
}

inline void WriteOp_Load_S_Pri(JitWriter *jit)
{
	//mov eax, [ebx+<val>]
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, AMX_REG_FRM, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_PRI, AMX_REG_FRM, val);
}

inline void WriteOp_Load_S_Alt(JitWriter *jit)
{
	//mov edx, [ebx+<val>]
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, AMX_REG_FRM, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_ALT, AMX_REG_FRM, val);
}

inline void WriteOp_Lref_Pri(JitWriter *jit)
{
	//mov eax, [edi+<val>]
	//mov eax, [edi+eax]
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, AMX_REG_DAT, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_PRI, AMX_REG_DAT, val);
	IA32_Mov_Reg_Rm_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
}

inline void WriteOp_Lref_Alt(JitWriter *jit)
{
	//mov edx, [edi+<val>]
	//mov edx, [edi+edx]
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, AMX_REG_DAT, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_ALT, AMX_REG_DAT, val);
	IA32_Mov_Reg_Rm_Disp_Reg(jit, AMX_REG_ALT, AMX_REG_DAT, AMX_REG_ALT, NOSCALE);
}

inline void WriteOp_Lref_S_Pri(JitWriter *jit)
{
	//mov eax, [ebx+<val>]
	//mov eax, [edi+eax]
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, AMX_REG_FRM, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_PRI, AMX_REG_FRM, val);
	IA32_Mov_Reg_Rm_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
}

inline void WriteOp_Lref_S_Alt(JitWriter *jit)
{
	//mov edx, [ebx+<val>]
	//mov edx, [edi+edx]
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, AMX_REG_FRM, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_ALT, AMX_REG_FRM, val);
	IA32_Mov_Reg_Rm_Disp_Reg(jit, AMX_REG_ALT, AMX_REG_DAT, AMX_REG_ALT, NOSCALE);
}

inline void WriteOp_Const_Pri(JitWriter *jit)
{
	//mov eax, <val>
	cell_t val = jit->read_cell();
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, val);
}

inline void WriteOp_Const_Alt(JitWriter *jit)
{
	//mov edx, <val>
	cell_t val = jit->read_cell();
	IA32_Mov_Reg_Imm32(jit, AMX_REG_ALT, val);
}

inline void WriteOp_Addr_Pri(JitWriter *jit)
{
	//mov eax, frm
	//add eax, <val>
	cell_t val = jit->read_cell();
	IA32_Mov_Reg_Rm(jit, AMX_REG_PRI, AMX_INFO_FRM, MOD_MEM_REG);
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Add_Rm_Imm8(jit, AMX_REG_PRI, (jit_int8_t)val, MOD_REG);
	else
		IA32_Add_Eax_Imm32(jit, val);
}

inline void WriteOp_Addr_Alt(JitWriter *jit)
{
	//mov edx, frm
	//add edx, <val>
	cell_t val = jit->read_cell();
	IA32_Mov_Reg_Rm(jit, AMX_REG_ALT, AMX_INFO_FRM, MOD_MEM_REG);
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Add_Rm_Imm8(jit, AMX_REG_ALT, (jit_int8_t)val, MOD_REG);
	else
		IA32_Add_Rm_Imm32(jit, AMX_REG_ALT, val, MOD_REG);
}

inline void WriteOp_Stor_Pri(JitWriter *jit)
{
	//mov [edi+<val>], eax
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_DAT, AMX_REG_PRI, (jit_int8_t)val);
	else
		IA32_Mov_Rm_Reg_Disp32(jit, AMX_REG_DAT, AMX_REG_PRI, val);
}

inline void WriteOp_Stor_Alt(JitWriter *jit)
{
	//mov [edi+<val>], edx
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_DAT, AMX_REG_ALT, (jit_int8_t)val);
	else
		IA32_Mov_Rm_Reg_Disp32(jit, AMX_REG_DAT, AMX_REG_ALT, val);
}

inline void WriteOp_Stor_S_Pri(JitWriter *jit)
{
	//mov [ebx+<val>], eax
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_FRM, AMX_REG_PRI, (jit_int8_t)val);
	else
		IA32_Mov_Rm_Reg_Disp32(jit, AMX_REG_FRM, AMX_REG_PRI, val);
}

inline void WriteOp_Stor_S_Alt(JitWriter *jit)
{
	//mov [ebx+<val>], edx
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_FRM, AMX_REG_ALT, (jit_int8_t)val);
	else
		IA32_Mov_Rm_Reg_Disp32(jit, AMX_REG_FRM, AMX_REG_ALT, val);
}

inline void WriteOp_Idxaddr(JitWriter *jit)
{
	//lea eax, [edx+4*eax]
	IA32_Lea_Reg_DispRegMult(jit, AMX_REG_PRI, AMX_REG_ALT, AMX_REG_PRI, SCALE4);
}

inline void WriteOp_Idxaddr_B(JitWriter *jit)
{
	//shl eax, <val>
	//add eax, edx
	cell_t val = jit->read_cell();
	IA32_Shl_Rm_Imm8(jit, AMX_REG_PRI, (jit_uint8_t)val, MOD_REG);
	IA32_Add_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_Sref_Pri(JitWriter *jit)
{
	//mov ecx, [edi+<val>]
	//mov [edi+ecx], eax
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_DAT, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_DAT, val);
	IA32_Mov_Rm_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_TMP, NOSCALE, AMX_REG_PRI);
}

inline void WriteOp_Sref_Alt(JitWriter *jit)
{
	//mov ecx, [edi+<val>]
	//mov [edi+ecx], edx
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_DAT, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_DAT, val);
	IA32_Mov_Rm_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_TMP, NOSCALE, AMX_REG_ALT);
}

inline void WriteOp_Sref_S_Pri(JitWriter *jit)
{
	//mov ecx, [ebx+<val>]
	//mov [edi+ecx], eax
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_FRM, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_FRM, val);
	IA32_Mov_Rm_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_TMP, NOSCALE, AMX_REG_PRI);
}

inline void WriteOp_Sref_S_Alt(JitWriter *jit)
{
	//mov ecx, [ebx+<val>]
	//mov [edi+ecx], edx
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_FRM, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_FRM, val);
	IA32_Mov_Rm_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_TMP, NOSCALE, AMX_REG_ALT);
}

inline void WriteOp_Align_Pri(JitWriter *jit)
{
	//xor eax, <cellsize - val>
	cell_t val = sizeof(cell_t) - jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Xor_Rm_Imm8(jit, AMX_REG_PRI, MOD_REG, (jit_int8_t)val);
	else
		IA32_Xor_Eax_Imm32(jit, val);
}

inline void WriteOp_Align_Alt(JitWriter *jit)
{
	//xor edx, <cellsize - val>
	cell_t val = sizeof(cell_t) - jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Xor_Rm_Imm8(jit, AMX_REG_ALT, MOD_REG, (jit_int8_t)val);
	else
		IA32_Xor_Rm_Imm32(jit, AMX_REG_ALT, MOD_REG, val);
}

inline void WriteOp_Pop_Pri(JitWriter *jit)
{
	//mov eax, [ebp]
	//add ebp, 4
	IA32_Mov_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_STK, MOD_MEM_REG);
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
}

inline void WriteOp_Pop_Alt(JitWriter *jit)
{
	//mov edx, [ebp]
	//add ebp, 4
	IA32_Mov_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_STK, MOD_MEM_REG);
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
}

inline void WriteOp_Swap_Pri(JitWriter *jit)
{
	//add [ebp], eax
	//sub eax, [ebp]
	//add [ebp], eax
	//neg eax
	IA32_Add_Rm_Reg(jit, AMX_REG_STK, AMX_REG_PRI, MOD_MEM_REG);
	IA32_Sub_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_STK, MOD_MEM_REG);
	IA32_Add_Rm_Reg(jit, AMX_REG_STK, AMX_REG_PRI, MOD_MEM_REG);
	IA32_Neg_Rm(jit, AMX_REG_PRI, MOD_REG);
}

inline void WriteOp_Swap_Alt(JitWriter *jit)
{
	//add [ebp], edx
	//sub edx, [ebp]
	//add [ebp], edx
	//neg edx
	IA32_Add_Rm_Reg(jit, AMX_REG_STK, AMX_REG_ALT, MOD_MEM_REG);
	IA32_Sub_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_STK, MOD_MEM_REG);
	IA32_Add_Rm_Reg(jit, AMX_REG_STK, AMX_REG_ALT, MOD_MEM_REG);
	IA32_Neg_Rm(jit, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_PushAddr(JitWriter *jit)
{
	//mov ecx, frm		;get address (offset from frame)
	//add ecx, <val>
	//mov [ebp-4], ecx
	//sub ebp, 4
	cell_t val = jit->read_cell();
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_INFO_FRM, MOD_MEM_REG);
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Add_Rm_Imm8(jit, AMX_REG_TMP, (jit_int8_t)val, MOD_REG);
	else
		IA32_Add_Rm_Imm32(jit, AMX_REG_TMP, val, MOD_REG);
	IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_STK, AMX_REG_TMP, -4);
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
}

inline void WriteOp_Movs(JitWriter *jit)
{
	//cld
	//push esi
	//lea esi, [edi+eax]
	//add edi, edx
	//if dword:
	// mov ecx, dword
	// rep movsd
	//if byte:
	// mov ecx, byte
	// rep movsb
	//sub edi, edx
	//sub edi, <val>
	//pop esi
	cell_t val = jit->read_cell();
	unsigned int dwords = val >> 2;
	unsigned int bytes = val & 0x3;

	IA32_Cld(jit);
	IA32_Push_Reg(jit, REG_ESI);
	IA32_Lea_Reg_DispRegMult(jit, REG_ESI, REG_EDI, AMX_REG_PRI, NOSCALE);
	IA32_Add_Rm_Reg(jit, REG_EDI, AMX_REG_ALT, MOD_REG);
	if (dwords)
	{
		IA32_Mov_Reg_Imm32(jit, AMX_REG_TMP, dwords);
		IA32_Rep(jit);
		IA32_Movsd(jit);
	}
	if (bytes)
	{
		IA32_Mov_Reg_Imm32(jit, AMX_REG_TMP, bytes);
		IA32_Rep(jit);
		IA32_Movsb(jit);
	}
	IA32_Sub_Rm_Reg(jit, REG_EDI, AMX_REG_ALT, MOD_REG);
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Sub_Rm_Imm8(jit, REG_EDI, (jit_uint8_t)val, MOD_REG);
	else
		IA32_Sub_Rm_Imm32(jit, REG_EDI, val, MOD_REG);
	IA32_Pop_Reg(jit, REG_ESI);
}

inline void WriteOp_Fill(JitWriter *jit)
{
	//add edi, edx
	//mov ecx, <val> >> 2
	//cld
	//rep stosd
	//sub edi, edx
	//sub edi, <val> >> 2
	unsigned int val = jit->read_cell() >> 2;

	IA32_Add_Rm_Reg(jit, REG_EDI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_TMP, val);
	IA32_Cld(jit);
	IA32_Rep(jit);
	IA32_Stosd(jit);
	IA32_Sub_Rm_Reg(jit, REG_EDI, AMX_REG_ALT, MOD_REG);
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Sub_Rm_Imm8(jit, REG_EDI, (jit_uint8_t)val, MOD_REG);
	else
		IA32_Sub_Rm_Imm32(jit, REG_EDI, val, MOD_REG);
}

inline void WriteOp_Heap_Pri(JitWriter *jit)
{
	//mov edx, [hea]
	//add [hea], eax
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, AMX_REG_INFO, AMX_INFO_HEAP);
	IA32_Add_Rm_Reg_Disp8(jit, AMX_REG_INFO, AMX_REG_PRI, AMX_INFO_HEAP);

	Write_CheckMargin_Heap(jit);
}

inline void WriteOp_Push_Heap_C(JitWriter *jit)
{
	//mov ecx, [hea]
	//mov [edi+ecx], <val>
	//add [hea], 4
	cell_t val = jit->read_cell();
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_HEAP);
	IA32_Mov_Rm_Imm32_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_TMP, NOSCALE, val);
	IA32_Add_Rm_Imm8_Disp8(jit, AMX_REG_INFO, 4, AMX_INFO_HEAP);

	Write_CheckMargin_Heap(jit);
}

inline void WriteOp_Pop_Heap_Pri(JitWriter *jit)
{
	//sub [hea], 4
	//mov ecx, [hea]
	//mov eax, [edi+ecx]
	IA32_Sub_Rm_Imm8_Disp8(jit, AMX_REG_INFO, 4, AMX_INFO_HEAP);
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_HEAP);
	IA32_Mov_Reg_Rm_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_TMP, NOSCALE);

	Write_CheckMargin_Heap(jit);
}

inline void WriteOp_Load_Both(JitWriter *jit)
{
	WriteOp_Const_Pri(jit);
	WriteOp_Const_Alt(jit);
}

inline void WriteOp_Load_S_Both(JitWriter *jit)
{
	WriteOp_Load_S_Pri(jit);
	WriteOp_Load_S_Alt(jit);
}

inline void WriteOp_Const(JitWriter *jit)
{
	//mov [edi+<addr>], <val>
	cell_t addr = jit->read_cell();
	cell_t val = jit->read_cell();
	if (addr < SCHAR_MAX && addr > SCHAR_MIN)
	{
		IA32_Mov_Rm_Imm32_Disp8(jit, AMX_REG_DAT, val, (jit_int8_t)addr);
	} else {
		IA32_Mov_Rm_Imm32_Disp32(jit, AMX_REG_DAT, val, addr);
	}
}

inline void WriteOp_Const_S(JitWriter *jit)
{
	//mov [ebx+<offs>], <val>
	cell_t offs = jit->read_cell();
	cell_t val = jit->read_cell();

	if (offs < SCHAR_MAX && offs > SCHAR_MIN)
	{
		IA32_Mov_Rm_Imm32_Disp8(jit, AMX_REG_FRM, val, (jit_int8_t)offs);
	} else {
		IA32_Mov_Rm_Imm32_Disp32(jit, AMX_REG_FRM, val, offs);
	}
}

inline void WriteOp_Load_I(JitWriter *jit)
{
	//mov eax, [edi+eax]
	Write_Check_VerifyAddr(jit, AMX_REG_PRI, false);
	IA32_Mov_Reg_Rm_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
}

inline void WriteOp_Lodb_I(JitWriter *jit)
{
	Write_Check_VerifyAddr(jit, AMX_REG_PRI, false);

	//mov eax, [edi+eax]
	IA32_Mov_Reg_Rm_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);

	//and eax, <bitmask>
	cell_t val = jit->read_cell();
	switch(val)
	{
	case 1:
		{
			IA32_And_Rm_Imm32(jit, AMX_REG_PRI, 0x000000FF);
			break;
		}
	case 2:
		{
			IA32_And_Rm_Imm32(jit, AMX_REG_PRI, 0x0000FFFF);
			break;
		}
	}
}

inline void WriteOp_Stor_I(JitWriter *jit)
{
	//mov [edi+edx], eax
	Write_Check_VerifyAddr(jit, AMX_REG_ALT, false);
	IA32_Mov_Rm_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_ALT, NOSCALE, AMX_REG_PRI);
}

inline void WriteOp_Strb_I(JitWriter *jit)
{
	Write_Check_VerifyAddr(jit, AMX_REG_ALT, false);
	//mov [edi+edx], eax
	cell_t val = jit->read_cell();
	switch (val)
	{
	case 1:
		{
			IA32_Mov_Rm8_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_ALT, NOSCALE, AMX_REG_PRI);
			break;
		}
	case 2:
		{
			IA32_Mov_Rm16_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_ALT, NOSCALE, AMX_REG_PRI);
			break;
		}
	case 3:
		{
			IA32_Mov_Rm_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_ALT, NOSCALE, AMX_REG_PRI);
			break;
		}
	}
}

inline void WriteOp_Lidx(JitWriter *jit)
{
	//lea eax, [edx+4*eax]
	//mov eax, [edi+eax]
	IA32_Lea_Reg_DispRegMult(jit, AMX_REG_PRI, AMX_REG_ALT, AMX_REG_PRI, SCALE4);
	Write_Check_VerifyAddr(jit, AMX_REG_PRI, false);
	IA32_Mov_Reg_Rm_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
}

inline void WriteOp_Lidx_B(JitWriter *jit)
{
	cell_t val = jit->read_cell();
	//shl eax, <val>
	//add eax, edx
	//mov eax, [edi+eax]
	IA32_Shl_Rm_Imm8(jit, AMX_REG_PRI, (jit_uint8_t)val, MOD_REG);
	IA32_Add_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	Write_Check_VerifyAddr(jit, AMX_REG_PRI, false);
	IA32_Mov_Reg_Rm_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
}

inline void WriteOp_Lctrl(JitWriter *jit)
{
	cell_t val = jit->read_cell();
	switch (val)
	{
	case 0:
		{
			//mov ecx, [esi+ctx]
			//mov eax, [ecx+<offs>]
			IA32_Mov_Reg_Rm_Disp8(jit, REG_ECX, AMX_REG_INFO, AMX_INFO_CONTEXT);
			IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, REG_ECX, offsetof(sp_context_t, base));
			break;
		}
	case 1:
		{
			//mov eax, edi
			IA32_Mov_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_DAT, MOD_REG);
			break;
		}
	case 2:
		{
			//mov eax, [esi+hea]
			IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, AMX_REG_INFO, AMX_INFO_HEAP);
			break;
		}
	case 3:
		{
			//mov ecx, [esi+ctx]
			//mov eax, [ecx+ctx.memory]
			IA32_Mov_Reg_Rm_Disp8(jit, REG_ECX, AMX_REG_INFO, AMX_INFO_CONTEXT);
			IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, REG_ECX, offsetof(sp_context_t, memory));
			break;
		}
	case 4:
		{
			//mov eax, ebp
			//sub eax, edi	- unrelocate
			IA32_Mov_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_STK, MOD_REG);
			IA32_Sub_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, MOD_REG);
			break;
		}
	case 5:
		{
			//mov eax, [esi+frm]
			IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, AMX_REG_INFO, AMX_INFO_FRM);
			break;
		}
	case 6:
		{
			//mov eax, [cip]
			jitoffs_t imm32 = IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
			jitoffs_t save = jit->jit_curpos();
			jit->setpos(imm32);
			jit->write_int32((uint32_t)(jit->outbase + save));
			jit->setpos(save);
			break;
		}
	}
}

inline void WriteOp_Sctrl(JitWriter *jit)
{
	cell_t val = jit->read_cell();
	switch (val)
	{
	case 2:
		{
			//mov [esi+hea], eax
			IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_INFO, AMX_INFO_HEAP, AMX_REG_PRI);
			break;
		}
	case 4:
		{
			//lea ebp, [edi+eax]
			IA32_Lea_Reg_DispRegMult(jit, AMX_REG_STK, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
			break;
		}
	case 5:
		{
			//mov ebx, eax	- overwrite frm
			//mov frm, eax	- overwrite stacked frame
			//add ebx, edi	- relocate local frm
			IA32_Mov_Reg_Rm(jit, AMX_REG_FRM, AMX_REG_PRI, MOD_REG);
			IA32_Mov_Rm_Reg(jit, AMX_INFO_FRM, AMX_REG_PRI, MOD_MEM_REG);
			IA32_Add_Rm_Reg(jit, AMX_REG_FRM, AMX_REG_DAT, MOD_REG);
			break;
		}
	case 6:
		{
			IA32_Jump_Reg(jit, AMX_REG_PRI);
			break;
		}
	}
}

inline void WriteOp_Stack(JitWriter *jit)
{
	//mov edx, ebp
	//add ebp, <val>
	//sub edx, edi
	cell_t val = jit->read_cell();
	IA32_Mov_Rm_Reg(jit, AMX_REG_ALT, AMX_REG_STK, MOD_REG);
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Add_Rm_Imm8(jit, AMX_REG_STK, (jit_int8_t)val, MOD_REG);
	else
		IA32_Add_Rm_Imm32(jit, AMX_REG_STK, val, MOD_REG);
	IA32_Sub_Rm_Reg(jit, AMX_REG_ALT, AMX_REG_DAT, MOD_REG);

	Write_CheckMargin_Stack(jit);
}

inline void WriteOp_Heap(JitWriter *jit)
{
	//mov edx, hea
	//add hea, <val>
	cell_t val = jit->read_cell();
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, AMX_INFO_FRM, AMX_INFO_HEAP);
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Add_Rm_Imm8_Disp8(jit, AMX_INFO_FRM, (jit_int8_t)val, AMX_INFO_HEAP);
	else
		IA32_Add_Rm_Imm32_Disp8(jit, AMX_INFO_FRM, val, AMX_INFO_HEAP);

	Write_CheckMargin_Heap(jit);
}

inline void WriteOp_SDiv(JitWriter *jit)
{
	//mov ecx, edx
	//mov edx, eax
	//sar edx, 31
	//idiv ecx
	IA32_Mov_Rm_Reg(jit, AMX_REG_TMP, AMX_REG_ALT, MOD_REG);
	Write_Check_DivZero(jit, AMX_REG_TMP);
	IA32_Mov_Rm_Reg(jit, AMX_REG_ALT, AMX_REG_PRI, MOD_REG);
	IA32_Sar_Rm_Imm8(jit, AMX_REG_ALT, 31, MOD_REG);
	IA32_IDiv_Rm(jit, AMX_REG_TMP, MOD_REG);
}


inline void WriteOp_SDiv_Alt(JitWriter *jit)
{
	//mov ecx, eax
	//mov eax, edx
	//sar edx, 31
	//idiv ecx
	IA32_Mov_Rm_Reg(jit, AMX_REG_TMP, AMX_REG_PRI, MOD_REG);
	Write_Check_DivZero(jit, AMX_REG_TMP);
	IA32_Mov_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Sar_Rm_Imm8(jit, AMX_REG_ALT, 31, MOD_REG);
	IA32_IDiv_Rm(jit, AMX_REG_TMP, MOD_REG);
}

inline void WriteOp_UDiv(JitWriter *jit)
{
	//mov ecx, edx
	//xor edx, edx
	//div ecx
	IA32_Mov_Rm_Reg(jit, AMX_REG_TMP, AMX_REG_ALT, MOD_REG);
	Write_Check_DivZero(jit, AMX_REG_TMP);
	IA32_Xor_Rm_Reg(jit, AMX_REG_ALT, AMX_REG_ALT, MOD_REG);
	IA32_Div_Rm(jit, AMX_REG_TMP, MOD_REG);
}

inline void WriteOp_UDiv_Alt(JitWriter *jit)
{
	//mov ecx, eax
	//mov eax, edx
	//xor edx, edx
	//div ecx
	IA32_Mov_Rm_Reg(jit, AMX_REG_TMP, AMX_REG_PRI, MOD_REG);
	Write_Check_DivZero(jit, AMX_REG_TMP);
	IA32_Mov_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Xor_Rm_Reg(jit, AMX_REG_ALT, AMX_REG_ALT, MOD_REG);
	IA32_Div_Rm(jit, AMX_REG_TMP, MOD_REG);
}

inline void WriteOp_Ret(JitWriter *jit)
{
	//mov ebx, [ebp]		- get old FRM
	//add ebp, 4			- pop stack
	//mov [esi+frm], ebx	- restore
	//add ebx, edi			- relocate
	//ret
	IA32_Mov_Reg_Rm(jit, AMX_REG_FRM, AMX_REG_STK, MOD_MEM_REG);
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_INFO, AMX_REG_FRM, AMX_INFO_FRM);
	IA32_Add_Rm_Reg(jit, AMX_REG_FRM, AMX_REG_DAT, MOD_REG);
	IA32_Return(jit);
}

inline void WriteOp_Retn(JitWriter *jit)
{
	//mov ebx, [ebp]		- get old frm
	//mov ecx, [ebp+4]		- get return eip
	//add ebp, 8			- pop stack
	//mov [esi+frm], ebx	- restore frame pointer
	//add ebx, edi			- relocate
	IA32_Mov_Reg_Rm(jit, AMX_REG_FRM, AMX_REG_STK, MOD_MEM_REG);
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_STK, 4);
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 8, MOD_REG);
	IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_INFO, AMX_REG_FRM, AMX_INFO_FRM);
	IA32_Add_Rm_Reg(jit, AMX_REG_FRM, AMX_REG_DAT, MOD_REG);
	
	//add ebp, [ebp]		- reduce by this # of params
	//add ebp, 4			- pop one extra for the # itself
	IA32_Add_Reg_Rm(jit, AMX_REG_STK, AMX_REG_STK, MOD_MEM_REG);
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);

	//jmp ecx				- jump to return eip
	IA32_Jump_Reg(jit, AMX_REG_TMP);
}

/*************************************************
 *************************************************
 * JIT PROPER ************************************
 * The rest of the from now on is the JIT engine *
 *************************************************
 *************************************************/


IPluginContext *JITX86::CompileToContext(ICompilation *co, int *err)
{
	CompData *data = (CompData *)co;
	sp_plugin_t *plugin = data->plugin;

	/* The first phase is to browse */
	uint8_t *code = plugin->pcode;
	uint8_t *cip;
	uint8_t *end_cip = plugin->pcode + plugin->pcode_size;
	OPCODE op;
	int op_c;
	uint32_t reloc_count = 0;

	/* FIRST PASS (light load) - Get initial opcode information */
	for (cip = code; cip < end_cip;)
	{
		op = (OPCODE)*(ucell_t *)cip;
		if ((unsigned)op >= OP_NUM_OPCODES)
		{
			AbortCompilation(co);
			*err = SP_ERR_INVALID_INSTRUCTION;
			return NULL;
		}
		cip += sizeof(cell_t);
		op_c = OpAdvTable[op];
		if (op_c >= 0)
		{
			cip += op_c;
		} else if (op_c == -3) {
			AbortCompilation(co);
			*err = SP_ERR_INVALID_INSTRUCTION;
			return NULL;
		} else if (op_c == -2) {
			reloc_count++;
			cip += sizeof(cell_t);
		} else if (op_c == -1) {
			switch (op)
			{
			case OP_CASETBL:
				{
					ucell_t num = *(ucell_t *)cip;
					cip += sizeof(cell_t);
					reloc_count += (num + 1);
					cip += ((2*num) + 1) * sizeof(cell_t);
					break;
				}
			default:
				{
					AbortCompilation(co);
					*err = SP_ERR_INVALID_INSTRUCTION;
					return NULL;
				}
			}
		}
	}

	/*********************************************
	 * SECOND PASS (medium load): writer.outbase is NULL, getting size only
	 * THIRD PASS (heavy load!!): writer.outbase is valid and output is written
	 *********************************************/

	JitWriter writer;
	JitWriter *jit = &writer;
	cell_t *endptr = (cell_t *)(end_cip);
	cell_t jitpos;

	/* Initial code is written "blank,"
	 * so we can check the exact memory usage.
	 */
	writer.inptr = (cell_t *)code;
	writer.outptr = NULL;
	writer.outbase = NULL;

//:TODO: Jump back here once finished!

	/* Initialize pass vars */
	data->jit_chkmargin_heap = 0;
	data->jit_verify_addr_eax = 0;
	data->jit_verify_addr_edx = 0;

	/* Start writing the actual code */
	data->jit_return = Write_Execute_Function(jit);

	/* Write error checking routines in case they are needed */
	jitpos = jit->jit_curpos();
	Write_Check_VerifyAddr(jit, REG_EAX, true);
	data->jit_verify_addr_eax = jitpos;

	jitpos = jit->jit_curpos();
	Write_Check_VerifyAddr(jit, REG_EDX, true);
	data->jit_verify_addr_edx = jitpos;

	jitpos = jit->jit_curpos();
	Write_CheckMargin_Heap(jit);
	data->jit_chkmargin_heap = jitpos;

	/* Begin opcode browsing */

	for (; writer.inptr <= endptr;)
	{
		op = (OPCODE)writer.read_cell();
		switch (op)
		{
		case OP_MOVE_PRI:
			{
				WriteOp_Move_Pri(jit);
				break;
			}
		case OP_MOVE_ALT:
			{
				WriteOp_Move_Alt(jit);
				break;
			}
		case OP_XCHG:
			{
				WriteOp_Xchg(jit);
				break;
			}
		case OP_PUSH:
			{
				WriteOp_Push(jit);
				break;
			}
		case OP_PUSH_S:
			{
				WriteOp_Push_S(jit);
				break;
			}
		case OP_PUSH2_C:
			{
				WriteOp_Push2_C(jit);
				break;
			}
		case OP_PUSH3_C:
			{
				WriteOp_Push3(jit);
				break;
			}
		case OP_PUSH4_C:
			{
				WriteOp_Push4_C(jit);
				break;
			}
		case OP_PUSH5_C:
			{
				WriteOp_Push5_C(jit);
				break;
			}
		case OP_PUSH2_ADR:
			{
				WriteOp_Push2_Adr(jit);
				break;
			}
		case OP_PUSH3_ADR:
			{
				WriteOp_Push3_Adr(jit);
				break;
			}
		case OP_PUSH4_ADR:
			{
				WriteOp_Push4_Adr(jit);
				break;
			}
		case OP_PUSH5_ADR:
			{
				WriteOp_Push5_Adr(jit);
				break;
			}
		case OP_PUSH2_S:
			{
				WriteOp_Push2_S(jit);
				break;
			}
		case OP_PUSH3_S:
			{
				WriteOp_Push3_S(jit);
				break;
			}
		case OP_PUSH4_S:
			{
				WriteOp_Push4_S(jit);
				break;
			}
		case OP_PUSH5_S:
			{
				WriteOp_Push5_S(jit);
				break;
			}
		case OP_PUSH5:
			{
				WriteOp_Push5(jit);
				break;
			}
		case OP_PUSH4:
			{
				WriteOp_Push4(jit);
				break;
			}
		case OP_PUSH3:
			{
				WriteOp_Push3(jit);
				break;
			}
		case OP_PUSH2:
			{
				WriteOp_Push2(jit);
				break;
			}
		case OP_ZERO_PRI:
			{
				WriteOp_Zero_Pri(jit);
				break;
			}
		case OP_ZERO_ALT:
			{
				WriteOp_Zero_Alt(jit);
				break;
			}
		case OP_PROC:
			{
				WriteOp_Proc(jit);
				break;
			}
		case OP_SHL:
			{
				WriteOp_Shl(jit);
				break;
			}
		case OP_SHR:
			{
				WriteOp_Shr(jit);
				break;
			}
		case OP_SSHR:
			{
				WriteOp_Sshr(jit);
				break;
			}
		case OP_SHL_C_PRI:
			{
				WriteOp_Shl_C_Pri(jit);
				break;
			}
		case OP_SHL_C_ALT:
			{
				WriteOp_Shl_C_Alt(jit);
				break;
			}
		case OP_SHR_C_PRI:
			{
				WriteOp_Shr_C_Pri(jit);
				break;
			}
		case OP_SHR_C_ALT:
			{
				WriteOp_Shr_C_Alt(jit);
				break;
			}
		case OP_SMUL:
			{
				WriteOp_SMul(jit);
				break;
			}
		case OP_UMUL:
			{
				WriteOp_UMul(jit);
				break;
			}
		case OP_ADD:
			{
				WriteOp_Add(jit);
				break;
			}
		case OP_SUB:
			{
				WriteOp_Sub(jit);
				break;
			}
		case OP_SUB_ALT:
			{
				WriteOp_Sub_Alt(jit);
				break;
			}
		case OP_NOP:
			{
				/* do nothing */
				break;
			}
		case OP_NOT:
			{
				WriteOp_Not(jit);
				break;
			}
		case OP_NEG:
			{
				WriteOp_Neg(jit);
				break;
			}
		case OP_XOR:
			{
				WriteOp_Xor(jit);
				break;
			}
		case OP_OR:
			{
				WriteOp_Or(jit);
				break;
			}
		case OP_AND:
			{
				WriteOp_And(jit);
				break;
			}
		case OP_INVERT:
			{
				WriteOp_Invert(jit);
				break;
			}
		case OP_ADD_C:
			{
				WriteOp_Add_C(jit);
				break;
			}
		case OP_SMUL_C:
			{
				WriteOp_SMul_C(jit);
				break;
			}
		case OP_SIGN_PRI:
			{
				WriteOp_Sign_Pri(jit);
				break;
			}
		case OP_SIGN_ALT:
			{
				WriteOp_Sign_Alt(jit);
				break;
			}
		case OP_EQ:
			{
				WriteOp_Eq(jit);
				break;
			}
		case OP_NEQ:
			{
				WriteOp_Neq(jit);
				break;
			}
		case OP_LESS:
			{
				WriteOp_Less(jit);
				break;
			}
		case OP_LEQ:
			{
				WriteOp_Leq(jit);
				break;
			}
		case OP_GRTR:
			{
				WriteOp_Grtr(jit);
				break;
			}
		case OP_GEQ:
			{
				WriteOp_Geq(jit);
				break;
			}
		case OP_SLESS:
			{
				WriteOp_Sless(jit);
				break;
			}
		case OP_SLEQ:
			{
				WriteOp_Sleq(jit);
				break;
			}
		case OP_SGRTR:
			{
				WriteOp_Sgrtr(jit);
				break;
			}
		case OP_SGEQ:
			{
				WriteOp_Sgeq(jit);
				break;
			}
		case OP_EQ_C_PRI:
			{
				WriteOp_Eq_C_Pri(jit);
				break;
			}
		case OP_EQ_C_ALT:
			{
				WriteOp_Eq_C_Alt(jit);
				break;
			}
		case OP_INC_PRI:
			{
				WriteOp_Inc_Pri(jit);
				break;
			}
		case OP_INC_ALT:
			{
				WriteOp_Inc_Alt(jit);
				break;
			}
		case OP_INC:
			{
				WriteOp_Inc(jit);
				break;
			}
		case OP_INC_S:
			{
				WriteOp_Inc_S(jit);
				break;
			}
		case OP_INC_I:
			{
				WriteOp_Inc_I(jit);
				break;
			}
		case OP_DEC_PRI:
			{
				WriteOp_Dec_Pri(jit);
				break;
			}
		case OP_DEC_ALT:
			{
				WriteOp_Dec_Alt(jit);
				break;
			}
		case OP_DEC:
			{
				WriteOp_Dec(jit);
				break;
			}
		case OP_DEC_S:
			{
				WriteOp_Dec_S(jit);
				break;
			}
		case OP_DEC_I:
			{
				WriteOp_Dec_I(jit);
				break;
			}
		case OP_LOAD_PRI:
			{
				WriteOp_Load_Pri(jit);
				break;
			}
		case OP_LOAD_ALT:
			{
				WriteOp_Load_Alt(jit);
				break;
			}
		case OP_LOAD_S_PRI:
			{
				WriteOp_Load_S_Pri(jit);
				break;
			}
		case OP_LOAD_S_ALT:
			{
				WriteOp_Load_S_Alt(jit);
				break;
			}
		case OP_LREF_PRI:
			{
				WriteOp_Lref_Pri(jit);
				break;
			}
		case OP_LREF_ALT:
			{
				WriteOp_Lref_Alt(jit);
				break;
			}
		case OP_LREF_S_PRI:
			{
				WriteOp_Lref_Pri(jit);
				break;
			}
		case OP_LREF_S_ALT:
			{
				WriteOp_Lref_Alt(jit);
				break;
			}
		case OP_CONST_PRI:
			{
				WriteOp_Const_Pri(jit);
				break;
			}
		case OP_CONST_ALT:
			{
				WriteOp_Const_Alt(jit);
				break;
			}
		case OP_ADDR_PRI:
			{
				WriteOp_Addr_Pri(jit);
				break;
			}
		case OP_ADDR_ALT:
			{
				WriteOp_Addr_Alt(jit);
				break;
			}
		case OP_STOR_PRI:
			{
				WriteOp_Stor_Pri(jit);
				break;
			}
		case OP_STOR_ALT:
			{
				WriteOp_Stor_Alt(jit);
				break;
			}
		case OP_STOR_S_PRI:
			{
				WriteOp_Stor_S_Pri(jit);
				break;
			}
		case OP_STOR_S_ALT:
			{
				WriteOp_Stor_S_Alt(jit);
				break;
			}
		case OP_IDXADDR:
			{
				WriteOp_Idxaddr(jit);
				break;
			}
		case OP_SREF_PRI:
			{
				WriteOp_Sref_Pri(jit);
				break;
			}
		case OP_SREF_ALT:
			{
				WriteOp_Sref_Alt(jit);
				break;
			}
		case OP_SREF_S_PRI:
			{
				WriteOp_Sref_S_Pri(jit);
				break;
			}
		case OP_SREF_S_ALT:
			{
				WriteOp_Sref_S_Alt(jit);
				break;
			}
		case OP_ALIGN_PRI:
			{
				WriteOp_Align_Pri(jit);
				break;
			}
		case OP_ALIGN_ALT:
			{
				WriteOp_Align_Alt(jit);
				break;
			}
		case OP_POP_PRI:
			{
				WriteOp_Pop_Pri(jit);
				break;
			}
		case OP_POP_ALT:
			{
				WriteOp_Pop_Alt(jit);
				break;
			}
		case OP_SWAP_PRI:
			{
				WriteOp_Swap_Pri(jit);
				break;
			}
		case OP_SWAP_ALT:
			{
				WriteOp_Swap_Alt(jit);
				break;
			}
		case OP_PUSH_ADR:
			{
				WriteOp_PushAddr(jit);
				break;
			}
		case OP_MOVS:
			{
				WriteOp_Movs(jit);
				break;
			}
		case OP_FILL:
			{
				WriteOp_Fill(jit);
				break;
			}
		case OP_HEAP_PRI:
			{
				WriteOp_Heap_Pri(jit);
				break;
			}
		case OP_PUSH_HEAP_C:
			{
				WriteOp_Push_Heap_C(jit);
				break;
			}
		case OP_POP_HEAP_PRI:
			{
				WriteOp_Pop_Heap_Pri(jit);
				break;
			}
		case OP_PUSH_C:
			{
				WriteOp_Push_C(jit);
				break;
			}
		case OP_ZERO:
			{
				WriteOp_Zero(jit);
				break;
			}
		case OP_ZERO_S:
			{
				WriteOp_Zero_S(jit);
				break;
			}
		case OP_PUSH_PRI:
			{
				WriteOp_Push_Pri(jit);
				break;
			}
		case OP_PUSH_ALT:
			{
				WriteOp_Push_Alt(jit);
				break;
			}
		case OP_LOAD_BOTH:
			{
				WriteOp_Load_Both(jit);
				break;
			}
		case OP_LOAD_S_BOTH:
			{
				WriteOp_Load_S_Both(jit);
				break;
			}
		case OP_CONST:
			{
				WriteOp_Const(jit);
				break;
			}
		case OP_CONST_S:
			{
				WriteOp_Const_S(jit);
				break;
			}
		case OP_LOAD_I:
			{
				WriteOp_Load_I(jit);
				break;
			}
		case OP_LODB_I:
			{
				WriteOp_Lodb_I(jit);
				break;
			}
		case OP_STOR_I:
			{
				WriteOp_Stor_I(jit);
				break;
			}
		case OP_STRB_I:
			{
				WriteOp_Strb_I(jit);
				break;
			}
		case OP_LIDX:
			{
				WriteOp_Lidx(jit);
				break;
			}
		case OP_LIDX_B:
			{
				WriteOp_Lidx_B(jit);
				break;
			}
		case OP_IDXADDR_B:
			{
				WriteOp_Idxaddr_B(jit);
				break;
			}
		case OP_LCTRL:
			{
				WriteOp_Lctrl(jit);
				break;
			}
		case OP_SCTRL:
			{
				WriteOp_Sctrl(jit);
				break;
			}
		case OP_STACK:
			{
				WriteOp_Stack(jit);
				break;
			}
		case OP_HEAP:
			{
				WriteOp_Heap(jit);
				break;
			}
		case OP_SDIV:
			{
				WriteOp_SDiv(jit);
				break;
			}
		case OP_SDIV_ALT:
			{
				WriteOp_SDiv_Alt(jit);
				break;
			}
		case OP_UDIV:
			{
				WriteOp_UDiv(jit);
				break;
			}
		case OP_UDIV_ALT:
			{
				WriteOp_UDiv_Alt(jit);
				break;
			}
		case OP_RET:
			{
				WriteOp_Ret(jit);
				break;
			}
		case OP_RETN:
			{
				WriteOp_Retn(jit);
				break;
			}
		default:
			{
				AbortCompilation(co);
				*err = SP_ERR_INVALID_INSTRUCTION;
				return NULL;
			}
		}
	}

	*err = SP_ERR_NONE;

	return NULL;
}

const char *JITX86::GetVMName()
{
	return "JIT (x86)";
}

ICompilation *JITX86::StartCompilation(sp_plugin_t *plugin)
{
	CompData *data = new CompData;

	data->plugin = plugin;

	return data;
}

void JITX86::AbortCompilation(ICompilation *co)
{
	delete (CompData *)co;
}

bool JITX86::SetCompilationOption(ICompilation *co, const char *key, const char *val)
{
	CompData *data = (CompData *)co;

	if (strcmp(key, "debug") == 0)
	{
		data->debug = (atoi(val) == 1);
		if (data->debug && !(data->plugin->flags & SP_FLAG_DEBUG))
		{
			data->debug = false;
			return false;
		}
		return true;
	}

	return false;
}
