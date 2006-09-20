#include <limits.h>
#include <string.h>
#include "jit_x86.h"
#include "opcode_helpers.h"
#include "x86_macros.h"

int OpAdvTable[OP_NUM_OPCODES];

jitoffs_t Write_Execute_Function(JitWriter *jit, bool never_inline)
{
	/** 
	 * The state expected by our plugin is:
	 * #define AMX_REG_PRI		REG_EAX		(done)
	   #define AMX_REG_ALT		REG_EDX		(done)
	   #define AMX_REG_STK		REG_EBP		(done)
	   #define AMX_REG_DAT		REG_EDI		(done)
	   #define AMX_REG_TMP		REG_ECX		(nothing)
	   #define AMX_REG_INFO		REG_ESI		(done)
	   #define AMX_REG_FRM		REG_EBX		(done)
	   #define AMX_INFO_FRM		AMX_REG_INFO	(done)
	   #define AMX_INFO_HEAP	4			(done)
	   #define AMX_INFO_RETVAL	8			(done)
	   #define AMX_INFO_CONTEXT	12			(done)
	 *
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

	//sub esp, 4*4	- allocate info array
	//mov esi, esp	- save info pointer
	IA32_Sub_Rm_Imm8(jit, REG_ESP, 4*4, MOD_REG);
	IA32_Mov_Reg_Rm(jit, AMX_REG_INFO, REG_ESP, MOD_REG);

	//mov eax, [ebp+16]	- get result pointer
	//mov [esi+8], eax	- store into info pointer
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, REG_EBP, 16);
	IA32_Mov_Rm_Reg_Disp8(jit, REG_ESI, REG_EAX, AMX_INFO_RETVAL);

	//mov eax, [ebp+8]		- get context
	//mov [esi+12], eax		- store context into info pointer
	//mov ecx, [eax+<offs>]	- get heap pointer
	//mov [esi+4], ecx		- store heap into info pointer
	//mov edi, [eax+<offs>]	- get data pointer
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, REG_EBP, 8);
	IA32_Mov_Rm_Reg_Disp8(jit, REG_ESI, REG_EAX, AMX_INFO_CONTEXT);
	IA32_Mov_Reg_Rm_Disp8(jit, REG_ECX, REG_EAX, offsetof(sp_context_t, hp));
	IA32_Mov_Rm_Reg_Disp8(jit, REG_ESI, REG_ECX, AMX_INFO_HEAP);
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_DAT, REG_EAX, offsetof(sp_context_t, data));

	//mov ebp, [eax+<offs>]	- get stack pointer
	//add ebp, edi			- relocate to data section
	//mov ebx, ebp			- copy sp to frm
	//mov ecx, [ebp+12]		- get code index
	//add ecx, [eax+<offs>] - add code base to index
	//mov edx, [eax+<offs>]	- get alt
	//mov eax, [eax+<offs>] - get pri
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_STK, REG_EAX, offsetof(sp_context_t, sp));
	IA32_Add_Rm_Reg(jit, REG_EBP, AMX_REG_STK, AMX_REG_DAT);
	IA32_Mov_Reg_Rm(jit, AMX_REG_FRM, AMX_REG_STK, MOD_REG);
	IA32_Mov_Reg_Rm_Disp8(jit, REG_ECX, REG_EBP, 12);
	IA32_Add_Reg_Rm_Disp8(jit, REG_ECX, REG_EAX, offsetof(sp_context_t, base));
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_ALT, REG_EAX, offsetof(sp_context_t, alt));
	IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_PRI, REG_EAX, offsetof(sp_context_t, pri));

	/* by now, everything is set up, so we can call into the plugin */

	//call ecx
	IA32_Call_Rm(jit, REG_ECX);

	/* if the code flow gets to here, there was a normal return */
	//mov ebp, [esi+8]		- get retval pointer
	//mov [ebp], eax		- store retval from PRI
	//mov eax, SP_ERR_NONE	- set no error
	IA32_Mov_Reg_Rm_Disp8(jit, REG_EBP, AMX_REG_INFO, AMX_INFO_RETVAL);
	IA32_Mov_Rm_Reg(jit, REG_EBP, AMX_REG_PRI, MOD_MEM_REG);
	IA32_Mov_Reg_Imm32(jit, REG_EAX, SP_ERR_NONE);

	/* where error checking/return functions should go to */
	jitoffs_t offs_return;
	if (never_inline)
	{
		/* We have to write code assume we're breaking out of a call */
		//jmp [past the next instruction]
		//add esp, 4
		jitoffs_t offs = IA32_Jump_Imm8(jit, 0);
		offs_return = jit->jit_curpos();
		IA32_Sub_Rm_Imm8(jit, REG_ESP, 4, MOD_REG);
		IA32_Send_Jump8_Here(jit, offs);
	} else {
		offs_return = jit->jit_curpos();
	}

	/* _FOR NOW_ ... 
	 * We are _not_ going to restore anything that was on the stack.
	 * This is a tiny, useless optimization based on the fact that 
	 * BaseContext::Execute() automatically restores our values anyway.
	 */

	//add esp, 4*4
	//pop ebx
	//pop edi
	//pop esi
	//pop ebp
	//ret
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4*4, MOD_REG);
	IA32_Pop_Reg(jit, REG_EBX);
	IA32_Pop_Reg(jit, REG_EDI);
	IA32_Pop_Reg(jit, REG_ESI);
	IA32_Pop_Reg(jit, REG_EBP);
	IA32_Return(jit);

	return offs_return;
}

void Macro_PushN_Addr(JitWriter *jit, int i)
{
	//push eax
	//mov eax, frm
	//loop i times:
	// lea ecx, [eax+<val>]
	// mov [ebp-4*i], ecx
	//sub ebp, 4*N
	//pop eax

	cell_t val;
	int n = 1;
	IA32_Push_Reg(jit, AMX_REG_PRI);
	IA32_Mov_Reg_Rm(jit, AMX_REG_PRI, AMX_INFO_FRM, MOD_MEM_REG);
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
	// mov [ebp-4*i], ecx
	//sub ebp, 4*N

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
	// mov [ebp-4*i], <val>
	//sub ebp, 4*N

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
	// mov ecx, [edi+<val>]
	// mov [ebp-4*i], ecx
	//sub ebp, 4*N

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
	OpAdvTable[OP_LODB_I] = sizeof(cell_t);
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
	OpAdvTable[OP_STRB_I] = sizeof(cell_t);
	OpAdvTable[OP_LIDX_B] = sizeof(cell_t);
	OpAdvTable[OP_IDXADDR_B] = sizeof(cell_t);
	OpAdvTable[OP_ALIGN_PRI] = sizeof(cell_t);
	OpAdvTable[OP_ALIGN_ALT] = sizeof(cell_t);
	OpAdvTable[OP_LCTRL] = sizeof(cell_t);
	OpAdvTable[OP_SCTRL] = sizeof(cell_t);
	OpAdvTable[OP_PUSH_C] = sizeof(cell_t);
	OpAdvTable[OP_PUSH] = sizeof(cell_t);
	OpAdvTable[OP_PUSH_S] = sizeof(cell_t);
	OpAdvTable[OP_STACK] = sizeof(cell_t);
	OpAdvTable[OP_HEAP] = sizeof(cell_t);
	OpAdvTable[OP_JREL] = sizeof(cell_t);
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
	OpAdvTable[OP_CMPS] = sizeof(cell_t);
	OpAdvTable[OP_FILL] = sizeof(cell_t);
	OpAdvTable[OP_HALT] = sizeof(cell_t);
	OpAdvTable[OP_BOUNDS] = sizeof(cell_t);
	OpAdvTable[OP_PUSH_ADR] = sizeof(cell_t);
	OpAdvTable[OP_PUSH_HEAP_C] = sizeof(cell_t);
	OpAdvTable[OP_SYSREQ_C] = sizeof(cell_t);

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
	OpAdvTable[OP_UMUL] = 0;
	OpAdvTable[OP_UDIV] = 0;
	OpAdvTable[OP_UDIV_ALT] = 0;
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
	OpAdvTable[OP_LESS] = 0;
	OpAdvTable[OP_LEQ] = 0;
	OpAdvTable[OP_GRTR] = 0;
	OpAdvTable[OP_GEQ] = 0;
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

	/* opcodes that need relocation */
	OpAdvTable[OP_CALL] = -2;
	OpAdvTable[OP_JUMP] = -2;
	OpAdvTable[OP_JZER] = -2;
	OpAdvTable[OP_JNZ] = -2;
	OpAdvTable[OP_JEQ] = -2;
	OpAdvTable[OP_JNEQ] = -2;
	OpAdvTable[OP_JLESS] = -2;
	OpAdvTable[OP_JLEQ] = -2;
	OpAdvTable[OP_JGRTR] = -2;
	OpAdvTable[OP_JGEQ] = -2;
	OpAdvTable[OP_JSLESS] = -2;
	OpAdvTable[OP_JSLEQ] = -2;
	OpAdvTable[OP_JSGRTR] = -2;
	OpAdvTable[OP_JSGEQ] = -2;
	OpAdvTable[OP_SWITCH] = -2;

	/* opcodes that are totally invalid */
	OpAdvTable[OP_FILE] = -3;
	OpAdvTable[OP_SYMBOL] = -3;
	OpAdvTable[OP_LINE] = -3;
	OpAdvTable[OP_SRANGE] = -3;
	OpAdvTable[OP_SYMTAG] = -3;
	OpAdvTable[OP_SYSREQ_D] = -3;
	OpAdvTable[OP_SYSREQ_ND] = -3;
	OpAdvTable[OP_PUSH_R] = -3;
}

