#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "jit_x86.h"
#include "opcode_helpers.h"
#include "x86_macros.h"

#if defined USE_UNGEN_OPCODES
#include "ungen_opcodes.h"
#endif

inline void WriteOp_Move_Pri(JitWriter *jit)
{
	//mov eax, edx
	IA32_Mov_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_Move_Alt(JitWriter *jit)
{
	//mov edx, eax
	IA32_Mov_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_PRI, MOD_REG);
}

inline void WriteOp_Xchg(JitWriter *jit)
{
	//xchg eax, edx
	IA32_Xchg_Eax_Reg(jit, AMX_REG_ALT);
}

inline void WriteOp_Push(JitWriter *jit)
{
	//push stack, DAT offset based
	//sub edi, 4
	//mov ecx, [ebp+<val>]
	//mov [edi], ecx
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
	//mov [edi-4], <val>
	//sub edi, 4
	cell_t val = jit->read_cell();
	IA32_Mov_Rm_Imm32_Disp8(jit, AMX_REG_STK, val, -4);
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
}

inline void WriteOp_Zero(JitWriter *jit)
{
	//mov [ebp+<val>], 0
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
	//sub edi, 4
	//mov ecx, [ebx+<val>]
	//mov [edi], ecx
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
	//mov [edi-4], eax
	//sub edi, 4
	IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_STK, AMX_REG_PRI, -4);
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
}

inline void WriteOp_Push_Alt(JitWriter *jit)
{
	//mov [edi-4], edx
	//sub edi, 4
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
	IA32_Xor_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_PRI, MOD_REG);
}

inline void WriteOp_Zero_Alt(JitWriter *jit)
{
	//xor edx, edx
	IA32_Xor_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_Add(JitWriter *jit)
{
	//add eax, edx
	IA32_Add_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_Sub(JitWriter *jit)
{
	//sub eax, edx
	IA32_Sub_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_Sub_Alt(JitWriter *jit)
{
	//neg eax
	//add eax, edx
	IA32_Neg_Rm(jit, AMX_REG_PRI, MOD_REG);
	IA32_Add_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_Proc(JitWriter *jit)
{
	//push old frame on stack:
	//sub edi, 4
	//mov ecx, [esi+frm]
	//mov [edi], ecx
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_INFO, MOD_MEM_REG);
	IA32_Mov_Rm_Reg(jit, AMX_REG_STK, AMX_REG_TMP, MOD_MEM_REG);
	//save frame:
	//:TODO: move to a temp reg, subtract and then move to mem, faster??
	//mov [esi+frm], edi	- get new frame
	//mov ebx, edi			- store frame back
	//sub [esi+frm], ebp	- relocate local frame
	IA32_Mov_Rm_Reg(jit, AMX_REG_INFO, AMX_REG_STK, MOD_MEM_REG);
	IA32_Mov_Reg_Rm(jit, AMX_REG_FRM, AMX_REG_STK, MOD_REG);
	IA32_Sub_Rm_Reg(jit, AMX_REG_INFO, AMX_REG_DAT, MOD_MEM_REG);
}

inline void WriteOp_Lidx_B(JitWriter *jit)
{
	cell_t val = jit->read_cell();
	//shl eax, <val>
	//add eax, edx
	//mov eax, [ebp+eax]
	IA32_Shl_Rm_Imm8(jit, AMX_REG_PRI, (jit_uint8_t)val, MOD_REG);
	IA32_Add_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	Write_Check_VerifyAddr(jit, AMX_REG_PRI, false);
	IA32_Mov_Reg_RmEBP_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
}

inline void WriteOp_Idxaddr_B(JitWriter *jit)
{
	//shl eax, <val>
	//add eax, edx
	cell_t val = jit->read_cell();
	IA32_Shl_Rm_Imm8(jit, AMX_REG_PRI, (jit_uint8_t)val, MOD_REG);
	IA32_Add_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_Shl(JitWriter *jit)
{
	//mov ecx, edx
	//shl eax, cl
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_ALT, MOD_REG);
	IA32_Shl_Rm_CL(jit, AMX_REG_PRI, MOD_REG);
}

inline void WriteOp_Shr(JitWriter *jit)
{
	//mov ecx, edx
	//shr eax, cl
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_ALT, MOD_REG);
	IA32_Shr_Rm_CL(jit, AMX_REG_PRI, MOD_REG);
}

inline void WriteOp_Sshr(JitWriter *jit)
{
	//mov ecx, edx
	//sar eax, cl
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_ALT, MOD_REG);
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
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_ALT, MOD_REG);
	IA32_IMul_Rm(jit, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_TMP, MOD_REG);
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
	IA32_Xor_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_Or(JitWriter *jit)
{
	//or eax, edx
	IA32_Or_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_And(JitWriter *jit)
{
	//and eax, edx
	IA32_And_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
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
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_E);
}

inline void WriteOp_Neq(JitWriter *jit)
{
	//cmp eax, edx	; PRI != ALT ?
	//mov eax, 0
	//setne al
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_NE);
}

inline void WriteOp_Sless(JitWriter *jit)
{
	//cmp eax, edx	; PRI < ALT ? (signed)
	//mov eax, 0
	//setl al
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_L);
}

inline void WriteOp_Sleq(JitWriter *jit)
{
	//cmp eax, edx	; PRI <= ALT ? (signed)
	//mov eax, 0
	//setle al
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_LE);
}

inline void WriteOp_Sgrtr(JitWriter *jit)
{
	//cmp eax, edx	; PRI > ALT ? (signed)
	//mov eax, 0
	//setg al
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_G);
}

inline void WriteOp_Sgeq(JitWriter *jit)
{
	//cmp eax, edx	; PRI >= ALT ? (signed)
	//mov eax, 0
	//setge al
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
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
	IA32_Xor_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_PRI, MOD_REG);
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
	//add [ebp+<val>], 1
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
	//add [ebp+eax], 1
	IA32_Add_RmEBP_Imm8_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_PRI, NOSCALE, 1);
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
	//sub [ebp+<val>], 1
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
	//sub [ebp+eax], 1
	IA32_Sub_RmEBP_Imm8_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_PRI, NOSCALE, 1);
}

inline void WriteOp_Load_Pri(JitWriter *jit)
{
	//mov eax, [ebp+<val>]
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, AMX_REG_DAT, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_PRI, AMX_REG_DAT, val);
}

inline void WriteOp_Load_Alt(JitWriter *jit)
{
	//mov edx, [ebp+<val>]
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
	//mov eax, [ebp+<val>]
	//mov eax, [ebp+eax]
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, AMX_REG_DAT, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_PRI, AMX_REG_DAT, val);
	IA32_Mov_Reg_RmEBP_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
}

inline void WriteOp_Lref_Alt(JitWriter *jit)
{
	//mov edx, [ebp+<val>]
	//mov edx, [ebp+edx]
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, AMX_REG_DAT, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_ALT, AMX_REG_DAT, val);
	IA32_Mov_Reg_RmEBP_Disp_Reg(jit, AMX_REG_ALT, AMX_REG_DAT, AMX_REG_ALT, NOSCALE);
}

inline void WriteOp_Lref_S_Pri(JitWriter *jit)
{
	//mov eax, [ebx+<val>]
	//mov eax, [ebp+eax]
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, AMX_REG_FRM, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_PRI, AMX_REG_FRM, val);
	IA32_Mov_Reg_RmEBP_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
}

inline void WriteOp_Lref_S_Alt(JitWriter *jit)
{
	//mov edx, [ebx+<val>]
	//mov edx, [ebp+edx]
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, AMX_REG_FRM, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_ALT, AMX_REG_FRM, val);
	IA32_Mov_Reg_RmEBP_Disp_Reg(jit, AMX_REG_ALT, AMX_REG_DAT, AMX_REG_ALT, NOSCALE);
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
	//mov eax, [esi+frm]
	//add eax, <val>
	cell_t val = jit->read_cell();
	IA32_Mov_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_INFO, MOD_MEM_REG);
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Add_Rm_Imm8(jit, AMX_REG_PRI, (jit_int8_t)val, MOD_REG);
	else
		IA32_Add_Eax_Imm32(jit, val);
}

inline void WriteOp_Addr_Alt(JitWriter *jit)
{
	//mov edx, [esi+frm]
	//add edx, <val>
	cell_t val = jit->read_cell();
	IA32_Mov_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_INFO, MOD_MEM_REG);
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Add_Rm_Imm8(jit, AMX_REG_ALT, (jit_int8_t)val, MOD_REG);
	else
		IA32_Add_Rm_Imm32(jit, AMX_REG_ALT, val, MOD_REG);
}

inline void WriteOp_Stor_Pri(JitWriter *jit)
{
	//mov [ebp+<val>], eax
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_DAT, AMX_REG_PRI, (jit_int8_t)val);
	else
		IA32_Mov_Rm_Reg_Disp32(jit, AMX_REG_DAT, AMX_REG_PRI, val);
}

inline void WriteOp_Stor_Alt(JitWriter *jit)
{
	//mov [ebp+<val>], edx
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

inline void WriteOp_Sref_Pri(JitWriter *jit)
{
	//mov ecx, [ebp+<val>]
	//mov [ebp+ecx], eax
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_DAT, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_DAT, val);
	IA32_Mov_RmEBP_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_TMP, NOSCALE, AMX_REG_PRI);
}

inline void WriteOp_Sref_Alt(JitWriter *jit)
{
	//mov ecx, [ebp+<val>]
	//mov [ebp+ecx], edx
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_DAT, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_DAT, val);
	IA32_Mov_RmEBP_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_TMP, NOSCALE, AMX_REG_ALT);
}

inline void WriteOp_Sref_S_Pri(JitWriter *jit)
{
	//mov ecx, [ebx+<val>]
	//mov [ebp+ecx], eax
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_FRM, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_FRM, val);
	IA32_Mov_RmEBP_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_TMP, NOSCALE, AMX_REG_PRI);
}

inline void WriteOp_Sref_S_Alt(JitWriter *jit)
{
	//mov ecx, [ebx+<val>]
	//mov [ebp+ecx], edx
	cell_t val = jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_FRM, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_FRM, val);
	IA32_Mov_RmEBP_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_TMP, NOSCALE, AMX_REG_ALT);
}

inline void WriteOp_Pop_Pri(JitWriter *jit)
{
	//mov eax, [edi]
	//add edi, 4
	IA32_Mov_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_STK, MOD_MEM_REG);
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
}

inline void WriteOp_Pop_Alt(JitWriter *jit)
{
	//mov edx, [edi]
	//add edi, 4
	IA32_Mov_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_STK, MOD_MEM_REG);
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
}

inline void WriteOp_Swap_Pri(JitWriter *jit)
{
	//add [edi], eax
	//sub eax, [edi]
	//add [edi], eax
	//neg eax
	IA32_Add_Rm_Reg(jit, AMX_REG_STK, AMX_REG_PRI, MOD_MEM_REG);
	IA32_Sub_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_STK, MOD_MEM_REG);
	IA32_Add_Rm_Reg(jit, AMX_REG_STK, AMX_REG_PRI, MOD_MEM_REG);
	IA32_Neg_Rm(jit, AMX_REG_PRI, MOD_REG);
}

inline void WriteOp_Swap_Alt(JitWriter *jit)
{
	//add [edi], edx
	//sub edx, [edi]
	//add [edi], edx
	//neg edx
	IA32_Add_Rm_Reg(jit, AMX_REG_STK, AMX_REG_ALT, MOD_MEM_REG);
	IA32_Sub_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_STK, MOD_MEM_REG);
	IA32_Add_Rm_Reg(jit, AMX_REG_STK, AMX_REG_ALT, MOD_MEM_REG);
	IA32_Neg_Rm(jit, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_PushAddr(JitWriter *jit)
{
	//mov ecx, [esi+frm]	;get address (offset from frame)
	//add ecx, <val>
	//mov [edi-4], ecx
	//sub edi, 4
	cell_t val = jit->read_cell();
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_INFO, MOD_MEM_REG);
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
	//push edi
	//lea esi, [ebp+eax]
	//lea edi, [ebp+edx]
	//if dword:
	// mov ecx, dword
	// rep movsd
	//if byte:
	// mov ecx, byte
	// rep movsb
	//pop edi
	//pop esi
	cell_t val = jit->read_cell();
	unsigned int dwords = val >> 2;
	unsigned int bytes = val & 0x3;

	IA32_Cld(jit);
	IA32_Push_Reg(jit, REG_ESI);
	IA32_Push_Reg(jit, REG_EDI);
	IA32_Lea_Reg_DispEBPRegMult(jit, REG_ESI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
	IA32_Lea_Reg_DispEBPRegMult(jit, REG_EDI, AMX_REG_DAT, AMX_REG_ALT, NOSCALE);
	if (dwords)
	{
		IA32_Mov_Reg_Imm32(jit, REG_ECX, dwords);
		IA32_Rep(jit);
		IA32_Movsd(jit);
	}
	if (bytes)
	{
		IA32_Mov_Reg_Imm32(jit, REG_ECX, bytes);
		IA32_Rep(jit);
		IA32_Movsb(jit);
	}
	IA32_Pop_Reg(jit, REG_EDI);
	IA32_Pop_Reg(jit, REG_ESI);
}

inline void WriteOp_Fill(JitWriter *jit)
{
	//push edi
	//lea edi, [ebp+edx]
	//mov ecx, <val> >> 2
	//cld
	//rep stosd
	//pop edi
	unsigned int val = jit->read_cell() >> 2;

	IA32_Push_Reg(jit, REG_EDI);
	IA32_Lea_Reg_DispEBPRegMult(jit, REG_EDI, AMX_REG_DAT, AMX_REG_ALT, NOSCALE);
	IA32_Mov_Reg_Imm32(jit, REG_ECX, val);
	IA32_Cld(jit);
	IA32_Rep(jit);
	IA32_Stosd(jit);
	IA32_Pop_Reg(jit, REG_EDI);
}

inline void WriteOp_Heap_Pri(JitWriter *jit)
{
	//mov edx, [esi+hea]
	//add [esi+hea], eax
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, AMX_REG_INFO, AMX_INFO_HEAP);
	IA32_Add_Rm_Reg_Disp8(jit, AMX_REG_INFO, AMX_REG_PRI, AMX_INFO_HEAP);

	Write_CheckMargin_Heap(jit);
}

inline void WriteOp_Push_Heap_C(JitWriter *jit)
{
	//mov ecx, [esi+hea]
	//mov [ebp+ecx], <val>
	//add [esi+hea], 4
	cell_t val = jit->read_cell();
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_HEAP);
	IA32_Mov_RmEBP_Imm32_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_TMP, NOSCALE, val);
	IA32_Add_Rm_Imm8_Disp8(jit, AMX_REG_INFO, 4, AMX_INFO_HEAP);

	Write_CheckMargin_Heap(jit);
}

inline void WriteOp_Pop_Heap_Pri(JitWriter *jit)
{
	//sub [esi+hea], 4
	//mov ecx, [esi+hea]
	//mov eax, [ebp+ecx]
	IA32_Sub_Rm_Imm8_Disp8(jit, AMX_REG_INFO, 4, AMX_INFO_HEAP);
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_HEAP);
	IA32_Mov_Reg_RmEBP_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_TMP, NOSCALE);

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
	//mov [ebp+<addr>], <val>
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
	//mov eax, [ebp+eax]
	Write_Check_VerifyAddr(jit, AMX_REG_PRI, false);
	IA32_Mov_Reg_RmEBP_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
}

inline void WriteOp_Stor_I(JitWriter *jit)
{
	//mov [ebp+edx], eax
	Write_Check_VerifyAddr(jit, AMX_REG_ALT, false);
	IA32_Mov_RmEBP_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_ALT, NOSCALE, AMX_REG_PRI);
}

inline void WriteOp_Lidx(JitWriter *jit)
{
	//lea eax, [edx+4*eax]
	//mov eax, [ebp+eax]
	IA32_Lea_Reg_DispRegMult(jit, AMX_REG_PRI, AMX_REG_ALT, AMX_REG_PRI, SCALE4);
	Write_Check_VerifyAddr(jit, AMX_REG_PRI, false);
	IA32_Mov_Reg_RmEBP_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
}

inline void WriteOp_Stack(JitWriter *jit)
{
	//mov edx, edi
	//add edi, <val>
	//sub edx, ebp
	cell_t val = jit->read_cell();
	IA32_Mov_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_STK, MOD_REG);
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Add_Rm_Imm8(jit, AMX_REG_STK, (jit_int8_t)val, MOD_REG);
	else
		IA32_Add_Rm_Imm32(jit, AMX_REG_STK, val, MOD_REG);
	IA32_Sub_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_DAT, MOD_REG);

	Write_CheckMargin_Stack(jit);
}

inline void WriteOp_Heap(JitWriter *jit)
{
	//mov edx, [esi+hea]
	//add [esi+hea], <val>
	cell_t val = jit->read_cell();
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, AMX_REG_INFO, AMX_INFO_HEAP);
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Add_Rm_Imm8_Disp8(jit, AMX_REG_INFO, (jit_int8_t)val, AMX_INFO_HEAP);
	else
		IA32_Add_Rm_Imm32_Disp8(jit, AMX_REG_INFO, val, AMX_INFO_HEAP);

	Write_CheckMargin_Heap(jit);
}

inline void WriteOp_SDiv(JitWriter *jit)
{
	//mov ecx, edx
	//mov edx, eax
	//sar edx, 31
	//idiv ecx
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_PRI, MOD_REG);
	IA32_Sar_Rm_Imm8(jit, AMX_REG_ALT, 31, MOD_REG);
	Write_Check_DivZero(jit, AMX_REG_TMP);
	IA32_IDiv_Rm(jit, AMX_REG_TMP, MOD_REG);
}

inline void WriteOp_SDiv_Alt(JitWriter *jit)
{
	//mov ecx, eax
	//mov eax, edx
	//sar edx, 31
	//idiv ecx
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_PRI, MOD_REG);
	IA32_Mov_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Sar_Rm_Imm8(jit, AMX_REG_ALT, 31, MOD_REG);
	Write_Check_DivZero(jit, AMX_REG_TMP);
	IA32_IDiv_Rm(jit, AMX_REG_TMP, MOD_REG);
}

inline void WriteOp_Retn(JitWriter *jit)
{
	//mov ebx, [edi]		- get old frm
	//mov ecx, [edi+4]		- get return eip
	//add edi, 8			- pop stack
	//mov [esi+frm], ebx	- restore frame pointer
	//add ebx, ebp			- relocate
	IA32_Mov_Reg_Rm(jit, AMX_REG_FRM, AMX_REG_STK, MOD_MEM_REG);
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_STK, 4);
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 8, MOD_REG);
	IA32_Mov_Rm_Reg(jit, AMX_REG_INFO, AMX_REG_FRM, MOD_MEM_REG);
	IA32_Add_Reg_Rm(jit, AMX_REG_FRM, AMX_REG_DAT, MOD_REG);

	//add edi, [edi]		- reduce by this # of params
	//add edi, 4			- pop one extra for the # itself
	IA32_Add_Reg_Rm(jit, AMX_REG_STK, AMX_REG_STK, MOD_MEM_REG);
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);

	//jmp ecx				- jump to return eip
	IA32_Jump_Reg(jit, AMX_REG_TMP);
}

inline void WriteOp_Call(JitWriter *jit)
{
	cell_t offs = jit->read_cell();

	jitoffs_t jmp = IA32_Call_Imm32(jit, 0);
	IA32_Write_Jump32(jit, jmp, RelocLookup(jit, offs, false));
}

inline void WriteOp_Bounds(JitWriter *jit)
{
	Write_BoundsCheck(jit);
}

inline void WriteOp_Halt(JitWriter *jit)
{
	//mov ecx, [esi+ret]
	//mov [ecx], eax
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_RETVAL);
	IA32_Mov_Rm_Reg(jit, AMX_REG_TMP, AMX_REG_PRI, MOD_MEM_REG);
	
	/* :TODO: 
	 * We don't support sleeping or halting with weird values.
	 * So we're omitting the mov eax <val> that was here.
	 */
	jit->read_cell();

	CompData *data = (CompData *)jit->data;
	jitoffs_t reloc;
	if (data->inline_level & JIT_INLINE_ERRORCHECKS)
	{
		reloc = IA32_Jump_Imm32(jit, 0);
	} else {
		reloc = IA32_Call_Imm32(jit, 0);
	}
	IA32_Write_Jump32(jit, reloc, data->jit_return);
}

inline void WriteOp_Break(JitWriter *jit)
{
	CompData *data = (CompData *)jit->data;
	if (data->debug)
	{
		//mov ecx, <cip>
		jitoffs_t wr = IA32_Mov_Reg_Imm32(jit, AMX_REG_TMP, 0);
		jitoffs_t save = jit->jit_curpos();
		jit->setpos(wr);
		jit->write_uint32((uint32_t)(jit->outbase + wr));
		jit->setpos(save);
		
		wr = IA32_Call_Imm32(jit, 0);
		IA32_Write_Jump32(jit, wr, data->jit_break);
	}
}

inline void WriteOp_Jump(JitWriter *jit)
{
	//jmp <offs>
	cell_t amx_offs = jit->read_cell();

	IA32_Jump_Imm32_Abs(jit, RelocLookup(jit, amx_offs, false));
}

inline void WriteOp_Jzer(JitWriter *jit)
{
	//test eax, eax
	//jz <target>
	cell_t target = jit->read_cell();
	IA32_Test_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_PRI, MOD_REG);
	IA32_Jump_Cond_Imm32_Abs(jit, CC_Z, RelocLookup(jit, target, false));
}

inline void WriteOp_Jnz(JitWriter *jit)
{
	//test eax, eax
	//jnz <target>
	cell_t target = jit->read_cell();
	IA32_Test_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_PRI, MOD_REG);
	IA32_Jump_Cond_Imm32_Abs(jit, CC_NZ, RelocLookup(jit, target, false));
}

inline void WriteOp_Jeq(JitWriter *jit)
{
	//cmp eax, edx
	//je <target>
	cell_t target = jit->read_cell();
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Jump_Cond_Imm32_Abs(jit, CC_E, RelocLookup(jit, target, false));
}

inline void WriteOp_Jneq(JitWriter *jit)
{
	//cmp eax, edx
	//jne <target>
	cell_t target = jit->read_cell();
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Jump_Cond_Imm32(jit, CC_NE, RelocLookup(jit, target, false));
}

inline void WriteOp_Jsless(JitWriter *jit)
{
	//cmp eax, edx
	//jl <target>
	cell_t target = jit->read_cell();
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Jump_Cond_Imm32(jit, CC_L, RelocLookup(jit, target, false));
}

inline void WriteOp_Jsleq(JitWriter *jit)
{
	//cmp eax, edx
	//jle <target>
	cell_t target = jit->read_cell();
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Jump_Cond_Imm32(jit, CC_LE, RelocLookup(jit, target, false));
}

inline void WriteOp_JsGrtr(JitWriter *jit)
{
	//cmp eax, edx
	//jg <target>
	cell_t target = jit->read_cell();
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Jump_Cond_Imm32(jit, CC_G, RelocLookup(jit, target, false));
}

inline void WriteOp_JsGeq(JitWriter *jit)
{
	//cmp eax, edx
	//jge <target>
	cell_t target = jit->read_cell();
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Jump_Cond_Imm32(jit, CC_GE, RelocLookup(jit, target, false));
}

inline void WriteOp_Switch(JitWriter *jit)
{
	cell_t offs = jit->read_cell();
	cell_t *tbl = (cell_t *)((char *)jit->inbase + offs);

	struct casetbl
	{
		cell_t val;
		cell_t offs;
	};

	/* Read the number of cases, then advance by one */
	cell_t num_cases = *tbl++;
	if (!num_cases)
	{
		/* Special treatment for 0 cases */
		//jmp <default>
		IA32_Jump_Imm32_Abs(jit, RelocLookup(jit, *tbl, false));
	} else {
		/* Check if the case layout is fully sequential */
		casetbl *iter = (casetbl *)(tbl + 1);
		casetbl *cases = iter;
		cell_t first = iter[0].val;
		cell_t last = first;
		bool sequential = true;
		for (cell_t i=1; i<num_cases; i++)
		{
			if (iter[i].val != ++last)
			{
				sequential = false;
				break;
			}
		}
		/* Time to generate. 
		 * First check whether the bounds are correct: if (a < LOW || a > HIGH)
		 * This check is valid for both sequential and non-sequential.
		 */
		cell_t low_bound = cases[0].val;
		if (low_bound != 0)
		{
			/* negate it so we'll get a lower bound of 0 */
			//lea ecx, [eax-<LOWER_BOUND>]
			low_bound = -low_bound;
			if (low_bound > SCHAR_MIN && low_bound < SCHAR_MAX)
			{
				IA32_Lea_DispRegImm8(jit, AMX_REG_TMP, AMX_REG_PRI, low_bound);
			} else {
				IA32_Lea_DispRegImm32(jit, AMX_REG_TMP, AMX_REG_PRI, low_bound);
			}
		}
		cell_t high_bound = abs(cases[0].val - cases[num_cases-1].val);
		//cmp ecx, <UPPER BOUND BOUND>
		if (high_bound > SCHAR_MIN && high_bound < SCHAR_MAX)
		{
			IA32_Cmp_Rm_Imm8(jit, MOD_REG, AMX_REG_TMP, high_bound);
		} else {
			IA32_Cmp_Rm_Imm32(jit, MOD_REG, AMX_REG_TMP, high_bound);
		}
		//ja <default case>
		IA32_Jump_Cond_Imm32_Abs(jit, CC_A, RelocLookup(jit, *tbl, false));

		/**
		 * Now we've taken the default case out of the way, it's time to do the
		 * full check, which is different for sequential vs. non-sequential.
		 */
		if (sequential)
		{
			/* we're now theoretically guaranteed to be jumping to a correct offset.
			 * ECX still has the correctly bound offset in it, luckily!
			 * thus, we simply need to relocate ECX and store the cases.
			 */
			//shr ecx, 2
			//add ecx, <case table start>
			IA32_Shr_Rm_Imm8(jit, AMX_REG_TMP, 2, MOD_REG);
			jitoffs_t tbl_offs = IA32_Add_Rm_Imm32_Later(jit, AMX_REG_TMP, MOD_REG);
			IA32_Jump_Rm(jit, AMX_REG_TMP, MOD_MEM_REG);
			/* The case table starts here.  Go back and write the output pointer. */
			jitoffs_t cur_pos = jit->jit_curpos();
			jit->setpos(tbl_offs);
			jit->write_uint32((jit_uint32_t)(jit->outbase + cur_pos));
			jit->setpos(cur_pos);
			//now we can write the case table, finally!
			jit_uint32_t base = (jit_uint32_t)jit->outbase;
			for (cell_t i=0; i<num_cases; i++)
			{
				jit->write_uint32(base + RelocLookup(jit, cases[i].offs, false));
			}
		} else {
			/* The slow version.  Go through each case and generate a check.
			 * In the future we should replace case tables of more than ~8 cases with a
			 * hash table lookup.
			 */
			cell_t val;
			for (cell_t i=0; i<num_cases; i++)
			{
				val = cases[i].val;
				//cmp eax, <val> OR cmp al, <val>
				if (val > SCHAR_MIN && val < SCHAR_MAX)
				{
					IA32_Cmp_Al_Imm8(jit, val);
				} else {
					IA32_Cmp_Eax_Imm32(jit, cases[i].val);
				}
				IA32_Jump_Cond_Imm32_Abs(jit, CC_E, RelocLookup(jit, cases[i].offs, false));
			}
		}
	}
}

inline void WriteOp_Casetbl(JitWriter *jit)
{
	/* do nothing here, switch does all ze work */
	cell_t num_cases = jit->read_cell();

	/* Two cells per case, one extra case for the default jump */
	num_cases = (num_cases * 2) + 1;
	jit->inptr += num_cases;
}


/*************************************************
 *************************************************
 * JIT PROPER ************************************
 * The rest from now on is the JIT engine        *
 *************************************************
 *************************************************/


jitoffs_t RelocLookup(JitWriter *jit, cell_t pcode_offs, bool relative)
{
	if (jit->outptr)
	{
		CompData *data = (CompData *)jit->data;
		if (relative)
		{
			/* The actual offset is EIP relative. We need to relocate it.
			 * Note that this assumes that we're pointing to the next op.
			 */
			pcode_offs += jit->inputrel();
		}
		/* Offset must always be 1)positive and 2)less than the codesize */
		assert(pcode_offs >= 0 && (uint32_t)pcode_offs < data->codesize);
		/* Do the lookup in the native dictionary. */
		return *(jitoffs_t *)(data->rebase + pcode_offs);
	} else {
		return 0;
	}
}

sp_context_t *JITX86::CompileToContext(ICompilation *co, int *err)
{
	CompData *data = (CompData *)co;
	sp_plugin_t *plugin = data->plugin;

	/* The first phase is to browse */
	uint8_t *code = plugin->pcode;
	uint8_t *cip;
	uint8_t *end_cip = plugin->pcode + plugin->pcode_size;
	OPCODE op;
	int op_c;

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
		} else if (op_c == -1) {
			switch (op)
			{
			case OP_CASETBL:
				{
					ucell_t num = *(ucell_t *)cip;
					cip += sizeof(cell_t);
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
	data->codesize = plugin->pcode_size;
	writer.inbase = (cell_t *)code;
	writer.outptr = NULL;
	writer.outbase = NULL;
	data->rebase = (jitcode_t *)engine->BaseAlloc(plugin->pcode_size);

	/* Jump back here for second pass */
jit_rewind:
	/* Initialize pass vars */
	writer.inptr = writer.inbase;
	data->jit_chkmargin_heap = 0;
	data->jit_verify_addr_eax = 0;
	data->jit_verify_addr_edx = 0;
	data->jit_bounds = 0;

	/* Start writing the actual code */
	data->jit_return = Write_Execute_Function(jit);

	/* Write error checking routines in case they are needed */
	if (!(data->inline_level & JIT_INLINE_ERRORCHECKS))
	{
		jitpos = jit->jit_curpos();
		Write_Check_VerifyAddr(jit, REG_EAX, true);
		data->jit_verify_addr_eax = jitpos;
	
		jitpos = jit->jit_curpos();
		Write_Check_VerifyAddr(jit, REG_EDX, true);
		data->jit_verify_addr_edx = jitpos;

		jitpos = jit->jit_curpos();
		Write_CheckMargin_Heap(jit);
		data->jit_chkmargin_heap = jitpos;

		jitpos = jit->jit_curpos();
		Write_BoundsCheck(jit);
		data->jit_bounds = jitpos;
	}

	if (writer.outbase == NULL)
	{
		/*******
		 * SECOND PASS - get opcode sizes+info
		 *******/
		jitoffs_t pcode_offs;
		jitoffs_t native_offs;

		for (; writer.inptr <= endptr;)
		{
			/* Store the native offset into the rebase memory.
			 * This large chunk of memory lets us do an instant lookup
			 * based on an original pcode offset.
			 */
			pcode_offs = (jitoffs_t)((uint8_t *)writer.inptr - code);
			native_offs = jit->jit_curpos();
			*((jitoffs_t *)(data->rebase + pcode_offs)) = native_offs;

			/* Now read the opcode and continue. */
			op = (OPCODE)writer.read_cell();
			switch (op)
			{
				#include "opcode_switch.inc"
			}
		}

		/* the total codesize is now known! */
		uint32_t mem = writer.jit_curpos();
		writer.outbase = (jitcode_t)engine->ExecAlloc(mem);
		writer.outptr = writer.outbase;
		/* go back for third pass */
		goto jit_rewind;
	} else {
		/*******
		 * THIRD PASS - write opcode info
		 *******/
		for (; writer.inptr <= endptr;)
		{
			op = (OPCODE)writer.read_cell();
			switch (op)
			{
				#include "opcode_switch.inc"
			}
		}
	}

	/*************
	 * FOURTH PASS - Context Setup
	 *************/

	sp_context_t *ctx = new sp_context_t;
	memset(ctx, 0, sizeof(sp_context_t));

	/* setup  basics */
	ctx->codebase = writer.outbase;
	ctx->plugin = plugin;
	ctx->vmbase = this;
	ctx->flags = (data->debug ? SPFLAG_PLUGIN_DEBUG : 0);

	/* setup memory */
	ctx->data = new uint8_t[plugin->memory];
	memcpy(ctx->data, plugin->data, plugin->data_size);
	ctx->memory = plugin->memory;
	ctx->heapbase = plugin->data_size;
	ctx->hp = ctx->heapbase;
	ctx->sp = ctx->memory - sizeof(cell_t);

	const char *strbase = plugin->info.stringbase;
	uint32_t max, iter;

	/* relocate public info */
	if ((max = plugin->info.publics_num))
	{
		ctx->publics = new sp_public_t[max];
		for (iter=0; iter<max; iter++)
		{
			ctx->publics[iter].name = strbase + plugin->info.publics[iter].name;
			ctx->publics[iter].offs = RelocLookup(jit, plugin->info.publics[iter].address, false);
		}
	}

	/* relocate pubvar info */
	if ((max = plugin->info.pubvars_num))
	{
		uint8_t *dat = ctx->data;
		ctx->pubvars = new sp_pubvar_t[max];
		for (iter=0; iter<max; iter++)
		{
			ctx->pubvars[iter].name = strbase + plugin->info.pubvars[iter].name;
			ctx->pubvars[iter].offs = (cell_t *)(dat + plugin->info.pubvars[iter].address);
		}
	}

	/* relocate native info */
	if ((max = plugin->info.natives_num))
	{
		ctx->natives = new sp_native_t[max];
		for (iter=0; iter<max; iter++)
		{
			ctx->natives[iter].name = strbase + plugin->info.natives[iter].name;
			ctx->natives[iter].pfn = NULL;
			ctx->natives[iter].status = SP_NATIVE_NONE;
		}
	}

	/**
	 * If we're debugging, make sure we copy the necessary info.
	 */
	if (data->debug)
	{
		strbase = plugin->debug.stringbase;
		
		/* relocate files */
		max = plugin->debug.files_num;
		ctx->files = new sp_debug_file_t[max];
		for (iter=0; iter<max; iter++)
		{
			ctx->files[iter].addr = RelocLookup(jit, plugin->debug.files[iter].addr, false);
			ctx->files[iter].name = strbase + plugin->debug.files[iter].name;
		}

		/* relocate lines */
		max = plugin->debug.lines_num;
		ctx->lines = new sp_debug_line_t[max];
		for (iter=0; iter<max; iter++)
		{
			ctx->lines[iter].addr = RelocLookup(jit, plugin->debug.lines[iter].addr, false);
			ctx->lines[iter].line = plugin->debug.lines[iter].line;
		}

		/* relocate arrays */
		sp_fdbg_symbol_t *sym;
		sp_fdbg_arraydim_t *arr;
		uint8_t *cursor = (uint8_t *)(plugin->debug.symbols);
		
		max = plugin->debug.syms_num;
		ctx->symbols = new sp_debug_symbol_t[max];
		for (iter=0; iter<max; iter++)
		{
			sym = (sp_fdbg_symbol_t *)cursor;

			ctx->symbols[iter].codestart = RelocLookup(jit, sym->codestart, false);
			ctx->symbols[iter].codeend = RelocLookup(jit, sym->codeend, false);
			ctx->symbols[iter].name = strbase + sym->name;
			ctx->symbols[iter].sym = sym;

			if (sym->dimcount > 0)
			{
				cursor += sizeof(sp_fdbg_symbol_t);
				arr = (sp_fdbg_arraydim_t *)cursor;
				ctx->symbols[iter].dims = arr;
				cursor += sizeof(sp_fdbg_arraydim_t) * sym->dimcount;
				continue;
			}

			ctx->symbols[iter].dims = NULL;
			cursor += sizeof(sp_fdbg_symbol_t);
		}
	}

	/* clean up relocation+compilation memory */
	engine->BaseFree(data->rebase);
	delete data;

	*err = SP_ERR_NONE;

	return ctx;
}

const char *JITX86::GetVMName()
{
	return "JIT (x86)";
}

int JITX86::ContextExecute(sp_context_t *ctx, uint32_t code_idx, cell_t *result)
{
	typedef int (*CONTEXT_EXECUTE)(sp_context_t *, uint32_t, cell_t *);
	CONTEXT_EXECUTE fn = (CONTEXT_EXECUTE)ctx->codebase;
	return fn(ctx, code_idx, result);
}

void JITX86::FreeContext(sp_context_t *ctx)
{
	engine->ExecFree(ctx->codebase);
	delete [] ctx->data;
	delete [] ctx->files;
	delete [] ctx->lines;
	delete [] ctx->natives;
	delete [] ctx->publics;
	delete [] ctx->pubvars;
	delete [] ctx->symbols;
	delete ctx;
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
