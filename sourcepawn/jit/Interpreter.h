#ifndef _INCLUDE_SSA_INTERPRETER_H_
#define _INCLUDE_SSA_INTERPRETER_H_

#include "sp_vm_basecontext.h"
#include "JSI.h"

namespace SourcePawn
{
	int InterpretSSA(BaseContext *pContext,
		const JsiStream *stream,
		cell_t *result);
}

#endif //_INCLUDE_SSA_INTERPRETER_H_
