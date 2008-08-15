#include "jit_function.h"
#include "sp_vm_engine.h"
#include "jit_x86.h"

JitFunction::JitFunction(void *entry_addr, cell_t pcode_offs)
: m_pEntryAddr(entry_addr), m_PcodeOffs(pcode_offs)
{
}

JitFunction::~JitFunction()
{
	g_Jit.FreeCode(m_pEntryAddr);
}

void *JitFunction::GetEntryAddress()
{
	return m_pEntryAddr;
}

cell_t JitFunction::GetPCodeAddress()
{
	return m_PcodeOffs;
}
