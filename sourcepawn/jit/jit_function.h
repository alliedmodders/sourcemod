#ifndef _INCLUDE_SOURCEPAWN_JIT2_FUNCTION_H_
#define _INCLUDE_SOURCEPAWN_JIT2_FUNCTION_H_

#include <sp_vm_types.h>

class JitFunction
{
public:
	JitFunction(void *entry_addr, cell_t pcode_offs);
	~JitFunction();
public:
	void *GetEntryAddress();
	cell_t GetPCodeAddress();
private:
	void *m_pEntryAddr;
	cell_t m_PcodeOffs;
};

#endif //_INCLUDE_SOURCEPAWN_JIT2_FUNCTION_H_
