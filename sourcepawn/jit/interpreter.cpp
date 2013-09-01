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
#include <limits.h>
#include <string.h>
#include "interpreter.h"
#include "opcodes.h"
#include "watchdog_timer.h"

#define STACK_MARGIN 64

using namespace SourcePawn;

static inline bool
IsValidOffset(uint32_t cip)
{
  return cip % 4 == 0;
}

static inline cell_t
Read(const sp_plugin_t *plugin, cell_t offset)
{
  return *reinterpret_cast<cell_t *>(plugin->memory + offset);
}

static inline void
Write(const sp_plugin_t *plugin, cell_t offset, cell_t value)
{
  *reinterpret_cast<cell_t *>(plugin->memory + offset) = value;
}

static inline cell_t *
Jump(const sp_plugin_t *plugin, sp_context_t *ctx, cell_t target)
{
  if (!IsValidOffset(target) || uint32_t(target) >= plugin->pcode_size) {
    ctx->err = SP_ERROR_INVALID_INSTRUCTION;
    return NULL;
  }

  return reinterpret_cast<cell_t *>(plugin->pcode + target);
}

static inline cell_t *
JumpTarget(const sp_plugin_t *plugin, sp_context_t *ctx, cell_t *cip, bool cond)
{
  if (!cond)
    return cip + 1;

  cell_t target = *cip;
  if (!IsValidOffset(target) || uint32_t(target) >= plugin->pcode_size) {
    ctx->err = SP_ERROR_INVALID_INSTRUCTION;
    return NULL;
  }

  cell_t *next = reinterpret_cast<cell_t *>(plugin->pcode + target);
  if (next < cip && !g_WatchdogTimer.HandleInterrupt()) {
    ctx->err = SP_ERROR_TIMEOUT;
    return NULL;
  }

  return next;
}

static inline bool
CheckAddress(const sp_plugin_t *plugin, sp_context_t *ctx, cell_t *stk, cell_t addr)
{
  if (uint32_t(addr) >= plugin->mem_size) {
    ctx->err = SP_ERROR_MEMACCESS;
    return false;
  }

  if (addr < ctx->hp)
    return true;

  if (reinterpret_cast<cell_t *>(plugin->memory + addr) < stk) {
    ctx->err = SP_ERROR_MEMACCESS;
    return false;
  }

  return true;
}

int
PopTrackerAndSetHeap(BaseRuntime *rt)
{
  sp_context_t *ctx = rt->GetBaseContext()->GetCtx();
  tracker_t *trk = ctx->tracker;
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

int
PushTracker(sp_context_t *ctx, size_t amount)
{
  tracker_t *trk = ctx->tracker;

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

cell_t
NativeCallback(sp_context_t *ctx, ucell_t native_idx, cell_t *params)
{
  cell_t save_sp = ctx->sp;
  cell_t save_hp = ctx->hp;

  ctx->n_idx = native_idx;

  sp_native_t *native = &ctx->plugin->natives[native_idx];

  if (native->status == SP_NATIVE_UNBOUND) {
    ctx->n_err = SP_ERROR_INVALID_NATIVE;
    return 0;
  }

  cell_t result = native->pfn(ctx->basecx, params);

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

static inline bool
GenerateArray(BaseRuntime *rt, sp_context_t *ctx, cell_t dims, cell_t *stk, bool autozero)
{
  if (dims == 1) {
    uint32_t size = *stk;
    if (size == 0 || !ke::IsUint32MultiplySafe(size, 4)) {
      ctx->err = SP_ERROR_ARRAY_TOO_BIG;
      return false;
    }
    *stk = ctx->hp;

    uint32_t bytes = size * 4;

    ctx->hp += bytes;
    if (uintptr_t(ctx->plugin->memory + ctx->hp) >= uintptr_t(stk)) {
      ctx->err = SP_ERROR_HEAPLOW;
      return false;
    }

    if ((ctx->err = PushTracker(ctx, bytes)) != SP_ERROR_NONE)
      return false;

    if (autozero)
      memset(ctx->plugin->memory + ctx->hp, 0, bytes);

    return true;
  }

  if ((ctx->err = GenerateFullArray(rt, dims, stk, autozero)) != SP_ERROR_NONE)
    return false;

  return true;
}

int
Interpret(BaseRuntime *rt, uint32_t aCodeStart, cell_t *rval)
{
  const sp_plugin_t *plugin = rt->plugin();
  cell_t *code = reinterpret_cast<cell_t *>(plugin->pcode);
  cell_t *codeend = reinterpret_cast<cell_t *>(plugin->pcode + plugin->pcode_size);

  if (!IsValidOffset(aCodeStart) || aCodeStart > plugin->pcode_size)
    return SP_ERROR_INVALID_INSTRUCTION;

  sp_context_t *ctx = rt->GetBaseContext()->GetCtx();
  ctx->err = SP_ERROR_NONE;

  // Save the original frm. BaseContext won't, and if we error, we won't hit
  // the stack unwinding code.
  cell_t orig_frm = ctx->frm;

  cell_t pri = 0;
  cell_t alt = 0;
  cell_t *cip = code + (aCodeStart / 4);
  cell_t *stk = reinterpret_cast<cell_t *>(plugin->memory + ctx->sp);

  for (;;) {
    if (cip >= codeend) {
      ctx->err = SP_ERROR_INVALID_INSTRUCTION;
      goto error;
    }

#if 0
    SpewOpcode(plugin, reinterpret_cast<cell_t *>(plugin->pcode + aCodeStart), cip);
#endif

    OPCODE op = (OPCODE)*cip++;

    switch (op) {
      case OP_MOVE_PRI:
        pri = alt;
        break;
      case OP_MOVE_ALT:
        alt = pri;
        break;

      case OP_XCHG:
      {
        cell_t tmp = pri;
        pri = alt;
        alt = tmp;
        break;
      }

      case OP_ZERO:
        Write(plugin, *cip++, 0);
        break;

      case OP_ZERO_S:
        Write(plugin, *cip++, 0);
        break;

      case OP_PUSH_PRI:
        *--stk = pri;
        break;
      case OP_PUSH_ALT:
        *--stk = alt;
        break;

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
          *--stk = *cip++;
        } while (i++ < n);
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

        do {
          cell_t addr = ctx->frm + *cip++;
          *--stk = addr;
        } while (i++ < n);
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
          cell_t value = Read(plugin, ctx->frm + *cip++);
          *--stk = value;
        } while (i++ < n);
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
          cell_t value = Read(plugin, *cip++);
          *--stk = value;
        } while (i++ < n);
        break;
      }

      case OP_ZERO_PRI:
        pri = 0;
        break;
      case OP_ZERO_ALT:
        alt = 0;
        break;

      case OP_ADD:
        pri += alt;
        break;

      case OP_SUB:
        pri -= alt;
        break;

      case OP_SUB_ALT:
        pri = alt - pri;
        break;

      case OP_PROC:
      {
        *--stk = ctx->frm;
        *--stk = 0;
        ctx->frm = uintptr_t(stk) - uintptr_t(plugin->memory);
        break;
      }

      case OP_IDXADDR_B:
        pri <<= *cip++;
        pri += alt;
        break;

      case OP_SHL:
        pri <<= alt;
        break;

      case OP_SHR:
        pri = unsigned(pri) >> unsigned(alt);
        break;

      case OP_SSHR:
        pri >>= alt;
        break;

      case OP_SHL_C_PRI:
        pri <<= *cip++;
        break;
      case OP_SHL_C_ALT:
        alt <<= *cip++;
        break;

      case OP_SHR_C_PRI:
        pri >>= *cip++;
        break;
      case OP_SHR_C_ALT:
        alt >>= *cip++;
        break;

      case OP_SMUL:
        pri *= alt;
        break;

      case OP_NOT:
        pri = pri ? 0 : 1;
        break;

      case OP_NEG:
        pri = -pri;
        break;

      case OP_XOR:
        pri ^= alt;
        break;

      case OP_OR:
        pri |= alt;
        break;

      case OP_AND:
        pri &= alt;
        break;

      case OP_INVERT:
        pri = ~pri;
        break;

      case OP_ADD_C:
        pri += *cip++;
        break;

      case OP_SMUL_C:
        pri *= *cip++;
        break;

      case OP_EQ:
        pri = pri == alt;
        break;

      case OP_NEQ:
        pri = pri != alt;
        break;

      case OP_SLESS:
        pri = pri < alt;
        break;

      case OP_SLEQ:
        pri = pri <= alt;
        break;

      case OP_SGRTR:
        pri = pri > alt;
        break;
        
      case OP_SGEQ:
        pri = pri >= alt;
        break;

      case OP_EQ_C_PRI:
        pri = pri == *cip++;
        break;
      case OP_EQ_C_ALT:
        pri = alt == *cip++;
        break;

      case OP_INC_PRI:
        pri++;
        break;
      case OP_INC_ALT:
        alt++;
        break;

      case OP_INC:
      {
        cell_t offset = *cip++;
        Write(plugin, offset, Read(plugin, offset) + 1);
        break;
      }

      case OP_INC_S:
      {
        cell_t offset = *cip++;
        cell_t value = Read(plugin, ctx->frm + offset);
        Write(plugin, ctx->frm + offset, value + 1);
        break;
      }

      case OP_INC_I:
        if (!CheckAddress(plugin, ctx, stk, pri))
          goto error;
        Write(plugin, pri, Read(plugin, pri) + 1);
        break;

      case OP_DEC_PRI:
        pri--;
        break;
      case OP_DEC_ALT:
        alt--;
        break;

      case OP_DEC:
      {
        cell_t offset = *cip++;
        Write(plugin, offset, Read(plugin, offset) - 1);
        break;
      }

      case OP_DEC_S:
      {
        cell_t offset = *cip++;
        cell_t value = Read(plugin, ctx->frm + offset);
        Write(plugin, ctx->frm + offset, value - 1);
        break;
      }

      case OP_DEC_I:
        if (!CheckAddress(plugin, ctx, stk, pri))
          goto error;
        Write(plugin, pri, Read(plugin, pri) - 1);
        break;

      case OP_LOAD_PRI:
        pri = Read(plugin, *cip++);
        break;
      case OP_LOAD_ALT:
        alt = Read(plugin, *cip++);
        break;

      case OP_LOAD_S_PRI:
        pri = Read(plugin, ctx->frm + *cip++);
        break;
      case OP_LOAD_S_ALT:
        alt = Read(plugin, ctx->frm + *cip++);
        break;

      case OP_LOAD_S_BOTH:
        pri = Read(plugin, ctx->frm + *cip++);
        alt = Read(plugin, ctx->frm + *cip++);
        break;

      case OP_LREF_S_PRI:
      {
        pri = Read(plugin, ctx->frm + *cip++);
        pri = Read(plugin, pri);
        break;
      }

      case OP_LREF_S_ALT:
      {
        alt = Read(plugin, ctx->frm + *cip++);
        alt = Read(plugin, alt);
        break;
      }

      case OP_CONST_PRI:
        pri = *cip++;
        break;
      case OP_CONST_ALT:
        alt = *cip++;
        break;

      case OP_ADDR_PRI:
        pri = ctx->frm + *cip++;
        break;
      case OP_ADDR_ALT:
        alt = ctx->frm + *cip++;
        break;

      case OP_STOR_PRI:
        Write(plugin, *cip++, pri);
        break;
      case OP_STOR_ALT:
        Write(plugin, *cip++, alt);
        break;

      case OP_STOR_S_PRI:
        Write(plugin, ctx->frm + *cip++, pri);
        break;
      case OP_STOR_S_ALT:
        Write(plugin, ctx->frm +*cip++, alt);
        break;

      case OP_IDXADDR:
        pri = alt + pri * 4;
        break;

      case OP_SREF_S_PRI:
      {
        cell_t offset = *cip++;
        cell_t addr = Read(plugin, ctx->frm + offset);
        Write(plugin, addr, pri);
        break;
      }

      case OP_SREF_S_ALT:
      {
        cell_t offset = *cip++;
        cell_t addr = Read(plugin, ctx->frm + offset);
        Write(plugin, addr, alt);
        break;
      }

      case OP_POP_PRI:
        pri = *stk++;
        break;
      case OP_POP_ALT:
        alt = *stk++;
        break;

      case OP_SWAP_PRI:
      case OP_SWAP_ALT:
      {
        cell_t reg = (op == OP_SREF_S_PRI) ? pri : alt;
        cell_t temp = *stk;
        *stk = reg;
        reg = temp;
        break;
      }

      case OP_LIDX:
        pri = alt + pri * 4;
        if (!CheckAddress(plugin, ctx, stk, pri))
          goto error;
        pri = Read(plugin, pri);
        break;

      case OP_LIDX_B:
      {
        cell_t val = *cip++;
        pri = alt + (pri << val);
        if (!CheckAddress(plugin, ctx, stk, pri))
          goto error;
        pri = Read(plugin, pri);
        break;
      }

      case OP_CONST:
      {
        cell_t offset = *cip++;
        cell_t value = *cip++;
        Write(plugin, offset, value);
        break;
      }

      case OP_CONST_S:
      {
        cell_t offset = *cip++;
        cell_t value = *cip++;
        Write(plugin, ctx->frm + offset, value);
        break;
      }

      case OP_LOAD_I:
        if (!CheckAddress(plugin, ctx, stk, pri))
          goto error;
        pri = Read(plugin, pri);
        break;

      case OP_STOR_I:
        if (!CheckAddress(plugin, ctx, stk, alt))
          goto error;
        Write(plugin, alt, pri);
        break;

      case OP_SDIV:
      case OP_SDIV_ALT:
      {
        cell_t dividend = (op == OP_SDIV) ? pri : alt;
        cell_t divisor = (op == OP_SDIV) ? alt : pri;
        if (divisor == 0) {
          ctx->err = SP_ERROR_DIVIDE_BY_ZERO;
          goto error;
        }
        if (dividend == INT_MIN && divisor == -1) {
          ctx->err = SP_ERROR_INTEGER_OVERFLOW;
          goto error;
        }
        pri = dividend / divisor;
        alt = dividend % divisor;
        break;
      }
      
      case OP_LODB_I:
      {
        cell_t val = *cip++;
        if (!CheckAddress(plugin, ctx, stk, pri))
          goto error;
        pri = Read(plugin, pri);
        if (val == 1)
          pri &= 0xff;
        else if (val == 2)
          pri &= 0xffff;
        break;
      }

      case OP_STRB_I:
      {
        cell_t val = *cip++;
        if (!CheckAddress(plugin, ctx, stk, alt))
          goto error;
        if (val == 1)
          *reinterpret_cast<int8_t *>(plugin->memory + alt) = pri;
        else if (val == 2)
          *reinterpret_cast<int16_t *>(plugin->memory + alt) = pri;
        else if (val == 4)
          *reinterpret_cast<int32_t *>(plugin->memory + alt) = pri;
        break;
      }

      case OP_RETN:
      {
        stk++;
        ctx->frm = *stk++;
        stk += *stk + 1;
        *rval = pri;
        ctx->err = SP_ERROR_NONE;
        goto done;
      }

      case OP_MOVS:
      {
        uint8_t *src = plugin->memory + pri;
        uint8_t *dest = plugin->memory + alt;
        memcpy(dest, src, *cip++);
        break;
      }

      case OP_FILL:
      {
        uint8_t *dest = plugin->memory + alt;
        memset(dest, pri, *cip++);
        break;
      }

      case OP_STRADJUST_PRI:
        pri += 4;
        pri >>= 2;
        break;

      case OP_STACK:
      {
        cell_t amount = *cip++;
        if (!IsValidOffset(amount)) {
          ctx->err = SP_ERROR_INVALID_INSTRUCTION;
          goto error;
        }

        stk += amount / 4;
        if (amount > 0) {
          if (uintptr_t(stk) >= uintptr_t(plugin->memory + plugin->mem_size)) {
            ctx->err = SP_ERROR_STACKMIN;
            goto error;
          }
        } else {
          if (uintptr_t(stk) < uintptr_t(plugin->memory + ctx->hp + STACK_MARGIN)) {
            ctx->err = SP_ERROR_STACKLOW;
            goto error;
          }
        }
        break;
      }

      case OP_HEAP:
      {
        cell_t amount = *cip++;

        alt = ctx->hp;
        ctx->hp += amount;

        if (amount > 0) {
          if (uintptr_t(plugin->memory + ctx->hp) > uintptr_t(stk)) {
            ctx->err = SP_ERROR_HEAPLOW;
            goto error;
          }
        } else {
          if (uint32_t(ctx->hp) < plugin->data_size) {
            ctx->err = SP_ERROR_HEAPMIN;
            goto error;
          }
        }
        break;
      }

      case OP_JUMP:
        if ((cip = JumpTarget(plugin, ctx, cip, true)) == NULL)
          goto error;
        break;

      case OP_JZER:
        if ((cip = JumpTarget(plugin, ctx, cip, pri == 0)) == NULL)
          goto error;
        break;
      case OP_JNZ:
        if ((cip = JumpTarget(plugin, ctx, cip, pri != 0)) == NULL)
          goto error;
        break;

      case OP_JEQ:
        if ((cip = JumpTarget(plugin, ctx, cip, pri == alt)) == NULL)
          goto error;
        break;
      case OP_JNEQ:
        if ((cip = JumpTarget(plugin, ctx, cip, pri != alt)) == NULL)
          goto error;
        break;
      case OP_JSLESS:
        if ((cip = JumpTarget(plugin, ctx, cip, pri < alt)) == NULL)
          goto error;
        break;
      case OP_JSLEQ:
        if ((cip = JumpTarget(plugin, ctx, cip, pri <= alt)) == NULL)
          goto error;
        break;
      case OP_JSGRTR:
        if ((cip = JumpTarget(plugin, ctx, cip, pri > alt)) == NULL)
          goto error;
        break;
      case OP_JSGEQ:
        if ((cip = JumpTarget(plugin, ctx, cip, pri >= alt)) == NULL)
          goto error;
        break;

      case OP_TRACKER_PUSH_C:
      {
        cell_t amount = *cip++;
        int error = PushTracker(ctx, amount * 4);
        if (error != SP_ERROR_NONE) {
          ctx->err = error;
          goto error;
        }
        break;
      }

      case OP_TRACKER_POP_SETHEAP:
      {
        int error = PopTrackerAndSetHeap(rt);
        if (error != SP_ERROR_NONE) {
          ctx->err = error;
          goto error;
        }
        break;
      }

      case OP_BREAK:
        ctx->cip = uintptr_t(cip - 1) - uintptr_t(plugin->pcode);
        break;

      case OP_BOUNDS:
      {
        cell_t value = *cip++;
        if (uint32_t(pri) > uint32_t(value)) {
          ctx->err = SP_ERROR_ARRAY_BOUNDS;
          goto error;
        }
        break;
      }

      case OP_CALL:
      {
        cell_t offset = *cip++;

        if (!IsValidOffset(offset) || uint32_t(offset) >= plugin->pcode_size) {
          ctx->err = SP_ERROR_INSTRUCTION_PARAM;
          goto error;
        }

        if (ctx->rp >= SP_MAX_RETURN_STACK) {
          ctx->err = SP_ERROR_STACKLOW;
          goto error;
        }

        // For debugging.
        uintptr_t rcip = uintptr_t(cip - 2) - uintptr_t(plugin->pcode);
        ctx->rstk_cips[ctx->rp++] = rcip;
        ctx->cip = offset;
        ctx->sp = uintptr_t(stk) - uintptr_t(plugin->memory);

        int err = Interpret(rt, offset, &pri);

        stk = reinterpret_cast<cell_t *>(plugin->memory + ctx->sp);
        ctx->cip = rcip;
        ctx->rp--;

        if (err != SP_ERROR_NONE)
          goto error;
        break;
      }

      case OP_GENARRAY:
      case OP_GENARRAY_Z:
      {
        cell_t val = *cip++;
        if (!GenerateArray(rt, ctx, val, stk, op == OP_GENARRAY_Z))
          goto error;

        stk += (val - 1) * 4;
        break;
      }

      case OP_SYSREQ_C:
      case OP_SYSREQ_N:
      {
        uint32_t native_index = *cip++;

        if (native_index >= plugin->num_natives) {
          ctx->err = SP_ERROR_INSTRUCTION_PARAM;
          goto error;
        }

        uint32_t num_params;
        if (op == OP_SYSREQ_N) {
          num_params = *cip++;
          *--stk = num_params;
        }

        ctx->sp = uintptr_t(stk) - uintptr_t(plugin->memory);
        pri = NativeCallback(ctx, native_index, stk);
        if (ctx->n_err != SP_ERROR_NONE) {
          ctx->err = ctx->n_err;
          goto error;
        }

        if (op == OP_SYSREQ_N)
          stk += num_params + 1;
        break;
      }

      case OP_SWITCH:
      {
        cell_t offset = *cip++;
        cell_t *table = reinterpret_cast<cell_t *>(plugin->pcode + offset + sizeof(cell_t));

        size_t ncases = *table++;
        cell_t target = *table++;   // default case

        for (size_t i = 0; i < ncases; i++) {
          if (table[i * 2] == pri) {
            target = table[i * 2 + 1];
            break;
          }
        }

        if ((cip = Jump(plugin, ctx, target)) == NULL)
          goto error;
        break;
      }

      default:
      {
        ctx->err = SP_ERROR_INVALID_INSTRUCTION;
        goto error;
      }
    } // switch
  }

 done:
  assert(orig_frm == ctx->frm);
  ctx->sp = uintptr_t(stk) - uintptr_t(plugin->memory);
  return ctx->err;

 error:
  ctx->frm = orig_frm;
  goto done;
}

