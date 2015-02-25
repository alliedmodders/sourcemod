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
#include <macro-assembler-x86.h>
#include <am-vector.h>
#include "jit_shared.h"
#include "plugin-runtime.h"
#include "plugin-context.h"
#include "compiled-function.h"
#include "opcodes.h"

using namespace SourcePawn;

namespace sp {
class Environment;
}

#define JIT_INLINE_ERRORCHECKS  (1<<0)
#define JIT_INLINE_NATIVES      (1<<1)
#define STACK_MARGIN            64      //8 parameters of safety, I guess
#define JIT_FUNCMAGIC           0x214D4148  //magic function offset

#define JITVARS_PROFILER        2    //profiler

#define sDIMEN_MAX              5    //this must mirror what the compiler has.

typedef struct funcinfo_s
{
  unsigned int magic;
  unsigned int index;
} funcinfo_t;

typedef struct functracker_s
{
  unsigned int num_functions;
  unsigned int code_size;
} functracker_t;

struct CallThunk
{
  Label call;
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

  CompiledFunction *emit(int *errp);

 private:
  bool setup(cell_t pcode_offs);
  bool emitOp(OPCODE op);
  cell_t readCell();

 private:
  Label *labelAt(size_t offset);
  bool emitCall();
  bool emitNativeCall(OPCODE op);
  bool emitSwitch();
  void emitGenArray(bool autozero);
  void emitCallThunks();
  void emitCheckAddress(Register reg);
  void emitErrorPath(Label *dest, int code);
  void emitErrorPaths();
  void emitFloatCmp(ConditionCode cc);

  ExternalAddress cipAddr() {
    return ExternalAddress(context_->addressOfCip());
  }
  ExternalAddress hpAddr() {
    sp_context_t *ctx = rt_->GetBaseContext()->GetCtx();
    return ExternalAddress(&ctx->hp);
  }
  ExternalAddress frmAddr() {
    return ExternalAddress(context_->addressOfFrm());
  }

 private:
  AssemblerX86 masm;
  sp::Environment *env_;
  PluginRuntime *rt_;
  PluginContext *context_;
  const sp_plugin_t *plugin_;
  int error_;
  uint32_t pcode_start_;
  cell_t *code_start_;
  cell_t *cip_;
  cell_t *code_end_;
  Label *jump_map_;
  ke::Vector<size_t> backward_jumps_;

  // Errors
  Label error_bounds_;
  Label error_heap_low_;
  Label error_heap_min_;
  Label error_stack_low_;
  Label error_stack_min_;
  Label error_divide_by_zero_;
  Label error_memaccess_;
  Label error_integer_overflow_;
  Label extern_error_;

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

#endif //_INCLUDE_SOURCEPAWN_JIT_X86_H_

