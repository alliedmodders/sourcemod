#include "ConstantFolding.h"

using namespace SourcePawn;

ConstantFolding::ConstantFolding(JsiPipeline *other) : JsiPipeline(other)
{
}

JIns *ConstantFolding::ins_add(JIns *op1, JIns *op2)
{
	if (op1->op == J_imm && op2->op == J_imm)
	{
		return ins_imm(op1->param1.imm + op2->param1.imm);
	}

	return m_pOut->ins_add(op1, op2);
}

JIns *ConstantFolding::ins_sub(JIns *op1, JIns *op2)
{
	if (op1->op == J_imm && op2->op == J_imm)
	{
		return ins_imm(op1->param1.imm - op2->param1.imm);
	}

	return m_pOut->ins_sub(op1, op2);
}
