/**
 * vim: set ts=4 :
 * =============================================================================
 * SourcePawn JIT
 * Copyright (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is not open source and may not be copied without explicit wriiten
 * permission of AlliedModders LLC.  This file may not be redistributed in whole
 * or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEPAWN_JIT_X86_OPCODE_INFO_H_
#define _INCLUDE_SOURCEPAWN_JIT_X86_OPCODE_INFO_H_

#include "../jit_helpers.h"

/**
 * This outputs the execution function for a plugin.
 * It also returns the 'return' offset, which is used for 
 * breaking out of the JIT during runtime.
 */
jitoffs_t Write_Execute_Function(JitWriter *jit);

/**
 * Writes the Sysreq.* opcodes as a function call.
 */
void WriteOp_Sysreq_N_Function(JitWriter *jit);
void WriteOp_Sysreq_C_Function(JitWriter *jit);

/**
 * Write the GENARRAY intrinsic function.
 */
void WriteIntrinsic_GenArray(JitWriter *jit);

/**
 * Generates code to set an error state in the VM and return.
 * This is used for generating the error set points in the VM.
 * GetError writes the error from the context. SetError hardcodes.
 */
void Write_GetError(JitWriter *jit);
void Write_SetError(JitWriter *jit, int error);

/**
 * Checks the stacks for min and low errors.
 * :TODO: Should a variation of this go in the pushN opcodes?
 */
void Write_CheckStack_Min(JitWriter *jit);
void Write_CheckStack_Low(JitWriter *jit);

/**
 * Checks the heap for min and low errors.
 */
void Write_CheckHeap_Min(JitWriter *jit);
void Write_CheckHeap_Low(JitWriter *jit);

/**
 * Verifies an address by register.  The address must reside
 * between DAT and HP and SP and STP.
 */
void Write_Check_VerifyAddr(JitWriter *jit, jit_uint8_t reg);

/**
 * Checks for division by zero.
 */
void Write_Check_DivZero(JitWriter *jit, jit_uint8_t reg);

/**
 * Writes the break debug function.
 */
void Write_BreakDebug(JitWriter *jit);

/** 
 * These are for writing the PushN opcodes.
 */
void Macro_PushN_Addr(JitWriter *jit, int i);
void Macro_PushN_S(JitWriter *jit, int i);
void Macro_PushN_C(JitWriter *jit, int i);
void Macro_PushN(JitWriter *jit, int i);

/** 
* Bound checking for the tracker stack,
*/
int JIT_VerifyLowBoundTracker(sp_context_t *ctx);
int JIT_VerifyOrAllocateTracker(sp_context_t *ctx);

/** 
* Writes the push into tracker function.
*/
void WriteOp_Tracker_Push_Reg(JitWriter *jit, uint8_t reg);

/** 
* Writes the rounding table for the float compare opcode.
*/
void Write_RoundingTable(JitWriter *jit);

/** 
* Aligns the current code position to 16 bytes.
*/
void AlignMe(JitWriter *jit);

/**
 * Legend for Statuses:
 * ****** *** ********
 * DONE       -> code generation is done
 * !GEN       -> code generation is deliberate skipped because:
 *                 (default): compiler does not generate
 *                DEPRECATED: this feature no longer exists/supported
 *               UNSUPPORTED: this opcode is not supported
 *                      TODO: done in case needed
 * VERIFIED   -> code generation is checked as run-time working.  prefixes:
 *                ! errors are not checked yet.
 *                - non-inline errors are not checked yet.
 *                ~ assumed checked because of related variation, but not actually checked
 */

typedef enum
{
	OP_NONE,              /* invalid opcode */
	OP_LOAD_PRI,			//!VERIFIED
	OP_LOAD_ALT,			//~!VERIFIED (load.pri)
	OP_LOAD_S_PRI,			//VERIFIED
	OP_LOAD_S_ALT,			//VERIFIED
	OP_LREF_PRI,			//VERIFIED
	OP_LREF_ALT,			//~VERIFIED
	OP_LREF_S_PRI,			//VERIFIED
	OP_LREF_S_ALT,			//~VERIFIED (lref.s.pri)
	OP_LOAD_I,				//VERIFIED
	OP_LODB_I,				//VERIFIED
	OP_CONST_PRI,			//VERIFIED
	OP_CONST_ALT,			//~VERIFIED (const.pri)
	OP_ADDR_PRI,			//VERIFIED
	OP_ADDR_ALT,			//VERIFIED
	OP_STOR_PRI,			//VERIFIED
	OP_STOR_ALT,			//~VERIFIED (stor.pri)
	OP_STOR_S_PRI,			//VERIFIED
	OP_STOR_S_ALT,			//~VERIFIED (stor.s.pri)
	OP_SREF_PRI,			//VERIFIED
	OP_SREF_ALT,			//~VERIFIED
	OP_SREF_S_PRI,			//VERIFIED
	OP_SREF_S_ALT,			//~VERIFIED (stor.s.alt)
	OP_STOR_I,				//VERIFIED
	OP_STRB_I,				//VERIFIED
	OP_LIDX,				//VERIFIED
	OP_LIDX_B,				//DONE
	OP_IDXADDR,				//VERIFIED
	OP_IDXADDR_B,			//DONE
	OP_ALIGN_PRI,			// !GEN :TODO: - only used for pack access, drop support in compiler first
	OP_ALIGN_ALT,			// !GEN :TODO: - only used for pack access, drop support in compiler first
	OP_LCTRL,				// !GEN
	OP_SCTRL,				// !GEN
	OP_MOVE_PRI,			//~VERIFIED (move.alt)
	OP_MOVE_ALT,			//VERIFIED
	OP_XCHG,				//DONE
	OP_PUSH_PRI,			//DONE
	OP_PUSH_ALT,			//DONE
	OP_PUSH_R,				// !GEN DEPRECATED
	OP_PUSH_C,				//VERIFIED
	OP_PUSH,				//DONE
	OP_PUSH_S,				//VERIFIED
	OP_POP_PRI,				//VERIFIED
	OP_POP_ALT,				//VERIFIED
	OP_STACK,				//VERIFIED
	OP_HEAP,				//VERIFIED
	OP_PROC,				//VERIFIED
	OP_RET,					// !GEN
	OP_RETN,				//VERIFIED
	OP_CALL,				//VERIFIED
	OP_CALL_PRI,			// !GEN
	OP_JUMP,				//VERIFIED
	OP_JREL,				// !GEN
	OP_JZER,				//VERIFIED
	OP_JNZ,					//DONE
	OP_JEQ,					//VERIFIED
	OP_JNEQ,				//VERIFIED
	OP_JLESS,				// !GEN
	OP_JLEQ,				// !GEN
	OP_JGRTR,				// !GEN
	OP_JGEQ,				// !GEN
	OP_JSLESS,				//VERIFIED
	OP_JSLEQ,				//VERIFIED
	OP_JSGRTR,				//VERIFIED
	OP_JSGEQ,				//VERIFIED
	OP_SHL,					//VERIFIED
	OP_SHR,					//VERIFIED (Note: operator >>>)
	OP_SSHR,				//VERIFIED (Note: operator >>)
	OP_SHL_C_PRI,			//DONE
	OP_SHL_C_ALT,			//DONE
	OP_SHR_C_PRI,			//DONE
	OP_SHR_C_ALT,			//DONE
	OP_SMUL,				//VERIFIED
	OP_SDIV,				//DONE
	OP_SDIV_ALT,			//VERIFIED
	OP_UMUL,				// !GEN
	OP_UDIV,				// !GEN
	OP_UDIV_ALT,			// !GEN
	OP_ADD,					//VERIFIED
	OP_SUB,					//DONE
	OP_SUB_ALT,				//VERIFIED
	OP_AND,					//VERIFIED
	OP_OR,					//VERIFIED
	OP_XOR,					//VERIFIED
	OP_NOT,					//VERIFIED
	OP_NEG,					//VERIFIED
	OP_INVERT,				//VERIFIED
	OP_ADD_C,				//VERIFIED
	OP_SMUL_C,				//VERIFIED
	OP_ZERO_PRI,			//VERIFIED
	OP_ZERO_ALT,			//~VERIFIED
	OP_ZERO,				//VERIFIED
	OP_ZERO_S,				//VERIFIED
	OP_SIGN_PRI,			//DONE
	OP_SIGN_ALT,			//DONE
	OP_EQ,					//VERIFIED
	OP_NEQ,					//VERIFIED
	OP_LESS,				// !GEN
	OP_LEQ,					// !GEN
	OP_GRTR,				// !GEN
	OP_GEQ,					// !GEN
	OP_SLESS,				//VERIFIED
	OP_SLEQ,				//VERIFIED
	OP_SGRTR,				//VERIFIED
	OP_SGEQ,				//VERIFIED
	OP_EQ_C_PRI,			//DONE
	OP_EQ_C_ALT,			//DONE
	OP_INC_PRI,				//VERIFIED
	OP_INC_ALT,				//~VERIFIED (inc.pri)
	OP_INC,					//VERIFIED
	OP_INC_S,				//VERIFIED
	OP_INC_I,				//VERIFIED
	OP_DEC_PRI,				//VERIFIED
	OP_DEC_ALT,				//~VERIFIED (dec.pri)
	OP_DEC,					//VERIFIED
	OP_DEC_S,				//VERIFIED
	OP_DEC_I,				//VERIFIED
	OP_MOVS,				//VERIFIED
	OP_CMPS,				// !GEN
	OP_FILL,				//VERIFIED
	OP_HALT,				//DONE
	OP_BOUNDS,				//VERIFIED
	OP_SYSREQ_PRI,			// !GEN
	OP_SYSREQ_C,			//VERIFIED
	OP_FILE,				// !GEN DEPRECATED
	OP_LINE,				// !GEN DEPRECATED
	OP_SYMBOL,				// !GEN DEPRECATED
	OP_SRANGE,				// !GEN DEPRECATED
	OP_JUMP_PRI,			// !GEN
	OP_SWITCH,				//VERIFIED
	OP_CASETBL,				//VERIFIED
	OP_SWAP_PRI,			//VERIFIED
	OP_SWAP_ALT,			//~VERIFIED (swap.alt)
	OP_PUSH_ADR,			//VERIFIED
	OP_NOP,					//VERIFIED (lol)
	OP_SYSREQ_N,			//VERIFIED
	OP_SYMTAG,				// !GEN DEPRECATED
	OP_BREAK,				//DONE
	OP_PUSH2_C,				//~VERIFIED (push3.c)
	OP_PUSH2,				//VERIFIED
	OP_PUSH2_S,				//VERIFIED
	OP_PUSH2_ADR,			//VERIFIED
	OP_PUSH3_C,				//VERIFIED
	OP_PUSH3,				//~VERIFIED (push2)
	OP_PUSH3_S,				//~VERIFIED (push2.s)
	OP_PUSH3_ADR,			//~VERIFIED (push2.adr)
	OP_PUSH4_C,				//~VERIFIED (push3.c)
	OP_PUSH4,				//~VERIFIED (push2)
	OP_PUSH4_S,				//~VERIFIED (push2.s)
	OP_PUSH4_ADR,			//~VERIFIED (push2.adr)
	OP_PUSH5_C,				//~VERIFIED (push3.c)
	OP_PUSH5,				//~VERIFIED (push2)
	OP_PUSH5_S,				//~VERIFIED (push2.s)
	OP_PUSH5_ADR,			//~VERIFIED (push2.adr)
	OP_LOAD_BOTH,			//VERIFIED
	OP_LOAD_S_BOTH,			//VERIFIED
	OP_CONST,				//VERIFIED
	OP_CONST_S,				//DONE
	/* ----- */
	OP_SYSREQ_D,			// !GEN UNSUPPORT
	OP_SYSREQ_ND,			// !GEN UNSUPPORT
	/* ----- */
	OP_TRACKER_PUSH_C,		//DONE
	OP_TRACKER_POP_SETHEAP,	//VERIFIED
	OP_GENARRAY,			//VERIFIED
	OP_GENARRAY_Z,			//-VERIFIED (not tested for 1D arrays)
	OP_STRADJUST_PRI,		//VERIFIED
	OP_STACKADJUST,			//:TODO: VERIFY
	OP_FABS,				//VERIFIED
	OP_FLOAT,				//VERIFIED
	OP_FLOATADD,			//VERIFIED
	OP_FLOATSUB,			//VERIFIED
	OP_FLOATMUL,			//VERIFIED
	OP_FLOATDIV,			//VERIFIED
	OP_RND_TO_NEAREST,		//VERIFIED
	OP_RND_TO_FLOOR,		//VERIFIED
	OP_RND_TO_CEIL,			//VERIFIED
	OP_RND_TO_ZERO,			//VERIFIED
	OP_FLOATCMP,			//VERIFIED
	/* ----- */
	OP_NUM_OPCODES
} OPCODE;

#endif //_INCLUDE_SOURCEPAWN_JIT_X86_OPCODE_INFO_H_
