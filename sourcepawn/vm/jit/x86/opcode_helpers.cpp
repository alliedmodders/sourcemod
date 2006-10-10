#include <limits.h>
#include <string.h>
#include "jit_x86.h"
#include "opcode_helpers.h"
#include "x86_macros.h"

int OpAdvTable[OP_NUM_OPCODES];

#define NUM_INFO_PARAMS	7

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
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_DAT, REG_EAX, offsetof(sp_context_t, data));

	/* Frame setup */
	//mov edi, [eax+<offs>]	- get stack pointer
	//add edi, ebp			- relocate to data section
	//mov ebx, edi			- copy sp to frm
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_STK, REG_EAX, offsetof(sp_context_t, sp));
	IA32_Add_Rm_Reg(jit, AMX_REG_STK, AMX_REG_DAT, MOD_REG);
	IA32_Mov_Reg_Rm(jit, AMX_REG_FRM, AMX_REG_STK, MOD_REG);

	/* Info memory setup */
	//mov ecx, [eax+<offs>] - copy memsize to temp var
	//mov [esi+x], ecx		- store unrelocated
	//add ecx, ebp			- relocate
	//mov [esi+x], ecx		- store relocated
	//mov ecx, [eax+<offs>] - get heap low
	//mov [esi+20], ecx		- store heap low into info pointer
	IA32_Mov_Reg_Rm_Disp8(jit, REG_ECX, REG_EAX, offsetof(sp_context_t, memory));
	IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_INFO, REG_ECX, AMX_INFO_STACKTOP_U);
	IA32_Add_Reg_Rm(jit, AMX_REG_TMP, AMX_REG_DAT, MOD_REG);
	IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_INFO, REG_ECX, AMX_INFO_STACKTOP);
	IA32_Mov_Reg_Rm_Disp8(jit, REG_ECX, REG_EAX, offsetof(sp_context_t, heapbase));
	IA32_Mov_Rm_Reg_Disp8(jit, AMX_REG_INFO, REG_ECX, AMX_INFO_HEAPLOW);

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
	//mov eax, SP_ERR_NONE	- set no error
	IA32_Mov_Reg_Rm_Disp8(jit, REG_ECX, AMX_REG_INFO, AMX_INFO_RETVAL);
	IA32_Mov_Rm_Reg(jit, REG_ECX, AMX_REG_PRI, MOD_MEM_REG);
	IA32_Mov_Reg_Imm32(jit, REG_EAX, SP_ERR_NONE);

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
	IA32_Mov_Rm_Reg(jit, REG_ECX, REG_EDX, MOD_REG);

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
	//push ecx
	//mov ecx, [esi+ctx]
	//cmp [ecx+dbreak], 0
	//jnz :nocall
	IA32_Push_Reg(jit, AMX_REG_TMP);
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_CONTEXT);
	IA32_Cmp_Rm_Disp8_Imm8(jit, AMX_REG_TMP, offsetof(sp_context_t, dbreak), 0);
	jitoffs_t jmp = IA32_Jump_Cond_Imm8(jit, CC_NZ, 0);

	//pushad
	IA32_Pushad(jit);

	//push [esi+frm]
	//push [ecx+context]
	//mov ecx, [ecx+dbreak]
	//call ecx
	//add esp, 8
	//popad
	IA32_Push_Rm_Disp8(jit, AMX_REG_INFO, AMX_INFO_FRAME); //:TODO: move to regs and push? and dont disp for 0
	IA32_Push_Rm_Disp8(jit, AMX_REG_TMP, offsetof(sp_context_t, context));
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_TMP, offsetof(sp_context_t, dbreak));
	IA32_Call_Reg(jit, AMX_REG_TMP);
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4*2, MOD_REG);
	IA32_Popad(jit);

	//:nocall
	IA32_Send_Jump8_Here(jit, jmp);
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4*1, MOD_REG);
	IA32_Return(jit);
}

void Write_SetError(JitWriter *jit, bool always_inline, int error)
{
	CompData *data = (CompData *)jit->data;

	/* These are so small that we always inline them! */
	//mov eax, <error>
	//jmp [...jit_return]
	IA32_Mov_Reg_Imm32(jit, REG_EAX, error);
	jitoffs_t jmp = IA32_Jump_Imm32(jit, 0);
	IA32_Write_Jump32(jit, jmp, data->jit_return);
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
	//cmp <reg>, [esi+info.stpu]
	//jae :error
	IA32_Cmp_Reg_Rm_Disp8(jit, reg, AMX_REG_INFO, AMX_INFO_STACKTOP_U);
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
	//:TODO: fix the case of this size > imm8!
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
	//sub edi, ebp
	//mov ecx, [esi+hea]
	//mov eax, [esi+context]
	//mov [eax+hp], ecx
	//mov [eax+sp], edi
	//mov ecx, [esi+frm]
	//mov [eax+frm], ecx
	IA32_Sub_Rm_Reg(jit, AMX_REG_STK, AMX_REG_DAT, MOD_REG);
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_INFO, AMX_INFO_HEAP);
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, AMX_REG_INFO, AMX_INFO_CONTEXT);
	IA32_Mov_Rm_Reg_Disp8(jit, REG_EAX, AMX_REG_TMP, offsetof(sp_context_t, hp));
	IA32_Mov_Rm_Reg_Disp8(jit, REG_EAX, AMX_REG_STK, offsetof(sp_context_t, sp));
	IA32_Mov_Reg_Rm(jit, AMX_REG_TMP, AMX_INFO_FRM, MOD_REG);
	IA32_Mov_Rm_Reg_Disp8(jit, REG_EAX, AMX_REG_TMP, offsetof(sp_context_t, frm));

	/* finally, push the last parameter and make the call */
	//push eax		; context
	//mov eax, [eax+context]
	//call NativeCallback
	IA32_Push_Reg(jit, REG_EAX);
	jitoffs_t call = IA32_Call_Imm32(jit, 0);
	IA32_Write_Jump32(jit, call, (jitoffs_t)(char *)&NativeCallback);

	/* restore what we damaged */
	//add esp, 4*3
	//add edi, ebp
	//pop edx
	//pop ecx	; num_params
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4*3, MOD_REG);
	IA32_Add_Rm_Reg(jit, AMX_REG_STK, AMX_REG_DAT, MOD_REG);
	IA32_Pop_Reg(jit, AMX_REG_ALT);
	IA32_Pop_Reg(jit, REG_ECX);

	//Note: always safe, we're in a call
	//test eax, eax
	//jne :error
	IA32_Test_Rm_Reg(jit, REG_EAX, REG_EAX, MOD_REG);
	IA32_Jump_Cond_Imm32_Abs(jit, CC_NE, data->jit_return);

	/* pop the AMX stack.  do not check the margins.
	 * Note that this is not a true macro - we don't bother to
	 * set ALT here because nothing will be using it.
	 */
	//lea edi, [edi+ecx*4+4]
	IA32_Lea_Reg_DispRegMultImm8(jit, AMX_REG_STK, AMX_REG_STK, REG_ECX, SCALE4, 4);

	//ret
	IA32_Return(jit);
}

JITX86::JITX86()
{
	memset(OpAdvTable, -1, sizeof(OpAdvTable));

	/* instructions with 5 parameters */
	OpAdvTable[OP_PUSH5_C] = sizeof(cell_t)*5;
	OpAdvTable[OP_PUSH5] = sizeof(cell_t)*5;
	OpAdvTable[OP_PUSH5_S] = sizeof(cell_t)*5;
	OpAdvTable[OP_PUSH5_ADR] = sizeof(cell_t)*5;

	/* instructions with 4 parameters */
	OpAdvTable[OP_PUSH4_C] = sizeof(cell_t)*4;
	OpAdvTable[OP_PUSH4] = sizeof(cell_t)*4;
	OpAdvTable[OP_PUSH4_S] = sizeof(cell_t)*4;
	OpAdvTable[OP_PUSH4_ADR] = sizeof(cell_t)*4;

	/* instructions with 3 parameters */
	OpAdvTable[OP_PUSH3_C] = sizeof(cell_t)*3;
	OpAdvTable[OP_PUSH3] = sizeof(cell_t)*3;
	OpAdvTable[OP_PUSH3_S] = sizeof(cell_t)*3;
	OpAdvTable[OP_PUSH3_ADR] = sizeof(cell_t)*3;

	/* instructions with 2 parameters */
	OpAdvTable[OP_PUSH2_C] = sizeof(cell_t)*2;
	OpAdvTable[OP_PUSH2] = sizeof(cell_t)*2;
	OpAdvTable[OP_PUSH2_S] = sizeof(cell_t)*2;
	OpAdvTable[OP_PUSH2_ADR] = sizeof(cell_t)*2;
	OpAdvTable[OP_LOAD_BOTH] = sizeof(cell_t)*2;
	OpAdvTable[OP_LOAD_S_BOTH] = sizeof(cell_t)*2;
	OpAdvTable[OP_CONST] = sizeof(cell_t)*2;
	OpAdvTable[OP_CONST_S] = sizeof(cell_t)*2;
	OpAdvTable[OP_SYSREQ_N] = sizeof(cell_t)*2;

	/* instructions with 1 parameter */
	OpAdvTable[OP_LOAD_PRI] = sizeof(cell_t);
	OpAdvTable[OP_LOAD_ALT] = sizeof(cell_t);
	OpAdvTable[OP_LOAD_S_PRI] = sizeof(cell_t);
	OpAdvTable[OP_LOAD_S_ALT] = sizeof(cell_t);
	OpAdvTable[OP_LREF_PRI] = sizeof(cell_t);
	OpAdvTable[OP_LREF_ALT] = sizeof(cell_t);
	OpAdvTable[OP_LREF_S_PRI] = sizeof(cell_t);
	OpAdvTable[OP_LREF_S_ALT] = sizeof(cell_t);
	OpAdvTable[OP_CONST_PRI] = sizeof(cell_t);
	OpAdvTable[OP_CONST_ALT] = sizeof(cell_t);
	OpAdvTable[OP_ADDR_PRI] = sizeof(cell_t);
	OpAdvTable[OP_ADDR_ALT] = sizeof(cell_t);
	OpAdvTable[OP_STOR_PRI] = sizeof(cell_t);
	OpAdvTable[OP_STOR_ALT] = sizeof(cell_t);
	OpAdvTable[OP_STOR_S_PRI] = sizeof(cell_t);
	OpAdvTable[OP_STOR_S_ALT] = sizeof(cell_t);
	OpAdvTable[OP_SREF_PRI] = sizeof(cell_t);
	OpAdvTable[OP_SREF_ALT] = sizeof(cell_t);
	OpAdvTable[OP_SREF_S_PRI] = sizeof(cell_t);
	OpAdvTable[OP_SREF_S_ALT] = sizeof(cell_t);
	OpAdvTable[OP_LIDX_B] = sizeof(cell_t);
	OpAdvTable[OP_IDXADDR_B] = sizeof(cell_t);
	OpAdvTable[OP_PUSH_C] = sizeof(cell_t);
	OpAdvTable[OP_PUSH] = sizeof(cell_t);
	OpAdvTable[OP_PUSH_S] = sizeof(cell_t);
	OpAdvTable[OP_STACK] = sizeof(cell_t);
	OpAdvTable[OP_HEAP] = sizeof(cell_t);
	OpAdvTable[OP_SHL_C_PRI] = sizeof(cell_t);
	OpAdvTable[OP_SHL_C_ALT] = sizeof(cell_t);
	OpAdvTable[OP_SHR_C_PRI] = sizeof(cell_t);
	OpAdvTable[OP_SHR_C_ALT] = sizeof(cell_t);
	OpAdvTable[OP_ADD_C] = sizeof(cell_t);
	OpAdvTable[OP_SMUL_C] = sizeof(cell_t);
	OpAdvTable[OP_ZERO] = sizeof(cell_t);
	OpAdvTable[OP_ZERO_S] = sizeof(cell_t);
	OpAdvTable[OP_EQ_C_PRI] = sizeof(cell_t);
	OpAdvTable[OP_EQ_C_ALT] = sizeof(cell_t);
	OpAdvTable[OP_INC] = sizeof(cell_t);
	OpAdvTable[OP_INC_S] = sizeof(cell_t);
	OpAdvTable[OP_DEC] = sizeof(cell_t);
	OpAdvTable[OP_DEC_S] = sizeof(cell_t);
	OpAdvTable[OP_MOVS] = sizeof(cell_t);
	OpAdvTable[OP_FILL] = sizeof(cell_t);
	OpAdvTable[OP_HALT] = sizeof(cell_t);
	OpAdvTable[OP_BOUNDS] = sizeof(cell_t);
	OpAdvTable[OP_PUSH_ADR] = sizeof(cell_t);
	OpAdvTable[OP_PUSH_HEAP_C] = sizeof(cell_t);
	OpAdvTable[OP_SYSREQ_C] = sizeof(cell_t);
	OpAdvTable[OP_CALL] = sizeof(cell_t);
	OpAdvTable[OP_JUMP] = sizeof(cell_t);
	OpAdvTable[OP_JZER] = sizeof(cell_t);
	OpAdvTable[OP_JNZ] = sizeof(cell_t);
	OpAdvTable[OP_JEQ] = sizeof(cell_t);
	OpAdvTable[OP_JNEQ] = sizeof(cell_t);
	OpAdvTable[OP_JSLESS] = sizeof(cell_t);
	OpAdvTable[OP_JSLEQ] = sizeof(cell_t);
	OpAdvTable[OP_JSGRTR] = sizeof(cell_t);
	OpAdvTable[OP_JSGEQ] = sizeof(cell_t);
	OpAdvTable[OP_SWITCH] = sizeof(cell_t);

	/* instructions with 0 parameters */
	OpAdvTable[OP_LOAD_I] = 0;
	OpAdvTable[OP_STOR_I] = 0;
	OpAdvTable[OP_LIDX] = 0;
	OpAdvTable[OP_IDXADDR] = 0;
	OpAdvTable[OP_MOVE_PRI] = 0;
	OpAdvTable[OP_MOVE_ALT] = 0;
	OpAdvTable[OP_XCHG] = 0;
	OpAdvTable[OP_PUSH_PRI] = 0;
	OpAdvTable[OP_PUSH_ALT] = 0;
	OpAdvTable[OP_POP_PRI] = 0;
	OpAdvTable[OP_POP_ALT] = 0;
	OpAdvTable[OP_PROC] = 0;
	OpAdvTable[OP_RET] = 0;
	OpAdvTable[OP_RETN] = 0;
	OpAdvTable[OP_CALL_PRI] = 0;
	OpAdvTable[OP_SHL] = 0;
	OpAdvTable[OP_SHR] = 0;
	OpAdvTable[OP_SSHR] = 0;
	OpAdvTable[OP_SMUL] = 0;
	OpAdvTable[OP_SDIV] = 0;
	OpAdvTable[OP_SDIV_ALT] = 0;
	OpAdvTable[OP_ADD] = 0;
	OpAdvTable[OP_SUB] = 0;
	OpAdvTable[OP_SUB_ALT] = 0;
	OpAdvTable[OP_AND] = 0;
	OpAdvTable[OP_OR] = 0;
	OpAdvTable[OP_XOR] = 0;
	OpAdvTable[OP_NOT] = 0;
	OpAdvTable[OP_NEG] = 0;
	OpAdvTable[OP_INVERT] = 0;
	OpAdvTable[OP_ZERO_PRI] = 0;
	OpAdvTable[OP_ZERO_ALT] = 0;
	OpAdvTable[OP_SIGN_PRI] = 0;
	OpAdvTable[OP_SIGN_ALT] = 0;
	OpAdvTable[OP_EQ] = 0;
	OpAdvTable[OP_NEQ] = 0;
	OpAdvTable[OP_SLESS] = 0;
	OpAdvTable[OP_SLEQ] = 0;
	OpAdvTable[OP_SGRTR] = 0;
	OpAdvTable[OP_SGEQ] = 0;
	OpAdvTable[OP_INC_PRI] = 0;
	OpAdvTable[OP_INC_ALT] = 0;
	OpAdvTable[OP_INC_I] = 0;
	OpAdvTable[OP_DEC_PRI] = 0;
	OpAdvTable[OP_DEC_ALT] = 0;
	OpAdvTable[OP_DEC_I] = 0;
	OpAdvTable[OP_JUMP_PRI] = 0;
	OpAdvTable[OP_SWAP_PRI] = 0;
	OpAdvTable[OP_SWAP_ALT] = 0;
	OpAdvTable[OP_NOP] = 0;
	OpAdvTable[OP_BREAK] = 0;
	OpAdvTable[OP_HEAP_PRI] = 0;
	OpAdvTable[OP_POP_HEAP_PRI] = 0;
	OpAdvTable[OP_SYSREQ_PRI] = 0;

	/* opcodes that are totally invalid */
	/* :TODO: make an alternate table if USE_UNGEN_OPCODES is on? */
	OpAdvTable[OP_FILE] = -3;
	OpAdvTable[OP_SYMBOL] = -3;
	OpAdvTable[OP_LINE] = -3;
	OpAdvTable[OP_SRANGE] = -3;
	OpAdvTable[OP_SYMTAG] = -3;
	OpAdvTable[OP_SYSREQ_D] = -3;
	OpAdvTable[OP_SYSREQ_ND] = -3;
	OpAdvTable[OP_PUSH_R] = -3;
	OpAdvTable[OP_LODB_I] = -3;
	OpAdvTable[OP_STRB_I] = -3;
	OpAdvTable[OP_LCTRL] = -3;
	OpAdvTable[OP_SCTRL] = -3;
	OpAdvTable[OP_ALIGN_PRI] = -3;
	OpAdvTable[OP_ALIGN_ALT] = -3;
	OpAdvTable[OP_JREL] = -3;
	OpAdvTable[OP_CMPS] = -3;
	OpAdvTable[OP_UMUL] = -3;
	OpAdvTable[OP_UDIV] = -3;
	OpAdvTable[OP_UDIV_ALT] = -3;
	OpAdvTable[OP_LESS] = -3;
	OpAdvTable[OP_LEQ] = -3;
	OpAdvTable[OP_GRTR] = -3;
	OpAdvTable[OP_GEQ] = -3;
	OpAdvTable[OP_JLESS] = -3;
	OpAdvTable[OP_JLEQ] = -3;
	OpAdvTable[OP_JGRTR] = -3;
	OpAdvTable[OP_JGEQ] = -3;
}
