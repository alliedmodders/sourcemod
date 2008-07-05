#ifndef _INCLUDE_SOURCEPAWN_JIT2_AMX_LISTENER_H_
#define _INCLUDE_SOURCEPAWN_JIT2_AMX_LISTENER_H_

#include <sp_vm_types.h>

namespace SourcePawn
{
	enum AmxReg
	{
		Amx_Pri,
		Amx_Alt
	};

	class IAmxListener
	{
	public:
		virtual bool OP_BREAK() = 0;
		virtual bool OP_PROC() = 0;
		virtual bool OP_RETN() = 0;
		virtual bool OP_STACK(cell_t offs) = 0;
		virtual bool OP_CONST_S(cell_t offs, cell_t value) = 0;
		virtual bool OP_PUSH_C(cell_t value) = 0;
		virtual bool OP_STOR_S_REG(AmxReg reg, cell_t offs) = 0;
		virtual bool OP_LOAD_S_REG(AmxReg reg, cell_t offs) = 0;
		virtual bool OP_ADD() = 0;
		virtual bool OP_ADD_C(cell_t value) = 0;
		virtual bool OP_SUB_ALT() = 0;
	};
}

#endif //_INCLUDE_SOURCEPAWN_JIT2_AMX_LISTENER_H_
