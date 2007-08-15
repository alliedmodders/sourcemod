/**
 * vim: set ts=4 :
 * =============================================================================
 * SourcePawn JIT
 * Copyright (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is not open source and may not be copied without explicit wriiten
 * permission of AlliedModders LLC.  This file may not be redistributed in whole
 * or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEPAWN_JIT_X86_UNGEN_OPCODES_H_
#define _INCLUDE_SOURCEPAWN_JIT_X86_UNGEN_OPCODES_H_

inline void WriteOp_UMul(JitWriter *jit)
{
	//mov ecx, edx
	//mul edx
	//mov edx, ecx
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_ALT, MOD_REG);
	IA32_Mul_Rm(jit, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_TMP, MOD_REG);
}

inline void WriteOp_Less(JitWriter *jit)
{
	//cmp eax, edx	; PRI < ALT ? (unsigned)
	//mov eax, 0
	//setb al
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_B);
}

inline void WriteOp_Leq(JitWriter *jit)
{
	//cmp eax, edx	; PRI <= ALT ? (unsigned)
	//mov eax, 0
	//setbe al
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_BE);
}

inline void WriteOp_Grtr(JitWriter *jit)
{
	//cmp eax, edx	; PRI > ALT ? (unsigned)
	//mov eax, 0
	//seta al
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_A);
}

inline void WriteOp_Geq(JitWriter *jit)
{
	//cmp eax, edx	; PRI >= ALT ? (unsigned)
	//mov eax, 0
	//setae al
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
	IA32_SetCC_Rm8(jit, AMX_REG_PRI, CC_AE);
}

inline void WriteOp_Align_Pri(JitWriter *jit)
{
	//xor eax, <cellsize - val>
	cell_t val = sizeof(cell_t) - jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
	{
		IA32_Xor_Rm_Imm8(jit, AMX_REG_PRI, MOD_REG, (jit_int8_t)val);
	} else {
		IA32_Xor_Eax_Imm32(jit, val);
	}
}

inline void WriteOp_Align_Alt(JitWriter *jit)
{
	//xor edx, <cellsize - val>
	cell_t val = sizeof(cell_t) - jit->read_cell();
	if (val < SCHAR_MAX && val > SCHAR_MIN)
	{
		IA32_Xor_Rm_Imm8(jit, AMX_REG_ALT, MOD_REG, (jit_int8_t)val);
	} else {
		IA32_Xor_Rm_Imm32(jit, AMX_REG_ALT, MOD_REG, val);
	}
}

inline void WriteOp_Cmps(JitWriter *jit)
{
	//push edi
	//push esi
	//lea esi, [ebp+edx]
	//lea edi, [ebp+eax]
	//mov ecx, <val>
	unsigned int val = jit->read_cell();

	IA32_Push_Reg(jit, REG_EDI);
	IA32_Push_Reg(jit, REG_ESI);
	IA32_Lea_Reg_DispEBPRegMult(jit, REG_ESI, AMX_REG_DAT, AMX_REG_ALT, NOSCALE);
	IA32_Lea_Reg_DispEBPRegMult(jit, REG_EDI, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
	IA32_Mov_Reg_Imm32(jit, REG_ECX, val);

	//xor eax, eax
	//repe cmpsb
	//je :cmps1
	IA32_Xor_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_PRI, MOD_REG);
	IA32_Rep(jit);
	IA32_Cmpsb(jit);
	jitoffs_t jmp = IA32_Jump_Cond_Imm8(jit, CC_E, 0);

	//sbb eax, eax
	//sbb eax, -1
	IA32_Sbb_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_PRI, MOD_REG);
	IA32_Sbb_Rm_Imm8(jit, AMX_REG_PRI, -1, MOD_REG);

	//:cmps1
	//pop esi
	//pop edi
	IA32_Send_Jump8_Here(jit, jmp);
	IA32_Pop_Reg(jit, REG_ESI);
	IA32_Pop_Reg(jit, REG_EDI);
}

inline void WriteOp_Lctrl(JitWriter *jit)
{
	cell_t val = jit->read_cell();
	switch (val)
	{
	case 0:
		{
			//mov ecx, [esi+ctx]
			//mov eax, [ecx+ctx.codebase]
			IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_CONTEXT);
			IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, AMX_REG_TMP, offsetof(sp_context_t, codebase));
			break;
		}
	case 1:
		{
			//mov eax, ebp
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
			IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_CONTEXT);
			IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, AMX_REG_TMP, offsetof(sp_context_t, memory));
			break;
		}
	case 4:
		{
			//mov eax, edi
			//sub eax, ebp	- unrelocate
			IA32_Mov_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_STK, MOD_REG);
			IA32_Sub_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_DAT, MOD_REG);
			break;
		}
	case 5:
		{
			//mov eax, [esi+frm]
			IA32_Mov_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_INFO, MOD_MEM_REG);
			break;
		}
	case 6:
		{
			//mov eax, [cip]
			jitoffs_t imm32 = IA32_Mov_Reg_Imm32(jit, AMX_REG_PRI, 0);
			jitoffs_t save = jit->get_outputpos();
			jit->set_outputpos(imm32);
			jit->write_int32((uint32_t)(jit->outbase + save));
			jit->set_outputpos(save);
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
			IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_INFO, AMX_REG_PRI, AMX_INFO_HEAP);
			break;
		}
	case 4:
		{
			//lea edi, [ebp+eax]
			IA32_Lea_Reg_DispEBPRegMult(jit, AMX_REG_STK, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
			break;
		}
	case 5:
		{
			//lea ebx, [ebp+eax]	- overwrite frm
			//mov [esi+frm], eax	- overwrite stacked frame
			IA32_Lea_Reg_DispEBPRegMult(jit, AMX_REG_FRM, AMX_REG_DAT, AMX_REG_PRI, NOSCALE);
			IA32_Mov_Rm_Reg(jit, AMX_REG_INFO, AMX_REG_PRI, MOD_MEM_REG);
			break;
		}
	case 6:
		{
			IA32_Jump_Reg(jit, AMX_REG_PRI);
			break;
		}
	}
}

inline void WriteOp_UDiv(JitWriter *jit)
{
	//mov ecx, edx
	//xor edx, edx
	//div ecx
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_ALT, MOD_REG);
	IA32_Xor_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_ALT, MOD_REG);
	Write_Check_DivZero(jit, AMX_REG_TMP);
	IA32_Div_Rm(jit, AMX_REG_TMP, MOD_REG);
}

inline void WriteOp_UDiv_Alt(JitWriter *jit)
{
	//mov ecx, eax
	//mov eax, edx
	//xor edx, edx
	//div ecx
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_PRI, MOD_REG);
	IA32_Mov_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Xor_Reg_Rm(jit, AMX_REG_ALT, AMX_REG_ALT, MOD_REG);
	Write_Check_DivZero(jit, AMX_REG_TMP);
	IA32_Div_Rm(jit, AMX_REG_TMP, MOD_REG);
}

inline void WriteOp_Ret(JitWriter *jit)
{
	//mov ebx, [edi]		- get old FRM
	//add edi, 4			- pop stack
	//mov [esi+frm], ebx	- restore
	//add ebx, ebp			- relocate
	//ret
	IA32_Mov_Reg_Rm(jit, AMX_REG_FRM, AMX_REG_STK, MOD_MEM_REG);
	IA32_Add_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
	IA32_Mov_Rm_Reg(jit, AMX_REG_INFO, AMX_REG_FRM, MOD_MEM_REG);
	IA32_Add_Reg_Rm(jit, AMX_REG_FRM, AMX_REG_DAT, MOD_REG);
	IA32_Return(jit);
}

inline void WriteOp_JRel(JitWriter *jit)
{
	//jmp <offs>	;relative jump
	cell_t cip_offs = jit->read_cell();

	/* Note that since code size calculation has to be done in the same
	* phase as building relocation information, we cannot know the jump size
	* beforehand.  Thus, we always write full 32bit jumps for safety.
	*/
	jitoffs_t jmp = IA32_Jump_Imm32(jit, 0);
	IA32_Write_Jump32(jit, jmp, RelocLookup(jit, cip_offs));
}

inline void WriteOp_Jless(JitWriter *jit)
{
	//cmp eax, edx
	//jb <target>
	cell_t target = jit->read_cell();
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Jump_Cond_Imm32(jit, CC_B, RelocLookup(jit, target, false));
}

inline void WriteOp_Jleq(JitWriter *jit)
{
	//cmp eax, edx
	//jbe <target>
	cell_t target = jit->read_cell();
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Jump_Cond_Imm32(jit, CC_BE, RelocLookup(jit, target, false));
}

inline void WriteOp_Jgrtr(JitWriter *jit)
{
	//cmp eax, edx
	//ja <target>
	cell_t target = jit->read_cell();
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Jump_Cond_Imm32(jit, CC_A, RelocLookup(jit, target, false));
}

inline void WriteOp_Jgeq(JitWriter *jit)
{
	//cmp eax, edx
	//jae <target>
	cell_t target = jit->read_cell();
	IA32_Cmp_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
	IA32_Jump_Cond_Imm32(jit, CC_AE, RelocLookup(jit, target, false));
}

#endif //_INCLUDE_SOURCEPAWN_JIT_X86_UNGEN_OPCODES_H_
