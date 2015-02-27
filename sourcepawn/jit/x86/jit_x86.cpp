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
#include "plugin-runtime.h"
#include "plugin-context.h"
#include "watchdog_timer.h"
#include "environment.h"
#include "code-stubs.h"
#include "x86-utils.h"

using namespace sp;

#if defined USE_UNGEN_OPCODES
#include "ungen_opcodes.h"
#endif

#define __ masm.

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

#if 0 && !defined NDEBUG
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

      if (sym->ident == sp::IDENT_FUNCTION && sym->codestart <= offs && sym->codeend > offs)
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

      if (sym->ident == sp::IDENT_FUNCTION && sym->codestart <= offs && sym->codeend > offs)
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

CompiledFunction *
sp::CompileFunction(PluginRuntime *prt, cell_t pcode_offs, int *err)
{
  Compiler cc(prt, pcode_offs);
  CompiledFunction *fun = cc.emit(err);
  if (!fun)
    return NULL;

  // Grab the lock before linking code in, since the watchdog timer will look
  // at this list on another thread.
  ke::AutoLock lock(Environment::get()->lock());

  prt->AddJittedFunction(fun);
  return fun;
}

static int
CompileFromThunk(PluginRuntime *runtime, cell_t pcode_offs, void **addrp, char *pc)
{
  // If the watchdog timer has declared a timeout, we must process it now,
  // and possibly refuse to compile, since otherwise we will compile a
  // function that is not patched for timeouts.
  if (!Environment::get()->watchdog()->HandleInterrupt())
    return SP_ERROR_TIMEOUT;

  CompiledFunction *fn = runtime->GetJittedFunctionByOffset(pcode_offs);
  if (!fn) {
    int err;
    fn = CompileFunction(runtime, pcode_offs, &err);
    if (!fn)
      return err;
  }

#if defined JIT_SPEW
  Environment::get()->debugger()->OnDebugSpew(
      "Patching thunk to %s::%s\n",
      runtime->plugin()->name,
      GetFunctionName(runtime->plugin(), pcode_offs));
#endif

  *addrp = fn->GetEntryAddress();

  /* Right now, we always keep the code RWE */
  *(intptr_t *)(pc - 4) = intptr_t(fn->GetEntryAddress()) - intptr_t(pc);
  return SP_ERROR_NONE;
}

Compiler::Compiler(PluginRuntime *rt, cell_t pcode_offs)
  : env_(Environment::get()),
    rt_(rt),
    context_(rt->GetBaseContext()),
    image_(rt_->image()),
    error_(SP_ERROR_NONE),
    pcode_start_(pcode_offs),
    code_start_(reinterpret_cast<const cell_t *>(rt_->code().bytes + pcode_start_)),
    cip_(code_start_),
    code_end_(reinterpret_cast<const cell_t *>(rt_->code().bytes + rt_->code().length))
{
  size_t nmaxops = rt_->code().length / sizeof(cell_t) + 1;
  jump_map_ = new Label[nmaxops];
}

Compiler::~Compiler()
{
  delete [] jump_map_;
}

CompiledFunction *
Compiler::emit(int *errp)
{
  if (cip_ >= code_end_ || *cip_ != OP_PROC) {
    *errp = SP_ERROR_INVALID_INSTRUCTION;
    return NULL;
  }

#if defined JIT_SPEW
  g_engine1.GetDebugHook()->OnDebugSpew(
      "Compiling function %s::%s\n",
      rt_->Name(),
      GetFunctionName(plugin_, pcode_start_));

  SpewOpcode(plugin_, code_start_, cip_);
#endif

  const cell_t *codeseg = reinterpret_cast<const cell_t *>(rt_->code().bytes);

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

#if defined JIT_SPEW
    SpewOpcode(plugin_, code_start_, cip_);
#endif

    // We assume every instruction is a jump target, so before emitting
    // an opcode, we bind its corresponding label.
    __ bind(&jump_map_[cip_ - codeseg]);

    // Save the start of the opcode for emitCipMap().
    op_cip_ = cip_;

    OPCODE op = (OPCODE)readCell();
    if (!emitOp(op) || error_ != SP_ERROR_NONE) {
      *errp = (error_ == SP_ERROR_NONE) ? SP_ERROR_OUT_OF_MEMORY : error_;
      return NULL;
    }
  }

  emitCallThunks();

  // For each backward jump, emit a little thunk so we can exit from a timeout.
  // Track the offset of where the thunk is, so the watchdog timer can patch it.
  for (size_t i = 0; i < backward_jumps_.length(); i++) {
    BackwardJump &jump = backward_jumps_[i];
    jump.timeout_offset = masm.pc();
    __ call(&throw_timeout_);
    emitCipMapping(jump.cip);
  }

  // This has to come last.
  emitErrorPaths();

  uint8_t *code = LinkCode(env_, masm);
  if (!code) {
    *errp = SP_ERROR_OUT_OF_MEMORY;
    return NULL;
  }

  AutoPtr<FixedArray<LoopEdge>> edges(
    new FixedArray<LoopEdge>(backward_jumps_.length()));
  for (size_t i = 0; i < backward_jumps_.length(); i++) {
    const BackwardJump &jump = backward_jumps_[i];
    edges->at(i).offset = jump.pc;
    edges->at(i).disp32 = int32_t(jump.timeout_offset) - int32_t(jump.pc);
  }

  AutoPtr<FixedArray<CipMapEntry>> cipmap(
    new FixedArray<CipMapEntry>(cip_map_.length()));
  memcpy(cipmap->buffer(), cip_map_.buffer(), cip_map_.length() * sizeof(CipMapEntry));

  return new CompiledFunction(code, masm.length(), pcode_start_, edges.take(), cipmap.take());
}

// No exit frame - error code is returned directly.
static int
InvokePushTracker(PluginContext *cx, uint32_t amount)
{
  return cx->pushTracker(amount);
}

// No exit frame - error code is returned directly.
static int
InvokePopTrackerAndSetHeap(PluginContext *cx)
{
  return cx->popTrackerAndSetHeap();
}

// Error code must be checked in the environment.
static cell_t
InvokeNativeHelper(PluginContext *cx, ucell_t native_idx, cell_t *params)
{
  return cx->invokeNative(native_idx, params);
}

// Error code must be checked in the environment.
static cell_t
InvokeBoundNativeHelper(PluginContext *cx, SPVM_NATIVE_FUNC fn, cell_t *params)
{
  return cx->invokeBoundNative(fn, params);
}

// No exit frame - error code is returned directly.
static int
InvokeGenerateFullArray(PluginContext *cx, uint32_t argc, cell_t *argv, int autozero)
{
  return cx->generateFullArray(argc, argv, autozero);
}

// Exit frame is a JitExitFrameForHelper.
static void
InvokeReportError(int err)
{
  Environment::get()->ReportError(err);
}

// Exit frame is a JitExitFrameForHelper. This is a special function since we
// have to notify the watchdog timer that we're unblocked.
static void
InvokeReportTimeout()
{
  Environment::get()->watchdog()->NotifyTimeoutReceived();
  InvokeReportError(SP_ERROR_TIMEOUT);
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

      // Store the function cip for stack traces.
      __ push(pcode_start_);

      // Align the stack to 16-bytes (each call adds 8 bytes).
      __ subl(esp, 8);
#if defined(DEBUG)
      // Debug guards.
      __ movl(Operand(esp, 0), 0xffaaee00);
      __ movl(Operand(esp, 4), 0xffaaee04);
#endif
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
      jumpOnError(zero, SP_ERROR_DIVIDE_BY_ZERO);

      // A more subtle case; -INT_MIN / -1 yields an overflow exception.
      Label ok;
      __ cmpl(divisor, -1);
      __ j(not_equal, &ok);
      __ cmpl(dividend, 0x80000000);
      jumpOnError(equal, SP_ERROR_INTEGER_OVERFLOW);
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
      // Note: set edi first, since we need esi.
      __ lea(edi, Operand(dat, alt, NoScale));
      __ lea(esi, Operand(dat, pri, NoScale));
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

    // This is the old float cmp, which returns ordered results. In newly
    // compiled code it should not be used or generated.
    //
    // Note that the checks here are inverted: the test is |rhs OP lhs|.
    case OP_FLOATCMP:
    {
      Label bl, ab, done;
      if (MacroAssemblerX86::Features().sse) {
        __ movss(xmm0, Operand(stk, 4));
        __ ucomiss(Operand(stk, 0), xmm0);
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

    case OP_FLOAT_GT:
      emitFloatCmp(above);
      break;

    case OP_FLOAT_GE:
      emitFloatCmp(above_equal);
      break;

    case OP_FLOAT_LE:
      emitFloatCmp(below_equal);
      break;

    case OP_FLOAT_LT:
      emitFloatCmp(below);
      break;

    case OP_FLOAT_EQ:
      emitFloatCmp(equal);
      break;

    case OP_FLOAT_NE:
      emitFloatCmp(not_equal);
      break;

    case OP_FLOAT_NOT:
    {
      if (MacroAssemblerX86::Features().sse) {
        __ xorps(xmm0, xmm0);
        __ ucomiss(Operand(stk, 0), xmm0);
      } else {
        __ fld32(Operand(stk, 0));
        __ fldz();
        __ fucomip(st1);
        __ fstp(st0);
      }

      // See emitFloatCmp() - this is a shorter version.
      Label done;
      __ movl(eax, 1);
      __ j(parity, &done);
      __ set(zero, r8_al);
      __ bind(&done);

      __ addl(stk, 4);
      break;
    }

    case OP_STACK:
    {
      cell_t amount = readCell();
      __ addl(stk, amount);

     if (amount > 0) {
       // Check if the stack went beyond the stack top - usually a compiler error.
       __ cmpl(stk, intptr_t(context_->memory() + context_->HeapSize()));
      jumpOnError(not_below, SP_ERROR_STACKMIN);
     } else {
       // Check if the stack is going to collide with the heap.
       __ movl(tmp, Operand(hpAddr()));
       __ lea(tmp, Operand(dat, ecx, NoScale, STACK_MARGIN));
       __ cmpl(stk, tmp);
       jumpOnError(below, SP_ERROR_STACKLOW);
     }
     break;
    }

    case OP_HEAP:
    {
      cell_t amount = readCell();
      __ movl(alt, Operand(hpAddr()));
      __ addl(Operand(hpAddr()), amount);

      if (amount < 0) {
        __ cmpl(Operand(hpAddr()), context_->DataSize());
        jumpOnError(below, SP_ERROR_HEAPMIN);
      } else {
        __ movl(tmp, Operand(hpAddr()));
        __ lea(tmp, Operand(dat, ecx, NoScale, STACK_MARGIN));
        __ cmpl(tmp, stk);
        jumpOnError(above, SP_ERROR_HEAPLOW);
      }
      break;
    }

    case OP_JUMP:
    {
      Label *target = labelAt(readCell());
      if (!target)
        return false;
      if (target->bound()) {
        __ jmp32(target);
        backward_jumps_.append(BackwardJump(masm.pc(), op_cip_));
      } else {
        __ jmp(target);
      }
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
      if (target->bound()) {
        __ j32(cc, target);
        backward_jumps_.append(BackwardJump(masm.pc(), op_cip_));
      } else {
        __ j(cc, target);
      }
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
      if (target->bound()) {
        __ j32(cc, target);
        backward_jumps_.append(BackwardJump(masm.pc(), op_cip_));
      } else {
        __ j(cc, target);
      }
      break;
    }

    case OP_TRACKER_PUSH_C:
    {
      cell_t amount = readCell();

      __ push(pri);
      __ push(alt);

      __ push(amount * 4);
      __ push(intptr_t(rt_->GetBaseContext()));
      __ call(ExternalAddress((void *)InvokePushTracker));
      __ addl(esp, 8);
      __ testl(eax, eax);
      jumpOnError(not_zero);

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
      __ push(intptr_t(rt_->GetBaseContext()));
      __ call(ExternalAddress((void *)InvokePopTrackerAndSetHeap));
      __ addl(esp, 4);
      __ testl(eax, eax);
      jumpOnError(not_zero);

      __ pop(alt);
      __ pop(pri);
      break;
    }

    // This opcode is used to note where line breaks occur. We don't support
    // live debugging, and if we did, we could build this map from the lines
    // table. So we don't generate any code here.
    case OP_BREAK:
      break;

    // This should never be hit.
    case OP_HALT:
      __ align(16);
      __ movl(pri, readCell());
      __ testl(eax, eax);
      jumpOnError(not_zero);
      break;

    case OP_BOUNDS:
    {
      cell_t value = readCell();
      __ cmpl(eax, value);
      jumpOnError(above, SP_ERROR_ARRAY_BOUNDS);
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
      offset > rt_->code().length ||
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
  __ cmpl(reg, context_->HeapSize());
  jumpOnError(not_below, SP_ERROR_MEMACCESS);

  // Check if we're in the invalid region between hp and sp.
  Label done;
  __ cmpl(reg, Operand(hpAddr()));
  __ j(below, &done);
  __ lea(tmp, Operand(dat, reg, NoScale));
  __ cmpl(tmp, stk);
  jumpOnError(below, SP_ERROR_MEMACCESS);
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
    jumpOnError(not_below, SP_ERROR_HEAPLOW);

    __ shll(tmp, 2);
    __ push(tmp);
    __ push(intptr_t(rt_->GetBaseContext()));
    __ call(ExternalAddress((void *)InvokePushTracker));
    __ addl(esp, 4);
    __ pop(tmp);
    __ shrl(tmp, 2);
    __ testl(eax, eax);
    jumpOnError(not_zero);

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

    // int GenerateArray(cx, vars[], uint32_t, cell_t *, int, unsigned *);
    __ push(autozero ? 1 : 0);
    __ push(stk);
    __ push(val);
    __ push(intptr_t(context_));
    __ call(ExternalAddress((void *)InvokeGenerateFullArray));
    __ addl(esp, 4 * sizeof(void *));

    // restore pri to tmp
    __ pop(tmp);

    __ testl(eax, eax);
    jumpOnError(not_zero);

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
  if (offset % 4 != 0 || uint32_t(offset) >= rt_->code().length) {
    error_ = SP_ERROR_INSTRUCTION_PARAM;
    return false;
  }

  CompiledFunction *fun = rt_->GetJittedFunctionByOffset(offset);
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

  // Map the return address to the cip that started this call.
  emitCipMapping(op_cip_);
  return true;
}

void
Compiler::emitCallThunks()
{
  for (size_t i = 0; i < thunks_.length(); i++) {
    CallThunk *thunk = thunks_[i];

    Label error;
    __ bind(&thunk->call);

    // Get the return address, since that is the call that we need to patch.
    __ movl(eax, Operand(esp, 0));

    // Push an OP_PROC frame as if we already called the function. This helps
    // error reporting.
    __ push(thunk->pcode_offset);
    __ subl(esp, 8);

    // Create the exit frame, then align the stack.
    __ push(0);
    __ movl(ecx, intptr_t(&Environment::get()->exit_frame()));
    __ movl(Operand(ecx, ExitFrame::offsetOfExitNative()), -1);
    __ movl(Operand(ecx, ExitFrame::offsetOfExitSp()), esp);

    // We need to push 4 arguments, and one of them will need an extra word
    // on the stack. Allocate a big block so we're aligned.
    //
    // Note: we add 12 since the push above misaligned the stack.
    static const size_t kStackNeeded = 5 * sizeof(void *);
    static const size_t kStackReserve = ke::Align(kStackNeeded, 16) + 3 * sizeof(void *);
    __ subl(esp, kStackReserve);

    // Set arguments.
    __ movl(Operand(esp, 3 * sizeof(void *)), eax);
    __ lea(edx, Operand(esp, 4 * sizeof(void *)));
    __ movl(Operand(esp, 2 * sizeof(void *)), edx);
    __ movl(Operand(esp, 1 * sizeof(void *)), intptr_t(thunk->pcode_offset));
    __ movl(Operand(esp, 0 * sizeof(void *)), intptr_t(rt_));

    __ call(ExternalAddress((void *)CompileFromThunk));
    __ movl(edx, Operand(esp, 4 * sizeof(void *)));
    __ addl(esp, kStackReserve + 4 * sizeof(void *)); // Drop the exit frame and fake frame.
    __ testl(eax, eax);
    jumpOnError(not_zero);
    __ jmp(edx);
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

  if (native_index >= image_->NumNatives()) {
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

  // Create the exit frame. This is a JitExitFrameForNative, so everything we
  // push up to the return address of the call instruction is reflected in
  // that structure.
  __ movl(eax, intptr_t(&Environment::get()->exit_frame()));
  __ movl(Operand(eax, ExitFrame::offsetOfExitNative()), native_index);
  __ movl(Operand(eax, ExitFrame::offsetOfExitSp()), esp);

  // Save registers.
  __ push(edx);

  // Push the last parameter for the C++ function.
  __ push(stk);

  // Relocate our absolute stk to be dat-relative, and update the context's
  // view.
  __ subl(stk, dat);
  __ movl(eax, intptr_t(context_));
  __ movl(Operand(eax, PluginContext::offsetOfSp()), stk);

  const sp_native_t *native = rt_->GetNative(native_index);
  if ((native->status != SP_NATIVE_BOUND) ||
      (native->flags & (SP_NTVFLAG_OPTIONAL | SP_NTVFLAG_EPHEMERAL)))
  {
    // The native is either unbound, or it could become unbound in the
    // future. Invoke the slower native callback.
    __ push(native_index);
    __ push(intptr_t(rt_->GetBaseContext()));
    __ call(ExternalAddress((void *)InvokeNativeHelper));
  } else {
    // The native is bound so we have a few more guarantees.
    __ push(intptr_t(native->pfn));
    __ push(intptr_t(rt_->GetBaseContext()));
    __ call(ExternalAddress((void *)InvokeBoundNativeHelper));
  }

  // Map the return address to the cip that initiated this call.
  emitCipMapping(op_cip_);

  // Check for errors. Note we jump directly to the return stub since the
  // error has already been reported.
  __ movl(ecx, intptr_t(Environment::get()));
  __ cmpl(Operand(ecx, Environment::offsetOfExceptionCode()), 0);
  __ j(not_zero, &return_reported_error_);
  
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

  cell_t *tbl = (cell_t *)((char *)rt_->code().bytes + offset + sizeof(cell_t));

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
Compiler::emitFloatCmp(ConditionCode cc)
{
  unsigned lhs = 4;
  unsigned rhs = 0;
  if (cc == below || cc == below_equal) {
    // NaN results in ZF=1 PF=1 CF=1
    //
    // ja/jae check for ZF,CF=0 and CF=0. If we make all relational compares
    // look like ja/jae, we'll guarantee all NaN comparisons will fail (which
    // would not be true for jb/jbe, unless we checked with jp).
    if (cc == below)
      cc = above;
    else
      cc = above_equal;
    rhs = 4;
    lhs = 0;
  }

  if (MacroAssemblerX86::Features().sse) {
    __ movss(xmm0, Operand(stk, rhs));
    __ ucomiss(Operand(stk, lhs), xmm0);
  } else {
    __ fld32(Operand(stk, rhs));
    __ fld32(Operand(stk, lhs));
    __ fucomip(st1);
    __ fstp(st0);
  }

  // An equal or not-equal needs special handling for the parity bit.
  if (cc == equal || cc == not_equal) {
    // If NaN, PF=1, ZF=1, and E/Z tests ZF=1.
    //
    // If NaN, PF=1, ZF=1 and NE/NZ tests Z=0. But, we want any != with NaNs
    // to return true, including NaN != NaN.
    //
    // To make checks simpler, we set |eax| to the expected value of a NaN
    // beforehand. This also clears the top bits of |eax| for setcc.
    Label done;
    __ movl(eax, (cc == equal) ? 0 : 1);
    __ j(parity, &done);
    __ set(cc, r8_al);
    __ bind(&done);
  } else {
    __ movl(eax, 0);
    __ set(cc, r8_al);
  }
  __ addl(stk, 8);
}

void
Compiler::jumpOnError(ConditionCode cc, int err)
{
  // Note: we accept 0 for err. In this case we expect the error to be in eax.
  {
    ErrorPath path(op_cip_, err);
    error_paths_.append(path);
  }

  ErrorPath &path = error_paths_.back();
  __ j(cc, &path.label);
}

void
Compiler::emitErrorPaths()
{
  // For each path that had an error check, bind it to an error routine and
  // add it to the cip map. What we'll get is something like:
  //
  //   cmp dividend, 0
  //   jz error_thunk_0
  //
  // error_thunk_0:
  //   call integer_overflow
  //
  // integer_overflow:
  //   mov eax, SP_ERROR_DIVIDE_BY_ZERO
  //   jmp report_error
  //
  // report_error:
  //   create exit frame
  //   push eax
  //   call InvokeReportError(int err)
  //
  for (size_t i = 0; i < error_paths_.length(); i++) {
    ErrorPath &path = error_paths_[i];

    // If there's no error code, it should be in eax. Otherwise we'll jump to
    // a path that sets eax to a hardcoded value.
    __ bind(&path.label);
    if (path.err == 0)
      __ call(&report_error_);
    else
      __ call(&throw_error_code_[path.err]);

    emitCipMapping(path.cip);
  }

  emitThrowPathIfNeeded(SP_ERROR_DIVIDE_BY_ZERO);
  emitThrowPathIfNeeded(SP_ERROR_STACKLOW);
  emitThrowPathIfNeeded(SP_ERROR_STACKMIN);
  emitThrowPathIfNeeded(SP_ERROR_ARRAY_BOUNDS);
  emitThrowPathIfNeeded(SP_ERROR_MEMACCESS);
  emitThrowPathIfNeeded(SP_ERROR_HEAPLOW);
  emitThrowPathIfNeeded(SP_ERROR_HEAPMIN);
  emitThrowPathIfNeeded(SP_ERROR_INTEGER_OVERFLOW);

  if (report_error_.used()) {
    __ bind(&report_error_);

    // Create the exit frame. We always get here through a call from the opcode
    // (and always via an out-of-line thunk).
    __ movl(ecx, intptr_t(&Environment::get()->exit_frame()));
    __ movl(Operand(ecx, ExitFrame::offsetOfExitNative()), -1);
    __ movl(Operand(ecx, ExitFrame::offsetOfExitSp()), esp);

    // Since the return stub wipes out the stack, we don't need to subl after
    // the call.
    __ push(eax);
    __ call(ExternalAddress((void *)InvokeReportError));
    __ jmp(ExternalAddress(env_->stubs()->ReturnStub()));
  }

  // We get here if we know an exception is already pending.
  if (return_reported_error_.used()) {
    __ bind(&return_reported_error_);
    __ movl(eax, intptr_t(Environment::get()));
    __ movl(eax, Operand(eax, Environment::offsetOfExceptionCode()));
    __ jmp(ExternalAddress(env_->stubs()->ReturnStub()));
  }

  // The timeout uses a special stub.
  if (throw_timeout_.used()) {
    __ bind(&throw_timeout_);

    // Create the exit frame.
    __ movl(ecx, intptr_t(&Environment::get()->exit_frame()));
    __ movl(Operand(ecx, ExitFrame::offsetOfExitNative()), -1);
    __ movl(Operand(ecx, ExitFrame::offsetOfExitSp()), esp);

    // Since the return stub wipes out the stack, we don't need to subl after
    // the call.
    __ call(ExternalAddress((void *)InvokeReportTimeout));
    __ jmp(ExternalAddress(env_->stubs()->ReturnStub()));
  }
}

void
Compiler::emitThrowPathIfNeeded(int err)
{
  assert(err < SP_MAX_ERROR_CODES);

  if (!throw_error_code_[err].used())
    return;

  __ bind(&throw_error_code_[err]);
  __ movl(eax, err);
  __ jmp(&report_error_);
}
