#include <sh_stack.h>
#include "DeadCodeEliminator.h"

using namespace SourceHook;
using namespace SourcePawn;

void SourcePawn::DeadCodeEliminator(const JsiStream *stream)
{
	JIns *ins;
	cell_t sp;
	CStack<JIns *> worklist;
	JsiReverseReader rdr(*stream);

	/* Eliminate dead stores to the stack. */
	sp = 0;
	while ((ins = rdr.next()) != NULL)
	{
		ins->value.imm = 0;
		switch (ins->op)
		{
		case J_return:
			{
				worklist.push(ins);
				break;
			}
		case J_store:
		case J_storei:
			{
				if (ins->param1.instr->op != J_frm)
				{
					worklist.push(ins);
				}
				else
				{
					ins->op = J_nop;
				}
				break;
			}
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
				case J_sub:
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

	JsiForwardReader fwd(*stream);
	while ((ins = fwd.next()) != NULL)
	{
		if (ins->op == J_store || ins->op == J_storei || ins->value.imm == 1)
		{
			continue;
		}
		ins->op = J_nop;
	}
}
