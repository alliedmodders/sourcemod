#ifndef _INCLUDE_AMX_TO_SSA_H_
#define _INCLUDE_AMX_TO_SSA_H_

#include "sp_vm_basecontext.h"
#include "JSI.h"

class BaseContext;

namespace SourcePawn
{
	JsiStream *ConvertAMXToSSA(BaseContext *pContext,
		uint32_t code_addr, 
		int *err,
		char *buffer, 
		size_t maxlength);
};

#endif //_INCLUDE_AMX_TO_SSA_H_
