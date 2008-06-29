#include <stdio.h>
#include <stdarg.h>
#include "AmxToSSA.h"

using namespace SourcePawn;

typedef enum
{
	OP_NONE,                /* invalid opcode */
	OP_LOAD_PRI,			
	OP_LOAD_ALT,			
	OP_LOAD_S_PRI,			
	OP_LOAD_S_ALT,			
	OP_LREF_PRI,			
	OP_LREF_ALT,			
	OP_LREF_S_PRI,			
	OP_LREF_S_ALT,			
	OP_LOAD_I,				
	OP_LODB_I,				
	OP_CONST_PRI,			
	OP_CONST_ALT,			
	OP_ADDR_PRI,			
	OP_ADDR_ALT,			
	OP_STOR_PRI,			
	OP_STOR_ALT,			
	OP_STOR_S_PRI,			
	OP_STOR_S_ALT,			
	OP_SREF_PRI,			
	OP_SREF_ALT,			
	OP_SREF_S_PRI,			
	OP_SREF_S_ALT,			
	OP_STOR_I,				
	OP_STRB_I,				
	OP_LIDX,				
	OP_LIDX_B,				
	OP_IDXADDR,				
	OP_IDXADDR_B,			
	OP_ALIGN_PRI,			
	OP_ALIGN_ALT,			
	OP_LCTRL,				
	OP_SCTRL,				
	OP_MOVE_PRI,			
	OP_MOVE_ALT,			
	OP_XCHG,				
	OP_PUSH_PRI,			
	OP_PUSH_ALT,			
	OP_PUSH_R,				
	OP_PUSH_C,				
	OP_PUSH,				
	OP_PUSH_S,				
	OP_POP_PRI,				
	OP_POP_ALT,				
	OP_STACK,				
	OP_HEAP,				
	OP_PROC,				
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
	OP_SHL,					
	OP_SHR,					 
	OP_SSHR,				 
	OP_SHL_C_PRI,			
	OP_SHL_C_ALT,			
	OP_SHR_C_PRI,			
	OP_SHR_C_ALT,			
	OP_SMUL,				
	OP_SDIV,				
	OP_SDIV_ALT,			
	OP_UMUL,				
	OP_UDIV,				
	OP_UDIV_ALT,			
	OP_ADD,					
	OP_SUB,					
	OP_SUB_ALT,				
	OP_AND,					
	OP_OR,					
	OP_XOR,					
	OP_NOT,					
	OP_NEG,					
	OP_INVERT,				
	OP_ADD_C,				
	OP_SMUL_C,				
	OP_ZERO_PRI,			
	OP_ZERO_ALT,			
	OP_ZERO,				
	OP_ZERO_S,				
	OP_SIGN_PRI,			
	OP_SIGN_ALT,			
	OP_EQ,					
	OP_NEQ,					
	OP_LESS,				
	OP_LEQ,					
	OP_GRTR,				
	OP_GEQ,					
	OP_SLESS,				
	OP_SLEQ,				
	OP_SGRTR,				
	OP_SGEQ,				
	OP_EQ_C_PRI,			
	OP_EQ_C_ALT,			
	OP_INC_PRI,				
	OP_INC_ALT,				
	OP_INC,					
	OP_INC_S,				
	OP_INC_I,				
	OP_DEC_PRI,				
	OP_DEC_ALT,				
	OP_DEC,					
	OP_DEC_S,				
	OP_DEC_I,				
	OP_MOVS,				
	OP_CMPS,				
	OP_FILL,				
	OP_HALT,				
	OP_BOUNDS,				
	OP_SYSREQ_PRI,			
	OP_SYSREQ_C,			
	OP_FILE,				
	OP_LINE,				
	OP_SYMBOL,				
	OP_SRANGE,				
	OP_JUMP_PRI,			
	OP_SWITCH,				
	OP_CASETBL,				
	OP_SWAP_PRI,			
	OP_SWAP_ALT,			
	OP_PUSH_ADR,			
	OP_NOP,					 
	OP_SYSREQ_N,			
	OP_SYMTAG,				
	OP_BREAK,				
	OP_PUSH2_C,				
	OP_PUSH2,				
	OP_PUSH2_S,				
	OP_PUSH2_ADR,			
	OP_PUSH3_C,				
	OP_PUSH3,				
	OP_PUSH3_S,				
	OP_PUSH3_ADR,			
	OP_PUSH4_C,				
	OP_PUSH4,				
	OP_PUSH4_S,				
	OP_PUSH4_ADR,			
	OP_PUSH5_C,				
	OP_PUSH5,				
	OP_PUSH5_S,				
	OP_PUSH5_ADR,			
	OP_LOAD_BOTH,			
	OP_LOAD_S_BOTH,			
	OP_CONST,				
	OP_CONST_S,				
	/* ----- */
	OP_SYSREQ_D,			
	OP_SYSREQ_ND,			
	/* ----- */
	OP_TRACKER_PUSH_C,		
	OP_TRACKER_POP_SETHEAP,	
	OP_GENARRAY,			
	OP_GENARRAY_Z,			
	OP_STRADJUST_PRI,				
	/* ----- */
	OP_NUM_OPCODES
} OPCODE;

PageAllocator g_PageAlloc;

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	size_t len = vsnprintf(buffer, maxlength, fmt, ap);
	va_end(ap);

	if (len >= maxlength)
	{
		buffer[maxlength - 1] = '\0';
		return (maxlength - 1);
	}
	else
	{
		return len;
	}
}

JsiStream *SourcePawn::ConvertAMXToSSA(BaseContext *pContext, 
									   uint32_t code_addr, 
									   int *err,
									   char *buffer,
									   size_t maxlength)
{
	bool done;
	cell_t *code;
	JIns *memory;
	JIns *pri, *alt;
	JIns *param_ctx;
	JsiBufWriter *buf;
	const sp_plugin_t *plugin;

	pri = NULL;
	alt = NULL;
	done = false;
	plugin = pContext->GetPlugin();

	code = (cell_t *)(plugin->pcode + code_addr);
	
	if (*code != OP_PROC)
	{
		UTIL_Format(buffer, maxlength, "Function must start with PROC");
		*err = SP_ERROR_INVALID_INSTRUCTION;
		return NULL;
	}
	code++;

	*err = SP_ERROR_NONE;
	buf = new JsiBufWriter(&g_PageAlloc);
	
	param_ctx = buf->ins_imm_ptr((void *)pContext->GetContext());
	memory = buf->ins_imm_ptr((void *)pContext->GetPlugin()->base);

	while (!done)
	{
		cell_t op;

		if (code >= (cell_t *)(plugin->pcode + plugin->pcode_size))
		{
			UTIL_Format(buffer, maxlength, "Reached end of code stream with no RETN");
			*err = SP_ERROR_INVALID_INSTRUCTION;
			break;
		}

		op = *code++;
		switch (op)
		{
		case OP_BREAK:
			{
				break;
			}
		case OP_SYSREQ_N:
			{
				/* Adjust stack */
				buf->ins_storei(
					param_ctx,
					offsetof(sp_context_t, sp),
					buf->ins_add(
						buf->ins_loadi(
							param_ctx,
							offsetof(sp_context_t, sp)),
						buf->ins_imm(-4)
						)
					);
				/* Store 0 in stack */
				buf->ins_store(
					memory,
					buf->ins_loadi(
						param_ctx,
						offsetof(sp_context_t, sp)),
					buf->ins_imm(0)
					);
				/* Make call */
				pri = buf->ins_sysreq(*code, *(code + 1));
				/* Adjust stack again */
				buf->ins_storei(
					param_ctx,
					offsetof(sp_context_t, sp),
					buf->ins_add(
						buf->ins_loadi(
							param_ctx,
							offsetof(sp_context_t, sp)),
						buf->ins_imm(4)
						)
					);
				code += 2;

				break;
			}
		case OP_RETN:
			{
				if (pri != NULL)
				{
					buf->ins_return(pri);
				}
				else
				{
					UTIL_Format(buffer, maxlength, "RETN without PRI");
					*err = SP_ERROR_INVALID_INSTRUCTION;
				}
				done = true;
				break;
			}
		default:
			{
				UTIL_Format(buffer, maxlength, "Instruction is not supported");
				*err = SP_ERROR_INVALID_INSTRUCTION;
				done = true;
			}
		}
	}

	if (*err != SP_ERROR_NONE)
	{
		buf->destroy();
		return NULL;
	}

	JsiStream *stream = new JsiStream(buf->getstream());
	delete buf;
	
	return stream;
}
