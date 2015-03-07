// vim: set ts=8 sts=2 sw=2 tw=99 et:
//
// This file is part of SourcePawn.
// 
// SourcePawn is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// SourcePawn is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with SourcePawn.  If not, see <http://www.gnu.org/licenses/>.
#ifndef _INCLUDE_SOURCEPAWN_JIT_X86_H_
#define _INCLUDE_SOURCEPAWN_JIT_X86_H_

#include <sp_vm_types.h>
#include <sp_vm_api.h>
#include <am-vector.h>
#include "plugin-runtime.h"
#include "plugin-context.h"
#include "compiled-function.h"
#include "opcodes.h"
#include "macro-assembler-x86.h"

using namespace SourcePawn;

namespace sp {
class LegacyImage;
class Environment;
class CompiledFunction;

struct ErrorPath
{
  SilentLabel label;
  const cell_t *cip;
  int err;

  ErrorPath(const cell_t *cip, int err)
   : cip(cip),
     err(err)
  {}
  ErrorPath()
  {}
};

struct BackwardJump {
  // The pc at the jump instruction (i.e. after it).
  uint32_t pc;
  // The cip of the jump.
  const cell_t *cip;
  // The offset of the timeout thunk. This is filled in at the end.
  uint32_t timeout_offset;

  BackwardJump()
  {}
  BackwardJump(uint32_t pc, const cell_t *cip)
   : pc(pc),
     cip(cip)
  {}
};

#define JIT_INLINE_ERRORCHECKS  (1<<0)
#define JIT_INLINE_NATIVES      (1<<1)
#define STACK_MARGIN            64      //8 parameters of safety, I guess
#define JIT_FUNCMAGIC           0x214D4148  //magic function offset

#define JITVARS_PROFILER        2    //profiler

#define sDIMEN_MAX              5    //this must mirror what the compiler has.

struct CallThunk
{
  SilentLabel call;
  cell_t pcode_offset;

  CallThunk(cell_t pcode_offset)
    : pcode_offset(pcode_offset)
  {
  }
};

class Compiler
{
 public:
  Compiler(PluginRuntime *rt, cell_t pcode_offs);
  ~Compiler();

  sp::CompiledFunction *emit(int *errp);

 private:
  bool setup(cell_t pcode_offs);
  bool emitOp(sp::OPCODE op);
  cell_t readCell();

 private:
  Label *labelAt(size_t offset);
  bool emitCall();
  bool emitNativeCall(sp::OPCODE op);
  bool emitSwitch();
  void emitGenArray(bool autozero);
  void emitCallThunks();
  void emitCheckAddress(Register reg);
  void emitErrorPath(Label *dest, int code);
  void emitErrorPaths();
  void emitFloatCmp(ConditionCode cc);
  void jumpOnError(ConditionCode cc, int err = 0);
  void emitThrowPathIfNeeded(int err);

  ExternalAddress hpAddr() {
    return ExternalAddress(context_->addressOfHp());
  }
  ExternalAddress frmAddr() {
    return ExternalAddress(context_->addressOfFrm());
  }

  // Map a return address (i.e. an exit point from a function) to its source
  // cip. This lets us avoid tracking the cip during runtime. These are
  // sorted by definition since we assemble and emit in forward order.
  void emitCipMapping(const cell_t *cip) {
    CipMapEntry entry;
    entry.cipoffs = uintptr_t(cip) - uintptr_t(code_start_);
    entry.pcoffs = masm.pc();
    cip_map_.append(entry);
  }

 private:
  AssemblerX86 masm;
  Environment *env_;
  PluginRuntime *rt_;
  PluginContext *context_;
  LegacyImage *image_;
  int error_;
  uint32_t pcode_start_;
  const cell_t *code_start_;
  const cell_t *cip_;
  const cell_t *op_cip_;
  const cell_t *code_end_;
  Label *jump_map_;
  ke::Vector<BackwardJump> backward_jumps_;

  ke::Vector<CipMapEntry> cip_map_;

  // Errors.
  ke::Vector<ErrorPath> error_paths_;
  Label throw_timeout_;
  Label throw_error_code_[SP_MAX_ERROR_CODES];
  Label report_error_;
  Label return_reported_error_;

  ke::Vector<CallThunk *> thunks_; //:TODO: free
};

const Register pri = eax;
const Register alt = edx;
const Register stk = edi;
const Register dat = esi;
const Register tmp = ecx;
const Register frm = ebx;

CompiledFunction *
CompileFunction(PluginRuntime *prt, cell_t pcode_offs, int *err);

}

#endif //_INCLUDE_SOURCEPAWN_JIT_X86_H_

