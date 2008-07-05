#include "Interpreter.h"

using namespace SourcePawn;

int SourcePawn::InterpretSSA(BaseContext *pContext,
							 const JsiStream *stream,
							 cell_t *result)
{
	JIns *ins;
	const sp_context_t *ctx;
	const sp_plugin_t *plugin;
	JsiForwardReader rdr(*stream);

	ctx = pContext->GetContext();
	plugin = pContext->GetPlugin();

	while (true)
	{
		if ((ins = rdr.next()) == NULL)
		{
			return SP_ERROR_INVALID_INSTRUCTION;
		}

		switch (ins->op)
		{
		case J_frm:
			{
				ins->value.imm = ctx->frm;
				break;
			}
		case J_imm:
			{
				ins->value = ins->param1;
				break;
			}
		case J_ld:
			{
				ins->value.imm = *(int32_t *)
					((char *)(plugin->memory + ins->param1.instr->value.imm) + ins->param2.imm);
				break;
			}
		case J_st:
			{
				*(int32_t *)((char *)(ins->param1.instr->value.ptr) + ins->value.imm) 
					=
					ins->param2.instr->value.imm;
				break;
			}
		case J_add:
			{
				ins->value.imm = ins->param1.instr->value.imm + ins->param2.instr->value.imm;
				break;
			}
		case J_sub:
			{
				ins->value.imm = ins->param1.instr->value.imm - ins->param2.instr->value.imm;
				break;
			}
		case J_return:
			{
				if (result)
				{
					*result = ins->param1.instr->value.imm;
				}
				return SP_ERROR_NONE;
				break;
			}
		default:
			{
				return SP_ERROR_INVALID_INSTRUCTION;
			}
		}
	}

	return SP_ERROR_INVALID_INSTRUCTION;
}
