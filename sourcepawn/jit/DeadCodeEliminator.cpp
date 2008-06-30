#include <sh_stack.h>
#include "DeadCodeEliminator.h"

using namespace SourceHook;
using namespace SourcePawn;

void SourcePawn::DeadCodeEliminator(const JsiStream *stream)
{
	JIns *ins;
	CStack<JIns *> worklist;
	JsiForwardReader rdr(*stream);

	while ((ins = rdr.next()) != NULL)
	{
		if (ins->op == J_store 
			|| ins->op == J_storei
			|| ins->op == J_return)
		{
			worklist.push(ins);
		}
		else
		{
			ins->value.imm = 0;
		}
	}

	while (!worklist.empty())
	{
		ins = worklist.front();
		worklist.pop();

		if (ins->op != J_store && ins->op != J_storei)
		{
			ins->value.imm = 1;
		}
		
		switch (ins->op)
		{
		case J_return:
		case J_load:
		case J_loadi:
		case J_store:
		case J_storei:
		case J_add:
			{
				worklist.push(ins->param1.instr);

				switch(ins->op)
				{
				case J_load:
				case J_store:
				case J_add:
					{
						worklist.push(ins->param2.instr);
						break;
					}
				}

				if (ins->op == J_store)
				{
					worklist.push(ins->value.instr);
				}

				break;
			}
		}
	}

	JsiBufWriter buf(&g_PageAlloc);

	rdr = JsiForwardReader(*stream);
	while ((ins = rdr.next()) != NULL)
	{
		if (ins->op == J_store || ins->op == J_storei || ins->value.imm == 1)
		{
			continue;
		}
		ins->op = J_nop;
	}
}
