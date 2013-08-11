/**
 * vim: set ts=4 sw=4 tw=99 et:
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEPAWN_JIT_X86_H_
#define _INCLUDE_SOURCEPAWN_JIT_X86_H_

#include <sp_vm_types.h>
#include <sp_vm_api.h>
#include <KeCodeAllocator.h>
#include <macro-assembler-x86.h>
#include <ke_vector.h>
#include "jit_shared.h"
#include "BaseRuntime.h"
#include "sp_vm_basecontext.h"
#include "jit_function.h"
#include "opcodes.h"

using namespace SourcePawn;

#define JIT_INLINE_ERRORCHECKS  (1<<0)
#define JIT_INLINE_NATIVES      (1<<1)
#define STACK_MARGIN            64      //8 parameters of safety, I guess
#define JIT_FUNCMAGIC           0x214D4148  //magic function offset

#define JITVARS_TRACKER         0    //important: don't change this to avoid trouble
#define JITVARS_BASECTX         1    //important: don't change this aWOAWOGJQG I LIKE HAM
#define JITVARS_PROFILER        2    //profiler
#define JITVARS_PLUGIN          3    //sp_plugin_t

#define sDIMEN_MAX              5    //this must mirror what the compiler has.

#define GET_CONTEXT(c)  ((IPluginContext *)c->vm[JITVARS_BASECTX])

typedef struct tracker_s
{
  size_t size; 
  ucell_t *pBase; 
  ucell_t *pCur;
} tracker_t;

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

class CompData : public ICompilation
{
public:
  CompData() 
  : profile(0),
    inline_level(0)
  {
  };
  bool SetOption(const char *key, const char *val);
  void Abort();
public:
  cell_t cur_func;            /* current func pcode offset */
  /* Options */
  int profile;                /* profiling flags */
  int inline_level;           /* inline optimization level */
  /* Per-compilation properties */
  unsigned int func_idx;      /* current function index */
};

class Compiler
{
 public:
  Compiler(BaseRuntime *rt, cell_t pcode_offs);
  ~Compiler();

  JitFunction *emit(int *errp);

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

  ExternalAddress cipAddr() {
    sp_context_t *ctx = rt_->GetBaseContext()->GetCtx();
    return ExternalAddress(&ctx->cip);
  }

 private:
  AssemblerX86 masm;
  BaseRuntime *rt_;
  const sp_plugin_t *plugin_;
  int error_;
  uint32_t pcode_start_;
  cell_t *code_start_;
  cell_t *cip_;
  cell_t *code_end_;
  Label *jump_map_;

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

class JITX86
{
 public:
  JITX86();

 public:
  bool InitializeJIT();
  void ShutdownJIT();
  ICompilation *StartCompilation(BaseRuntime *runtime);
  ICompilation *StartCompilation();
  void SetupContextVars(BaseRuntime *runtime, BaseContext *pCtx, sp_context_t *ctx);
  void FreeContextVars(sp_context_t *ctx);
  SPVM_NATIVE_FUNC CreateFakeNative(SPVM_FAKENATIVE_FUNC callback, void *pData);
  void DestroyFakeNative(SPVM_NATIVE_FUNC func);
  JitFunction *CompileFunction(BaseRuntime *runtime, cell_t pcode_offs, int *err);
  ICompilation *ApplyOptions(ICompilation *_IN, ICompilation *_OUT);
  int InvokeFunction(BaseRuntime *runtime, JitFunction *fn, cell_t *result);

 public:
  ExternalAddress GetUniversalReturn() {
      return ExternalAddress(m_pJitReturn);
  }
  void *AllocCode(size_t size);
  void FreeCode(void *code);

 private:
  void *m_pJitEntry;         /* Entry function */
  void *m_pJitReturn;        /* Universal return address */
};

const Register pri = eax;
const Register alt = edx;
const Register stk = edi;
const Register dat = ebp;
const Register tmp = ecx;
const Register info = esi;
const Register frm = ebx;

struct InfoVars {
    ucell_t frm;
    ucell_t hp;
    void *esp;
};

#define AMX_INFO_FRAME          offsetof(InfoVars, frm)
#define AMX_INFO_HEAP           offsetof(InfoVars, hp)
#define AMX_INFO_NSTACK         offsetof(InfoVars, esp)

extern Knight::KeCodeCache *g_pCodeCache;
extern JITX86 g_Jit;

#endif //_INCLUDE_SOURCEPAWN_JIT_X86_H_
