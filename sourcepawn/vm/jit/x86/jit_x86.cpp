#include <string.h>
#include <stdlib.h>
#include "jit_x86.h"
#include "..\jit_helpers.h"
#include "opcode_helpers.h"
#include "x86_macros.h"

inline void WriteOp_Move_Pri(JitWriter *jit)
{
	//mov eax, edx
	IA32_Mov_Rm_Reg(jit, AMX_REG_PRI, AMX_REG_ALT, MOD_REG);
}

inline void WriteOp_Move_Alt(JitWriter *jit)
{
	//mov edx, eax
	IA32_Mov_Rm_Reg(jit, AMX_REG_ALT, AMX_REG_PRI, MOD_REG);
}

inline void WriteOp_Xchg(JitWriter *jit)
{
	/* :TODO: change this? */
	//xchg eax, edx
	IA32_Xchg_Eax_Reg(jit, AMX_REG_ALT);
}

inline void WriteOp_Push(JitWriter *jit)
{
	//push stack, DAT offset based
	//sub ebp, 4
	//mov ecx, [edi+<val>]
	//mov [ebp], ecx
	cell_t val = jit->read_cell();
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
	//optimize encoding a bit...
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_DAT, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_DAT, val);
	IA32_Mov_Rm_Reg(jit, AMX_REG_STK, AMX_REG_TMP, MOD_MEM_REG);
}

inline void WriteOp_Push_S(JitWriter *jit)
{
	//push stack, FRM offset based
	//sub ebp, 4
	//mov ecx, [ebx+<val>]
	//mov [ebp], ecx
	cell_t val = jit->read_cell();
	IA32_Sub_Rm_Imm8(jit, AMX_REG_STK, 4, MOD_REG);
	//optimize encoding a bit...
	if (val < SCHAR_MAX && val > SCHAR_MIN)
		IA32_Mov_Reg_Rm_Disp8(jit, AMX_REG_TMP, AMX_REG_FRM, (jit_int8_t)val);
	else
		IA32_Mov_Reg_Rm_Disp32(jit, AMX_REG_TMP, AMX_REG_FRM, val);
	IA32_Mov_Rm_Reg(jit, AMX_REG_STK, AMX_REG_TMP, MOD_MEM_REG);
}

inline void WriteOp_Push4_C(JitWriter *jit)
{
	Macro_PushN_C(jit, 4);
}

inline void WriteOp_Push5_C(JitWriter *jit)
{
	Macro_PushN_C(jit, 5);
}

inline void WriteOp_Push2_Adr(JitWriter *jit)
{
	Macro_PushN_Addr(jit, 2);
}

inline void WriteOp_Push3_Adr(JitWriter *jit)
{
	Macro_PushN_Addr(jit, 3);
}

inline void WriteOp_Push4_Adr(JitWriter *jit)
{
	Macro_PushN_Addr(jit, 4);
}

inline void WriteOp_Push5_Adr(JitWriter *jit)
{
	Macro_PushN_Addr(jit, 5);
}

inline void WriteOp_Push2_S(JitWriter *jit)
{
	Macro_PushN_S(jit, 2);
}

inline void WriteOp_Push3_S(JitWriter *jit)
{
	Macro_PushN_S(jit, 3);
}

inline void WriteOp_Push4_S(JitWriter *jit)
{
	Macro_PushN_S(jit, 4);
}

inline void WriteOp_Push5_S(JitWriter *jit)
{
	Macro_PushN_S(jit, 5);
}

inline void WriteOp_Push5(JitWriter *jit)
{
	Macro_PushN(jit, 5);
}

inline void WriteOp_Push4(JitWriter *jit)
{
	Macro_PushN(jit, 4);
}

inline void WriteOp_Push3(JitWriter *jit)
{
	Macro_PushN(jit, 3);
}

inline void WriteOp_Push2(JitWriter *jit)
{
	Macro_PushN(jit, 2);
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

	JitWriter writer;

	writer.inptr = (cell_t *)code;
	writer.outptr = NULL;
	writer.outbase = NULL;

//redo_pass:
	/* SECOND PASS (medium load): writer.outbase is NULL, getting size only
	 * THIRD PASS (heavy load!!): writer.outbase is valid and output is written
	 */
	cell_t *endptr = (cell_t *)(end_cip);
	JitWriter *jit = &writer;
	for (; writer.inptr <= endptr;)
	{
		op = (OPCODE)writer.read_cell();
		switch (op)
		{
		case OP_MOVE_PRI:
			{
				WriteOp_Move_Pri(jit);
				break;
			}
		case OP_MOVE_ALT:
			{
				WriteOp_Move_Alt(jit);
				break;
			}
		case OP_XCHG:
			{
				WriteOp_Xchg(jit);
				break;
			}
		case OP_PUSH:
			{
				WriteOp_Push(jit);
				break;
			}
		case OP_PUSH_S:
			{
				WriteOp_Push_S(jit);
				break;
			}
		case OP_PUSH4_C:
			{
				WriteOp_Push4_C(jit);
				break;
			}
		case OP_PUSH5_C:
			{
				WriteOp_Push5_C(jit);
				break;
			}
		case OP_PUSH2_ADR:
			{
				WriteOp_Push2_Adr(jit);
				break;
			}
		case OP_PUSH3_ADR:
			{
				WriteOp_Push3_Adr(jit);
				break;
			}
		case OP_PUSH4_ADR:
			{
				WriteOp_Push4_Adr(jit);
				break;
			}
		case OP_PUSH5_ADR:
			{
				WriteOp_Push5_Adr(jit);
				break;
			}
		case OP_PUSH2_S:
			{
				WriteOp_Push2_S(jit);
				break;
			}
		case OP_PUSH3_S:
			{
				WriteOp_Push3_S(jit);
				break;
			}
		case OP_PUSH4_S:
			{
				WriteOp_Push4_S(jit);
				break;
			}
		case OP_PUSH5_S:
			{
				WriteOp_Push5_S(jit);
				break;
			}
		case OP_PUSH5:
			{
				WriteOp_Push5(jit);
				break;
			}
		case OP_PUSH4:
			{
				WriteOp_Push4(jit);
				break;
			}
		case OP_PUSH3:
			{
				WriteOp_Push3(jit);
				break;
			}
		case OP_PUSH2:
			{
				WriteOp_Push2(jit);
				break;
			}
		default:
			{
				/* :TODO: error! */
				break;
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
