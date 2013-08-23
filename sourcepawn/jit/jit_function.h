// vim: set ts=8 sts=2 sw=2 tw=99 et:
#ifndef _INCLUDE_SOURCEPAWN_JIT2_FUNCTION_H_
#define _INCLUDE_SOURCEPAWN_JIT2_FUNCTION_H_

#include <sp_vm_types.h>
#include <am-vector.h>

struct LoopEdge
{
  uint32_t offset;
  int32_t disp32;
};

class JitFunction
{
 public:
  JitFunction(void *entry_addr, cell_t pcode_offs, LoopEdge *edges, uint32_t nedges);
  ~JitFunction();

 public:
  void *GetEntryAddress() const;
  cell_t GetPCodeAddress() const;
  uint32_t NumLoopEdges() const {
    return nedges_;
  }
  const LoopEdge &GetLoopEdge(size_t i) const {
    assert(i < nedges_);
    return edges_[i];
  }

 private:
  void *m_pEntryAddr;
  cell_t m_PcodeOffs;
  LoopEdge *edges_;
  uint32_t nedges_;
};

#endif //_INCLUDE_SOURCEPAWN_JIT2_FUNCTION_H_
