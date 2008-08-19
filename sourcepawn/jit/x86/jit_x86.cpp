/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
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

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "jit_x86.h"
#include "opcode_helpers.h"
#include <x86_macros.h>
#include "../jit_version.h"
#include "../sp_vm_engine.h"
#include "../engine2.h"
#include "../BaseRuntime.h"
#include "../sp_vm_basecontext.h"

using namespace Knight;

#if defined USE_UNGEN_OPCODES
#include "ungen_opcodes.h"
#endif

JITX86 g_Jit;
KeCodeCache *g_pCodeCache = NULL;
ISourcePawnEngine *engine = &g_engine1;

inline sp_plugin_t *GETPLUGIN(sp_context_t *ctx)
{
	return (sp_plugin_t *)(ctx->vm[JITVARS_PLUGIN]);
}

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
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_DAT, (jit_int8_t)val);
	} else {
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_DAT, val);
	}
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
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Rm_Imm32_Disp8(jit, AMX_REG_DAT, 0, (jit_int8_t)val);
	} else {
		IA32_Mov_Rm_Imm32_Disp32(jit, AMX_REG_DAT, 0, val);
	}
}

inline void WriteOp_Zero_S(JitWriter *jit)
{
	//mov [ebx+<val>], 0
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Rm_Imm32_Disp8(jit, AMX_REG_FRM, 0, (jit_int8_t)val);
	} else {
		IA32_Mov_Rm_Imm32_Disp32(jit, AMX_REG_FRM, 0, val);
	}
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
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_FRM, (jit_int8_t)val);
	} else {
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_FRM, val);
	}
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
#if 0 /* :TODO: We no longer use this */
	/* Specialized code to align this taking in account the function magic number */
	jitoffs_t cur_offs = jit->get_outputpos();
	jitoffs_t offset = ((cur_offs & 0xFFFFFFF8) + 8) - cur_offs;
	if (!((offset + cur_offs) & 8))
	{
		offset += 8;
	}
	if (offset)
	{
		for (jit_uint32_t i=0; i<offset; i++)
		{
			jit->write_ubyte(IA32_INT3);
		}
	}

	/* Write the info struct about this function */
	jit->write_uint32(JIT_FUNCMAGIC);
	jit->write_uint32(co->func_idx);

	/* Now we have to backpatch our reloction offset! */
	{
		jitoffs_t offs = jit->get_inputpos() - sizeof(cell_t);
		uint8_t *rebase = ((CompData *)jit->data)->rebase;
		*(jitoffs_t *)(rebase + offs) = jit->get_outputpos();
	}

	/* Lastly, if we're writing, keep track of the function count */
	if (jit->outbase)
	{
		co->func_idx++;
	}
#endif

	//push old frame on stack:
	//mov ecx, [esi+frm]
	//mov [edi-4], ecx
	//sub edi, 8			;extra un-used slot for non-existant CIP
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_INFO, MOD_MEM_REG);
	IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_STK, AMX_REG_TMP, -4);
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 8, MOD_REG);

	//save frame:
	//mov ecx, edi			- get new frame
	//mov ebx, edi			- store frame back
	//sub ecx, ebp			- relocate local frame
	//mov [esi+frm], ecx
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_STK, MOD_REG);
	IA32_Mov_Reg_Rm(jit, AMX_REG_FRM, AMX_REG_STK, MOD_REG);
	IA32_Sub_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_DAT, MOD_REG);
	IA32_Mov_Rm_Reg(jit, AMX_REG_INFO, AMX_REG_TMP, MOD_MEM_REG);
}

inline void WriteOp_Lidx_B(JitWriter *jit)
{
	cell_t val = jit->read_cell();
	//shl eax, <val>
	//add eax, edx
	//mov eax, [ebp+eax]
	IA32_Shl_Rm_Imm8(jit, AMX_REG_PRI, (jit_uint8_t)val, MOD_REG);
	IA32_Add_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	Write_Check_VerifyAddr(jit, AMX_REG_PRI);
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
	if (val == 1)
	{
		IA32_Shl_Rm_1(jit, AMX_REG_PRI, MOD_REG);
	} else {
		IA32_Shl_Rm_Imm8(jit, AMX_REG_PRI, val, MOD_REG);
	}
}

inline void WriteOp_Shl_C_Alt(JitWriter *jit)
{
	//shl edx, <val>
	jit_uint8_t val = (jit_uint8_t)jit->read_cell();
	if (val == 1)
	{
		IA32_Shl_Rm_1(jit, AMX_REG_ALT, MOD_REG);
	} else {
		IA32_Shl_Rm_Imm8(jit, AMX_REG_ALT, val, MOD_REG);
	}
}

inline void WriteOp_Shr_C_Pri(JitWriter *jit)
{
	//shr eax, <val>
	jit_uint8_t val = (jit_uint8_t)jit->read_cell();
	if (val == 1)
	{
		IA32_Shr_Rm_1(jit, AMX_REG_PRI, MOD_REG);
	} else {
		IA32_Shr_Rm_Imm8(jit, AMX_REG_PRI, val, MOD_REG);
	}
}

inline void WriteOp_Shr_C_Alt(JitWriter *jit)
{
	//shr edx, <val>
	jit_uint8_t val = (jit_uint8_t)jit->read_cell();
	if (val == 1)
	{
		IA32_Shr_Rm_1(jit, AMX_REG_ALT, MOD_REG);
	} else {
		IA32_Shr_Rm_Imm8(jit, AMX_REG_ALT, val, MOD_REG);
	}
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
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
		IA32_Add_Rm_Imm8(jit, AMX_REG_PRI, (jit_int8_t)val, MOD_REG);
	else
		IA32_Add_Eax_Imm32(jit, val);
}

inline void WriteOp_SMul_C(JitWriter *jit)
{
	//imul eax, <val>
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
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
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
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
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
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
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
		IA32_Add_Rm_Imm8_Disp8(jit, AMX_REG_DAT, 1, (jit_int8_t)val);
	else
		IA32_Add_Rm_Imm8_Disp32(jit, AMX_REG_DAT, 1, val);
}

inline void WriteOp_Inc_S(JitWriter *jit)
{
	//add [ebx+<val>], 1
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
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
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Sub_Rm_Imm8_Disp8(jit, AMX_REG_DAT, 1, (jit_int8_t)val);
	} else {
		IA32_Sub_Rm_Imm8_Disp32(jit, AMX_REG_DAT, 1, val);
	}
}

inline void WriteOp_Dec_S(JitWriter *jit)
{
	//sub [ebx+<val>], 1
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Sub_Rm_Imm8_Disp8(jit, AMX_REG_FRM, 1, (jit_int8_t)val);
	} else {
		IA32_Sub_Rm_Imm8_Disp32(jit, AMX_REG_FRM, 1, val);
	}
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
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, AMX_REG_DAT, (jit_int8_t)val);
	} else {
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_PRI, AMX_REG_DAT, val);
	}
}

inline void WriteOp_Load_Alt(JitWriter *jit)
{
	//mov edx, [ebp+<val>]
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, AMX_REG_DAT, (jit_int8_t)val);
	} else {
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_ALT, AMX_REG_DAT, val);
	}
}

inline void WriteOp_Load_S_Pri(JitWriter *jit)
{
	//mov eax, [ebx+<val>]
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, AMX_REG_FRM, (jit_int8_t)val);
	} else {
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_PRI, AMX_REG_FRM, val);
	}
}

inline void WriteOp_Load_S_Alt(JitWriter *jit)
{
	//mov edx, [ebx+<val>]
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, AMX_REG_FRM, (jit_int8_t)val);
	} else {
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_ALT, AMX_REG_FRM, val);
	}
}

inline void WriteOp_Lref_Pri(JitWriter *jit)
{
	//mov eax, [ebp+<val>]
	//mov eax, [ebp+eax]
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, AMX_REG_DAT, (jit_int8_t)val);
	} else {
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_PRI, AMX_REG_DAT, val);
	}
	IA32_Mov_Reg_RmEBP_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
}

inline void WriteOp_Lref_Alt(JitWriter *jit)
{
	//mov edx, [ebp+<val>]
	//mov edx, [ebp+edx]
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, AMX_REG_DAT, (jit_int8_t)val);
	} else {
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_ALT, AMX_REG_DAT, val);
	}
	IA32_Mov_Reg_RmEBP_Disp_Reg(jit, AMX_REG_ALT, AMX_REG_DAT, AMX_REG_ALT, NOSCALE);
}

inline void WriteOp_Lref_S_Pri(JitWriter *jit)
{
	//mov eax, [ebx+<val>]
	//mov eax, [ebp+eax]
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, AMX_REG_FRM, (jit_int8_t)val);
	} else {
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_PRI, AMX_REG_FRM, val);
	}
	IA32_Mov_Reg_RmEBP_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
}

inline void WriteOp_Lref_S_Alt(JitWriter *jit)
{
	//mov edx, [ebx+<val>]
	//mov edx, [ebp+edx]
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, AMX_REG_FRM, (jit_int8_t)val);
	} else {
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_ALT, AMX_REG_FRM, val);
	}
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
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Add_Rm_Imm8(jit, AMX_REG_PRI, (jit_int8_t)val, MOD_REG);
	} else {
		IA32_Add_Eax_Imm32(jit, val);
	}
}

inline void WriteOp_Addr_Alt(JitWriter *jit)
{
	//mov edx, [esi+frm]
	//add edx, <val>
	cell_t val = jit->read_cell();
	IA32_Mov_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_INFO, MOD_MEM_REG);
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Add_Rm_Imm8(jit, AMX_REG_ALT, (jit_int8_t)val, MOD_REG);
	} else {
		IA32_Add_Rm_Imm32(jit, AMX_REG_ALT, val, MOD_REG);
	}
}

inline void WriteOp_Stor_Pri(JitWriter *jit)
{
	//mov [ebp+<val>], eax
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_DAT, AMX_REG_PRI, (jit_int8_t)val);
	} else {
		IA32_Mov_Rm_Reg_Disp32(jit, AMX_REG_DAT, AMX_REG_PRI, val);
	}
}

inline void WriteOp_Stor_Alt(JitWriter *jit)
{
	//mov [ebp+<val>], edx
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_DAT, AMX_REG_ALT, (jit_int8_t)val);
	} else {
		IA32_Mov_Rm_Reg_Disp32(jit, AMX_REG_DAT, AMX_REG_ALT, val);
	}
}

inline void WriteOp_Stor_S_Pri(JitWriter *jit)
{
	//mov [ebx+<val>], eax
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_FRM, AMX_REG_PRI, (jit_int8_t)val);
	} else {
		IA32_Mov_Rm_Reg_Disp32(jit, AMX_REG_FRM, AMX_REG_PRI, val);
	}
}

inline void WriteOp_Stor_S_Alt(JitWriter *jit)
{
	//mov [ebx+<val>], edx
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_FRM, AMX_REG_ALT, (jit_int8_t)val);
	} else {
		IA32_Mov_Rm_Reg_Disp32(jit, AMX_REG_FRM, AMX_REG_ALT, val);
	}
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
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_DAT, (jit_int8_t)val);
	} else {
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_DAT, val);
	}
	IA32_Mov_RmEBP_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_TMP, NOSCALE, AMX_REG_PRI);
}

inline void WriteOp_Sref_Alt(JitWriter *jit)
{
	//mov ecx, [ebp+<val>]
	//mov [ebp+ecx], edx
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_DAT, (jit_int8_t)val);
	} else {
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_DAT, val);
	}
	IA32_Mov_RmEBP_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_TMP, NOSCALE, AMX_REG_ALT);
}

inline void WriteOp_Sref_S_Pri(JitWriter *jit)
{
	//mov ecx, [ebx+<val>]
	//mov [ebp+ecx], eax
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_FRM, (jit_int8_t)val);
	} else {
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_FRM, val);
	}
	IA32_Mov_RmEBP_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_TMP, NOSCALE, AMX_REG_PRI);
}

inline void WriteOp_Sref_S_Alt(JitWriter *jit)
{
	//mov ecx, [ebx+<val>]
	//mov [ebp+ecx], edx
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_FRM, (jit_int8_t)val);
	} else {
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_FRM, val);
	}
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
	//sub edi, 4
	//add ecx, <val>
	//mov [edi], ecx
	cell_t val = jit->read_cell();
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_INFO, MOD_MEM_REG);
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Add_Rm_Imm8(jit, AMX_REG_TMP, (jit_int8_t)val, MOD_REG);
	} else {
		IA32_Add_Rm_Imm32(jit, AMX_REG_TMP, val, MOD_REG);
	}
	IA32_Mov_Rm_Reg(jit, AMX_REG_STK, AMX_REG_TMP, MOD_MEM_REG);
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

inline void WriteOp_GenArray(JitWriter *jit, bool autozero)
{
	cell_t val = jit->read_cell();
	if (val == 1)
	{
		/* flat array.  we can generate this without indirection tables. */
		/* Note that we can overwrite ALT because technically STACK should be destroying ALT */
		//mov edx, [esi+info.heap]
		//mov ecx, [edi]
		//mov [edi], edx			;store base of array into stack
		//lea edx, [edx+ecx*4]		;get the final new heap pointer
		//mov [esi+info.heap], edx	;store heap pointer back
		//add edx, ebp				;relocate
		//cmp edx, edi				;compare against stack pointer
		//jae :error				;error out if not enough space
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, AMX_REG_INFO, AMX_INFO_HEAP);
		IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_STK, MOD_MEM_REG);
		IA32_Mov_Rm_Reg(jit, AMX_REG_STK, AMX_REG_ALT, MOD_MEM_REG);
		IA32_Lea_Reg_DispRegMult(jit, AMX_REG_ALT, AMX_REG_ALT, AMX_REG_TMP, SCALE4);
		IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_INFO, AMX_REG_ALT, AMX_INFO_HEAP);
		IA32_Add_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_DAT, MOD_REG);
		IA32_Cmp_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_STK, MOD_REG);
		IA32_Jump_Cond_Imm32_Rel(jit, CC_AE, ((CompData *)jit->data)->jit_error_heaplow);

		WriteOp_Tracker_Push_Reg(jit, REG_ECX);

		if (autozero)
		{
			/* Zero out the array - inline a quick fill */
			//push eax				;save pri
			//push edi				;save stk
			//xor eax, eax			;zero source
			//mov edi, [edi]		;get heap ptr out of the stack
			//add edi, ebp			;relocate
			//cld					;clear direction flag
			//rep stosd				;copy (note: ECX is sane from above)
			//pop edi				;pop stk
			//pop eax				;pop pri
			IA32_Push_Reg(jit, REG_EAX);
			IA32_Push_Reg(jit, REG_EDI);
			IA32_Xor_Reg_Rm(jit, REG_EAX, REG_EAX, MOD_REG);
			IA32_Mov_Reg_Rm(jit, REG_EDI, REG_EDI, MOD_MEM_REG);
			IA32_Add_Reg_Rm(jit, REG_EDI, REG_EBP, MOD_REG);
			IA32_Cld(jit);
			IA32_Rep(jit);
			IA32_Stosd(jit);
			IA32_Pop_Reg(jit, REG_EDI);
			IA32_Pop_Reg(jit, REG_EAX);
		}
	} else {
		//mov ecx, num_dims
		//mov edx, 0/1
		//call [genarray]
		IA32_Mov_Reg_Imm32(jit, AMX_REG_TMP, val);
		if (autozero)
		{
			IA32_Mov_Reg_Imm32(jit, REG_EDX, 1);
		} else {
			IA32_Mov_Reg_Imm32(jit, REG_EDX, 0);
		}
		jitoffs_t call = IA32_Call_Imm32(jit, 0);
		IA32_Write_Jump32_Abs(jit, call, g_Jit.GetGenArrayIntrinsic());
	}
}

inline void WriteOp_Load_Both(JitWriter *jit)
{
	WriteOp_Load_Pri(jit);
	WriteOp_Load_Alt(jit);
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
	if (addr <= SCHAR_MAX && addr >= SCHAR_MIN)
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
	if (offs <= SCHAR_MAX && offs >= SCHAR_MIN)
	{
		IA32_Mov_Rm_Imm32_Disp8(jit, AMX_REG_FRM, val, (jit_int8_t)offs);
	} else {
		IA32_Mov_Rm_Imm32_Disp32(jit, AMX_REG_FRM, val, offs);
	}
}

inline void WriteOp_Load_I(JitWriter *jit)
{
	//mov eax, [ebp+eax]
	Write_Check_VerifyAddr(jit, AMX_REG_PRI);
	IA32_Mov_Reg_RmEBP_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
}

inline void WriteOp_Lodb_I(JitWriter *jit)
{
	Write_Check_VerifyAddr(jit, AMX_REG_PRI);

	//mov eax, [ebp+eax]
	IA32_Mov_Reg_RmEBP_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);

	//and eax, <bitmask>
	cell_t val = jit->read_cell();
	switch(val)
	{
	case 1:
		{
			IA32_And_Eax_Imm32(jit, 0x000000FF);
			break;
		}
	case 2:
		{
			IA32_And_Eax_Imm32(jit, 0x0000FFFF);
			break;
		}
	}
}

inline void WriteOp_Stor_I(JitWriter *jit)
{
	//mov [ebp+edx], eax
	Write_Check_VerifyAddr(jit, AMX_REG_ALT);
	IA32_Mov_RmEBP_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_ALT, NOSCALE, AMX_REG_PRI);
}

inline void WriteOp_Strb_I(JitWriter *jit)
{
	Write_Check_VerifyAddr(jit, AMX_REG_ALT);
	//mov [ebp+edx], eax
	cell_t val = jit->read_cell();
	switch (val)
	{
	case 1:
		{
			IA32_Mov_Rm8EBP_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_ALT, NOSCALE, AMX_REG_PRI);
			break;
		}
	case 2:
		{
			IA32_Mov_Rm16EBP_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_ALT, NOSCALE, AMX_REG_PRI);
			break;
		}
	case 4:
		{
			IA32_Mov_RmEBP_Reg_Disp_Reg(jit, AMX_REG_DAT, AMX_REG_ALT, NOSCALE, AMX_REG_PRI);
			break;
		}
	}
}

inline void WriteOp_Lidx(JitWriter *jit)
{
	//lea eax, [edx+4*eax]
	//mov eax, [ebp+eax]
	IA32_Lea_Reg_DispRegMult(jit, AMX_REG_PRI, AMX_REG_ALT, AMX_REG_PRI, SCALE4);
	Write_Check_VerifyAddr(jit, AMX_REG_PRI);
	IA32_Mov_Reg_RmEBP_Disp_Reg(jit, AMX_REG_PRI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
}

inline void WriteOp_Stack(JitWriter *jit)
{
	//add edi, <val>
	cell_t val = jit->read_cell();
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Add_Rm_Imm8(jit, AMX_REG_STK, (jit_int8_t)val, MOD_REG);
	} else {
		IA32_Add_Rm_Imm32(jit, AMX_REG_STK, val, MOD_REG);
	}

	if (val > 0)
	{
		Write_CheckStack_Min(jit);
	} else if (val < 0) {
		Write_CheckStack_Low(jit);
	}
}

inline void WriteOp_StackAdjust(JitWriter *jit)
{
	cell_t val = jit->read_cell();

	assert(val <= 0);

	//lea edi, [ebx-val]
	
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Lea_DispRegImm8(jit, AMX_REG_STK, AMX_REG_FRM, val);
	}
	else
	{
		IA32_Lea_DispRegImm32(jit, AMX_REG_STK, AMX_REG_FRM, val);
	}

	/* We assume the compiler has generated good code -- 
	 * That is, we do not bother validating this amount!
	 */
}

inline void WriteOp_Heap(JitWriter *jit)
{
	//mov edx, [esi+hea]
	//add [esi+hea], <val>
	cell_t val = jit->read_cell();
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, AMX_REG_INFO, AMX_INFO_HEAP);
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Add_Rm_Imm8_Disp8(jit, AMX_REG_INFO, (jit_int8_t)val, AMX_INFO_HEAP);
	} else {
		IA32_Add_Rm_Imm32_Disp8(jit, AMX_REG_INFO, val, AMX_INFO_HEAP);
	}

	/* NOTE: Backwards from op.stack */
	if (val > 0)
	{
		Write_CheckHeap_Low(jit);
	} else if (val < 0) {
		Write_CheckHeap_Min(jit);
	}
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
	//mov ebx, [edi+4]		- get old frm
	//add edi, 8			- pop stack
	//mov [esi+frm], ebx	- restore frame pointer
	//add ebx, ebp			- relocate
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_FRM, AMX_REG_STK, 4);
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 8, MOD_REG);
	IA32_Mov_Rm_Reg(jit, AMX_REG_INFO, AMX_REG_FRM, MOD_MEM_REG);
	IA32_Add_Reg_Rm(jit, AMX_REG_FRM, AMX_REG_DAT, MOD_REG);

	/* pop params */
	//mov ecx, [edi]
	//lea edi, [edi+ecx*4+4] 
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_STK, MOD_MEM_REG);
	IA32_Lea_Reg_DispRegMultImm8(jit, AMX_REG_STK, AMX_REG_STK, AMX_REG_TMP, SCALE4, 4);

	//ret					- return (EIP on real stack!)
	IA32_Return(jit);
}

void ProfCallGate_Begin(sp_context_t *ctx, const char *name)
{
	((IProfiler *)ctx->vm[JITVARS_PROFILER])->OnFunctionBegin((IPluginContext *)ctx->vm[JITVARS_BASECTX], name);
}

void ProfCallGate_End(sp_context_t *ctx)
{
	((IProfiler *)ctx->vm[JITVARS_PROFILER])->OnFunctionEnd();
}

const char *find_func_name(sp_plugin_t *plugin, uint32_t offs)
{
	uint32_t max, iter;
	sp_fdbg_symbol_t *sym;
	sp_fdbg_arraydim_t *arr;
	uint8_t *cursor = (uint8_t *)(plugin->debug.symbols);

	max = plugin->debug.syms_num;
	for (iter = 0; iter < max; iter++)
	{
		sym = (sp_fdbg_symbol_t *)cursor;

		if (sym->ident == SP_SYM_FUNCTION
			&& sym->codestart <= offs 
			&& sym->codeend > offs)
		{
			return plugin->debug.stringbase + sym->name;
		}

		if (sym->dimcount > 0)
		{
			cursor += sizeof(sp_fdbg_symbol_t);
			arr = (sp_fdbg_arraydim_t *)cursor;
			cursor += sizeof(sp_fdbg_arraydim_t) * sym->dimcount;
			continue;
		}

		cursor += sizeof(sp_fdbg_symbol_t);
	}

	return NULL;
}

inline void WriteOp_Call(JitWriter *jit)
{
	cell_t offs;
	jitoffs_t jmp;
	CompData *data;
	uint32_t func_idx;
	
	data = (CompData *)jit->data; 
	offs = jit->read_cell();

	/* Get the context + rp */
	//mov eax, [esi+ctx]
	//mov ecx, [eax+rp]
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, AMX_REG_INFO, AMX_INFO_CONTEXT);
	IA32_Mov_Reg_Rm_Disp8(jit, REG_ECX, REG_EAX, offsetof(sp_context_t, rp));

	/* Check if the return stack is used up. */
	//cmp ecx, <max stack>
	//jae :stacklow
	IA32_Cmp_Rm_Imm32(jit, MOD_REG, REG_ECX, SP_MAX_RETURN_STACK);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_AE, data->jit_error_stacklow);
	
	/* Add us to the return stack */
	//mov [eax+ecx*4+cips], cip
	IA32_Mov_Rm_Imm32_SIB(jit,
		REG_EAX,
		(uint8_t *)(jit->inptr - 2) - data->plugin->pcode,
		offsetof(sp_context_t, rstk_cips),
		REG_ECX,
		SCALE4);

	/* Increment the return stack pointer */
	//inc [eax+rp]
	if ((int)offsetof(sp_context_t, rp) >= SCHAR_MIN && (int)offsetof(sp_context_t, rp) <= SCHAR_MAX)
	{
		IA32_Inc_Rm_Disp8(jit, REG_EAX, offsetof(sp_context_t, rp));
	}
	else
	{
		IA32_Inc_Rm_Disp32(jit, REG_EAX, offsetof(sp_context_t, rp));
	}

	/* Store the CIP of the function we're about to call. */
	IA32_Mov_Rm_Imm32_Disp8(jit, AMX_REG_INFO, offs, AMX_INFO_CIP);

	/* Call the function */
	func_idx = FuncLookup(data, offs);

	/* We need to emit a delayed thunk instead. */
	if (func_idx == 0)
	{
		if (jit->outbase == NULL)
		{
			data->num_thunks++;

			/* We still emit the call because we need consistent size counting */
			IA32_Call_Imm32(jit, 0);
		}
		else
		{
			call_thunk_t *thunk;

			/* Find the thunk we're associated with */
			thunk = &data->thunks[data->num_thunks];
			data->num_thunks++;

			/* Emit the call, save its target position.
			 * Save thunk info, then patch the target to the thunk.
			 */
			thunk->patch_addr = IA32_Call_Imm32(jit, 0);
			thunk->pcode_offs = offs;
			IA32_Write_Jump32(jit, thunk->patch_addr, thunk->thunk_addr);
		}
	}
	/* The function is already jitted.  We can emit a direct call. */
	else
	{
		JitFunction *fn;

		fn = data->runtime->GetJittedFunction(func_idx);
		jmp = IA32_Call_Imm32(jit, 0);
		IA32_Write_Jump32_Abs(jit, jmp, fn->GetEntryAddress());
	}

	/* Restore the last cip */
	//mov [esi+cip], <cip>
	IA32_Mov_Rm_Imm32_Disp8(jit, 
		AMX_REG_INFO, 
		(uint8_t *)(jit->inptr - 2) - data->plugin->pcode, 
		AMX_INFO_CIP);

	/* Mark us as leaving the last frame. */ 
	//mov ecx, [esi+ctx] 
	//dec [ecx+rp] 
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_CONTEXT); 
	IA32_Dec_Rm_Disp8(jit, REG_ECX, offsetof(sp_context_t, rp));
}

inline void WriteOp_Bounds(JitWriter *jit)
{
	cell_t val = jit->read_cell();
	
	//cmp eax, <val>
	//ja :error
	if (val <= SCHAR_MAX && val >= SCHAR_MIN)
	{
		IA32_Cmp_Rm_Imm8(jit, MOD_REG, AMX_REG_PRI, (jit_int8_t)val);
	} else {
		IA32_Cmp_Eax_Imm32(jit, val);
	}
	IA32_Jump_Cond_Imm32_Rel(jit, CC_A, ((CompData *)jit->data)->jit_error_bounds);
}

inline void WriteOp_Halt(JitWriter *jit)
{
	AlignMe(jit);

	//mov ecx, [esi+ret]
	//mov [ecx], eax
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_RETVAL);
	IA32_Mov_Rm_Reg(jit, AMX_REG_TMP, AMX_REG_PRI, MOD_MEM_REG);
	
	/* :TODO: 
	 * We don't support sleeping or halting with weird values.
	 * So we're omitting the mov eax <val> that was here.
	 */
	jit->read_cell();

	IA32_Jump_Imm32_Abs(jit, g_Jit.GetReturnPoint());
}

inline void WriteOp_Break(JitWriter *jit)
{
	CompData *data;

	data = (CompData *)jit->data;

	//mov [esi+cip], <cip>
	IA32_Mov_Rm_Imm32_Disp8(jit, 
		AMX_REG_INFO, 
		(uint8_t *)(jit->inptr - 1) - data->plugin->pcode, 
		AMX_INFO_CIP);
}

inline void WriteOp_Jump(JitWriter *jit)
{
	//jmp <offs>
	cell_t amx_offs = jit->read_cell();
	IA32_Jump_Imm32_Rel(jit, RelocLookup(jit, amx_offs, false));
}

inline void WriteOp_Jzer(JitWriter *jit)
{
	//test eax, eax
	//jz <target>
	cell_t target = jit->read_cell();
	IA32_Test_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_PRI, MOD_REG);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_Z, RelocLookup(jit, target, false));
}

inline void WriteOp_Jnz(JitWriter *jit)
{
	//test eax, eax
	//jnz <target>
	cell_t target = jit->read_cell();
	IA32_Test_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_PRI, MOD_REG);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_NZ, RelocLookup(jit, target, false));
}

inline void WriteOp_Jeq(JitWriter *jit)
{
	//cmp eax, edx
	//je <target>
	cell_t target = jit->read_cell();
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_E, RelocLookup(jit, target, false));
}

inline void WriteOp_Jneq(JitWriter *jit)
{
	//cmp eax, edx
	//jne <target>
	cell_t target = jit->read_cell();
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_NE, RelocLookup(jit, target, false));
}

inline void WriteOp_Jsless(JitWriter *jit)
{
	//cmp eax, edx
	//jl <target>
	cell_t target = jit->read_cell();
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_L, RelocLookup(jit, target, false));
}

inline void WriteOp_Jsleq(JitWriter *jit)
{
	//cmp eax, edx
	//jle <target>
	cell_t target = jit->read_cell();
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_LE, RelocLookup(jit, target, false));
}

inline void WriteOp_JsGrtr(JitWriter *jit)
{
	//cmp eax, edx
	//jg <target>
	cell_t target = jit->read_cell();
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_G, RelocLookup(jit, target, false));
}

inline void WriteOp_JsGeq(JitWriter *jit)
{
	//cmp eax, edx
	//jge <target>
	cell_t target = jit->read_cell();
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_GE, RelocLookup(jit, target, false));
}

inline void WriteOp_Switch(JitWriter *jit)
{
	CompData *data = (CompData *)jit->data;
	cell_t offs = jit->read_cell();
	cell_t *tbl = (cell_t *)((char *)data->plugin->pcode + offs + sizeof(cell_t));

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
		IA32_Jump_Imm32_Rel(jit, RelocLookup(jit, *tbl, false));
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
			if (low_bound >= SCHAR_MIN && low_bound <= SCHAR_MAX)
			{
				IA32_Lea_DispRegImm8(jit, AMX_REG_TMP, AMX_REG_PRI, low_bound);
			} else {
				IA32_Lea_DispRegImm32(jit, AMX_REG_TMP, AMX_REG_PRI, low_bound);
			}
		} else {
			//mov ecx, eax
			IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_PRI, MOD_REG);
		}
		cell_t high_bound = abs(cases[0].val - cases[num_cases-1].val);
		//cmp ecx, <UPPER BOUND BOUND>
		if (high_bound >= SCHAR_MIN && high_bound <= SCHAR_MAX)
		{
			IA32_Cmp_Rm_Imm8(jit, MOD_REG, AMX_REG_TMP, high_bound);
		} else {
			IA32_Cmp_Rm_Imm32(jit, MOD_REG, AMX_REG_TMP, high_bound);
		}
		//ja <default case>
		IA32_Jump_Cond_Imm32_Rel(jit, CC_A, RelocLookup(jit, *tbl, false));

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
			//lea ecx, [ecx*4]
			//add ecx, <case table start>
			IA32_Lea_Reg_RegMultImm32(jit, AMX_REG_TMP, AMX_REG_TMP, SCALE4, 0);
			jitoffs_t tbl_offs = IA32_Add_Rm_Imm32_Later(jit, AMX_REG_TMP, MOD_REG);
			IA32_Jump_Rm(jit, AMX_REG_TMP, MOD_MEM_REG);
			/* The case table starts here.  Go back and write the output pointer. */
			jitoffs_t cur_pos = jit->get_outputpos();
			jit->set_outputpos(tbl_offs);
			jit->write_uint32((jit_uint32_t)(jit->outbase + cur_pos));
			jit->set_outputpos(cur_pos);
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
			 * :THOUGHT: Another method of optimizing this - fill in the gaps with jumps to the default case,
			 *  but only if we're under N cases or so!
			 * :THOUGHT: Write out a static btree lookup :)
			 */
			cell_t val;
			for (cell_t i=0; i<num_cases; i++)
			{
				val = cases[i].val;
				//cmp eax, <val> OR cmp al, <val>
				if (val >= SCHAR_MIN && val <= SCHAR_MAX)
				{
					IA32_Cmp_Al_Imm8(jit, val);
				} else {
					IA32_Cmp_Eax_Imm32(jit, cases[i].val);
				}
				IA32_Jump_Cond_Imm32_Rel(jit, CC_E, RelocLookup(jit, cases[i].offs, false));
			}
			/* After all this, jump to the default case! */
			IA32_Jump_Imm32_Rel(jit, RelocLookup(jit, *tbl, false));
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

inline void WriteOp_Sysreq_C(JitWriter *jit)
{
	/* store the number of parameters on the stack, 
	 * and store the native index as well.
	 */
	cell_t native_index = jit->read_cell();

	if ((uint32_t)native_index >= ((CompData*)jit->data)->plugin->num_natives)
	{
		((CompData *)jit->data)->error_set = SP_ERROR_INSTRUCTION_PARAM;
		return;
	}

	//mov ecx, <native_index>
	IA32_Mov_Reg_Imm32(jit, REG_ECX, native_index);

	jitoffs_t call = IA32_Call_Imm32(jit, 0);
	IA32_Write_Jump32(jit, call, ((CompData *)jit->data)->jit_sysreq_c);
}

inline void WriteOp_Sysreq_N(JitWriter *jit)
{
	/* The big daddy of opcodes. */
	cell_t native_index = jit->read_cell();
	cell_t num_params = jit->read_cell();
	CompData *data = (CompData *)jit->data;

	if ((uint32_t)native_index >= data->plugin->num_natives)
	{
		data->error_set = SP_ERROR_INSTRUCTION_PARAM;
		return;
	}

	/* store the number of parameters on the stack */
	//mov [edi-4], num_params
	//sub edi, 4
	IA32_Mov_Rm_Imm32_Disp8(jit, AMX_REG_STK, num_params, -4);
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);

	/* save registers we will need */
	//push edx
	IA32_Push_Reg(jit, AMX_REG_ALT);

	/* Align the stack to 16 bytes */
	//push ebx
	//mov ebx, esp
	//and esp, 0xFFFFFF0
	//sub esp, 4
	IA32_Push_Reg(jit, REG_EBX);
	IA32_Mov_Reg_Rm(jit, REG_EBX, REG_ESP, MOD_REG);
	IA32_And_Rm_Imm8(jit, REG_ESP, MOD_REG, -16);
	IA32_Sub_Rm_Imm8(jit, REG_ESP, 4, MOD_REG);

	/* push some callback stuff */
	//push edi		; stack
	//push <native>	; native index
	IA32_Push_Reg(jit, AMX_REG_STK);
	if (native_index <= SCHAR_MAX && native_index >= SCHAR_MIN)
	{
		IA32_Push_Imm8(jit, (jit_int8_t)native_index);
	} else {
		IA32_Push_Imm32(jit, native_index);
	}

	/* Relocate stack, heap, frm information, then store back */
	//mov eax, [esi+context]
	//mov ecx, [esi+hea]
	//sub edi, ebp
	//mov [eax+hp], ecx
	//mov ecx, [esi]
	//mov [eax+sp], edi
	//mov [eax+frm], ecx
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, AMX_REG_INFO, AMX_INFO_CONTEXT);
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_HEAP);
	IA32_Sub_Reg_Rm(jit, AMX_REG_STK, AMX_REG_DAT, MOD_REG);
	IA32_Mov_Rm_Reg_Disp8(jit, REG_EAX, AMX_REG_TMP, offsetof(sp_context_t, hp));
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_INFO_FRM, MOD_MEM_REG);
	IA32_Mov_Rm_Reg_Disp8(jit, REG_EAX, AMX_REG_STK, offsetof(sp_context_t, sp));
	IA32_Mov_Rm_Reg_Disp8(jit, REG_EAX, AMX_REG_TMP, offsetof(sp_context_t, frm));

	/* finally, push the last parameter and make the call */
	//push eax		; context
	//call NativeCallback
	IA32_Push_Reg(jit, REG_EAX);
	jitoffs_t call = IA32_Call_Imm32(jit, 0);
	if ((data->profile & SP_PROF_NATIVES) == SP_PROF_NATIVES)
	{
		IA32_Write_Jump32_Abs(jit, call, (void *)NativeCallback_Profile);
	}
	else
	{
		IA32_Write_Jump32_Abs(jit, call, (void *)NativeCallback);
	}
	
	/* check for errors */
	//mov ecx, [esi+context]
	//cmp [ecx+err], 0
	//jnz :error
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_CONTEXT);
	IA32_Cmp_Rm_Disp8_Imm8(jit, AMX_REG_TMP, offsetof(sp_context_t, n_err), 0);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_NZ, data->jit_extern_error);

	/* restore what we damaged */
	//mov esp, ebx
	//pop ebx
	//add edi, ebp
	//pop edx
	IA32_Mov_Reg_Rm(jit, REG_ESP, REG_EBX, MOD_REG);
	IA32_Pop_Reg(jit, REG_EBX);
	IA32_Add_Reg_Rm(jit, AMX_REG_STK, AMX_REG_DAT, MOD_REG);
	IA32_Pop_Reg(jit, AMX_REG_ALT);

	/* pop the stack.  do not check the margins.
	 * Note that this is not a true macro - we don't bother to
	 * set ALT here because nothing will be using it.
	 */
	num_params++;
	num_params *= 4;
	//add edi, <val*4+4>
	if (num_params <= SCHAR_MAX && num_params >= SCHAR_MIN)
	{
		IA32_Add_Rm_Imm8(jit, AMX_REG_STK, (jit_int8_t)num_params, MOD_REG);
	} else {
		IA32_Add_Rm_Imm32(jit, AMX_REG_STK, num_params, MOD_REG);
	}
}

inline void WriteOp_Tracker_Push_C(JitWriter *jit)
{
	cell_t val = jit->read_cell();

	/* Save registers that may be damaged by the call */
	//push eax
	//push edx
	IA32_Push_Reg(jit, AMX_REG_PRI);
	IA32_Push_Reg(jit, AMX_REG_ALT);

	/* Get the context ptr, push it and call the check */
	//mov eax, [esi+context]
	//push eax
	//call JIT_VerifyOrAllocateTracker
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, AMX_REG_INFO, AMX_INFO_CONTEXT);
	IA32_Push_Reg(jit, REG_EAX);
	jitoffs_t call = IA32_Call_Imm32(jit, 0);
	IA32_Write_Jump32_Abs(jit, call, (void *)JIT_VerifyOrAllocateTracker);

	/* Check for errors */
	//cmp eax, 0
	//jnz :error
	IA32_Cmp_Rm_Imm8(jit, MOD_REG, REG_EAX, 0);
	IA32_Jump_Cond_Imm32_Abs(jit, CC_NZ, g_Jit.GetReturnPoint());

	/* Restore */
	//pop eax
	IA32_Pop_Reg(jit, REG_EAX);

	/* Push the value into the stack and increment pCur */
	//mov edx, [eax+vm[]]
	//mov ecx, [edx+pcur]
	//add [edx+pcur], 4
	//mov [ecx], <val>*4	; we want the count in bytes not in cells
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EDX, REG_EAX, offsetof(sp_context_t, vm[JITVARS_TRACKER]));
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, REG_EDX, offsetof(tracker_t, pCur));
	IA32_Add_Rm_Imm8_Disp8(jit, REG_EDX, 4, offsetof(tracker_t, pCur));
	IA32_Mov_Rm_Imm32(jit, AMX_REG_TMP, val*4, MOD_MEM_REG);

	/* Restore PRI & ALT */
	//pop edx
	//pop eax
	IA32_Pop_Reg(jit, AMX_REG_ALT);
	IA32_Pop_Reg(jit, AMX_REG_PRI);
}

inline void WriteOp_Tracker_Pop_SetHeap(JitWriter *jit)
{
	/* Save registers that may be damaged by the call */
	//push eax
	//push edx
	IA32_Push_Reg(jit, AMX_REG_PRI);
	IA32_Push_Reg(jit, AMX_REG_ALT);

	/* Get the context ptr, push it and call the check */
	//mov eax, [esi+context]
	//push eax
	//call JIT_VerifyLowBoundTracker
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, AMX_REG_INFO, AMX_INFO_CONTEXT);
	IA32_Push_Reg(jit, REG_EAX);
	jitoffs_t call = IA32_Call_Imm32(jit, 0);
	IA32_Write_Jump32_Abs(jit, call, (void *)JIT_VerifyLowBoundTracker);

	/* Check for errors */
	//cmp eax, 0
	//jnz :error
	IA32_Cmp_Rm_Imm8(jit, MOD_REG, REG_EAX, 0);
	IA32_Jump_Cond_Imm32_Abs(jit, CC_NZ, g_Jit.GetReturnPoint());

	/* Restore */
	//pop eax
	IA32_Pop_Reg(jit, REG_EAX);

	/* Pop the value from the stack and decrease the heap by it*/
	//mov edx, [eax+vm[]]
	//sub [edx+pcur], 4
	//mov ecx, [edx+pcur]
	//mov ecx, [ecx]
	//sub [esi+hea], ecx
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EDX, REG_EAX, offsetof(sp_context_t, vm[JITVARS_TRACKER]));
	IA32_Sub_Rm_Imm8_Disp8(jit, REG_EDX, 4, offsetof(tracker_t, pCur));
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, REG_EDX, offsetof(tracker_t, pCur));
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_TMP, MOD_MEM_REG);
	IA32_Sub_Rm_Reg_Disp8(jit, AMX_REG_INFO, AMX_REG_TMP, AMX_INFO_HEAP);

	Write_CheckHeap_Min(jit);

	/* Restore PRI & ALT */
	//pop edx
	//pop eax
	IA32_Pop_Reg(jit, AMX_REG_ALT);
	IA32_Pop_Reg(jit, AMX_REG_PRI);
}

inline void WriteOp_Stradjust_Pri(JitWriter *jit)
{
	//add eax, 4
	//sar eax, 2
	IA32_Add_Rm_Imm8(jit, AMX_REG_PRI, 4, MOD_REG);
	IA32_Sar_Rm_Imm8(jit, AMX_REG_PRI, 2, MOD_REG);
}

inline void WriteOp_FloatAbs(JitWriter *jit)
{
	//mov eax, [edi]
	//and eax, 0x7FFFFFFF
	IA32_Mov_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_STK, MOD_MEM_REG);
	IA32_And_Eax_Imm32(jit, 0x7FFFFFFF);

	/* Rectify the stack */
	//add edi, 4
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
}

inline void WriteOp_Float(JitWriter *jit)
{
	//fild [edi]
	//push eax
	//fstp [esp]
	//pop eax
	IA32_Fild_Mem32(jit, AMX_REG_STK);
	IA32_Push_Reg(jit, AMX_REG_PRI);
	IA32_Fstp_Mem32_ESP(jit);
	IA32_Pop_Reg(jit, AMX_REG_PRI);

	/* Rectify the stack */
	//add edi, 4
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
}

inline void WriteOp_FloatAdd(JitWriter *jit)
{
	//push eax
	//fld [edi]
	//fadd [edi+4]
	//fstp [esp]
	//pop eax
	IA32_Push_Reg(jit, AMX_REG_PRI);
	IA32_Fld_Mem32(jit, AMX_REG_STK);
	IA32_Fadd_Mem32_Disp8(jit, AMX_REG_STK, 4);
	IA32_Fstp_Mem32_ESP(jit);
	IA32_Pop_Reg(jit, AMX_REG_PRI);

	/* Rectify the stack */
	//add edi, 8
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 8, MOD_REG);
}

inline void WriteOp_FloatSub(JitWriter *jit)
{
	//push eax
	//fld [edi]
	//fsub [edi+4]
	//fstp [esp]
	//pop eax
	IA32_Push_Reg(jit, AMX_REG_PRI);
	IA32_Fld_Mem32(jit, AMX_REG_STK);
	IA32_Fsub_Mem32_Disp8(jit, AMX_REG_STK, 4);
	IA32_Fstp_Mem32_ESP(jit);
	IA32_Pop_Reg(jit, AMX_REG_PRI);

	/* Rectify the stack */
	//add edi, 8
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 8, MOD_REG);
}

inline void WriteOp_FloatMul(JitWriter *jit)
{
	//push eax
	//fld [edi]
	//fmul [edi+4]
	//fstp [esp]
	//pop eax
	IA32_Push_Reg(jit, AMX_REG_PRI);
	IA32_Fld_Mem32(jit, AMX_REG_STK);
	IA32_Fmul_Mem32_Disp8(jit, AMX_REG_STK, 4);
	IA32_Fstp_Mem32_ESP(jit);
	IA32_Pop_Reg(jit, AMX_REG_PRI);

	/* Rectify the stack */
	//add edi, 8
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 8, MOD_REG);
}

inline void WriteOp_FloatDiv(JitWriter *jit)
{
	//push eax
	//fld [edi]
	//fdiv [edi+4]
	//fstp [esp]
	//pop eax
	IA32_Push_Reg(jit, AMX_REG_PRI);
	IA32_Fld_Mem32(jit, AMX_REG_STK);
	IA32_Fdiv_Mem32_Disp8(jit, AMX_REG_STK, 4);
	IA32_Fstp_Mem32_ESP(jit);
	IA32_Pop_Reg(jit, AMX_REG_PRI);

	/* Rectify the stack */
	//add edi, 8
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 8, MOD_REG);
}

inline void WriteOp_RountToNearest(JitWriter *jit)
{
	//push eax
	//fld [edi]

	//fstcw [esp]
	//mov eax, [esp]
	//push eax

	//mov [esp+4], 0x3FF
	//fadd st(0), st(0)
	//fldcw [esp+4]

	//push eax	
	//push 0x3F000000 ; 0.5f
	//fadd [esp]
	//fistp [esp+4]
	//pop eax
	//pop eax

	//pop ecx
	//sar eax, 1
	//mov [esp], ecx
	//fldcw [esp]
	//add esp, 4

	IA32_Push_Reg(jit, REG_EAX);
	IA32_Fld_Mem32(jit, AMX_REG_STK);

	IA32_Fstcw_Mem16_ESP(jit);
	IA32_Mov_Reg_RmESP(jit, REG_EAX);
	IA32_Push_Reg(jit, REG_EAX);

	IA32_Mov_ESP_Disp8_Imm32(jit, 4, 0x3FF);
	IA32_Fadd_FPUreg_ST0(jit, 0);
	IA32_Fldcw_Mem16_Disp8_ESP(jit, 4);

	IA32_Push_Reg(jit, REG_EAX);
	IA32_Push_Imm32(jit, 0x3F000000);
	IA32_Fadd_Mem32_ESP(jit);
	IA32_Fistp_Mem32_Disp8_Esp(jit, 4);
	IA32_Pop_Reg(jit, REG_EAX);
	IA32_Pop_Reg(jit, AMX_REG_PRI);

	IA32_Pop_Reg(jit, REG_ECX);
	IA32_Sar_Rm_1(jit, REG_EAX, MOD_REG);
	IA32_Mov_RmESP_Reg(jit, REG_ECX);
	IA32_Fldcw_Mem16_ESP(jit);
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4, MOD_REG);

	/* Rectify the stack */
	//add edi, 4
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
}

inline void WriteOp_RoundToFloor(JitWriter *jit)
{
	//push eax
	//fld [edi]

	//fstcw [esp]
	//mov eax, [esp]
	//push eax

	//mov [esp+4], 0x7FF
	//fldcw [esp+4]

	//push eax	
	//fistp [esp]
	//pop eax

	//pop ecx
	//mov [esp], ecx
	//fldcw [esp]
	//add esp, 4

	IA32_Push_Reg(jit, REG_EAX);
	IA32_Fld_Mem32(jit, AMX_REG_STK);

	IA32_Fstcw_Mem16_ESP(jit);
	IA32_Mov_Reg_RmESP(jit, REG_EAX);
	IA32_Push_Reg(jit, REG_EAX);

	IA32_Mov_ESP_Disp8_Imm32(jit, 4, 0x7FF);
	IA32_Fldcw_Mem16_Disp8_ESP(jit, 4);

	IA32_Push_Reg(jit, REG_EAX);
	IA32_Fistp_Mem32_ESP(jit);
	IA32_Pop_Reg(jit, REG_EAX);

	IA32_Pop_Reg(jit, REG_ECX);
	IA32_Mov_RmESP_Reg(jit, REG_ECX);
	IA32_Fldcw_Mem16_ESP(jit);
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4, MOD_REG);

	/* Rectify the stack */
	//add edi, 4
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
}

inline void WriteOp_RoundToCeil(JitWriter *jit)
{
	//push eax
	//fld [edi]

	//fstcw [esp]
	//mov eax, [esp]
	//push eax

	//mov [esp+4], 0xBFF
	//fldcw [esp+4]

	//push eax	
	//fistp [esp]
	//pop eax

	//pop ecx
	//mov [esp], ecx
	//fldcw [esp]
	//add esp, 4

	IA32_Push_Reg(jit, REG_EAX);
	IA32_Fld_Mem32(jit, AMX_REG_STK);

	IA32_Fstcw_Mem16_ESP(jit);
	IA32_Mov_Reg_RmESP(jit, REG_EAX);
	IA32_Push_Reg(jit, REG_EAX);

	IA32_Mov_ESP_Disp8_Imm32(jit, 4, 0xBFF);
	IA32_Fldcw_Mem16_Disp8_ESP(jit, 4);

	IA32_Push_Reg(jit, REG_EAX);
	IA32_Fistp_Mem32_ESP(jit);
	IA32_Pop_Reg(jit, REG_EAX);

	IA32_Pop_Reg(jit, REG_ECX);
	IA32_Mov_RmESP_Reg(jit, REG_ECX);
	IA32_Fldcw_Mem16_ESP(jit);
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4, MOD_REG);

	/* Rectify the stack */
	//add edi, 4
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
}

inline void WriteOp_RoundToZero(JitWriter *jit)
{
	//push eax
	//fld [edi]

	//fstcw [esp]
	//mov eax, [esp]
	//push eax

	//mov [esp+4], 0xFFF
	//fldcw [esp+4]

	//push eax	
	//fistp [esp]
	//pop eax

	//pop ecx
	//mov [esp], ecx
	//fldcw [esp]
	//add esp, 4

	IA32_Push_Reg(jit, REG_EAX);
	IA32_Fld_Mem32(jit, AMX_REG_STK);

	IA32_Fstcw_Mem16_ESP(jit);
	IA32_Mov_Reg_RmESP(jit, REG_EAX);
	IA32_Push_Reg(jit, REG_EAX);

	IA32_Mov_ESP_Disp8_Imm32(jit, 4, 0xFFF);
	IA32_Fldcw_Mem16_Disp8_ESP(jit, 4);

	IA32_Push_Reg(jit, REG_EAX);
	IA32_Fistp_Mem32_ESP(jit);
	IA32_Pop_Reg(jit, REG_EAX);

	IA32_Pop_Reg(jit, REG_ECX);
	IA32_Mov_RmESP_Reg(jit, REG_ECX);
	IA32_Fldcw_Mem16_ESP(jit);
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4, MOD_REG);

	/* Rectify the stack */
	//add edi, 4
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
}

inline void WriteOp_FloatCompare(JitWriter *jit)
{
	//fld [edi]
	//fld [edi+4]
	//fucomip st(0), st(1)
	//mov ecx, <round_table>
	//cmovz eax, [ecx+4]
	//cmovb eax, [ecx+8]
	//cmova eax, [ecx]
	//fstp st(0)

	IA32_Fld_Mem32(jit, AMX_REG_STK);
	IA32_Fld_Mem32_Disp8(jit, AMX_REG_STK, 4);
	IA32_Fucomip_ST0_FPUreg(jit, 1);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_TMP, (jit_int32_t)g_Jit.GetRoundingTable());
	IA32_CmovCC_Rm_Disp8(jit, AMX_REG_TMP, CC_Z, 4);
	IA32_CmovCC_Rm_Disp8(jit, AMX_REG_TMP, CC_B, 8);
	IA32_CmovCC_Rm(jit, AMX_REG_TMP, CC_A);
	IA32_Fstp_FPUreg(jit, 0);

	/* Rectify the stack */
	//add edi, 8
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 8, MOD_REG);
}

inline void WriteOp_EndProc(JitWriter *jit)
{
}

void Write_CallThunk(JitWriter *jit, jitoffs_t jmploc, cell_t pcode_offs)
{
	CompData *data;
	jitoffs_t call;

	data = (CompData *)jit->data;

	//push <jmploc_addr>
	//push <pcode_offs>
	//push <runtime>
	//call CompileFromThunk
	//add esp, 4*3
	//test eax, eax
	//jz :error
	//call eax
	//ret
	IA32_Push_Imm32(jit, (jit_int32_t)(jit->outbase + jmploc));
	IA32_Push_Imm32(jit, pcode_offs);
	IA32_Push_Imm32(jit, (jit_int32_t)(data->runtime));
	call = IA32_Call_Imm32(jit, 0);
	IA32_Write_Jump32_Abs(jit, call, (void *)CompileThunk);
	IA32_Add_Rm_Imm32(jit, REG_ESP, 4*3, MOD_REG);
	IA32_Test_Rm_Reg(jit, REG_EAX, REG_EAX, MOD_REG);
	call = IA32_Jump_Cond_Imm8(jit, CC_Z, 0);
	IA32_Call_Reg(jit, REG_EAX);
	IA32_Return(jit);

	/* We decrement the frame and store the target cip. */
	//:error
	//mov [esi+cip], pcode_offs
	//goto error
	IA32_Send_Jump8_Here(jit, call);
	Write_SetError(jit, SP_ERROR_INVALID_INSTRUCTION);
}

/*************************************************
 *************************************************
 * JIT PROPER ************************************
 * The rest from now on is the JIT engine        *
 *************************************************
 *************************************************/

void *CompileThunk(BaseRuntime *runtime, cell_t pcode_offs, void *jmploc_addr)
{
	int err;
	JitFunction *fn;
	uint32_t func_idx;
	void *target_addr;
	
	if ((func_idx = FuncLookup((CompData *)runtime->m_pCo, pcode_offs)) == 0)
	{
		fn = g_Jit.CompileFunction(runtime, pcode_offs, &err);
	}
	else
	{
		fn = runtime->GetJittedFunction(func_idx);

#if defined _DEBUG
		g_engine1.GetDebugHook()->OnDebugSpew("Patching thunk to %s::%s", runtime->m_pPlugin->name, find_func_name(runtime->m_pPlugin, pcode_offs));
#endif

	}

	if (fn == NULL)
	{
		return NULL;
	}

	target_addr = fn->GetEntryAddress();

	/* Right now, we always keep the code RWE */
	*(intptr_t *)((char *)jmploc_addr) = 
		intptr_t(target_addr) - (intptr_t(jmploc_addr) + 4);

	return target_addr;
}

cell_t NativeCallback(sp_context_t *ctx, ucell_t native_idx, cell_t *params)
{
	sp_native_t *native;
	cell_t save_sp = ctx->sp;
	cell_t save_hp = ctx->hp;
	sp_plugin_t *pl = GETPLUGIN(ctx);

	ctx->n_idx = native_idx;

	if (ctx->hp < (cell_t)pl->data_size)
	{
		ctx->n_err = SP_ERROR_HEAPMIN;
		return 0;
	}

	if (ctx->hp + STACK_MARGIN > ctx->sp)
	{
		ctx->n_err = SP_ERROR_STACKLOW;
		return 0;
	}

	if ((uint32_t)ctx->sp >= pl->mem_size)
	{
		ctx->n_err = SP_ERROR_STACKMIN;
		return 0;
	}

	native = &pl->natives[native_idx];

	if (native->status == SP_NATIVE_UNBOUND)
	{
		ctx->n_err = SP_ERROR_INVALID_NATIVE;
		return 0;
	}

	cell_t result = native->pfn(GET_CONTEXT(ctx), params);

	if (ctx->n_err != SP_ERROR_NONE)
	{
		return result;
	}

	if (save_sp != ctx->sp)
	{
		ctx->n_err = SP_ERROR_STACKLEAK;
		return result;
	}
	else if (save_hp != ctx->hp)
	{
		ctx->n_err = SP_ERROR_HEAPLEAK;
		return result;
	}

	return result;
}

cell_t NativeCallback_Profile(sp_context_t *ctx, ucell_t native_idx, cell_t *params)
{
	/* :TODO: */
	return NativeCallback(ctx, native_idx, params);
}

uint32_t FuncLookup(CompData *data, cell_t pcode_offs)
{
	/* Offset must always be 1)positive and 2)less than or equal to the codesize */
	assert(pcode_offs >= 0 && (uint32_t)pcode_offs <= data->plugin->pcode_size);

	/* Do the lookup in the native dictionary. */
	return *(jitoffs_t *)(data->rebase + pcode_offs);
}

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
			pcode_offs += jit->get_inputpos();
		}
		/* Offset must always be 1)positive and 2)less than or equal to the codesize */
		assert(pcode_offs >= 0 && (uint32_t)pcode_offs <= data->plugin->pcode_size);
		/* Do the lookup in the native dictionary. */
#if defined _DEBUG
		if (jit->outbase != NULL)
		{
			assert(*(jitoffs_t *)(data->rebase + pcode_offs) != NULL);
		}
#endif
		return *(jitoffs_t *)(data->rebase + pcode_offs);
	}
	else
	{
		return 0;
	}
}

void WriteErrorRoutines(CompData *data, JitWriter *jit)
{
	AlignMe(jit);

	data->jit_error_divzero = jit->get_outputpos();
	Write_SetError(jit, SP_ERROR_DIVIDE_BY_ZERO);

	data->jit_error_stacklow = jit->get_outputpos();
	Write_SetError(jit, SP_ERROR_STACKLOW);

	data->jit_error_stackmin = jit->get_outputpos();
	Write_SetError(jit, SP_ERROR_STACKMIN);

	data->jit_error_bounds = jit->get_outputpos();
	Write_SetError(jit, SP_ERROR_ARRAY_BOUNDS);

	data->jit_error_memaccess = jit->get_outputpos();
	Write_SetError(jit, SP_ERROR_MEMACCESS);

	data->jit_error_heaplow = jit->get_outputpos();
	Write_SetError(jit, SP_ERROR_HEAPLOW);

	data->jit_error_heapmin = jit->get_outputpos();
	Write_SetError(jit, SP_ERROR_HEAPMIN);

	data->jit_extern_error = jit->get_outputpos();
	Write_GetError(jit);
}

ICompilation *JITX86::ApplyOptions(ICompilation *_IN, ICompilation *_OUT)
{
	if (_IN == NULL)
	{
		return _OUT;
	}

	CompData *_in = (CompData * )_IN;
	CompData *_out = (CompData * )_OUT;

	_in->inline_level = _out->inline_level;
	_in->profile = _out->profile;

	_out->Abort();

	return _in;
}

JITX86::JITX86()
{
	m_pJitEntry = NULL;
	m_pJitReturn = NULL;
	m_RoundTable[0] = -1;
	m_RoundTable[1] = 0;
	m_RoundTable[2] = 1;
	m_pJitGenArray = NULL;
}

bool JITX86::InitializeJIT()
{
	jitoffs_t offs;
	JitWriter writer, *jit;

	g_pCodeCache = KE_CreateCodeCache();

	jit = &writer;
	
	/* Build the genarray intrinsic */
	jit->outbase = NULL;
	jit->outptr = NULL;
	WriteIntrinsic_GenArray(jit);
	m_pJitGenArray = Knight::KE_AllocCode(g_pCodeCache, jit->get_outputpos());
	jit->outbase = (jitcode_t)m_pJitGenArray;
	jit->outptr = jit->outbase;
	WriteIntrinsic_GenArray(jit);

	/* Build the entry point */
	writer = JitWriter();
	jit->outbase = NULL;
	jit->outptr = NULL;
	Write_Execute_Function(jit);
	m_pJitEntry = Knight::KE_AllocCode(g_pCodeCache, jit->get_outputpos());
	jit->outbase = (jitcode_t)m_pJitEntry;
	jit->outptr = jit->outbase;
	offs = Write_Execute_Function(jit);
	m_pJitReturn = (uint8_t *)m_pJitEntry + offs;

	return true;
}

void JITX86::ShutdownJIT()
{
	KE_DestroyCodeCache(g_pCodeCache);
}

JitFunction *JITX86::CompileFunction(BaseRuntime *prt, cell_t pcode_offs, int *err)
{
	CompData *data = (CompData *)prt->m_pCo;
	sp_plugin_t *plugin = data->plugin;

	uint8_t *code = plugin->pcode + pcode_offs;
	uint8_t *end_code = plugin->pcode + plugin->pcode_size;

	assert(FuncLookup(data, pcode_offs) == 0);

	if (code >= end_code || *(cell_t *)code != OP_PROC)
	{
		*err = SP_ERROR_INVALID_INSTRUCTION;
		return NULL;
	}

#if defined _DEBUG
	g_engine1.GetDebugHook()->OnDebugSpew("Compiling function %s::%s", prt->m_pPlugin->name, find_func_name(prt->m_pPlugin, pcode_offs));
#endif

	code += sizeof(cell_t);

	OPCODE op;
	uint32_t code_size;
	cell_t *cip, *end_cip;
	JitWriter writer, *jit;

	jit = &writer;
	cip = (cell_t *)code;
	end_cip = (cell_t *)end_code;
	writer.data = data;
	writer.inbase = cip;
	writer.outptr = NULL;
	writer.outbase = NULL;
	data->cur_func = pcode_offs;

jit_rewind:
	data->num_thunks = 0;
	writer.inptr = writer.inbase;

	WriteOp_Proc(jit);

	/* Actual code generation! */
	if (writer.outbase == NULL)
	{
		/* First Pass - find codesize and resolve relocation */
		jitoffs_t jpcode_offs;
		jitoffs_t native_offs;

		for (; writer.inptr < end_cip;)
		{
			op = (OPCODE)writer.peek_cell();

			/* If we hit another function, we must stop. */
			if (op == OP_PROC || op == OP_ENDPROC)
			{
				break;
			}

			/* Store the native offset into the rebase memory.
			 * This large chunk of memory lets us do an instant lookup
			 * based on an original pcode offset.
			 */
			jpcode_offs = (jitoffs_t)((uint8_t *)writer.inptr - plugin->pcode);
			native_offs = jit->get_outputpos();
			*((jitoffs_t *)(data->rebase + jpcode_offs)) = native_offs;

			/* Read past the opcode. */
			writer.read_cell();

			/* Patch the floating point natives with our opcodes */
			if (op == OP_SYSREQ_N)
			{
				cell_t idx = writer.read_cell();
				if (data->jit_float_table[idx].found)
				{
					writer.inptr -= 2;
					*(writer.inptr++) = data->jit_float_table[idx].index;
					*(writer.inptr++) = OP_NOP;
					*(writer.inptr++) = OP_NOP;
					op = (OPCODE)data->jit_float_table[idx].index;
				}
				else
				{
					writer.inptr--;
				}
			}
			switch (op)
			{
				#include "opcode_switch.inc"
			}
			/* Check for errors.  This should only happen in the first pass. */
			if (data->error_set != SP_ERROR_NONE)
			{
				*err = data->error_set;
				return NULL;
			}
		}

		/* Write these last because error jumps should be predicted forwardly (not taken) */
		WriteErrorRoutines(data, jit);

		/* Build thunk tables */
		if (data->num_thunks > data->max_thunks)
		{
			data->max_thunks = data->num_thunks;
			data->thunks = (call_thunk_t *)realloc(
				data->thunks,
				data->max_thunks * sizeof(call_thunk_t));
		}

		/* Write the thunk offsets.
		 * :TODO: we can emit all but one call to Write_CallThunk().
		 */
		for (unsigned int i = 0; i < data->num_thunks; i++)
		{
			data->thunks[i].thunk_addr = jit->get_outputpos();
			Write_CallThunk(jit, 0, 0);
		}

		/**
		 * I don't understand the purpose of this code.
		 * Why do we want to know about the last opcode?
		 * It should already have gotten written.
		 */
		#if 0
		/* Write the final CIP to the last position in the reloc array */
		pcode_offs = (jitoffs_t)((uint8_t *)writer.inptr - code);
		native_offs = jit->get_outputpos();
		*((jitoffs_t *)(data->rebase + pcode_offs)) = native_offs;
		#endif

		/* the total codesize is now known! */
		code_size = writer.get_outputpos();
		writer.outbase = (jitcode_t)Knight::KE_AllocCode(g_pCodeCache, code_size);
		writer.outptr = writer.outbase;

		/* go back for second pass */
		goto jit_rewind;
	}
	else
	{
		/*******
		 * SECOND PASS - write opcode info
		 *******/
		for (; writer.inptr < end_cip;)
		{
			op = (OPCODE)writer.read_cell();

			/* If we hit another function, we must stop. */
			if (op == OP_PROC || op == OP_ENDPROC)
			{
				break;
			}

			switch (op)
			{
				#include "opcode_switch.inc"
			}
		}

		/* Write these last because error jumps should be predicted as not taken (forward) */
		WriteErrorRoutines(data, jit);

		/* Write the thunk offsets. */
		for (unsigned int i = 0; i < data->num_thunks; i++)
		{
			Write_CallThunk(jit, data->thunks[i].patch_addr, data->thunks[i].pcode_offs);
		}
	}

	plugin->prof_flags = data->profile;

	*err = SP_ERROR_NONE;

	JitFunction *fn;
	uint32_t func_idx;
	
	fn = new JitFunction(writer.outbase, pcode_offs);
	func_idx = prt->AddJittedFunction(fn);
	*(cell_t *)(data->rebase + pcode_offs) = func_idx;

	return fn;
}

void JITX86::SetupContextVars(BaseRuntime *runtime, BaseContext *pCtx, sp_context_t *ctx)
{
	tracker_t *trk = new tracker_t;

	ctx->vm[JITVARS_TRACKER] = trk;
	ctx->vm[JITVARS_BASECTX] = pCtx; /* GetDefaultContext() is not constructed yet */
	ctx->vm[JITVARS_PROFILER] = g_engine2.GetProfiler();
	ctx->vm[JITVARS_PLUGIN] = runtime->m_pPlugin;

	trk->pBase = (ucell_t *)malloc(1024);
	trk->pCur = trk->pBase;
	trk->size = 1024 / sizeof(cell_t);
}

SPVM_NATIVE_FUNC JITX86::CreateFakeNative(SPVM_FAKENATIVE_FUNC callback, void *pData)
{
	JitWriter jw;
	JitWriter *jit = &jw;

	jw.outbase = NULL;
	jw.outptr = NULL;
	jw.data = NULL;

	/* First pass, calculate size */
rewind:
	/* Align the stack to 16 bytes */
	//push ebx
	//push edi
	//push esi
	//mov edi, [esp+16]	;store ctx
	//mov esi, [esp+20]	;store params
	//mov ebx, esp
	//and esp, 0xFFFFFF0
	//sub esp, 4
	IA32_Push_Reg(jit, REG_EBX);
	IA32_Push_Reg(jit, REG_EDI);
	IA32_Push_Reg(jit, REG_ESI);
	IA32_Mov_Reg_Esp_Disp8(jit, REG_EDI, 16);
	IA32_Mov_Reg_Esp_Disp8(jit, REG_ESI, 20);
	IA32_Mov_Reg_Rm(jit, REG_EBX, REG_ESP, MOD_REG);
	IA32_And_Rm_Imm8(jit, REG_ESP, MOD_REG, -16);
	IA32_Sub_Rm_Imm8(jit, REG_ESP, 4, MOD_REG);

	//push pData		;push pData
	//push esi			;push params
	//push edi			;push ctx
	//call [callback]	;invoke the meta-callback
	//mov esp, ebx		;restore the stack
	//pop esi			;restore esi
	//pop edi			;restore edi
	//pop ebx			;restore ebx
	//ret				;return
	IA32_Push_Imm32(jit, (jit_int32_t)pData);
	IA32_Push_Reg(jit, REG_ESI);
	IA32_Push_Reg(jit, REG_EDI);
	uint32_t call = IA32_Call_Imm32(jit, 0);
	IA32_Write_Jump32_Abs(jit, call, (void *)callback);
	IA32_Mov_Reg_Rm(jit, REG_ESP, REG_EBX, MOD_REG);
	IA32_Pop_Reg(jit, REG_ESI);
	IA32_Pop_Reg(jit, REG_EDI);
	IA32_Pop_Reg(jit, REG_EBX);
	IA32_Return(jit);

	if (jw.outbase == NULL)
	{
		/* Second pass: Actually write */
		jw.outbase = (jitcode_t)KE_AllocCode(g_pCodeCache, jw.get_outputpos());
		if (!jw.outbase)
		{
			return NULL;
		}
		jw.outptr = jw.outbase;
	
		goto rewind;
	}

	return (SPVM_NATIVE_FUNC)jw.outbase;
}

void JITX86::DestroyFakeNative(SPVM_NATIVE_FUNC func)
{
	KE_FreeCode(g_pCodeCache, (void *)func);
}

ICompilation *JITX86::StartCompilation()
{
	CompData *data = new CompData;

	data->jit_float_table = NULL;

	return data;
}

void CompData::SetRuntime(BaseRuntime *runtime)
{
	plugin = runtime->m_pPlugin;

	uint32_t max_natives = plugin->num_natives;

	this->runtime = runtime;
	this->plugin = runtime->m_pPlugin;

	inline_level = JIT_INLINE_ERRORCHECKS|JIT_INLINE_NATIVES;
	error_set = SP_ERROR_NONE;

	jit_float_table = new floattbl_t[max_natives];
	for (uint32_t i=0; i<max_natives; i++)
	{
		const char *name = plugin->natives[i].name;
		if (!strcmp(name, "FloatAbs"))
		{
			jit_float_table[i].found = true;
			jit_float_table[i].index = OP_FABS;
		} 
		else if (!strcmp(name, "FloatAdd")) 
		{
			jit_float_table[i].found = true;
			jit_float_table[i].index = OP_FLOATADD;
		} 
		else if (!strcmp(name, "FloatSub")) 
		{
			jit_float_table[i].found = true;
			jit_float_table[i].index = OP_FLOATSUB;
		} 
		else if (!strcmp(name, "FloatMul")) 
		{
			jit_float_table[i].found = true;
			jit_float_table[i].index = OP_FLOATMUL;
		} 
		else if (!strcmp(name, "FloatDiv")) 
		{
			jit_float_table[i].found = true;
			jit_float_table[i].index = OP_FLOATDIV;
		} 
		else if (!strcmp(name, "float")) 
		{
			jit_float_table[i].found = true;
			jit_float_table[i].index = OP_FLOAT;
		} 
		else if (!strcmp(name, "FloatCompare")) 
		{
			jit_float_table[i].found = true;
			jit_float_table[i].index = OP_FLOATCMP;
		} 
		else if (!strcmp(name, "RoundToZero")) 
		{
			jit_float_table[i].found = true;
			jit_float_table[i].index = OP_RND_TO_ZERO;
		} 
		else if (!strcmp(name, "RoundToCeil")) 
		{
			jit_float_table[i].found = true;
			jit_float_table[i].index = OP_RND_TO_CEIL;
		} 
		else if (!strcmp(name, "RoundToFloor")) 
		{
			jit_float_table[i].found = true;
			jit_float_table[i].index = OP_RND_TO_FLOOR;
		} 
		else if (!strcmp(name, "RoundToNearest")) 
		{
			jit_float_table[i].found = true;
			jit_float_table[i].index = OP_RND_TO_NEAREST;
		}
	}

	/* We need a relocation table.  This is the relocation table we'll use for jumps 
	 * and calls alike.  One extra cell for final CIP.
	 */
	this->rebase = (uint8_t *)engine->BaseAlloc(plugin->pcode_size + sizeof(cell_t));
	memset(this->rebase, 0, plugin->pcode_size + sizeof(cell_t));
}

ICompilation *JITX86::StartCompilation(BaseRuntime *runtime)
{
	CompData *data = new CompData;

	data->SetRuntime(runtime);

	return data;
}

void CompData::Abort()
{
	if (rebase)
	{
		engine->BaseFree(rebase);
	}
	delete [] thunks;
	delete [] jit_float_table;
	delete this;
}

void JITX86::FreeContextVars(sp_context_t *ctx)
{
	free(((tracker_t *)(ctx->vm[JITVARS_TRACKER]))->pBase);
	delete (tracker_t *)ctx->vm[JITVARS_TRACKER];
}

bool CompData::SetOption(const char *key, const char *val)
{
	if (strcmp(key, SP_JITCONF_DEBUG) == 0)
	{
		return true;
	}
	else if (strcmp(key, SP_JITCONF_PROFILE) == 0)
	{
		profile = atoi(val);

		/** Callbacks must be profiled to profile functions! */
		if ((profile & SP_PROF_FUNCTIONS) == SP_PROF_FUNCTIONS)
		{
			profile |= SP_PROF_CALLBACKS;
		}

		return true;
	}

	return false;
}

void *JITX86::GetGenArrayIntrinsic()
{
	return m_pJitGenArray;
}

void *JITX86::GetReturnPoint()
{
	return m_pJitReturn;
}

void *JITX86::GetRoundingTable()
{
	return m_RoundTable;
}

typedef int (*JIT_EXECUTE)(cell_t *vars, void *addr);
int JITX86::InvokeFunction(BaseRuntime *runtime, JitFunction *fn, cell_t *result)
{
	int err;
	JIT_EXECUTE pfn;
	sp_context_t *ctx;
	cell_t vars[AMX_NUM_INFO_VARS];

	ctx = runtime->GetBaseContext()->GetCtx();

	vars[0] = ctx->sp;
	vars[1] = ctx->hp;
	vars[2] = (cell_t)result;
	vars[3] = (cell_t)ctx;
	vars[4] = (cell_t)(runtime->m_pPlugin->memory + runtime->m_pPlugin->mem_size);
	vars[5] = fn->GetPCodeAddress();
	vars[6] = runtime->m_pPlugin->data_size;
	vars[7] = (cell_t)(runtime->m_pPlugin->memory);
	/* vars[8] will be set to ESP */

	pfn = (JIT_EXECUTE)m_pJitEntry;
	err = pfn(vars, fn->GetEntryAddress());

	ctx->hp = vars[1];
	ctx->err_cip = vars[5];

	return err;
}

void *JITX86::AllocCode(size_t size)
{
	return Knight::KE_AllocCode(g_pCodeCache, size);
}

void JITX86::FreeCode(void *code)
{
	KE_FreeCode(g_pCodeCache, code);
}
