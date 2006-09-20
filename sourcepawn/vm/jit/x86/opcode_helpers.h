#ifndef _INCLUDE_SOURCEPAWN_JIT_X86_OPCODE_INFO_H_
#define _INCLUDE_SOURCEPAWN_JIT_X86_OPCODE_INFO_H_

#include "..\jit_helpers.h"

/**
 * This outputs the execution function for a plugin.
 * It also returns the 'return' offset, which is used for 
 * breaking out of the JIT during runtime.
 */
jitoffs_t Write_Execute_Function(JitWriter *jit);

/**
 * Generates code to set an error state in the VM and return.
 * Note that this should only be called from inside an error
 * checking function.
 */
void Write_Error(JitWriter *jit, int error);

/**
 * Verifies an address by register.
 */
void Write_Check_VerifyAddr(JitWriter *jit, jit_uint8_t reg, bool firstcall);

/** 
 * These are for writing the PushN opcodes.
 */
void Macro_PushN_Addr(JitWriter *jit, int i);
void Macro_PushN_S(JitWriter *jit, int i);
void Macro_PushN_C(JitWriter *jit, int i);
void Macro_PushN(JitWriter *jit, int i);

typedef enum
{
	OP_NONE,              /* invalid opcode */
	OP_LOAD_PRI,			//DONE
	OP_LOAD_ALT,			//DONE
	OP_LOAD_S_PRI,			//DONE
	OP_LOAD_S_ALT,			//DONE
	OP_LREF_PRI,			//DONE
	OP_LREF_ALT,			//DONE
	OP_LREF_S_PRI,			//DONE
	OP_LREF_S_ALT,			//DONE
	OP_LOAD_I,				//DONE
	OP_LODB_I,				//DONE
	OP_CONST_PRI,			//DONE
	OP_CONST_ALT,			//DONE
	OP_ADDR_PRI,			//DONE
	OP_ADDR_ALT,			//DONE
	OP_STOR_PRI,			//DONE
	OP_STOR_ALT,			//DONE
	OP_STOR_S_PRI,			//DONE
	OP_STOR_S_ALT,			//DONE
	OP_SREF_PRI,			//DONE
	OP_SREF_ALT,			//DONE
	OP_SREF_S_PRI,			//DONE
	OP_SREF_S_ALT,			//DONE
	OP_STOR_I,				//DONE
	OP_STRB_I,				//DONE
	OP_LIDX,				//DONE
	OP_LIDX_B,				//DONE
	OP_IDXADDR,				//DONE
	OP_IDXADDR_B,			//DONE
	OP_ALIGN_PRI,			//DONE
	OP_ALIGN_ALT,			//DONE
	OP_LCTRL,				//DONE
	OP_SCTRL,
	OP_MOVE_PRI,			//DONE
	OP_MOVE_ALT,			//DONE
	OP_XCHG,				//DONE
	OP_PUSH_PRI,			//DONE
	OP_PUSH_ALT,			//DONE
	OP_PUSH_R,				//DEPRECATED
	OP_PUSH_C,				//DONE
	OP_PUSH,				//DONE
	OP_PUSH_S,				//DONE
	OP_POP_PRI,				//DONE
	OP_POP_ALT,				//DONE
	OP_STACK,
	OP_HEAP,
	OP_PROC,				//DONE
	OP_RET,
	OP_RETN,
	OP_CALL,
	OP_CALL_PRI,
	OP_JUMP,
	OP_JREL,
	OP_JZER,
	OP_JNZ,
	OP_JEQ,
	OP_JNEQ,
	OP_JLESS,
	OP_JLEQ,
	OP_JGRTR,
	OP_JGEQ,
	OP_JSLESS,
	OP_JSLEQ,
	OP_JSGRTR,
	OP_JSGEQ,
	OP_SHL,					//DONE
	OP_SHR,					//DONE
	OP_SSHR,				//DONE
	OP_SHL_C_PRI,			//DONE
	OP_SHL_C_ALT,			//DONE
	OP_SHR_C_PRI,			//DONE
	OP_SHR_C_ALT,			//DONE
	OP_SMUL,				//DONE
	OP_SDIV,
	OP_SDIV_ALT,
	OP_UMUL,				//DONE
	OP_UDIV,
	OP_UDIV_ALT,
	OP_ADD,					//DONE
	OP_SUB,					//DONE
	OP_SUB_ALT,				//DONE
	OP_AND,					//DONE
	OP_OR,					//DONE
	OP_XOR,					//DONE
	OP_NOT,					//DONE
	OP_NEG,					//DONE
	OP_INVERT,				//DONE
	OP_ADD_C,				//DONE
	OP_SMUL_C,				//DONE
	OP_ZERO_PRI,			//DONE
	OP_ZERO_ALT,			//DONE
	OP_ZERO,				//DONE
	OP_ZERO_S,				//DONE
	OP_SIGN_PRI,			//DONE
	OP_SIGN_ALT,			//DONE
	OP_EQ,					//DONE
	OP_NEQ,					//DONE
	OP_LESS,				//DONE
	OP_LEQ,					//DONE
	OP_GRTR,				//DONE
	OP_GEQ,					//DONE
	OP_SLESS,				//DONE
	OP_SLEQ,				//DONE
	OP_SGRTR,				//DONE
	OP_SGEQ,				//DONE
	OP_EQ_C_PRI,			//DONE
	OP_EQ_C_ALT,			//DONE
	OP_INC_PRI,				//DONE
	OP_INC_ALT,				//DONE
	OP_INC,					//DONE
	OP_INC_S,				//DONE
	OP_INC_I,				//DONE
	OP_DEC_PRI,				//DONE
	OP_DEC_ALT,				//DONE
	OP_DEC,					//DONE
	OP_DEC_S,				//DONE
	OP_DEC_I,				//DONE
	OP_MOVS,				//DONE
	OP_CMPS,
	OP_FILL,				//DONE
	OP_HALT,
	OP_BOUNDS,
	OP_SYSREQ_PRI,
	OP_SYSREQ_C,
	OP_FILE,				//DEPRECATED
	OP_LINE,				//DEPRECATED
	OP_SYMBOL,				//DEPRECATED
	OP_SRANGE,				//DEPRECATED
	OP_JUMP_PRI,
	OP_SWITCH,
	OP_CASETBL,
	OP_SWAP_PRI,			//DONE
	OP_SWAP_ALT,			//DONE
	OP_PUSH_ADR,			//DONE
	OP_NOP,					//DONE
	OP_SYSREQ_N,
	OP_SYMTAG,				//DEPRECATED
	OP_BREAK,
	OP_PUSH2_C,				//DONE
	OP_PUSH2,				//DONE
	OP_PUSH2_S,				//DONE
	OP_PUSH2_ADR,			//DONE
	OP_PUSH3_C,				//DONE
	OP_PUSH3,				//DONE
	OP_PUSH3_S,				//DONE
	OP_PUSH3_ADR,			//DONE
	OP_PUSH4_C,				//DONE
	OP_PUSH4,				//DONE
	OP_PUSH4_S,				//DONE
	OP_PUSH4_ADR,			//DONE
	OP_PUSH5_C,				//DONE
	OP_PUSH5,				//DONE
	OP_PUSH5_S,				//DONE
	OP_PUSH5_ADR,			//DONE
	OP_LOAD_BOTH,			//DONE
	OP_LOAD_S_BOTH,			//DONE
	OP_CONST,				//DONE
	OP_CONST_S,				//DONE
	/* ----- */
	OP_SYSREQ_D,			//UNSUPPORTED
	OP_SYSREQ_ND,			//UNSUPPORTED
	/* ----- */
	OP_HEAP_PRI,			//DONE
	OP_PUSH_HEAP_C,			//DONE
	OP_POP_HEAP_PRI,		//DONE
	/* ----- */
	OP_NUM_OPCODES
} OPCODE;

extern int OpAdvTable[];

#endif //_INCLUDE_SOURCEPAWN_JIT_X86_OPCODE_INFO_H_
