#include <string.h>
#include <stdlib.h>
#include "jit_x86.h"

int OpAdvTable[OP_NUM_OPCODES];

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
	OpAdvTable[OP_PUSH_R] = sizeof(cell_t);
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
}

IPluginContext *JITX86::CompileToContext(ICompilation *co, int *err)
{
	CompData *data = (CompData *)co;
	sp_plugin_t *plugin = data->plugin;

	/* The first phase is to browse */
	uint8_t *code = plugin->pcode;
	uint8_t *cip;
	uint8_t *end_cip = plugin->pcode + plugin->pcode_size;
	OPCODE op;
	int op_c;
	uint32_t reloc_count = 0;

	/* FIRST PASS (light load) - Get initial opcode information */
	for (cip = code; cip < end_cip;)
	{
		op = (OPCODE)*(ucell_t *)cip;
		if ((unsigned)op >= OP_NUM_OPCODES)
		{
			AbortCompilation(co);
			*err = SP_ERR_INVALID_INSTRUCTION;
			return NULL;
		}
		cip += sizeof(cell_t);
		op_c = OpAdvTable[op];
		if (op_c >= 0)
		{
			cip += op_c;
		} else if (op_c == -3) {
			AbortCompilation(co);
			*err = SP_ERR_INVALID_INSTRUCTION;
			return NULL;
		} else if (op_c == -2) {
			reloc_count++;
			cip += sizeof(cell_t);
		} else if (op_c == -1) {
			switch (op)
			{
			case OP_SYSREQ_C:
			case OP_SYSREQ_PRI:
			case OP_SYSREQ_N:
				{
					if (!data->always_inline)
					{
						reloc_count++;
					}
					break;
				}
			case OP_CASETBL:
				{
					ucell_t num = *(ucell_t *)cip;
					cip += sizeof(cell_t);
					reloc_count += (num + 1);
					cip += ((2*num) + 1) * sizeof(cell_t);
					break;
				}
			default:
				{
					AbortCompilation(co);
					*err = SP_ERR_INVALID_INSTRUCTION;
					return NULL;
				}
			}
		}
	}

	*err = SP_ERR_NONE;

	return NULL;
}

const char *JITX86::GetVMName()
{
	return "JIT (x86)";
}

ICompilation *JITX86::StartCompilation(sp_plugin_t *plugin)
{
	CompData *data = new CompData;

	data->plugin = plugin;

	return data;
}

void JITX86::AbortCompilation(ICompilation *co)
{
	delete (CompData *)co;
}

bool JITX86::SetCompilationOption(ICompilation *co, const char *key, const char *val)
{
	CompData *data = (CompData *)co;

	if (strcmp(key, "debug") == 0)
	{
		data->debug = (atoi(val) == 1);
		if (data->debug && !(data->plugin->flags & SP_FLAG_DEBUG))
		{
			data->debug = false;
			return false;
		}
		return true;
	}

	return false;
}
