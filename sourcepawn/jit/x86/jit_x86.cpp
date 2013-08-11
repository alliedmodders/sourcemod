/**
 * vim: set ts=2 sw=2 tw=99 et:
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

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "jit_x86.h"
#include "../sp_vm_engine.h"
#include "../engine2.h"
#include "../BaseRuntime.h"
#include "../sp_vm_basecontext.h"

using namespace Knight;

#if defined USE_UNGEN_OPCODES
#include "ungen_opcodes.h"
#endif

#define __ masm.

JITX86 g_Jit;
KeCodeCache *g_pCodeCache = NULL;
ISourcePawnEngine *engine = &g_engine1;

static inline void *
LinkCode(AssemblerX86 &masm)
{
  if (masm.outOfMemory())
    return NULL;

  void *code = Knight::KE_AllocCode(g_pCodeCache, masm.length());
  if (!code)
    return NULL;

  masm.emitToExecutableMemory(code);
  return code;
}

static inline ConditionCode
OpToCondition(OPCODE op)
{
  switch (op) {
   case OP_EQ:
   case OP_JEQ:
    return equal;
   case OP_NEQ:
   case OP_JNEQ:
    return not_equal;
   case OP_SLESS:
   case OP_JSLESS:
    return less;
   case OP_SLEQ:
   case OP_JSLEQ:
    return less_equal;
   case OP_SGRTR:
   case OP_JSGRTR:
    return greater;
   case OP_SGEQ:
   case OP_JSGEQ:
    return greater_equal;
   default:
    assert(false);
    return negative;
  }
}

struct array_creation_t
{
  const cell_t *dim_list;     /* Dimension sizes */
  cell_t dim_count;           /* Number of dimensions */
  cell_t *data_offs;          /* Current offset AFTER the indirection vectors (data) */
  cell_t *base;               /* array base */
};

static cell_t
GenerateInnerArrayIndirectionVectors(array_creation_t *ar, int dim, cell_t cur_offs)
{
  cell_t write_offs = cur_offs;
  cell_t *data_offs = ar->data_offs;

  cur_offs += ar->dim_list[dim];

  // Dimension n-x where x > 2 will have sub-vectors.  
  // Otherwise, we just need to reference the data section.
  if (ar->dim_count > 2 && dim < ar->dim_count - 2) {
    // For each index at this dimension, write offstes to our sub-vectors.
    // After we write one sub-vector, we generate its sub-vectors recursively.
    // At the end, we're given the next offset we can use.
    for (int i = 0; i < ar->dim_list[dim]; i++) {
      ar->base[write_offs] = (cur_offs - write_offs) * sizeof(cell_t);
      write_offs++;
      cur_offs = GenerateInnerArrayIndirectionVectors(ar, dim + 1, cur_offs);
    }
  } else {
    // In this section, there are no sub-vectors, we need to write offsets 
    // to the data.  This is separate so the data stays in one big chunk.
    // The data offset will increment by the size of the last dimension, 
    // because that is where the data is finally computed as. 
    for (int i = 0; i < ar->dim_list[dim]; i++) {
      ar->base[write_offs] = (*data_offs - write_offs) * sizeof(cell_t);
      write_offs++;
      *data_offs = *data_offs + ar->dim_list[dim + 1];
    }
  }

  return cur_offs;
}

static cell_t
calc_indirection(const array_creation_t *ar, cell_t dim)
{
  cell_t size = ar->dim_list[dim];
  if (dim < ar->dim_count - 2)
    size += ar->dim_list[dim] * calc_indirection(ar, dim + 1);
  return size;
}

static cell_t
GenerateArrayIndirectionVectors(cell_t *arraybase, cell_t dims[], cell_t _dimcount, bool autozero)
{
  array_creation_t ar;
  cell_t data_offs;

  /* Reverse the dimensions */
  cell_t dim_list[sDIMEN_MAX];
  int cur_dim = 0;
  for (int i = _dimcount - 1; i >= 0; i--)
    dim_list[cur_dim++] = dims[i];
  
  ar.base = arraybase;
  ar.dim_list = dim_list;
  ar.dim_count = _dimcount;
  ar.data_offs = &data_offs;

  data_offs = calc_indirection(&ar, 0);
  GenerateInnerArrayIndirectionVectors(&ar, 0, 0);
  return data_offs;
}

static int
PopTrackerAndSetHeap(BaseRuntime *rt)
{
  sp_context_t *ctx = rt->GetBaseContext()->GetCtx();
  tracker_t *trk = (tracker_t *)(ctx->vm[JITVARS_TRACKER]);
  assert(trk->pCur > trk->pBase);

  trk->pCur--;
  if (trk->pCur < trk->pBase)
    return SP_ERROR_TRACKER_BOUNDS;

  ucell_t amt = *trk->pCur;
  if (amt > (ctx->hp - rt->plugin()->data_size))
    return SP_ERROR_HEAPMIN;

  ctx->hp -= amt;
  return SP_ERROR_NONE;
}

static int
PushTracker(sp_context_t *ctx, size_t amount)
{
  tracker_t *trk = (tracker_t *)(ctx->vm[JITVARS_TRACKER]);

  if ((size_t)(trk->pCur - trk->pBase) >= trk->size)
    return SP_ERROR_TRACKER_BOUNDS;

  if (trk->pCur + 1 - (trk->pBase + trk->size) == 0) {
    size_t disp = trk->size - 1;
    trk->size *= 2;
    trk->pBase = (ucell_t *)realloc(trk->pBase, trk->size * sizeof(cell_t));

    if (!trk->pBase)
      return SP_ERROR_TRACKER_BOUNDS;

    trk->pCur = trk->pBase + disp;
  }

  *trk->pCur++ = amount;
  return SP_ERROR_NONE;
}

static int
GenerateArray(BaseRuntime *rt, InfoVars &vars, uint32_t argc, cell_t *argv, int autozero)
{
  sp_context_t *ctx = rt->GetBaseContext()->GetCtx();

  // Calculate how many cells are needed.
  if (argv[0] <= 0)
    return SP_ERROR_ARRAY_TOO_BIG;

  uint32_t dim = 1;         // second to last dimension
  uint32_t cells = argv[0];

  for (uint32_t dim = 1; dim < argc; dim++) {
    cell_t dimsize = argv[dim];
    if (dimsize <= 0)
      return SP_ERROR_ARRAY_TOO_BIG;
    if (!ke::IsUint32MultiplySafe(cells, dimsize))
      return SP_ERROR_ARRAY_TOO_BIG;
    cells *= uint32_t(dimsize);
    if (!ke::IsUint32AddSafe(cells, dimsize))
      return SP_ERROR_ARRAY_TOO_BIG;
    cells += uint32_t(dimsize);
  }

  if (!ke::IsUint32MultiplySafe(cells, 4))
    return SP_ERROR_ARRAY_TOO_BIG;

  uint32_t bytes = cells * 4;
  if (!ke::IsUint32AddSafe(ctx->hp, bytes))
    return SP_ERROR_ARRAY_TOO_BIG;

  uint32_t new_hp = ctx->hp + bytes;
  cell_t *dat_hp = reinterpret_cast<cell_t *>(rt->plugin()->memory + new_hp);

  // argv, coincidentally, is STK.
  if (dat_hp >= argv - STACK_MARGIN)
    return SP_ERROR_HEAPLOW;

  if (int err = PushTracker(rt->GetBaseContext()->GetCtx(), bytes))
    return err;

  cell_t *base = reinterpret_cast<cell_t *>(rt->plugin()->memory + ctx->hp);
  cell_t offs = GenerateArrayIndirectionVectors(base, argv, argc, autozero);
  assert(size_t(offs) == cells);

  argv[argc - 1] = ctx->hp;
  ctx->hp = new_hp;
  return SP_ERROR_NONE;
}

#if !defined NDEBUG
static const char *
GetFunctionName(const sp_plugin_t *plugin, uint32_t offs)
{
  if (!plugin->debug.unpacked) {
    uint32_t max, iter;
    sp_fdbg_symbol_t *sym;
    sp_fdbg_arraydim_t *arr;
    uint8_t *cursor = (uint8_t *)(plugin->debug.symbols);

    max = plugin->debug.syms_num;
    for (iter = 0; iter < max; iter++) {
      sym = (sp_fdbg_symbol_t *)cursor;

      if (sym->ident == SP_SYM_FUNCTION && sym->codestart <= offs && sym->codeend > offs)
        return plugin->debug.stringbase + sym->name;

      if (sym->dimcount > 0) {
        cursor += sizeof(sp_fdbg_symbol_t);
        arr = (sp_fdbg_arraydim_t *)cursor;
        cursor += sizeof(sp_fdbg_arraydim_t) * sym->dimcount;
        continue;
      }

      cursor += sizeof(sp_fdbg_symbol_t);
    }
  } else {
    uint32_t max, iter;
    sp_u_fdbg_symbol_t *sym;
    sp_u_fdbg_arraydim_t *arr;
    uint8_t *cursor = (uint8_t *)(plugin->debug.symbols);

    max = plugin->debug.syms_num;
    for (iter = 0; iter < max; iter++) {
      sym = (sp_u_fdbg_symbol_t *)cursor;

      if (sym->ident == SP_SYM_FUNCTION && sym->codestart <= offs && sym->codeend > offs)
        return plugin->debug.stringbase + sym->name;

      if (sym->dimcount > 0) {
        cursor += sizeof(sp_u_fdbg_symbol_t);
        arr = (sp_u_fdbg_arraydim_t *)cursor;
        cursor += sizeof(sp_u_fdbg_arraydim_t) * sym->dimcount;
        continue;
      }

      cursor += sizeof(sp_u_fdbg_symbol_t);
    }
  }

  return NULL;
}
#endif

static int
CompileFromThunk(BaseRuntime *runtime, cell_t pcode_offs, void **addrp, char *pc)
{
  JitFunction *fn = runtime->GetJittedFunctionByOffset(pcode_offs);
  if (!fn) {
    int err;
    fn = g_Jit.CompileFunction(runtime, pcode_offs, &err);
    if (!fn)
      return err;
  }

#if !defined NDEBUG
  g_engine1.GetDebugHook()->OnDebugSpew(
      "Patching thunk to %s::%s\n",
      runtime->plugin()->name,
      GetFunctionName(runtime->plugin(), pcode_offs));
#endif

  *addrp = fn->GetEntryAddress();

  /* Right now, we always keep the code RWE */
  *(intptr_t *)(pc - 4) = intptr_t(fn->GetEntryAddress()) - intptr_t(pc);
  return SP_ERROR_NONE;
}

static cell_t
NativeCallback(sp_context_t *ctx, ucell_t native_idx, cell_t *params)
{
  sp_native_t *native;
  cell_t save_sp = ctx->sp;
  cell_t save_hp = ctx->hp;
  sp_plugin_t *pl = (sp_plugin_t *)(ctx->vm[JITVARS_PLUGIN]);

  ctx->n_idx = native_idx;

  if (ctx->hp < (cell_t)pl->data_size) {
    ctx->n_err = SP_ERROR_HEAPMIN;
    return 0;
  }

  if (ctx->hp + STACK_MARGIN > ctx->sp) {
    ctx->n_err = SP_ERROR_STACKLOW;
    return 0;
  }

  if ((uint32_t)ctx->sp >= pl->mem_size) {
    ctx->n_err = SP_ERROR_STACKMIN;
    return 0;
  }

  native = &pl->natives[native_idx];

  if (native->status == SP_NATIVE_UNBOUND) {
    ctx->n_err = SP_ERROR_INVALID_NATIVE;
    return 0;
  }

  cell_t result = native->pfn(GET_CONTEXT(ctx), params);

  if (ctx->n_err != SP_ERROR_NONE)
    return result;

  if (save_sp != ctx->sp) {
    ctx->n_err = SP_ERROR_STACKLEAK;
    return result;
  }
  if (save_hp != ctx->hp) {
    ctx->n_err = SP_ERROR_HEAPLEAK;
    return result;
  }

  return result;
}

Compiler::Compiler(BaseRuntime *rt, cell_t pcode_offs)
  : rt_(rt),
    plugin_(rt->plugin()),
    error_(SP_ERROR_NONE),
    pcode_start_(pcode_offs),
    code_start_(reinterpret_cast<cell_t *>(plugin_->pcode + pcode_start_)),
    cip_(code_start_),
    code_end_(reinterpret_cast<cell_t *>(plugin_->pcode + plugin_->pcode_size))
{
  size_t nmaxops = plugin_->pcode_size / sizeof(cell_t) + 1;
  jump_map_ = new Label[nmaxops];
}

Compiler::~Compiler()
{
  delete [] jump_map_;
}

JitFunction *
Compiler::emit(int *errp)
{
  if (cip_ >= code_end_ || *cip_ != OP_PROC) {
    *errp = SP_ERROR_INVALID_INSTRUCTION;
    return NULL;
  }

#if !defined NDEBUG
  g_engine1.GetDebugHook()->OnDebugSpew(
      "Compiling function %s::%s\n",
      plugin_->name,
      GetFunctionName(plugin_, pcode_start_));

  SpewOpcode(plugin_, code_start_, cip_);
#endif

  cell_t *codeseg = reinterpret_cast<cell_t *>(plugin_->pcode);

  cip_++;
  if (!emitOp(OP_PROC)) {
      *errp = (error_ == SP_ERROR_NONE) ? SP_ERROR_OUT_OF_MEMORY : error_;
      return NULL;
  }

  while (cip_ < code_end_) {
    // If we reach the end of this function, or the beginning of a new
    // procedure, then stop.
    if (*cip_ == OP_PROC || *cip_ == OP_ENDPROC)
      break;

#if !defined NDEBUG
    SpewOpcode(plugin_, code_start_, cip_);
#endif

    // We assume every instruction is a jump target, so before emitting
    // an opcode, we bind its corresponding label.
    __ bind(&jump_map_[cip_ - codeseg]);

    OPCODE op = (OPCODE)readCell();
    if (!emitOp(op) || error_ != SP_ERROR_NONE) {
      *errp = (error_ == SP_ERROR_NONE) ? SP_ERROR_OUT_OF_MEMORY : error_;
      return NULL;
    }
  }

  emitCallThunks();
  emitErrorPaths();

  void *code = LinkCode(masm);
  if (!code) {
    *errp = SP_ERROR_OUT_OF_MEMORY;
    return NULL;
  }

  return new JitFunction(code, pcode_start_);
}

bool
Compiler::emitOp(OPCODE op)
{
  switch (op) {
    case OP_MOVE_PRI:
      __ movl(pri, alt);
      break;

    case OP_MOVE_ALT:
      __ movl(alt, pri);
      break;

    case OP_XCHG:
      __ xchgl(pri, alt);
      break;

    case OP_ZERO:
    {
      cell_t offset = readCell();
      __ movl(Operand(dat, offset), 0);
      break;
    }

    case OP_ZERO_S:
    {
      cell_t offset = readCell();
      __ movl(Operand(frm, offset), 0);
      break;
    }

    case OP_PUSH_PRI:
    case OP_PUSH_ALT:
    {
      Register reg = (op == OP_PUSH_PRI) ? pri : alt;
      __ movl(Operand(stk, -4), reg);
      __ subl(stk, 4);
      break;
    }

    case OP_PUSH_C:
    case OP_PUSH2_C:
    case OP_PUSH3_C:
    case OP_PUSH4_C:
    case OP_PUSH5_C:
    {
      int n = 1;
      if (op >= OP_PUSH2_C)
        n = ((op - OP_PUSH2_C) / 4) + 2;

      int i = 1;
      do {
        cell_t val = readCell();
        __ movl(Operand(stk, -(4 * i)), val);
      } while (i++ < n);
      __ subl(stk, 4 * n);
      break;
    }

    case OP_PUSH_ADR:
    case OP_PUSH2_ADR:
    case OP_PUSH3_ADR:
    case OP_PUSH4_ADR:
    case OP_PUSH5_ADR:
    {
      int n = 1;
      if (op >= OP_PUSH2_ADR)
        n = ((op - OP_PUSH2_ADR) / 4) + 2;

      int i = 1;

      // We temporarily relocate FRM to be a local address instead of an
      // absolute address.
      __ subl(frm, dat);
      do {
        cell_t offset = readCell();
        __ lea(tmp, Operand(frm, offset));
        __ movl(Operand(stk, -(4 * i)), tmp);
      } while (i++ < n);
      __ subl(stk, 4 * n);
      __ addl(frm, dat);
      break;
    }

    case OP_PUSH_S:
    case OP_PUSH2_S:
    case OP_PUSH3_S:
    case OP_PUSH4_S:
    case OP_PUSH5_S:
    {
      int n = 1;
      if (op >= OP_PUSH2_S)
        n = ((op - OP_PUSH2_S) / 4) + 2;

      int i = 1;
      do {
        cell_t offset = readCell();
        __ movl(tmp, Operand(frm, offset));
        __ movl(Operand(stk, -(4 * i)), tmp);
      } while (i++ < n);
      __ subl(stk, 4 * n);
      break;
    }

    case OP_PUSH:
    case OP_PUSH2:
    case OP_PUSH3:
    case OP_PUSH4:
    case OP_PUSH5:
    {
      int n = 1;
      if (op >= OP_PUSH2)
        n = ((op - OP_PUSH2) / 4) + 2;

      int i = 1;
      do {
        cell_t offset = readCell();
        __ movl(tmp, Operand(dat, offset));
        __ movl(Operand(stk, -(4 * i)), tmp);
      } while (i++ < n);
      __ subl(stk, 4 * n);
      break;
    }

    case OP_ZERO_PRI:
      __ xorl(pri, pri);
      break;

    case OP_ZERO_ALT:
      __ xorl(alt, alt);
      break;

    case OP_ADD:
      __ addl(pri, alt);
      break;

    case OP_SUB:
      __ subl(pri, alt);
      break;

    case OP_SUB_ALT:
      __ movl(tmp, alt);
      __ subl(tmp, pri);
      __ movl(pri, tmp);
      break;

    case OP_PROC:
      // Push the old frame onto the stack.
      __ movl(tmp, Operand(frmAddr()));
      __ movl(Operand(stk, -4), tmp);
      __ subl(stk, 8);    // extra unused slot for non-existant CIP

      // Get and store the new frame.
      __ movl(tmp, stk);
      __ movl(frm, stk);
      __ subl(tmp, dat);
      __ movl(Operand(frmAddr()), tmp);

      // Align the stack to 16-bytes (each call adds 4 bytes).
      __ subl(esp, 12);
      break;

    case OP_IDXADDR_B:
    {
      cell_t val = readCell();
      __ shll(pri, val);
      __ addl(pri, alt);
      break;
    }

    case OP_SHL:
      __ movl(ecx, alt);
      __ shll_cl(pri);
      break;

    case OP_SHR:
      __ movl(ecx, alt);
      __ shrl_cl(pri);
      break;

    case OP_SSHR:
      __ movl(ecx, alt);
      __ sarl_cl(pri);
      break;

    case OP_SHL_C_PRI:
    case OP_SHL_C_ALT:
    {
      Register reg = (op == OP_SHL_C_PRI) ? pri : alt;
      cell_t val = readCell();
      __ shll(reg, val);
      break;
    }

    case OP_SHR_C_PRI:
    case OP_SHR_C_ALT:
    {
      Register reg = (op == OP_SHR_C_PRI) ? pri : alt;
      cell_t val = readCell();
      __ shrl(reg, val);
      break;
    }

    case OP_SMUL:
      __ imull(pri, alt);
      break;

    case OP_NOT:
      __ testl(eax, eax);
      __ movl(eax, 0);
      __ set(zero, r8_al);
      break;

    case OP_NEG:
      __ negl(eax);
      break;

    case OP_XOR:
      __ xorl(pri, alt);
      break;

    case OP_OR:
      __ orl(pri, alt);
      break;

    case OP_AND:
      __ andl(pri, alt);
      break;

    case OP_INVERT:
      __ notl(pri);
      break;

    case OP_ADD_C:
    {
      cell_t val = readCell();
      __ addl(pri, val);
      break;
    }

    case OP_SMUL_C:
    {
      cell_t val = readCell();
      __ imull(pri, pri, val);
      break;
    }

    case OP_EQ:
    case OP_NEQ:
    case OP_SLESS:
    case OP_SLEQ:
    case OP_SGRTR:
    case OP_SGEQ:
    {
      ConditionCode cc = OpToCondition(op);
      __ cmpl(pri, alt);
      __ movl(pri, 0);
      __ set(cc, r8_al);
      break;
    }

    case OP_EQ_C_PRI:
    case OP_EQ_C_ALT:
    {
      Register reg = (op == OP_EQ_C_PRI) ? pri : alt;
      cell_t val = readCell();
      __ cmpl(reg, val);
      __ movl(pri, 0);
      __ set(equal, r8_al);
      break;
    }

    case OP_INC_PRI:
    case OP_INC_ALT:
    {
      Register reg = (OP_INC_PRI) ? pri : alt;
      __ addl(reg, 1);
      break;
    }

    case OP_INC:
    case OP_INC_S:
    {
      Register base = (op == OP_INC) ? dat : frm;
      cell_t offset = readCell();
      __ addl(Operand(base, offset), 1);
      break;
    }

    case OP_INC_I:
      __ addl(Operand(dat, pri, NoScale), 1);
      break;

    case OP_DEC_PRI:
    case OP_DEC_ALT:
    {
      Register reg = (op == OP_DEC_PRI) ? pri : alt;
      __ subl(reg, 1);
      break;
    }

    case OP_DEC:
    case OP_DEC_S:
    {
      Register base = (op == OP_DEC) ? dat : frm;
      cell_t offset = readCell();
      __ subl(Operand(base, offset), 1);
      break;
    }

    case OP_DEC_I:
      __ subl(Operand(dat, pri, NoScale), 1);
      break;

    case OP_LOAD_PRI:
    case OP_LOAD_ALT:
    {
      Register reg = (op == OP_LOAD_PRI) ? pri : alt;
      cell_t offset = readCell();
      __ movl(reg, Operand(dat, offset));
      break;
    }

    case OP_LOAD_BOTH:
    {
      cell_t offs1 = readCell();
      cell_t offs2 = readCell();
      __ movl(pri, Operand(dat, offs1));
      __ movl(alt, Operand(dat, offs2));
      break;
    }

    case OP_LOAD_S_PRI:
    case OP_LOAD_S_ALT:
    {
      Register reg = (op == OP_LOAD_S_PRI) ? pri : alt;
      cell_t offset = readCell();
      __ movl(reg, Operand(frm, offset));
      break;
    }

    case OP_LOAD_S_BOTH:
    {
      cell_t offs1 = readCell();
      cell_t offs2 = readCell();
      __ movl(pri, Operand(frm, offs1));
      __ movl(alt, Operand(frm, offs2));
      break;
    }

    case OP_LREF_S_PRI:
    case OP_LREF_S_ALT:
    {
      Register reg = (op == OP_LREF_S_PRI) ? pri : alt;
      cell_t offset = readCell();
      __ movl(reg, Operand(frm, offset));
      __ movl(reg, Operand(dat, reg, NoScale));
      break;
    }

    case OP_CONST_PRI:
    case OP_CONST_ALT:
    {
      Register reg = (op == OP_CONST_PRI) ? pri : alt;
      cell_t val = readCell();
      __ movl(reg, val);
      break;
    }

    case OP_ADDR_PRI:
    case OP_ADDR_ALT:
    {
      Register reg = (op == OP_ADDR_PRI) ? pri : alt;
      cell_t offset = readCell();
      __ movl(reg, Operand(frmAddr()));
      __ addl(reg, offset);
      break;
    }

    case OP_STOR_PRI:
    case OP_STOR_ALT:
    {
      Register reg = (op == OP_STOR_PRI) ? pri : alt;
      cell_t offset = readCell();
      __ movl(Operand(dat, offset), reg);
      break;
    }

    case OP_STOR_S_PRI:
    case OP_STOR_S_ALT:
    {
      Register reg = (op == OP_STOR_S_PRI) ? pri : alt;
      cell_t offset = readCell();
      __ movl(Operand(frm, offset), reg);
      break;
    }

    case OP_IDXADDR:
      __ lea(pri, Operand(alt, pri, ScaleFour));
      break;

    case OP_SREF_S_PRI:
    case OP_SREF_S_ALT:
    {
      Register reg = (op == OP_SREF_S_PRI) ? pri : alt;
      cell_t offset = readCell();
      __ movl(tmp, Operand(frm, offset));
      __ movl(Operand(dat, tmp, NoScale), reg);
      break;
    }

    case OP_POP_PRI:
    case OP_POP_ALT:
    {
      Register reg = (op == OP_POP_PRI) ? pri : alt;
      __ movl(reg, Operand(stk, 0));
      __ addl(stk, 4);
      break;
    }

    case OP_SWAP_PRI:
    case OP_SWAP_ALT:
    {
      Register reg = (op == OP_SWAP_PRI) ? pri : alt;
      __ movl(tmp, Operand(stk, 0));
      __ movl(Operand(stk, 0), reg);
      __ movl(reg, tmp);
      break;
    }

    case OP_LIDX:
      __ lea(pri, Operand(alt, pri, ScaleFour));
      __ movl(pri, Operand(dat, pri, NoScale));
      break;

    case OP_LIDX_B:
    {
      cell_t val = readCell();
      if (val >= 0 && val <= 3) {
        __ lea(pri, Operand(alt, pri, Scale(val)));
      } else {
        __ shll(pri, val);
        __ addl(pri, alt);
      }
      emitCheckAddress(pri);
      __ movl(pri, Operand(dat, pri, NoScale));
      break;
    }

    case OP_CONST:
    case OP_CONST_S:
    {
      Register base = (op == OP_CONST) ? dat : frm;
      cell_t offset = readCell();
      cell_t val = readCell();
      __ movl(Operand(base, offset), val);
      break;
    }

    case OP_LOAD_I:
      emitCheckAddress(pri);
      __ movl(pri, Operand(dat, pri, NoScale));
      break;

    case OP_STOR_I:
      emitCheckAddress(alt);
      __ movl(Operand(dat, alt, NoScale), pri);
      break;

    case OP_SDIV:
    case OP_SDIV_ALT:
    {
      Register dividend = (op == OP_SDIV) ? pri : alt;
      Register divisor = (op == OP_SDIV) ? alt : pri;

      // Guard against divide-by-zero.
      __ testl(divisor, divisor);
      __ j(zero, &error_divide_by_zero_);

      // A more subtle case; -INT_MIN / -1 yields an overflow exception.
      Label ok;
      __ cmpl(divisor, -1);
      __ j(not_equal, &ok);
      __ cmpl(dividend, 0x80000000);
      __ j(equal, &error_integer_overflow_);
      __ bind(&ok);

      // Now we can actually perform the divide.
      __ movl(tmp, divisor);
      if (op == OP_SDIV)
        __ movl(edx, dividend);
      else
        __ movl(eax, dividend);
      __ sarl(edx, 31);
      __ idivl(tmp);
      break;
    }

    case OP_LODB_I:
    {
      cell_t val = readCell();
      emitCheckAddress(pri);
      __ movl(pri, Operand(dat, pri, NoScale));
      if (val == 1)
        __ andl(pri, 0xff);
      else if (val == 2)
        __ andl(pri, 0xffff);
      break;
    }

    case OP_STRB_I:
    {
      cell_t val = readCell();
      emitCheckAddress(alt);
      if (val == 1)
        __ movb(Operand(dat, alt, NoScale), pri);
      else if (val == 2)
        __ movw(Operand(dat, alt, NoScale), pri);
      else if (val == 4)
        __ movl(Operand(dat, alt, NoScale), pri);
      break;
    }

    case OP_RETN:
    {
      // Restore the old frame pointer.
      __ movl(frm, Operand(stk, 4));              // get the old frm
      __ addl(stk, 8);                            // pop stack
      __ movl(Operand(frmAddr()), frm);           // store back old frm
      __ addl(frm, dat);                          // relocate

      // Remove parameters.
      __ movl(tmp, Operand(stk, 0));
      __ lea(stk, Operand(stk, tmp, ScaleFour, 4));
      __ addl(esp, 12);
      __ ret();
      break;
    }

    case OP_MOVS:
    {
      cell_t val = readCell();
      unsigned dwords = val / 4;
      unsigned bytes = val % 4;

      __ cld();
      __ push(esi);
      __ push(edi);
      __ lea(esi, Operand(dat, pri, NoScale));
      __ lea(edi, Operand(dat, alt, NoScale));
      if (dwords) {
        __ movl(ecx, dwords);
        __ rep_movsd();
      }
      if (bytes) {
        __ movl(ecx, bytes);
        __ rep_movsb();
      }
      __ pop(edi);
      __ pop(esi);
      break;
    }

    case OP_FILL:
    {
      // eax/pri is used implicitly.
      unsigned dwords = readCell() / 4;
      __ push(edi);
      __ lea(edi, Operand(dat, alt, NoScale));
      __ movl(ecx, dwords);
      __ cld();
      __ rep_stosd();
      __ pop(edi);
      break;
    }

    case OP_STRADJUST_PRI:
      __ addl(pri, 4);
      __ sarl(pri, 2);
      break;

    case OP_FABS:
      __ movl(pri, Operand(stk, 0));
      __ andl(pri, 0x7fffffff);
      __ addl(stk, 4);
      break;

    case OP_FLOAT:
      if (MacroAssemblerX86::Features().sse2) {
        __ cvtsi2ss(xmm0, Operand(edi, 0));
        __ movd(pri, xmm0);
      } else {
        __ fild32(Operand(edi, 0));
        __ subl(esp, 4);
        __ fstp32(Operand(esp, 0));
        __ pop(pri);
      }
      __ addl(stk, 4);
      break;

    case OP_FLOATADD:
    case OP_FLOATSUB:
    case OP_FLOATMUL:
    case OP_FLOATDIV:
      if (MacroAssemblerX86::Features().sse2) {
        __ movss(xmm0, Operand(stk, 0));
        if (op == OP_FLOATADD)
          __ addss(xmm0, Operand(stk, 4));
        else if (op == OP_FLOATSUB)
          __ subss(xmm0, Operand(stk, 4));
        else if (op == OP_FLOATMUL)
          __ mulss(xmm0, Operand(stk, 4));
        else if (op == OP_FLOATDIV)
          __ divss(xmm0, Operand(stk, 4));
        __ movd(pri, xmm0);
      } else {
        __ subl(esp, 4);
        __ fld32(Operand(stk, 0));

        if (op == OP_FLOATADD)
          __ fadd32(Operand(stk, 4));
        else if (op == OP_FLOATSUB)
          __ fsub32(Operand(stk, 4));
        else if (op == OP_FLOATMUL)
          __ fmul32(Operand(stk, 4));
        else if (op == OP_FLOATDIV)
          __ fdiv32(Operand(stk, 4));

        __ fstp32(Operand(esp, 0));
        __ pop(pri);
      }
      __ addl(stk, 8);
      break;

    case OP_RND_TO_NEAREST:
    {
      if (MacroAssemblerX86::Features().sse) {
        // Assume no one is touching MXCSR.
        __ cvtss2si(pri, Operand(stk, 0));
      } else {
        static float kRoundToNearest = 0.5f;
        // From http://wurstcaptures.untergrund.net/assembler_tricks.html#fastfloorf
        __ fld32(Operand(stk, 0));
        __ fadd32(st0, st0);
        __ fadd32(Operand(ExternalAddress(&kRoundToNearest)));
        __ subl(esp, 4);
        __ fistp32(Operand(esp, 0));
        __ pop(pri);
        __ sarl(pri, 1);
      }
      __ addl(stk, 4);
      break;
    }

    case OP_RND_TO_CEIL:
    {
      static float kRoundToCeil = -0.5f;
      // From http://wurstcaptures.untergrund.net/assembler_tricks.html#fastfloorf
      __ fld32(Operand(stk, 0));
      __ fadd32(st0, st0);
      __ fsubr32(Operand(ExternalAddress(&kRoundToCeil)));
      __ subl(esp, 4);
      __ fistp32(Operand(esp, 0));
      __ pop(pri);
      __ sarl(pri, 1);
      __ negl(pri);
      __ addl(stk, 4);
      break;
    }

    case OP_RND_TO_ZERO:
      if (MacroAssemblerX86::Features().sse) {
        __ cvttss2si(pri, Operand(stk, 0));
      } else {
        __ fld32(Operand(stk, 0));
        __ subl(esp, 8);
        __ fstcw(Operand(esp, 4));
        __ movl(Operand(esp, 0), 0xfff);
        __ fldcw(Operand(esp, 0));
        __ fistp32(Operand(esp, 0));
        __ pop(pri);
        __ fldcw(Operand(esp, 0));
        __ addl(esp, 4);
      }
      __ addl(stk, 4);
      break;

    case OP_RND_TO_FLOOR:
      __ fld32(Operand(stk, 0));
      __ subl(esp, 8);
      __ fstcw(Operand(esp, 4));
      __ movl(Operand(esp, 0), 0x7ff);
      __ fldcw(Operand(esp, 0));
      __ fistp32(Operand(esp, 0));
      __ pop(eax);
      __ fldcw(Operand(esp, 0));
      __ addl(esp, 4);
      __ addl(stk, 4);
      break;

    case OP_FLOATCMP:
    {
      Label bl, ab, done;
      if (MacroAssemblerX86::Features().sse) {
        __ movss(xmm0, Operand(stk, 4));
        __ ucomiss(xmm0, Operand(stk, 0));
      } else {
        __ fld32(Operand(stk, 0));
        __ fld32(Operand(stk, 4));
        __ fucomip(st1);
        __ fstp(st0);
      }
      __ j(above, &ab);
      __ j(below, &bl);
      __ xorl(pri, pri);
      __ jmp(&done);
      __ bind(&ab);
      __ movl(pri, -1);
      __ jmp(&done);
      __ bind(&bl);
      __ movl(pri, 1);
      __ bind(&done);
      __ addl(stk, 8);
      break;
    }

    case OP_STACK:
    {
      cell_t amount = readCell();
      __ addl(stk, amount);

     if (amount > 0) {
       // Check if the stack went beyond the stack top - usually a compiler error.
       __ cmpl(stk, intptr_t(plugin_->memory + plugin_->mem_size));
       __ j(not_below, &error_stack_min_);
     } else {
       // Check if the stack is going to collide with the heap.
       __ movl(tmp, Operand(hpAddr()));
       __ lea(tmp, Operand(dat, ecx, NoScale, STACK_MARGIN));
       __ cmpl(stk, tmp);
       __ j(below, &error_stack_low_);
     }
     break;
    }

    case OP_HEAP:
    {
      cell_t amount = readCell();
      __ movl(alt, Operand(hpAddr()));
      __ addl(Operand(hpAddr()), amount);

      if (amount > 0) {
        __ cmpl(Operand(hpAddr()), plugin_->data_size);
        __ j(below, &error_heap_min_);
      } else {
        __ movl(tmp, Operand(hpAddr()));
        __ lea(tmp, Operand(dat, ecx, NoScale, STACK_MARGIN));
        __ cmpl(tmp, stk);
        __ j(above, &error_heap_low_);
      }
      break;
    }

    case OP_JUMP:
    {
      Label *target = labelAt(readCell());
      if (!target)
        return false;
      __ jmp(target);
      break;
    }

    case OP_JZER:
    case OP_JNZ:
    {
      ConditionCode cc = (op == OP_JZER) ? zero : not_zero;
      Label *target = labelAt(readCell());
      if (!target)
        return false;
      __ testl(pri, pri);
      __ j(cc, target);
      break;
    }

    case OP_JEQ:
    case OP_JNEQ:
    case OP_JSLESS:
    case OP_JSLEQ:
    case OP_JSGRTR:
    case OP_JSGEQ:
    {
      Label *target = labelAt(readCell());
      if (!target)
        return false;
      ConditionCode cc = OpToCondition(op);
      __ cmpl(pri, alt);
      __ j(cc, target);
      break;
    }

    case OP_TRACKER_PUSH_C:
    {
      cell_t amount = readCell();

      __ push(pri);
      __ push(alt);

      __ push(amount * 4);
      __ push(intptr_t(rt_->GetBaseContext()->GetCtx()));
      __ call(ExternalAddress((void *)PushTracker));
      __ addl(esp, 8);
      __ testl(eax, eax);
      __ j(not_zero, &extern_error_);

      __ pop(alt);
      __ pop(pri);
      break;
    }

    case OP_TRACKER_POP_SETHEAP:
    {
      // Save registers.
      __ push(pri);
      __ push(alt);

      // Get the context pointer and call the sanity checker.
      __ push(intptr_t(rt_));
      __ call(ExternalAddress((void *)PopTrackerAndSetHeap));
      __ addl(esp, 4);
      __ testl(eax, eax);
      __ j(not_zero, &extern_error_);

      __ pop(alt);
      __ pop(pri);
      break;
    }

    case OP_BREAK:
    {
      cell_t cip = uintptr_t(cip_ - 1) - uintptr_t(plugin_->pcode);
      __ movl(Operand(cipAddr()), cip);
      break;
    }

    case OP_HALT:
      __ align(16);
      __ movl(tmp, intptr_t(rt_->GetBaseContext()->GetCtx()));
      __ movl(Operand(tmp, offsetof(sp_context_t, rval)), pri);
      __ movl(pri, readCell());
      __ jmp(&extern_error_);
      break;

    case OP_BOUNDS:
    {
      cell_t value = readCell();
      __ cmpl(eax, value);
      __ j(above, &error_bounds_);
      break;
    }

    case OP_GENARRAY:
    case OP_GENARRAY_Z:
      emitGenArray(op == OP_GENARRAY_Z);
      break;

    case OP_CALL:
      if (!emitCall())
        return false;
      break;

    case OP_SYSREQ_C:
    case OP_SYSREQ_N:
      if (!emitNativeCall(op))
        return false;
      break;

    case OP_SWITCH:
      if (!emitSwitch())
        return false;
      break;

    case OP_CASETBL:
    {
      size_t ncases = readCell();

      // Two cells per case, and one extra cell for the default address.
      cip_ += (ncases * 2) + 1;
      break;
    }

    case OP_NOP:
      break;

    default:
      error_ = SP_ERROR_INVALID_INSTRUCTION;
      return false;
  }

  return true;
}

Label *
Compiler::labelAt(size_t offset)
{
  if (offset % 4 != 0 ||
      offset > plugin_->pcode_size ||
      offset <= pcode_start_)
  {
    // If the jump target is misaligned, or out of pcode bounds, or is an
    // address out of the function bounds, we abort. Unfortunately we can't
    // test beyond the end of the function since we don't have a precursor
    // pass (yet).
    error_ = SP_ERROR_INSTRUCTION_PARAM;
    return NULL;
  }

  return &jump_map_[offset / sizeof(cell_t)];
}

void
Compiler::emitCheckAddress(Register reg)
{
  // Check if we're in memory bounds.
  __ cmpl(reg, plugin_->mem_size);
  __ j(not_below, &error_memaccess_);

  // Check if we're in the invalid region between hp and sp.
  Label done;
  __ cmpl(reg, Operand(hpAddr()));
  __ j(below, &done);
  __ lea(tmp, Operand(dat, reg, NoScale));
  __ cmpl(tmp, stk);
  __ j(below, &error_memaccess_);
  __ bind(&done);
}

void
Compiler::emitGenArray(bool autozero)
{
  cell_t val = readCell();
  if (val == 1)
  {
    // flat array; we can generate this without indirection tables.
    // Note that we can overwrite ALT because technically STACK should be destroying ALT
    __ movl(alt, Operand(hpAddr()));
    __ movl(tmp, Operand(stk, 0));
    __ movl(Operand(stk, 0), alt);    // store base of the array into the stack.
    __ lea(alt, Operand(alt, tmp, ScaleFour));
    __ movl(Operand(hpAddr()), alt);
    __ addl(alt, dat);
    __ cmpl(alt, stk);
    __ j(not_below, &error_heap_low_);

    __ shll(tmp, 2);
    __ push(tmp);
    __ push(intptr_t(rt_->GetBaseContext()->GetCtx()));
    __ call(ExternalAddress((void *)PushTracker));
    __ addl(esp, 4);
    __ pop(tmp);
    __ shrl(tmp, 2);
    __ testl(eax, eax);
    __ j(not_zero, &extern_error_);

    if (autozero) {
      // Note - tmp is ecx and still intact.
      __ push(eax);
      __ push(edi);
      __ xorl(eax, eax);
      __ movl(edi, Operand(stk, 0));
      __ addl(edi, dat);
      __ cld();
      __ rep_stosd();
      __ pop(edi);
      __ pop(eax);
    }
  } else {
    __ push(pri);

    // int GenerateArray(sp_plugin_t, vars[], uint32_t, cell_t *, int, unsigned *);
    __ push(autozero ? 1 : 0);
    __ push(stk);
    __ push(val);
    __ push(info);
    __ push(intptr_t(rt_));
    __ call(ExternalAddress((void *)GenerateArray));
    __ addl(esp, 5 * sizeof(void *));

    // restore pri to tmp
    __ pop(tmp);

    __ testl(eax, eax);
    __ j(not_zero, &extern_error_);

    // Move tmp back to pri, remove pushed args.
    __ movl(pri, tmp);
    __ addl(stk, (val - 1) * 4);
  }
}

bool
Compiler::emitCall()
{
  cell_t offset = readCell();

  // If this offset looks crappy, i.e. not aligned or out of bounds, we just
  // abort.
  if (offset % 4 != 0 || uint32_t(offset) >= plugin_->pcode_size) {
    error_ = SP_ERROR_INSTRUCTION_PARAM;
    return false;
  }

  // eax = context
  // ecx = rp
  __ movl(eax, intptr_t(rt_->GetBaseContext()->GetCtx()));
  __ movl(ecx, Operand(eax, offsetof(sp_context_t, rp)));

  // Check if the return stack is used up.
  __ cmpl(ecx, SP_MAX_RETURN_STACK);
  __ j(not_below, &error_stack_low_);

  // Add to the return stack.
  uintptr_t cip = uintptr_t(cip_ - 2) - uintptr_t(plugin_->pcode);
  __ movl(Operand(eax, ecx, ScaleFour, offsetof(sp_context_t, rstk_cips)), cip);

  // Increment the return stack pointer.
  __ addl(Operand(eax, offsetof(sp_context_t, rp)), 1);

  // Store the CIP of the function we're about to call.
  __ movl(Operand(cipAddr()), offset);

  JitFunction *fun = rt_->GetJittedFunctionByOffset(offset);
  if (!fun) {
    // Need to emit a delayed thunk.
    CallThunk *thunk = new CallThunk(offset);
    __ call(&thunk->call);
    if (!thunks_.append(thunk))
      return false;
  } else {
    // Function is already emitted, we can do a direct call.
    __ call(ExternalAddress(fun->GetEntryAddress()));
  }

  // Restore the last cip.
  __ movl(Operand(cipAddr()), cip);

  // Mark us as leaving the last frame.
  __ movl(tmp, intptr_t(rt_->GetBaseContext()->GetCtx()));
  __ subl(Operand(tmp, offsetof(sp_context_t, rp)), 1);
  return true;
}

void
Compiler::emitCallThunks()
{
  for (size_t i = 0; i < thunks_.length(); i++) {
    CallThunk *thunk = thunks_[i];

    Label error;
    __ bind(&thunk->call);
    // Huge hack - get the return address, since that is the call that we
    // need to patch.
    __ movl(eax, Operand(esp, 0));

    // Align the stack. Unfortunately this is a pretty careful dance based
    // on the number of words we push (4 args, 1 saved esp), and someday
    // we should look into automating this.
    __ movl(edx, esp);
    __ andl(esp, 0xfffffff0);
    __ subl(esp, 12);
    __ push(edx);

    // Push arguments.
    __ push(eax);
    __ subl(esp, 4);
    __ movl(Operand(esp, 0), esp);
    __ push(thunk->pcode_offset);
    __ push(intptr_t(rt_));
    __ call(ExternalAddress((void *)CompileFromThunk));
    __ movl(edx, Operand(esp, 8));
    __ movl(esp, Operand(esp, 16));
    __ testl(eax, eax);
    __ j(not_zero, &error);
    __ call(edx);
    __ ret();

    __ bind(&error);
    __ movl(Operand(cipAddr()), thunk->pcode_offset);
    __ jmp(g_Jit.GetUniversalReturn());
  }
}

cell_t
Compiler::readCell()
{
  if (cip_ >= code_end_) {
    error_= SP_ERROR_INVALID_INSTRUCTION;
    return 0;
  }
  return *cip_++;
}

bool
Compiler::emitNativeCall(OPCODE op)
{
  uint32_t native_index = readCell();

  if (native_index >= plugin_->num_natives) {
    error_ = SP_ERROR_INSTRUCTION_PARAM;
    return false;
  }

  uint32_t num_params;
  if (op == OP_SYSREQ_N) {
    num_params = readCell();

    // See if we can get a replacement opcode. If we can, then recursively
    // call emitOp() to generate it. Note: it's important that we do this
    // before generating any code for the SYSREQ.N.
    unsigned replacement = rt_->GetNativeReplacement(native_index);
    if (replacement != OP_NOP)
      return emitOp((OPCODE)replacement);

    // Store the number of parameters.
    __ movl(Operand(stk, -4), num_params);
    __ subl(stk, 4);
  }

  // Save registers.
  __ push(edx);

  // Push the parameters for the C++ function.
  __ push(stk);
  __ push(native_index);

  // Relocate our absolute stk to be dat-relative, and update the context's
  // view.
  __ movl(eax, intptr_t(rt_->GetBaseContext()->GetCtx()));
  __ subl(stk, dat);
  __ movl(Operand(eax, offsetof(sp_context_t, sp)), stk);

  // Push context and make the call.
  __ push(eax);
  __ call(ExternalAddress((void *)NativeCallback));

  // Check for errors.
  __ movl(ecx, intptr_t(rt_->GetBaseContext()->GetCtx()));
  __ movl(ecx, Operand(ecx, offsetof(sp_context_t, n_err)));
  __ testl(ecx, ecx);
  __ j(not_zero, &extern_error_);
  
  // Restore local state.
  __ addl(stk, dat);
  __ addl(esp, 12);
  __ pop(edx);

  if (op == OP_SYSREQ_N) {
    // Pop the stack. Do not check the margins.
    __ addl(stk, (num_params + 1) * sizeof(cell_t));
  }
  return true;
}

bool
Compiler::emitSwitch()
{
  cell_t offset = readCell();
  if (!labelAt(offset))
    return false;

  cell_t *tbl = (cell_t *)((char *)plugin_->pcode + offset + sizeof(cell_t));

  struct Entry {
    cell_t val;
    cell_t offset;
  };

  size_t ncases = *tbl++;

  Label *defaultCase = labelAt(*tbl);
  if (!defaultCase)
    return false;

  // Degenerate - 0 cases.
  if (!ncases) {
    __ jmp(defaultCase);
    return true;
  }

  Entry *cases = (Entry *)(tbl + 1);

  // Degenerate - 1 case.
  if (ncases == 1) {
    Label *maybe = labelAt(cases[0].offset);
    if (!maybe)
      return false;
    __ cmpl(pri, cases[0].val);
    __ j(equal, maybe);
    __ jmp(defaultCase);
    return true;
  }

  // We have two or more cases, so let's generate a full switch. Decide
  // whether we'll make an if chain, or a jump table, based on whether
  // the numbers are strictly sequential.
  bool sequential = true;
  {
    cell_t first = cases[0].val;
    cell_t last = first;
    for (size_t i = 1; i < ncases; i++) {
      if (cases[i].val != ++last) {
        sequential = false;
        break;
      }
    }
  }

  // First check whether the bounds are correct: if (a < LOW || a > HIGH);
  // this check is valid whether or not we emit a sequential-optimized switch.
  cell_t low = cases[0].val;
  if (low != 0) {
    // negate it so we'll get a lower bound of 0.
    low = -low;
    __ lea(tmp, Operand(pri, low));
  } else {
    __ movl(tmp, pri);
  }

  cell_t high = abs(cases[0].val - cases[ncases - 1].val);
  __ cmpl(tmp, high);
  __ j(above, defaultCase);

  if (sequential) {
    // Optimized table version. The tomfoolery below is because we only have
    // one free register... it seems unlikely pri or alt will be used given
    // that we're at the end of a control-flow point, but we'll play it safe.
    DataLabel table;
    __ push(eax);
    __ movl(eax, &table);
    __ movl(ecx, Operand(eax, ecx, ScaleFour));
    __ pop(eax);
    __ jmp(ecx);

    __ bind(&table);
    for (size_t i = 0; i < ncases; i++) {
      Label *label = labelAt(cases[i].offset);
      if (!label)
        return false;
      __ emit_absolute_address(label);
    }
  } else {
    // Slower version. Go through each case and generate a check.
    for (size_t i = 0; i < ncases; i++) {
      Label *label = labelAt(cases[i].offset);
      if (!label)
        return false;
      __ cmpl(pri, cases[i].val);
      __ j(equal, label);
    }
    __ jmp(defaultCase);
  }
  return true;
}

void
Compiler::emitErrorPath(Label *dest, int code)
{
  if (dest->used()) {
    __ bind(dest);
    __ movl(eax, code);
    __ jmp(g_Jit.GetUniversalReturn());
  }
}

void
Compiler::emitErrorPaths()
{
  emitErrorPath(&error_divide_by_zero_, SP_ERROR_DIVIDE_BY_ZERO);
  emitErrorPath(&error_stack_low_, SP_ERROR_STACKLOW);
  emitErrorPath(&error_stack_min_, SP_ERROR_STACKMIN);
  emitErrorPath(&error_bounds_, SP_ERROR_ARRAY_BOUNDS);
  emitErrorPath(&error_memaccess_, SP_ERROR_MEMACCESS);
  emitErrorPath(&error_heap_low_, SP_ERROR_HEAPLOW);
  emitErrorPath(&error_heap_min_, SP_ERROR_HEAPMIN);
  emitErrorPath(&error_integer_overflow_, SP_ERROR_INTEGER_OVERFLOW);

  if (extern_error_.used()) {
    __ bind(&extern_error_);
    __ movl(eax, intptr_t(rt_->GetBaseContext()->GetCtx()));
    __ movl(eax, Operand(eax, offsetof(sp_context_t, n_err)));
    __ jmp(g_Jit.GetUniversalReturn());
  }
}

typedef int (*JIT_EXECUTE)(InfoVars *vars, void *addr, uint8_t *memory, sp_context_t *ctx);

static void *
GenerateEntry(void **retp)
{
  AssemblerX86 masm;

  // Variables we're passed in:
  //  InfoVars *vars, void *entry, uint8_t *memory, sp_context_t *

  __ push(ebp);
  __ movl(ebp, esp);

  __ push(esi);
  __ push(edi);
  __ push(ebx);

  __ movl(esi, Operand(ebp, 8 + 4 * 0));
  __ movl(ecx, Operand(ebp, 8 + 4 * 1));
  __ movl(eax, Operand(ebp, 8 + 4 * 2));

  // Set up run-time registers.
  __ movl(ebx, Operand(ebp, 8 + 4 * 3));
  __ movl(edi, Operand(ebx, offsetof(sp_context_t, sp)));
  __ addl(edi, eax);
  __ movl(ebp, eax);
  __ movl(ebx, edi);
  __ movl(Operand(esi, AMX_INFO_NSTACK), esp);

  // Align the stack.
  __ andl(esp, 0xfffffff0);

  // Call into plugin (align the stack first).
  __ call(ecx);

  // Restore stack.
  __ movl(esp, Operand(info, AMX_INFO_NSTACK));

  // Get input context, store rval.
  __ movl(ecx, Operand(esp, 32));
  __ movl(Operand(ecx, offsetof(sp_context_t, rval)), pri);

  // Set no error.
  __ movl(eax, SP_ERROR_NONE);

  // Store latest stk. If we have an error code, we'll jump directly to here,
  // so eax will already be set.
  Label ret;
  __ bind(&ret);
  __ subl(stk, dat);
  __ movl(Operand(ecx, offsetof(sp_context_t, sp)), stk);

  // Restore registers and gtfo.
  __ pop(ebx);
  __ pop(edi);
  __ pop(esi);
  __ pop(ebp);
  __ ret();

  // The universal emergency return will jump to here.
  Label error;
  __ bind(&error);
  __ movl(esp, Operand(info, AMX_INFO_NSTACK));
  __ movl(ecx, Operand(esp, 32)); // ret-path expects ecx = ctx
  __ jmp(&ret);

  void *code = LinkCode(masm);
  if (!code)
    return NULL;

  *retp = reinterpret_cast<uint8_t *>(code) + error.offset();
  return code;
}

ICompilation *JITX86::ApplyOptions(ICompilation *_IN, ICompilation *_OUT)
{
  if (_IN == NULL)
    return _OUT;

  CompData *_in = (CompData * )_IN;
  CompData *_out = (CompData * )_OUT;

  _in->inline_level = _out->inline_level;
  _in->profile = _out->profile;

  _out->Abort();
  return _in;
}

JITX86::JITX86()
{
  m_pJitEntry = NULL;
}

bool JITX86::InitializeJIT()
{
  g_pCodeCache = KE_CreateCodeCache();

  m_pJitEntry = GenerateEntry(&m_pJitReturn);
  if (!m_pJitEntry)
    return false;

  MacroAssemblerX86 masm;
  MacroAssemblerX86::GenerateFeatureDetection(masm);
  void *code = LinkCode(masm);
  if (!code)
    return false;
  MacroAssemblerX86::RunFeatureDetection(code);
  KE_FreeCode(g_pCodeCache, code);

  return true;
}

void
JITX86::ShutdownJIT()
{
  KE_DestroyCodeCache(g_pCodeCache);
}

JitFunction *
JITX86::CompileFunction(BaseRuntime *prt, cell_t pcode_offs, int *err)
{
  Compiler cc(prt, pcode_offs);
  JitFunction *fun = cc.emit(err);
  if (!fun)
    return NULL;
  prt->AddJittedFunction(fun);
  return fun;
}

void
JITX86::SetupContextVars(BaseRuntime *runtime, BaseContext *pCtx, sp_context_t *ctx)
{
  tracker_t *trk = new tracker_t;

  ctx->vm[JITVARS_TRACKER] = trk;
  ctx->vm[JITVARS_BASECTX] = pCtx; /* GetDefaultContext() is not constructed yet */
  ctx->vm[JITVARS_PROFILER] = g_engine2.GetProfiler();
  ctx->vm[JITVARS_PLUGIN] = const_cast<sp_plugin_t *>(runtime->plugin());

  trk->pBase = (ucell_t *)malloc(1024);
  trk->pCur = trk->pBase;
  trk->size = 1024 / sizeof(cell_t);
}

SPVM_NATIVE_FUNC
JITX86::CreateFakeNative(SPVM_FAKENATIVE_FUNC callback, void *pData)
{
  AssemblerX86 masm;

  __ push(ebx);
  __ push(edi);
  __ push(esi);
  __ movl(edi, Operand(esp, 16)); // store ctx
  __ movl(esi, Operand(esp, 20)); // store params
  __ movl(ebx, esp);
  __ andl(esp, 0xfffffff0);
  __ subl(esp, 4);

  __ push(intptr_t(pData));
  __ push(esi);
  __ push(edi);
  __ call(ExternalAddress((void *)callback));
  __ movl(esp, ebx);
  __ pop(esi);
  __ pop(edi);
  __ pop(ebx);
  __ ret();

  return (SPVM_NATIVE_FUNC)LinkCode(masm);
}

void JITX86::DestroyFakeNative(SPVM_NATIVE_FUNC func)
{
  KE_FreeCode(g_pCodeCache, (void *)func);
}

ICompilation *JITX86::StartCompilation()
{
  return new CompData;
}

ICompilation *JITX86::StartCompilation(BaseRuntime *runtime)
{
  return new CompData;
}

void CompData::Abort()
{
  delete this;
}

void JITX86::FreeContextVars(sp_context_t *ctx)
{
  free(((tracker_t *)(ctx->vm[JITVARS_TRACKER]))->pBase);
  delete (tracker_t *)ctx->vm[JITVARS_TRACKER];
}

bool CompData::SetOption(const char *key, const char *val)
{
  if (strcmp(key, SP_JITCONF_DEBUG) == 0)
    return true;
  if (strcmp(key, SP_JITCONF_PROFILE) == 0) {
    profile = atoi(val);

    /** Callbacks must be profiled to profile functions! */
    if ((profile & SP_PROF_FUNCTIONS) == SP_PROF_FUNCTIONS)
      profile |= SP_PROF_CALLBACKS;

    return true;
  }

  return false;
}

int JITX86::InvokeFunction(BaseRuntime *runtime, JitFunction *fn, cell_t *result)
{
  sp_context_t *ctx = runtime->GetBaseContext()->GetCtx();

  // Note that cip, hp, sp are saved and restored by Execute2().
  ctx->cip = fn->GetPCodeAddress();

  InfoVars vars;
  /* vars.esp will be set in the entry code */

  JIT_EXECUTE pfn = (JIT_EXECUTE)m_pJitEntry;
  int err = pfn(&vars, fn->GetEntryAddress(), runtime->plugin()->memory, ctx);

  *result = ctx->rval;
  return err;
}

void *JITX86::AllocCode(size_t size)
{
  return Knight::KE_AllocCode(g_pCodeCache, size);
}

void JITX86::FreeCode(void *code)
{
  KE_FreeCode(g_pCodeCache, code);
}
