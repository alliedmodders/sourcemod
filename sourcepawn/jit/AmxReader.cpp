#include "AmxReader.h"

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

bool AmxReader::Read(IAmxListener *pListener, const sp_plugin_t *pl, cell_t code_offs)
{
	bool success;
	cell_t *end, op;

	success = true;
	m_pCode = (cell_t *)(pl->pcode + code_offs);
	end = (cell_t *)(pl->pcode + pl->pcode_size);

	m_Done = false;

	while (success && !m_Done)
	{
		if (m_pCode >= end)
		{
			return false;
		}

		op = *m_pCode++;

		switch (op)
		{
		case OP_BREAK:
			{
				success = pListener->OP_BREAK();
				break;
			}
		case OP_PROC:
			{
				success = pListener->OP_PROC();
				break;
			}
		case OP_RETN:
			{
				success = pListener->OP_RETN();
				break;
			}
		case OP_STACK:
			{
				m_pCode++;
				success = pListener->OP_STACK(*(m_pCode - 1));
				break;
			}
		case OP_CONST_S:
			{
				m_pCode += 2;
				success = pListener->OP_CONST_S(*(m_pCode - 2), *(m_pCode - 1));
				break;
			}
		case OP_PUSH_C:
			{
				m_pCode++;
				success = pListener->OP_PUSH_C(*(m_pCode - 1));
				break;
			}
		case OP_PUSH_PRI:
			{
				success = pListener->OP_PUSH_REG(Amx_Pri);
				break;
			}
		case OP_PUSH_ALT:
			{
				success = pListener->OP_PUSH_REG(Amx_Alt);
				break;
			}
		case OP_POP_PRI:
			{
				success = pListener->OP_POP_REG(Amx_Pri);
				break;
			}
		case OP_POP_ALT:
			{
				success = pListener->OP_POP_REG(Amx_Alt);
				break;
			}
		case OP_STOR_S_PRI:
			{
				m_pCode++;
				success = pListener->OP_STOR_S_REG(Amx_Pri, *(m_pCode - 1));
				break;
			}
		case OP_STOR_S_ALT:
			{
				m_pCode++;
				success = pListener->OP_STOR_S_REG(Amx_Alt, *(m_pCode - 1));
				break;
			}
		case OP_LOAD_S_PRI:
			{
				m_pCode++;
				success = pListener->OP_LOAD_S_REG(Amx_Pri, *(m_pCode - 1));
				break;
			}
		case OP_LOAD_S_ALT:
			{
				m_pCode++;
				success = pListener->OP_LOAD_S_REG(Amx_Alt, *(m_pCode - 1));
				break;
			}
		case OP_LOAD_S_BOTH:
			{
				m_pCode += 2;
				if ((success = pListener->OP_LOAD_S_REG(Amx_Pri, *(m_pCode - 2))) == true)
				{
					success = pListener->OP_LOAD_S_REG(Amx_Alt, *(m_pCode - 1));
				}
				break;
			}
		case OP_LOAD_I:
			{
				success = pListener->OP_LOAD_I();
				break;
			}
		case OP_ADD:
			{
				success = pListener->OP_ADD();
				break;
			}
		case OP_ADD_C:
			{
				m_pCode++;
				success = pListener->OP_ADD_C(*(m_pCode - 1));
				break;
			}
		case OP_SUB_ALT:
			{
				success = pListener->OP_SUB_ALT();
				break;
			}
		case OP_ZERO_PRI:
			{
				success = pListener->OP_ZERO_REG(Amx_Pri);
				break;
			}
		case OP_ZERO_ALT:
			{
				success = pListener->OP_ZERO_REG(Amx_Alt);
				break;
			}
		default:
			{
				success = false;
				break;
			}
		}
	}

	return success;
}

void AmxReader::SetDone()
{
	m_Done=  true;
}
