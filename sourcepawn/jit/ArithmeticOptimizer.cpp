#include "ArithmeticOptimizer.h"

using namespace SourcePawn;

ArithmeticOptimizer::ArithmeticOptimizer(JsiPipeline *other) : JsiPipeline(other)
{
}

JIns *ArithmeticOptimizer::ins_add(JIns *op1, JIns *op2)
{
	if (op1->op == J_imm && op1->param1.imm == 0)
	{
		return op2;
	}
	else if (op2->op == J_imm && op2->param1.imm == 0)
	{
		return op1;
	}

	return m_pOut->ins_add(op1, op2);
}

JIns *ArithmeticOptimizer::ins_sub(JIns *op1, JIns *op2)
{
	if (op2->op == J_imm && op2->param1.imm == 0)
	{
		return op1;
	}

	return m_pOut->ins_sub(op1, op2);
}
