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

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include "jit_x86.h"
#include "opcode_helpers.h"
#include "x86_macros.h"

jitoffs_t Write_Execute_Function(JitWriter *jit)
{
	/** 
	 * The variables we're passed in:
	 *  void *vars[], void *entry_func
	 */

	/**
	 * !NOTE!
	 * Currently, we do not accept ctx->frm as the new frame pointer.
	 * Instead, we copy the frame from the stack pointer.
	 * This is because we do not support resuming or sleeping!
	 */

	//push ebp
	//mov ebp, esp
	IA32_Push_Reg(jit, REG_EBP);
	IA32_Mov_Reg_Rm(jit, REG_EBP, REG_ESP, MOD_REG);

	//push esi
	//push edi
	//push ebx
	IA32_Push_Reg(jit, REG_ESI);
	IA32_Push_Reg(jit, REG_EDI);
	IA32_Push_Reg(jit, REG_EBX);

	/* Prep us for doing the real work */
	//mov esi, [ebp+param0]			;get vars
	//mov ecx, [ebp+param1]			;get entry addr
	//mov eax, [esi+MEMORY]			;get memory base
	//mov edx, [esi+CONTEXT]		;get context
	IA32_Mov_Reg_Rm_Disp8(jit, REG_ESI, REG_EBP, 8 + 4*0);
	IA32_Mov_Reg_Rm_Disp8(jit, REG_ECX, REG_EBP, 8 + 4*1);
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, REG_ESI, AMX_INFO_MEMORY);
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EDX, REG_ESI, AMX_INFO_CONTEXT);

	/* Set up run-time registers */
	//mov edi, [edx+SP]				;non-reloc SP
	//add edi, eax					;reloc SP
	//mov ebp, eax					;DAT
	//mov ebx, edi					;reloc FRM
	//mov [esi+NSTACK], esp			;save ESP
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EDI, REG_EDX, offsetof(sp_context_t, sp));
	IA32_Add_Rm_Reg(jit, REG_EDI, REG_EAX, MOD_REG);
	IA32_Mov_Reg_Rm(jit, REG_EBP, REG_EAX, MOD_REG);
	IA32_Mov_Reg_Rm(jit, REG_EBX, REG_EDI, MOD_REG);
	IA32_Mov_Rm_Reg_Disp8(jit, REG_ESI, REG_ESP, AMX_INFO_NSTACK);

	/* by now, everything is set up, so we can call into the plugin */
	//call ecx
	IA32_Call_Reg(jit, REG_ECX);

	/* if the code flow gets to here, there was a normal return */
	//mov ecx, [esi+RETVAL]			;get retval pointer
	//mov [ecx], eax				;store retval from PRI
	//mov eax, SP_ERROR_NONE		;set no error
	IA32_Mov_Reg_Rm_Disp8(jit, REG_ECX, AMX_REG_INFO, AMX_INFO_RETVAL);
	IA32_Mov_Rm_Reg(jit, REG_ECX, AMX_REG_PRI, MOD_MEM_REG);
	IA32_Mov_Reg_Imm32(jit, REG_EAX, SP_ERROR_NONE);

	/* save where error checking/halting functions should go to */
	jitoffs_t offs_return = jit->get_outputpos();
	//mov esp, [esi+NSTACK]			;restore stack pointer
	IA32_Mov_Reg_Rm_Disp8(jit, REG_ESP, REG_ESI, AMX_INFO_NSTACK);

	/* Restore SP */
	//mov ecx, [esi+CONTEXT]
	//sub edi, ebp
	//mov [ecx+SP], edi
	IA32_Mov_Reg_Rm_Disp8(jit, REG_ECX, REG_ESI, AMX_INFO_CONTEXT);
	IA32_Sub_Reg_Rm(jit, REG_EDI, REG_EBP, MOD_REG);
	IA32_Mov_Rm_Reg_Disp8(jit, REG_ECX, REG_EDI, offsetof(sp_context_t, sp));

	//pop ebx
	//pop edi
	//pop esi
	//pop ebp
	//ret
	IA32_Pop_Reg(jit, REG_EBX);
	IA32_Pop_Reg(jit, REG_EDI);
	IA32_Pop_Reg(jit, REG_ESI);
	IA32_Pop_Reg(jit, REG_EBP);
	IA32_Return(jit);

	return offs_return;
}

void Write_GetError(JitWriter *jit)
{
	//mov eax, [esi+info.context]
	//mov eax, [eax+ctx.error]
	//jmp [jit_return]
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, AMX_REG_INFO, AMX_INFO_CONTEXT);
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, REG_EAX, offsetof(sp_context_t, n_err));
	IA32_Jump_Imm32_Abs(jit, g_Jit.GetReturnPoint());
}

void Write_SetError(JitWriter *jit, int error)
{
	//mov eax, <error>
	//jmp [jit_return]
	IA32_Mov_Reg_Imm32(jit, REG_EAX, error);
	IA32_Jump_Imm32_Abs(jit, g_Jit.GetReturnPoint());
}

void Write_Check_DivZero(JitWriter *jit, jit_uint8_t reg)
{
	//test reg, reg
	//jz :error
	IA32_Test_Rm_Reg(jit, reg, reg, MOD_REG);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_Z, ((CompData *)jit->data)->jit_error_divzero);
}

void Write_CheckHeap_Min(JitWriter *jit)
{
	/* Check if the stack went beyond the heap low.
	 * This usually means there was a compiler error.
	 * NOTE: Special optimization here.
	 * The heap low is always known ahead of time! :)
	 */
	CompData *data = (CompData *)jit->data;
	//cmp [esi+info.heap], <heaplow>
	//jb :error
	IA32_Cmp_Rm_Imm32_Disp8(jit, AMX_REG_INFO, AMX_INFO_HEAP, data->plugin->data_size);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_B, data->jit_error_heapmin);
}

void Write_CheckHeap_Low(JitWriter *jit)
{
	/* Check if the heap is trying to grow beyond the stack.
	 */
	//mov ecx, [esi+info.heap]
	//lea ecx, [ebp+ecx+STACK_MARGIN]
	//cmp ecx, edi
	//ja :error		; I think this is right
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_HEAP);
	IA32_Lea_Reg_DispRegMultImm8(jit, AMX_REG_TMP, AMX_REG_DAT, AMX_REG_TMP, NOSCALE, STACK_MARGIN);
	IA32_Cmp_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_STK, MOD_REG);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_A, ((CompData *)jit->data)->jit_error_heaplow);
}

void Write_CheckStack_Min(JitWriter *jit)
{
	/* Check if the stack went beyond the stack top
	 * This usually means there was a compiler error.
	 */
	//cmp edi, [esi+info.stacktop]
	//jae :error
	IA32_Cmp_Reg_Rm_Disp8(jit, AMX_REG_STK, AMX_REG_INFO, AMX_INFO_STACKTOP);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_AE, ((CompData *)jit->data)->jit_error_stackmin);
}

void Write_CheckStack_Low(JitWriter *jit)
{
	/* Check if the stack went beyond the heap boundary.
	 * Unfortunately this one isn't as quick as the other check.
	 * The stack margin check is important for sysreq.n having space.
	 */
	//mov ecx, [esi+info.heap]
	//lea ecx, [ebp+ecx+STACK_MARGIN]
	//cmp edi, ecx
	//jb :error		; I think this is right
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_HEAP);
	IA32_Lea_Reg_DispRegMultImm8(jit, AMX_REG_TMP, AMX_REG_DAT, AMX_REG_TMP, NOSCALE, STACK_MARGIN);
	IA32_Cmp_Reg_Rm(jit, AMX_REG_STK, AMX_REG_TMP, MOD_REG);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_B, ((CompData *)jit->data)->jit_error_stacklow);
}

void Write_Check_VerifyAddr(JitWriter *jit, jit_uint8_t reg)
{
	/* :TODO: Should this be checking for below heaplow?
	 * The old JIT did not.
	 */

	/** 
	 * :TODO: If we can't find a nicer way of doing this,
	 *  then scrap it on high optimizations.  The second portion is not needed at all!
	 */

	/* Part 1: Check if we're in the memory bounds */
	//cmp <reg>, <stpu>
	//jae :error
	IA32_Cmp_Rm_Imm32(jit, MOD_REG, reg, ((CompData *)jit->data)->plugin->mem_size);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_AE, ((CompData *)jit->data)->jit_error_memaccess);

	/* Part 2: Check if we're in the invalid region between HP and SP */
	jitoffs_t jmp;
	//cmp <reg>, [esi+info.heap]
	//jb :continue
	//lea ecx, [ebp+<reg>]
	//cmp edi, ecx
	//jb :error
	//:continue
	IA32_Cmp_Reg_Rm_Disp8(jit, reg, AMX_REG_INFO, AMX_INFO_HEAP);
	jmp = IA32_Jump_Cond_Imm8(jit, CC_B, 0);
	IA32_Lea_Reg_DispRegMultImm8(jit, AMX_REG_TMP, AMX_REG_DAT, reg, NOSCALE, 0);
	IA32_Cmp_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_STK, MOD_REG);
	IA32_Jump_Cond_Imm32_Rel(jit, CC_B, ((CompData *)jit->data)->jit_error_memaccess);
	IA32_Send_Jump8_Here(jit, jmp);
}

void Macro_PushN_Addr(JitWriter *jit, int i)
{
	//push eax
	//mov eax, [esi+frm]
	//loop i times:
	// lea ecx, [eax+<val>]
	// mov [edi-4*i], ecx
	//sub edi, 4*N
	//pop eax

	cell_t val;
	int n = 1;
	IA32_Push_Reg(jit, AMX_REG_PRI);
	IA32_Mov_Reg_Rm(jit, AMX_REG_PRI, AMX_REG_INFO, MOD_MEM_REG);
	do
	{
		val = jit->read_cell();
		if (val <= SCHAR_MAX && val >= SCHAR_MIN)
			IA32_Lea_DispRegImm8(jit, AMX_REG_TMP, AMX_REG_PRI, (jit_int8_t)val);
		else
			IA32_Lea_DispRegImm32(jit, AMX_REG_TMP, AMX_REG_PRI, val);
		IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_STK, AMX_REG_TMP, -4*n);
	} while (n++ < i);
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4*i, MOD_REG);
	IA32_Pop_Reg(jit, AMX_REG_PRI);
}

void Macro_PushN_S(JitWriter *jit, int i)
{
	//loop i times:
	// mov ecx, [ebx+<val>]
	// mov [edi-4*i], ecx
	//sub edi, 4*N

	cell_t val;
	int n = 1;
	do 
	{
		val = jit->read_cell();
		if (val <= SCHAR_MAX && val >= SCHAR_MIN)
			IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_FRM, (jit_int8_t)val);
		else
			IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_FRM, val);
		IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_STK, AMX_REG_TMP, -4*n);
	} while (n++ < i);
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4*i, MOD_REG);
}

void Macro_PushN_C(JitWriter *jit, int i)
{
	//loop i times:
	// mov [edi-4*i], <val>
	//sub edi, 4*N

	int n = 1;
	do 
	{
		IA32_Mov_Rm_Imm32_Disp8(jit, AMX_REG_STK, jit->read_cell(), -4*n);
	} while (n++ < i);
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4*i, MOD_REG);
}

void Macro_PushN(JitWriter *jit, int i)
{
	//loop i times:
	// mov ecx, [ebp+<val>]
	// mov [edi-4*i], ecx
	//sub edi, 4*N

	cell_t val;
	int n = 1;
	do 
	{
		val = jit->read_cell();
		if (val <= SCHAR_MAX && val >= SCHAR_MIN)
			IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_DAT, (jit_int8_t)val);
		else
			IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_DAT, val);
		IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_STK, AMX_REG_TMP, -4*n);
	} while (n++ < i);
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4*i, MOD_REG);
}

void WriteOp_Sysreq_C_Function(JitWriter *jit)
{
	/* The small daddy of the big daddy of opcodes.
	 * ecx - native index
	 */
	CompData *data = (CompData *)jit->data;

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
	//push ecx		; native index
	IA32_Push_Reg(jit, AMX_REG_STK);
	IA32_Push_Reg(jit, REG_ECX);

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
	IA32_Write_Jump32_Abs(jit, call, (void *)NativeCallback);

	/* Test for error */
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

	//ret
	IA32_Return(jit);
}

typedef struct array_creation_s
{
	const cell_t *dim_list;				/* Dimension sizes */
	cell_t dim_count;					/* Number of dimensions */
	cell_t *data_offs;					/* Current offset AFTER the indirection vectors (data) */
	cell_t *base;						/* array base */
} array_creation_t;

cell_t _GenerateArrayIndirectionVectors(array_creation_t *ar, int dim, cell_t cur_offs)
{
	cell_t write_offs = cur_offs;
	cell_t *data_offs = ar->data_offs;

	cur_offs += ar->dim_list[dim];

	/**
	 * Dimension n-x where x > 2 will have sub-vectors.  
	 * Otherwise, we just need to reference the data section.
	 */
	if (ar->dim_count > 2 && dim < ar->dim_count - 2)
	{
		/**
		 * For each index at this dimension, write offstes to our sub-vectors.
		 * After we write one sub-vector, we generate its sub-vectors recursively.
		 * At the end, we're given the next offset we can use.
		 */
		for (int i = 0; i < ar->dim_list[dim]; i++)
		{
			ar->base[write_offs] = (cur_offs - write_offs) * sizeof(cell_t);
			write_offs++;
			cur_offs = _GenerateArrayIndirectionVectors(ar, dim + 1, cur_offs);
		}
	} else {
		/**
		 * In this section, there are no sub-vectors, we need to write offsets 
		 * to the data.  This is separate so the data stays in one big chunk.
		 * The data offset will increment by the size of the last dimension, 
		 * because that is where the data is finally computed as. 
		 */
		for (int i = 0; i < ar->dim_list[dim]; i++)
		{
			ar->base[write_offs] = (*data_offs - write_offs) * sizeof(cell_t);
			write_offs++;
			*data_offs = *data_offs + ar->dim_list[dim + 1];
		}
	}

	return cur_offs;
}

static cell_t calc_indirection(const array_creation_t *ar, cell_t dim)
{
	cell_t size = ar->dim_list[dim];

	if (dim < ar->dim_count - 2)
	{
		size += ar->dim_list[dim] * calc_indirection(ar, dim + 1);
	}

	return size;
}

void GenerateArrayIndirectionVectors(cell_t *arraybase, cell_t dims[], cell_t _dimcount, bool autozero)
{
	array_creation_t ar;
	cell_t data_offs;

	/* Reverse the dimensions */
	cell_t dim_list[sDIMEN_MAX];
	int cur_dim = 0;
	for (int i = _dimcount - 1; i >= 0; i--)
	{
		dim_list[cur_dim++] = dims[i];
	}
	
	ar.base = arraybase;
	ar.dim_list = dim_list;
	ar.dim_count = _dimcount;
	ar.data_offs = &data_offs;

	data_offs = calc_indirection(&ar, 0);

	_GenerateArrayIndirectionVectors(&ar, 0, 0);
}

/**
 * A few notes about this function.
 * I was more concerned about efficient use of registers here, rather than 
 *  fine-tuned optimization.  The reason is that the code is already complicated,
 *  and it is very easy to mess up.
 */
void WriteIntrinsic_GenArray(JitWriter *jit)
{
	jitoffs_t err1, err2;

	/**
	 * save important values
	 */
	//push ebx
	//push eax
	//push edx
	//push ecx				;value is referenced on stack
	IA32_Push_Reg(jit, REG_EBX);
	IA32_Push_Reg(jit, REG_EAX);
	IA32_Push_Reg(jit, REG_EDX);
	IA32_Push_Reg(jit, REG_ECX);

	/**
	 * Calculate how many cells will be needed.
	 */
	//mov edx, [edi]		;get last dimension's count
	//mov eax, 1			;position at second to last dimension
	//:loop
	//cmp eax, [esp]		;compare to # of params
	//jae :done				;end loop if done
	//mov ecx, [edi+eax*4]	;get dimension size
	//imul edx, ecx			;multiply by size
	//add eax, 1			;increment
	//add edx, ecx			;add size (indirection vector)
	//jmp :loop				;jump back
	//:done
	IA32_Mov_Reg_Rm(jit, REG_EDX, AMX_REG_STK, MOD_MEM_REG);
	IA32_Mov_Reg_Imm32(jit, REG_EAX, 1);
	jitoffs_t loop1 = jit->get_outputpos();
	IA32_Cmp_Reg_Rm_ESP(jit, REG_EAX);
	jitoffs_t done1 = IA32_Jump_Cond_Imm8(jit, CC_AE, 0);
	IA32_Mov_Reg_Rm_Disp_Reg(jit, REG_ECX, AMX_REG_STK, REG_EAX, SCALE4);
	IA32_IMul_Reg_Rm(jit, REG_EDX, REG_ECX, MOD_REG);
	IA32_Add_Rm_Imm8(jit, REG_EAX, 1, MOD_REG);
	IA32_Add_Reg_Rm(jit, REG_EDX, REG_ECX, MOD_REG);
	IA32_Write_Jump8(jit, IA32_Jump_Imm8(jit, loop1), loop1);
	IA32_Send_Jump8_Here(jit, done1);

	/* Test if we have heap space for this */
	//mov eax, [esi+info.heap]	;get heap pointer
	//lea eax, [eax+edx*4]		;new heap pointer
	//cmp eax, [esi+info.datasz] ;compare to heap low
	//jbe :error				;die if we hit this (it should always be >)
	//add eax, ebp				;relocate to stack
	//cmp eax, edi				;die if above the stack pointer
	//jae :error
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, AMX_REG_INFO, AMX_INFO_HEAP);
	IA32_Lea_Reg_DispRegMult(jit, REG_EAX, REG_EAX, REG_EDX, SCALE4);
	IA32_Cmp_Reg_Rm_Disp8(jit, REG_EAX, AMX_REG_INFO, AMX_INFO_DATASIZE);
	err1 = IA32_Jump_Cond_Imm32(jit, CC_BE, 0);
	IA32_Add_Reg_Rm(jit, REG_EAX, AMX_REG_DAT, MOD_REG);
	IA32_Cmp_Reg_Rm(jit, REG_EAX, AMX_REG_STK, MOD_REG);
	err2 = IA32_Jump_Cond_Imm32(jit, CC_AE, 0);
	
	/* Prepare for indirection iteration */
	//mov eax, [esi+info.heap]	;get heap pointer
	//lea ebx, [eax+edx*4]		;new heap pointer
	//mov [esi+info.heap], ebx	;store back
	//push eax					;save heap pointer - we need it
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, AMX_REG_INFO, AMX_INFO_HEAP);
	IA32_Lea_Reg_DispRegMult(jit, REG_EBX, REG_EAX, REG_EDX, SCALE4);
	IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_INFO, REG_EBX, AMX_INFO_HEAP);
	IA32_Push_Reg(jit, REG_EAX);

	WriteOp_Tracker_Push_Reg(jit, REG_EDX);
	
	/* This part is too messy to do in straight assembly.
	 * I'm letting the compiler handle it and thus it's in C.
	 */
	//lea ebx, [ebp+eax]		;get base pointer
	//push dword [esp-8]		;push autozero
	//push dword [esp-8]		;push dimension count
	//push edi					;push dim array
	//push ebx
	//call GenerateArrayIndirectionVectors
	//add esp, 4*4
	IA32_Lea_Reg_DispRegMult(jit, REG_EBX, REG_EAX, REG_EBP, NOSCALE);
	IA32_Push_Rm_Disp8_ESP(jit, 8);
	IA32_Push_Rm_Disp8_ESP(jit, 8);
	IA32_Push_Reg(jit, REG_EDI);
	IA32_Push_Reg(jit, REG_EBX);
	IA32_Write_Jump32_Abs(jit, IA32_Call_Imm32(jit, 0), (void *)&GenerateArrayIndirectionVectors);
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4*4, MOD_REG);

	/* Store the heap pointer back into the stack */
	//pop eax					;restore heap pointer
	//pop ecx					;restore param count
	//lea edi, [edi+ecx*4-4]	;pop params-4 off the stack
	//mov [edi], eax			;store back the heap pointer
	IA32_Pop_Reg(jit, REG_EAX);
	IA32_Pop_Reg(jit, REG_ECX);
	IA32_Lea_Reg_DispRegMultImm8(jit, AMX_REG_STK, AMX_REG_STK, REG_ECX, SCALE4, -4);
	IA32_Mov_Rm_Reg(jit, AMX_REG_STK, REG_EAX, MOD_MEM_REG);

	/* Return to caller */
	//pop edx
	//pop eax
	//pop ebx
	//ret
	IA32_Pop_Reg(jit, REG_ECX);
	IA32_Pop_Reg(jit, REG_EAX);
	IA32_Pop_Reg(jit, REG_EBX);
	IA32_Return(jit);

	//:error
	IA32_Send_Jump32_Here(jit, err1);
	IA32_Send_Jump32_Here(jit, err2);
	Write_SetError(jit, SP_ERROR_ARRAY_TOO_BIG);
}

void WriteOp_Tracker_Push_Reg(JitWriter *jit, uint8_t reg)
{
	/* Save registers that may be damaged by the call */
	//push eax
	//push ecx
	//push edi
	//lea edi, [<reg>*4]		; we want the count in bytes not in cells
	IA32_Push_Reg(jit, AMX_REG_PRI);
	if (reg == REG_ECX)
	{
		IA32_Push_Reg(jit, AMX_REG_TMP);
	}
	IA32_Push_Reg(jit, AMX_REG_STK);
	IA32_Lea_Reg_RegMultImm32(jit, REG_EDI, reg, SCALE4, 0);

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
	
	/* Push the register into the stack and increment pCur */
	//mov edx, [eax+vm[]]
	//mov eax, [edx+pcur]
	//add [edx+pcur], 4
	//mov [eax], edi
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EDX, REG_EAX, offsetof(sp_context_t, vm[JITVARS_TRACKER]));
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, REG_EDX, offsetof(tracker_t, pCur));
	IA32_Add_Rm_Imm8_Disp8(jit, REG_EDX, 4, offsetof(tracker_t, pCur));
	IA32_Mov_Rm_Reg(jit, REG_EAX, REG_EDI, MOD_MEM_REG);

	/* Restore PRI, ALT and STK */
	//pop edi
	//pop ecx
	//pop eax
	IA32_Pop_Reg(jit, AMX_REG_STK);
	if (reg == REG_ECX)
	{
		IA32_Pop_Reg(jit, AMX_REG_TMP);
	}
	IA32_Pop_Reg(jit, AMX_REG_PRI);
}

int JIT_VerifyOrAllocateTracker(sp_context_t *ctx)
{
	tracker_t *trk = (tracker_t *)(ctx->vm[JITVARS_TRACKER]);

	if ((size_t)(trk->pCur - trk->pBase) >= trk->size)
	{
		return SP_ERROR_TRACKER_BOUNDS;
	}

	if (trk->pCur+1 - (trk->pBase + trk->size) == 0)
	{
		size_t disp = trk->size - 1;
		trk->size *= 2;
		trk->pBase = (ucell_t *)realloc(trk->pBase, trk->size * sizeof(cell_t));

		if (!trk->pBase)
		{
			return SP_ERROR_TRACKER_BOUNDS;
		}

		trk->pCur = trk->pBase + disp;
	}

	return SP_ERROR_NONE;
}

int JIT_VerifyLowBoundTracker(sp_context_t *ctx)
{
	tracker_t *trk = (tracker_t *)(ctx->vm[JITVARS_TRACKER]);

	if (trk->pCur <= trk->pBase)
	{
		return SP_ERROR_TRACKER_BOUNDS;
	}

	return SP_ERROR_NONE;
}

void AlignMe(JitWriter *jit)
{
	jitoffs_t cur_offs = jit->get_outputpos();
	jitoffs_t offset = ((cur_offs & 0xFFFFFFF0) + 16) - cur_offs;

	if (offset)
	{
		for (jit_uint32_t i=0; i<offset; i++)
		{
			jit->write_ubyte(IA32_INT3);
		}
	}
}
