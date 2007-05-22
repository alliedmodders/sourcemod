/**
 * vim: set ts=4 :
* ================================================================
* SourcePawn (C)2004-2007 AlliedModders LLC.  All rights reserved.
* ================================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#include <limits.h>
#include <string.h>
#include <malloc.h>
#include "jit_x86.h"
#include "opcode_helpers.h"
#include "x86_macros.h"

#define NUM_INFO_PARAMS	5

jitoffs_t Write_Execute_Function(JitWriter *jit)
{
	/** 
	 * The variables we're passed in:
	 *  sp_context_t *ctx, uint32_t code_idx, cell_t *result
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

	//sub esp, 4*n	- reserve info array
	//mov esi, esp	- save info pointer
	IA32_Sub_Rm_Imm8(jit, REG_ESP, 4*NUM_INFO_PARAMS, MOD_REG);
	IA32_Mov_Reg_Rm(jit, AMX_REG_INFO, REG_ESP, MOD_REG);

	/* Initial memory setup */
	//mov eax, [ebp+16]		- get result pointer
	//mov [esi+8], eax		- store into info pointer
	//mov eax, [ebp+8]		- get context
	//mov [esi+12], eax		- store context into info pointer
	//mov ecx, [eax+<offs>]	- get heap pointer
	//mov [esi+4], ecx		- store heap into info pointer
	//mov ebp, [eax+<offs>]	- get data pointer
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, REG_EBP, 16);
	IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_INFO, REG_EAX, AMX_INFO_RETVAL);
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, REG_EBP, 8);
	IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_INFO, REG_EAX, AMX_INFO_CONTEXT);
	IA32_Mov_Reg_Rm_Disp8(jit, REG_ECX, REG_EAX, offsetof(sp_context_t, hp));
	IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_INFO, REG_ECX, AMX_INFO_HEAP);
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_DAT, REG_EAX, offsetof(sp_context_t, memory));

	/* Frame setup */
	//mov edi, [eax+<offs>]	- get stack pointer
	//add edi, ebp			- relocate to data section
	//mov ebx, edi			- copy sp to frm
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_STK, REG_EAX, offsetof(sp_context_t, sp));
	IA32_Add_Rm_Reg(jit, AMX_REG_STK, AMX_REG_DAT, MOD_REG);
	IA32_Mov_Reg_Rm(jit, AMX_REG_FRM, AMX_REG_STK, MOD_REG);

	/* Info memory setup */
	//mov ecx, [eax+<offs>] - copy memsize to temp var
	//add ecx, ebp			- relocate
	//mov [esi+x], ecx		- store relocated
	IA32_Mov_Reg_Rm_Disp8(jit, REG_ECX, REG_EAX, offsetof(sp_context_t, mem_size));
	IA32_Add_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_DAT, MOD_REG);
	IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_INFO, REG_ECX, AMX_INFO_STACKTOP);

	/* Remaining needed vars */
	//mov ecx, [esp+(4*(NUM_INFO_PARAMS+3))+12] - get code index (normally esp+12, but we have another array on the stack)
	//add ecx, [eax+<offs>]   - add code base to index
	IA32_Mov_Reg_Esp_Disp8(jit, REG_ECX, 12+(4*(NUM_INFO_PARAMS+3)));
	IA32_Add_Reg_Rm_Disp8(jit, REG_ECX, REG_EAX, offsetof(sp_context_t, codebase));

	/* by now, everything is set up, so we can call into the plugin */
	//call ecx
	IA32_Call_Reg(jit, REG_ECX);

	/* if the code flow gets to here, there was a normal return */
	//mov ecx, [esi+8]		- get retval pointer
	//mov [ecx], eax		- store retval from PRI
	//mov eax, SP_ERROR_NONE	- set no error
	IA32_Mov_Reg_Rm_Disp8(jit, REG_ECX, AMX_REG_INFO, AMX_INFO_RETVAL);
	IA32_Mov_Rm_Reg(jit, REG_ECX, AMX_REG_PRI, MOD_MEM_REG);
	IA32_Mov_Reg_Imm32(jit, REG_EAX, SP_ERROR_NONE);

	/* save where error checking/halting functions should go to */
	jitoffs_t offs_return = jit->get_outputpos();
	//mov esp, esi			- restore stack pointer
	IA32_Mov_Reg_Rm(jit, REG_ESP, REG_ESI, MOD_REG);

	/* _FOR NOW_ ...
	 * We are going to restore SP, HP, and FRM for now.  This is for 
     * debugging only, to check for alignment errors.  As such:
	 * :TODO: probably remove this.
	 */
	//mov ecx, [esi+context]
	//sub edi, ebp
	//mov edx, [esi+heap]
	//mov [ecx+sp], edi
	//mov [ecx+hp], edx
	IA32_Mov_Reg_Rm_Disp8(jit, REG_ECX, REG_ESI, AMX_INFO_CONTEXT);
	IA32_Sub_Reg_Rm(jit, REG_EDI, REG_EBP, MOD_REG);
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EDX, REG_ESI, AMX_INFO_HEAP);
	IA32_Mov_Rm_Reg_Disp8(jit, REG_ECX, REG_EDI, offsetof(sp_context_t, sp));
	IA32_Mov_Rm_Reg_Disp8(jit, REG_ECX, REG_EDX, offsetof(sp_context_t, hp));

	//add esp, 4*NUM_INFO_PARAMS
	//pop ebx
	//pop edi
	//pop esi
	//pop ebp
	//ret
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4*NUM_INFO_PARAMS, MOD_REG);
	IA32_Pop_Reg(jit, REG_EBX);
	IA32_Pop_Reg(jit, REG_EDI);
	IA32_Pop_Reg(jit, REG_ESI);
	IA32_Pop_Reg(jit, REG_EBP);
	IA32_Return(jit);

	return offs_return;
}

void Write_BreakDebug(JitWriter *jit)
{
	//push edi
	//mov edi, ecx
	//mov ecx, [esi+ctx]
	//cmp [ecx+dbreak], 0
	//jnz :nocall
	IA32_Push_Reg(jit, REG_EDI);
	IA32_Mov_Reg_Rm(jit, REG_EDI, REG_ECX, MOD_REG);
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_CONTEXT);
	IA32_Cmp_Rm_Disp8_Imm8(jit, AMX_REG_TMP, offsetof(sp_context_t, dbreak), 0);
	jitoffs_t jmp = IA32_Jump_Cond_Imm8(jit, CC_Z, 0);

	/* NOTE, Hack! PUSHAD pushes EDI last which still has the CIP */
	//pushad
	//push [esi+frm]
	//push ctx
	//mov ecx, [ecx+dbreak]
	//call ecx
	//add esp, 8
	//popad
	IA32_Pushad(jit);
	IA32_Push_Rm_Disp8(jit, AMX_REG_INFO, AMX_INFO_FRAME); //:TODO: move to regs and push? and dont disp for 0
	IA32_Push_Reg(jit, AMX_REG_TMP);
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_TMP, offsetof(sp_context_t, dbreak));
	IA32_Call_Reg(jit, AMX_REG_TMP);
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4*2, MOD_REG);
	IA32_Popad(jit);

	//:nocall
	//pop edi
	//ret
	IA32_Pop_Reg(jit, REG_EDI);
	IA32_Send_Jump8_Here(jit, jmp);
	IA32_Return(jit);
}

void Write_GetError(JitWriter *jit)
{
	CompData *data = (CompData *)jit->data;

	//mov eax, [esi+info.context]
	//mov eax, [eax+ctx.error]
	//jmp [jit_return]
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, AMX_REG_INFO, AMX_INFO_CONTEXT);
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, REG_EAX, offsetof(sp_context_t, n_err));
	IA32_Jump_Imm32_Abs(jit, data->jit_return);
}

void Write_SetError(JitWriter *jit, int error)
{
	CompData *data = (CompData *)jit->data;

	//mov eax, <error>
	//jmp [jit_return]
	IA32_Mov_Reg_Imm32(jit, REG_EAX, error);
	IA32_Jump_Imm32_Abs(jit, data->jit_return);
}

void Write_Check_DivZero(JitWriter *jit, jit_uint8_t reg)
{
	//test reg, reg
	//jz :error
	IA32_Test_Rm_Reg(jit, reg, reg, MOD_REG);
	IA32_Jump_Cond_Imm32_Abs(jit, CC_Z, ((CompData *)jit->data)->jit_error_divzero);
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
	IA32_Jump_Cond_Imm32_Abs(jit, CC_B, data->jit_error_heapmin);
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
	IA32_Jump_Cond_Imm32_Abs(jit, CC_A, ((CompData *)jit->data)->jit_error_heaplow);
}

void Write_CheckStack_Min(JitWriter *jit)
{
	/* Check if the stack went beyond the stack top
	 * This usually means there was a compiler error.
	 */
	//cmp edi, [esi+info.stacktop]
	//jae :error
	IA32_Cmp_Reg_Rm_Disp8(jit, AMX_REG_STK, AMX_REG_INFO, AMX_INFO_STACKTOP);
	IA32_Jump_Cond_Imm32_Abs(jit, CC_AE, ((CompData *)jit->data)->jit_error_stackmin);
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
	IA32_Jump_Cond_Imm32_Abs(jit, CC_B, ((CompData *)jit->data)->jit_error_stacklow);
}

void Write_Check_VerifyAddr(JitWriter *jit, jit_uint8_t reg)
{
	CompData *data = (CompData *)jit->data;

	/* :TODO: Should this be checking for below heaplow?
	 * The old JIT did not.
	 */

	bool call = false;
	if (!(data->inline_level & JIT_INLINE_ERRORCHECKS))
	{
		/* If we're not in the initial generation phase,
		 * Write a call to the actual routine instead.
		 */
		if ((reg == REG_EAX) && data->jit_verify_addr_eax)
		{
			jitoffs_t call = IA32_Call_Imm32(jit, 0);
			IA32_Write_Jump32(jit, call, data->jit_verify_addr_eax);
			return;
		} else if ((reg == REG_EDX) && data->jit_verify_addr_edx) {
			jitoffs_t call = IA32_Call_Imm32(jit, 0);
			IA32_Write_Jump32(jit, call, data->jit_verify_addr_edx);
			return;
		}
		call = true;
	}

	/** 
	 * :TODO: If we can't find a nicer way of doing this,
	 *  then scrap it on high optimizations.  The second portion is not needed at all!
	 */

	/* Part 1: Check if we're in the memory bounds */
	//cmp <reg>, <stpu>
	//jae :error
	IA32_Cmp_Rm_Imm32(jit, MOD_REG, reg, ((CompData *)jit->data)->plugin->memory);
	IA32_Jump_Cond_Imm32_Abs(jit, CC_AE, ((CompData *)jit->data)->jit_error_memaccess);

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
	IA32_Jump_Cond_Imm32_Abs(jit, CC_B, ((CompData *)jit->data)->jit_error_memaccess);
	IA32_Send_Jump8_Here(jit, jmp);

	if (call)
	{
		IA32_Return(jit);
	}
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
		if (val < SCHAR_MAX && val > SCHAR_MIN)
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
		if (val < SCHAR_MAX && val > SCHAR_MIN)
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
		if (val < SCHAR_MAX && val > SCHAR_MIN)
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
	if (!data->debug)
	{
		IA32_Write_Jump32_Abs(jit, call, (void *)NativeCallback);
	} else {
		IA32_Write_Jump32_Abs(jit, call, (void *)NativeCallback_Debug);
	}

	/* Test for error */
	//mov ecx, [esi+context]
	//cmp [ecx+err], 0
	//jnz :error
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_CONTEXT);
	IA32_Cmp_Rm_Disp8_Imm8(jit, AMX_REG_TMP, offsetof(sp_context_t, n_err), 0);
	IA32_Jump_Cond_Imm32_Abs(jit, CC_NZ, data->jit_extern_error);

	/* restore what we damaged */
	//add esp, 4*3
	//add edi, ebp
	//pop edx
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4*3, MOD_REG);
	IA32_Add_Reg_Rm(jit, AMX_REG_STK, AMX_REG_DAT, MOD_REG);
	IA32_Pop_Reg(jit, AMX_REG_ALT);

	//ret
	IA32_Return(jit);
}

void GenerateArrayIndirectionVectors(cell_t *arraybase, cell_t dims[], ucell_t _dimcount, bool autozero)
{
	cell_t vectors = 1;				/* we need one vector to start off with */
	cell_t cur_offs = 0;
	cell_t cur_write = 0;
	cell_t dimcount = _dimcount;

	/* Initialize rotation */
	cur_write = dims[dimcount-1];
	
	while (--dimcount >= 1)
	{
		cell_t cur_dim = dims[dimcount];
		cell_t sub_dim = dims[dimcount-1];
		for (cell_t i=0; i<vectors; i++)
		{
			for (cell_t j=0; j<cur_dim; j++)
			{
				arraybase[cur_offs] = (cur_write - cur_offs)*sizeof(cell_t);
				cur_offs++;
				cur_write += sub_dim;
			}
		}
		vectors = cur_dim;
	}

	/* everything after cur_offs can be zeroed */
	if (autozero)
	{
		size_t size = 1;
		for (ucell_t i=0; i<_dimcount; i++)
		{
			size *= dims[i];
		}
		memset(&arraybase[cur_offs], 0, size*sizeof(cell_t));
	}

	return;
}

/**
 * A few notes about this function.
 * I was more concerned about efficient use of registers here, rather than 
 *  fine-tuned optimization.  The reason is that the code is already complicated,
 *  and it is very easy to mess up.
 */
void WriteIntrinsic_GenArray(JitWriter *jit)
{
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
	//cmp eax, <hlw>			;compare to heap low
	//jbe :error				;die if we hit this (it should always be >)
	//add eax, ebp				;relocate to stack
	//cmp eax, edi				;die if above the stack pointer
	//jae :error
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, AMX_REG_INFO, AMX_INFO_HEAP);
	IA32_Lea_Reg_DispRegMult(jit, REG_EAX, REG_EAX, REG_EDX, SCALE4);
	IA32_Cmp_Rm_Imm32(jit, MOD_REG, REG_EAX, ((CompData *)jit->data)->plugin->data_size);
	IA32_Jump_Cond_Imm32_Abs(jit, CC_BE, ((CompData *)jit->data)->jit_error_array_too_big);
	IA32_Add_Reg_Rm(jit, REG_EAX, AMX_REG_DAT, MOD_REG);
	IA32_Cmp_Reg_Rm(jit, REG_EAX, AMX_REG_STK, MOD_REG);
	IA32_Jump_Cond_Imm32_Abs(jit, CC_AE, ((CompData *)jit->data)->jit_error_array_too_big);
	
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
}

void WriteOp_Sysreq_N_Function(JitWriter *jit)
{
	/* The big daddy of opcodes.
	 * eax - num_params
	 * ecx - native index
	 */
	CompData *data = (CompData *)jit->data;

	/* store the number of parameters on the stack */
	//mov [edi-4], eax
	//sub edi, 4
	IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_STK, REG_EAX, -4);
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);

	/* save registers we will need */
	//push eax	; num_params for stack popping
	//push edx
	IA32_Push_Reg(jit, REG_EAX);
	IA32_Push_Reg(jit, AMX_REG_ALT);

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
	if (!data->debug)
	{
		IA32_Write_Jump32_Abs(jit, call, (void *)NativeCallback);
	} else {
		IA32_Write_Jump32_Abs(jit, call, (void *)NativeCallback_Debug);
	}

	/* Test for error */
	//mov ecx, [esi+context]
	//cmp [ecx+err], 0
	//jnz :error
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_CONTEXT);
	IA32_Cmp_Rm_Disp8_Imm8(jit, AMX_REG_TMP, offsetof(sp_context_t, n_err), 0);
	IA32_Jump_Cond_Imm32_Abs(jit, CC_NZ, data->jit_extern_error);

	/* restore what we damaged */
	//add esp, 4*3
	//add edi, ebp
	//pop edx
	//pop ecx	; num_params
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4*3, MOD_REG);
	IA32_Add_Reg_Rm(jit, AMX_REG_STK, AMX_REG_DAT, MOD_REG);
	IA32_Pop_Reg(jit, AMX_REG_ALT);
	IA32_Pop_Reg(jit, REG_ECX);

	/* pop the AMX stack.  do not check the margins.
	 * Note that this is not a true macro - we don't bother to
	 * set ALT here because nothing will be using it.
	 */
	//lea edi, [edi+ecx*4+4]
	IA32_Lea_Reg_DispRegMultImm8(jit, AMX_REG_STK, AMX_REG_STK, REG_ECX, SCALE4, 4);

	//ret
	IA32_Return(jit);
}

void WriteOp_Tracker_Push_Reg(JitWriter *jit, uint8_t reg)
{
	CompData *data = (CompData *)jit->data;

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
	IA32_Jump_Cond_Imm32_Abs(jit, CC_NZ, data->jit_return);

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

void Write_RoundingTable(JitWriter *jit)
{
	jit->write_int32(-1);
	jit->write_int32(0);
	jit->write_int32(1);
}
