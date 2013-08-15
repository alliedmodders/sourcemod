// vim: set ts=8 ts=2 sw=2 tw=99 et:
#include "jit_function.h"
#include "sp_vm_engine.h"
#include "jit_x86.h"

JitFunction::JitFunction(void *entry_addr, cell_t pcode_offs, LoopEdge *edges, uint32_t nedges)
  : m_pEntryAddr(entry_addr),
    m_PcodeOffs(pcode_offs),
    edges_(edges),
    nedges_(nedges)
{
}

JitFunction::~JitFunction()
{
  delete [] edges_;
  g_Jit.FreeCode(m_pEntryAddr);
}

void *
JitFunction::GetEntryAddress() const
{
  return m_pEntryAddr;
}

cell_t
JitFunction::GetPCodeAddress() const
{
  return m_PcodeOffs;
}

