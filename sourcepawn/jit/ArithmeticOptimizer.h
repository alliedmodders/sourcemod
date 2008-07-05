#ifndef _INCLUDE_SOURCEPAWN_JIT2_ALU_OPTIMIZER_H_
#define _INCLUDE_SOURCEPAWN_JIT2_ALU_OPTIMIZER_H_

#include "JSI.h"

namespace SourcePawn
{
	class ArithmeticOptimizer : public JsiPipeline
	{
	public:
		ArithmeticOptimizer(JsiPipeline *other);
	public:
		virtual JIns *ins_add(JIns *op1, JIns *op2);
		virtual JIns *ins_sub(JIns *op1, JIns *op2);
	};
}

#endif //_INCLUDE_SOURCEPAWN_JIT2_ALU_OPTIMIZER_H_
