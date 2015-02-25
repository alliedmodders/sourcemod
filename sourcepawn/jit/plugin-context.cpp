// vim: set sts=2 ts=8 sw=2 tw=99 et:
// 
// Copyright (C) 2006-2015 AlliedModders LLC
// 
// This file is part of SourcePawn. SourcePawn is free software: you can
// redistribute it and/or modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// You should have received a copy of the GNU General Public License along with
// SourcePawn. If not, see http://www.gnu.org/licenses/.
//
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <limits.h>
#include <sp_vm_api.h>
#include "plugin-context.h"
#include "watchdog_timer.h"
#include "x86/jit_x86.h"
#include "interpreter.h"
#include "environment.h"

using namespace SourcePawn;

#define CELLBOUNDMAX  (INT_MAX/sizeof(cell_t))
#define STACKMARGIN    ((cell_t)(16*sizeof(cell_t)))

PluginContext::PluginContext(PluginRuntime *pRuntime)
{
  m_pRuntime = pRuntime;

  m_InExec = false;
  m_CustomMsg = false;

  /* Initialize the null references */
  uint32_t index;
  if (FindPubvarByName("NULL_VECTOR", &index) == SP_ERROR_NONE) {
    sp_pubvar_t *pubvar;
    GetPubvarByIndex(index, &pubvar);
    m_pNullVec = pubvar->offs;
  } else {
    m_pNullVec = NULL;
  }

  if (FindPubvarByName("NULL_STRING", &index) == SP_ERROR_NONE) {
    sp_pubvar_t *pubvar;
    GetPubvarByIndex(index, &pubvar);
    m_pNullString = pubvar->offs;
  } else {
    m_pNullString = NULL;
  }

  m_ctx.hp = m_pRuntime->plugin()->data_size;
  m_ctx.sp = m_pRuntime->plugin()->mem_size - sizeof(cell_t);
  m_ctx.frm = m_ctx.sp;
  rp_ = 0;
  last_native_ = -1;
  native_error_ = SP_ERROR_NONE;

  tracker_.pBase = (ucell_t *)malloc(1024);
  tracker_.pCur = tracker_.pBase;
  tracker_.size = 1024 / sizeof(cell_t);
  m_ctx.basecx = this;
  m_ctx.plugin = const_cast<sp_plugin_t *>(pRuntime->plugin());
}

PluginContext::~PluginContext()
{
  free(tracker_.pBase);
}

IVirtualMachine *
PluginContext::GetVirtualMachine()
{
  return NULL;
}

sp_context_t *
PluginContext::GetContext()
{
  return reinterpret_cast<sp_context_t *>((IPluginContext * )this);
}

sp_context_t *
PluginContext::GetCtx()
{
  return &m_ctx;
}

bool
PluginContext::IsDebugging()
{
  return true;
}

int
PluginContext::SetDebugBreak(void *newpfn, void *oldpfn)
{
  return SP_ERROR_ABORTED;
}

IPluginDebugInfo *
PluginContext::GetDebugInfo()
{
  return NULL;
}

int
PluginContext::Execute(uint32_t code_addr, cell_t *result)
{
  return SP_ERROR_ABORTED;
}

void
PluginContext::SetErrorMessage(const char *msg, va_list ap)
{
  m_CustomMsg = true;

  vsnprintf(m_MsgCache, sizeof(m_MsgCache), msg, ap);
}

void
PluginContext::_SetErrorMessage(const char *msg, ...)
{
  va_list ap;
  va_start(ap, msg);
  SetErrorMessage(msg, ap);
  va_end(ap);
}

cell_t
PluginContext::ThrowNativeErrorEx(int error, const char *msg, ...)
{
  if (!m_InExec)
    return 0;

  native_error_ = error;
  
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    SetErrorMessage(msg, ap);
    va_end(ap);
  }

  return 0;
}

cell_t
PluginContext::ThrowNativeError(const char *msg, ...)
{
  if (!m_InExec)
    return 0;

  native_error_ = SP_ERROR_NATIVE;

  if (msg) {
    va_list ap;
    va_start(ap, msg);
    SetErrorMessage(msg, ap);
    va_end(ap);
  }

  return 0;
}

int
PluginContext::HeapAlloc(unsigned int cells, cell_t *local_addr, cell_t **phys_addr)
{
  cell_t *addr;
  ucell_t realmem;

#if 0
  if (cells > CELLBOUNDMAX)
  {
    return SP_ERROR_ARAM;
  }
#else
  assert(cells < CELLBOUNDMAX);
#endif

  realmem = cells * sizeof(cell_t);

  /**
   * Check if the space between the heap and stack is sufficient.
   */
  if ((cell_t)(m_ctx.sp - m_ctx.hp - realmem) < STACKMARGIN)
    return SP_ERROR_HEAPLOW;

  addr = (cell_t *)(m_pRuntime->plugin()->memory + m_ctx.hp);
  /* store size of allocation in cells */
  *addr = (cell_t)cells;
  addr++;
  m_ctx.hp += sizeof(cell_t);

  *local_addr = m_ctx.hp;

  if (phys_addr)
    *phys_addr = addr;

  m_ctx.hp += realmem;

  return SP_ERROR_NONE;
}

int
PluginContext::HeapPop(cell_t local_addr)
{
  cell_t cellcount;
  cell_t *addr;

  /* check the bounds of this address */
  local_addr -= sizeof(cell_t);
  if (local_addr < (cell_t)m_pRuntime->plugin()->data_size || local_addr >= m_ctx.sp)
    return SP_ERROR_INVALID_ADDRESS;

  addr = (cell_t *)(m_pRuntime->plugin()->memory + local_addr);
  cellcount = (*addr) * sizeof(cell_t);
  /* check if this memory count looks valid */
  if ((signed)(m_ctx.hp - cellcount - sizeof(cell_t)) != local_addr)
    return SP_ERROR_INVALID_ADDRESS;

  m_ctx.hp = local_addr;

  return SP_ERROR_NONE;
}


int
PluginContext::HeapRelease(cell_t local_addr)
{
  if (local_addr < (cell_t)m_pRuntime->plugin()->data_size)
    return SP_ERROR_INVALID_ADDRESS;

  m_ctx.hp = local_addr - sizeof(cell_t);

  return SP_ERROR_NONE;
}

int
PluginContext::FindNativeByName(const char *name, uint32_t *index)
{
  return m_pRuntime->FindNativeByName(name, index);
}

int
PluginContext::GetNativeByIndex(uint32_t index, sp_native_t **native)
{
  return m_pRuntime->GetNativeByIndex(index, native);
}

uint32_t
PluginContext::GetNativesNum()
{
  return m_pRuntime->GetNativesNum();
}

int
PluginContext::FindPublicByName(const char *name, uint32_t *index)
{
  return m_pRuntime->FindPublicByName(name, index);
}

int
PluginContext::GetPublicByIndex(uint32_t index, sp_public_t **pblic)
{
  return m_pRuntime->GetPublicByIndex(index, pblic);
}

uint32_t
PluginContext::GetPublicsNum()
{
  return m_pRuntime->GetPublicsNum();
}

int
PluginContext::GetPubvarByIndex(uint32_t index, sp_pubvar_t **pubvar)
{
  return m_pRuntime->GetPubvarByIndex(index, pubvar);
}

int
PluginContext::FindPubvarByName(const char *name, uint32_t *index)
{
  return m_pRuntime->FindPubvarByName(name, index);
}

int
PluginContext::GetPubvarAddrs(uint32_t index, cell_t *local_addr, cell_t **phys_addr)
{
  return m_pRuntime->GetPubvarAddrs(index, local_addr, phys_addr);
}

uint32_t
PluginContext::GetPubVarsNum()
{
  return m_pRuntime->GetPubVarsNum();
}

int
PluginContext::BindNatives(const sp_nativeinfo_t *natives, unsigned int num, int overwrite)
{
  return SP_ERROR_ABORTED;
}

int
PluginContext::BindNative(const sp_nativeinfo_t *native)
{
  return SP_ERROR_ABORTED;
}

int
PluginContext::BindNativeToIndex(uint32_t index, SPVM_NATIVE_FUNC func)
{
  return SP_ERROR_ABORTED;
}

int
PluginContext::BindNativeToAny(SPVM_NATIVE_FUNC native)
{
  return SP_ERROR_ABORTED;
}

int
PluginContext::LocalToPhysAddr(cell_t local_addr, cell_t **phys_addr)
{
  if (((local_addr >= m_ctx.hp) && (local_addr < m_ctx.sp)) ||
      (local_addr < 0) || ((ucell_t)local_addr >= m_pRuntime->plugin()->mem_size))
  {
    return SP_ERROR_INVALID_ADDRESS;
  }

  if (phys_addr)
    *phys_addr = (cell_t *)(m_pRuntime->plugin()->memory + local_addr);

  return SP_ERROR_NONE;
}

int
PluginContext::PushCell(cell_t value)
{
  return SP_ERROR_ABORTED;
}

int
PluginContext::PushCellsFromArray(cell_t array[], unsigned int numcells)
{
  return SP_ERROR_ABORTED;
}

int
PluginContext::PushCellArray(cell_t *local_addr, cell_t **phys_addr, cell_t array[], unsigned int numcells)
{
  return SP_ERROR_ABORTED;
}

int
PluginContext::LocalToString(cell_t local_addr, char **addr)
{
  if (((local_addr >= m_ctx.hp) && (local_addr < m_ctx.sp)) ||
      (local_addr < 0) || ((ucell_t)local_addr >= m_pRuntime->plugin()->mem_size))
  {
    return SP_ERROR_INVALID_ADDRESS;
  }
  *addr = (char *)(m_pRuntime->plugin()->memory + local_addr);

  return SP_ERROR_NONE;
}

int
PluginContext::PushString(cell_t *local_addr, char **phys_addr, const char *string)
{
  return SP_ERROR_ABORTED;
}

int
PluginContext::StringToLocal(cell_t local_addr, size_t bytes, const char *source)
{
  char *dest;
  size_t len;

  if (((local_addr >= m_ctx.hp) && (local_addr < m_ctx.sp)) ||
      (local_addr < 0) || ((ucell_t)local_addr >= m_pRuntime->plugin()->mem_size))
  {
    return SP_ERROR_INVALID_ADDRESS;
  }

  if (bytes == 0)
    return SP_ERROR_NONE;

  len = strlen(source);
  dest = (char *)(m_pRuntime->plugin()->memory + local_addr);

  if (len >= bytes)
    len = bytes - 1;

  memmove(dest, source, len);
  dest[len] = '\0';

  return SP_ERROR_NONE;
}

static inline int
__CheckValidChar(char *c)
{
  int count;
  int bytecount = 0;

  for (count=1; (*c & 0xC0) == 0x80; count++)
    c--;

  switch (*c & 0xF0)
  {
  case 0xC0:
  case 0xD0:
    {
      bytecount = 2;
      break;
    }
  case 0xE0:
    {
      bytecount = 3;
      break;
    }
  case 0xF0:
    {
      bytecount = 4;
      break;
    }
  }

  if (bytecount != count)
    return count;

  return 0;
}

int
PluginContext::StringToLocalUTF8(cell_t local_addr, size_t maxbytes, const char *source, size_t *wrtnbytes)
{
  char *dest;
  size_t len;
  bool needtocheck = false;

  if (((local_addr >= m_ctx.hp) && (local_addr < m_ctx.sp)) ||
      (local_addr < 0) ||
      ((ucell_t)local_addr >= m_pRuntime->plugin()->mem_size))
  {
    return SP_ERROR_INVALID_ADDRESS;
  }
  
  if (maxbytes == 0)
    return SP_ERROR_NONE;

  len = strlen(source);
  dest = (char *)(m_pRuntime->plugin()->memory + local_addr);

  if ((size_t)len >= maxbytes) {
    len = maxbytes - 1;
    needtocheck = true;
  }

  memmove(dest, source, len);
  if ((dest[len-1] & 1<<7) && needtocheck)
    len -= __CheckValidChar(dest+len-1);
  dest[len] = '\0';

  if (wrtnbytes)
    *wrtnbytes = len;

  return SP_ERROR_NONE;
}

IPluginFunction *
PluginContext::GetFunctionById(funcid_t func_id)
{
  return m_pRuntime->GetFunctionById(func_id);
}

IPluginFunction *
PluginContext::GetFunctionByName(const char *public_name)
{
  return m_pRuntime->GetFunctionByName(public_name);
}

int
PluginContext::LocalToStringNULL(cell_t local_addr, char **addr)
{
  int err;
  if ((err = LocalToString(local_addr, addr)) != SP_ERROR_NONE)
    return err;

  if ((cell_t *)*addr == m_pNullString)
    *addr = NULL;

  return SP_ERROR_NONE;
}

SourceMod::IdentityToken_t *
PluginContext::GetIdentity()
{
  SourceMod::IdentityToken_t *tok;

  if (GetKey(1, (void **)&tok))
    return tok;
  return NULL;
}

cell_t *
PluginContext::GetNullRef(SP_NULL_TYPE type)
{
  if (type == SP_NULL_VECTOR)
    return m_pNullVec;

  return NULL;
}

bool
PluginContext::IsInExec()
{
  return m_InExec;
}

int
PluginContext::Execute2(IPluginFunction *function, const cell_t *params, unsigned int num_params, cell_t *result)
{
  int ir;
  int serial;
  cell_t *sp;
  CompiledFunction *fn;
  cell_t _ignore_result;

  EnterProfileScope profileScope("SourcePawn", "EnterJIT");

  if (!Environment::get()->watchdog()->HandleInterrupt())
    return SP_ERROR_TIMEOUT;

  funcid_t fnid = function->GetFunctionID();
  if (!(fnid & 1))
    return SP_ERROR_INVALID_ADDRESS;

  unsigned public_id = fnid >> 1;
  ScriptedInvoker *cfun = m_pRuntime->GetPublicFunction(public_id);
  if (!cfun)
    return SP_ERROR_NOT_FOUND;

  if (m_pRuntime->IsPaused())
    return SP_ERROR_NOT_RUNNABLE;

  if ((cell_t)(m_ctx.hp + 16*sizeof(cell_t)) > (cell_t)(m_ctx.sp - (sizeof(cell_t) * (num_params + 1))))
    return SP_ERROR_STACKLOW;

  if (result == NULL)
    result = &_ignore_result;

  /* We got this far.  It's time to start profiling. */
  EnterProfileScope scriptScope("SourcePawn", cfun->FullName());

  /* See if we have to compile the callee. */
  if (Environment::get()->IsJitEnabled() &&
      (fn = m_pRuntime->m_PubJitFuncs[public_id]) == NULL)
  {
    /* We might not have to - check pcode offset. */
    fn = m_pRuntime->GetJittedFunctionByOffset(cfun->Public()->code_offs);
    if (fn) {
      m_pRuntime->m_PubJitFuncs[public_id] = fn;
    } else {
      if ((fn = CompileFunction(m_pRuntime, cfun->Public()->code_offs, &ir)) == NULL)
        return ir;
      m_pRuntime->m_PubJitFuncs[public_id] = fn;
    }
  }

  /* Save our previous state. */

  bool save_exec;
  uint32_t save_n_idx;
  cell_t save_sp, save_hp, save_rp, save_cip;

  save_sp = m_ctx.sp;
  save_hp = m_ctx.hp;
  save_exec = m_InExec;
  save_n_idx = last_native_;
  save_rp = rp_;
  save_cip = cip_;

  /* Push parameters */

  m_ctx.sp -= sizeof(cell_t) * (num_params + 1);
  sp = (cell_t *)(m_pRuntime->plugin()->memory + m_ctx.sp);

  sp[0] = num_params;
  for (unsigned int i = 0; i < num_params; i++)
    sp[i + 1] = params[i];

  /* Clear internal state */
  native_error_ = SP_ERROR_NONE;
  last_native_ = -1;
  m_MsgCache[0] = '\0';
  m_CustomMsg = false;
  m_InExec = true;

  // Enter the execution engine.
  Environment *env = Environment::get();
  if (env->IsJitEnabled())
    ir = env->Invoke(m_pRuntime, fn, result);
  else
    ir = Interpret(m_pRuntime, cfun->Public()->code_offs, result);

  /* Restore some states, stop the frame tracer */

  m_InExec = save_exec;

  if (ir == SP_ERROR_NONE) {
    native_error_ = SP_ERROR_NONE;
    if (m_ctx.sp != save_sp) {
      ir = SP_ERROR_STACKLEAK;
      _SetErrorMessage("Stack leak detected: sp:%d should be %d!", 
        m_ctx.sp, 
        save_sp);
    }
    if (m_ctx.hp != save_hp) {
      ir = SP_ERROR_HEAPLEAK;
      _SetErrorMessage("Heap leak detected: hp:%d should be %d!", 
        m_ctx.hp, 
        save_hp);
    }
    if (rp_ != save_rp) {
      ir = SP_ERROR_STACKLEAK;
      _SetErrorMessage("Return stack leak detected: rp:%d should be %d!",
        rp_,
        save_rp);
    }
  }

  if (ir == SP_ERROR_TIMEOUT)
    Environment::get()->watchdog()->NotifyTimeoutReceived();

  if (ir != SP_ERROR_NONE)
    Environment::get()->ReportError(m_pRuntime, ir, m_MsgCache, save_rp);

  m_ctx.sp = save_sp;
  m_ctx.hp = save_hp;
  rp_ = save_rp;
  
  cip_ = save_cip;
  last_native_ = save_n_idx;
  native_error_ = SP_ERROR_NONE;
  m_MsgCache[0] = '\0';
  m_CustomMsg = false;

  return ir;
}

IPluginRuntime *
PluginContext::GetRuntime()
{
  return m_pRuntime;
}

DebugInfo::DebugInfo(sp_plugin_t *plugin) : m_pPlugin(plugin)
{
}

#define USHR(x) ((unsigned int)(x)>>1)

int
DebugInfo::LookupFile(ucell_t addr, const char **filename)
{
  int high, low, mid;

  high = m_pPlugin->debug.files_num;
  low = -1;

  while (high - low > 1) {
    mid = USHR(low + high);
    if (m_pPlugin->debug.files[mid].addr <= addr)
      low = mid;
    else
      high = mid;
  }

  if (low == -1)
    return SP_ERROR_NOT_FOUND;

  *filename = m_pPlugin->debug.stringbase + m_pPlugin->debug.files[low].name;
  return SP_ERROR_NONE;
}

int
DebugInfo::LookupFunction(ucell_t addr, const char **name)
{
  if (!m_pPlugin->debug.unpacked) {
    uint32_t max, iter;
    sp_fdbg_symbol_t *sym;
    uint8_t *cursor = (uint8_t *)(m_pPlugin->debug.symbols);

    max = m_pPlugin->debug.syms_num;
    for (iter = 0; iter < max; iter++) {
      sym = (sp_fdbg_symbol_t *)cursor;

      if (sym->ident == sp::IDENT_FUNCTION &&
          sym->codestart <= addr &&
          sym->codeend > addr)
      {
        *name = m_pPlugin->debug.stringbase + sym->name;
        return SP_ERROR_NONE;
      }

      if (sym->dimcount > 0) {
        cursor += sizeof(sp_fdbg_symbol_t);
        cursor += sizeof(sp_fdbg_arraydim_t) * sym->dimcount;
        continue;
      }

      cursor += sizeof(sp_fdbg_symbol_t);
    }

    return SP_ERROR_NOT_FOUND;
  } else {
    uint32_t max, iter;
    sp_u_fdbg_symbol_t *sym;
    uint8_t *cursor = (uint8_t *)(m_pPlugin->debug.symbols);

    max = m_pPlugin->debug.syms_num;
    for (iter = 0; iter < max; iter++) {
      sym = (sp_u_fdbg_symbol_t *)cursor;

      if (sym->ident == sp::IDENT_FUNCTION &&
          sym->codestart <= addr &&
          sym->codeend > addr)
      {
        *name = m_pPlugin->debug.stringbase + sym->name;
        return SP_ERROR_NONE;
      }

      if (sym->dimcount > 0) {
        cursor += sizeof(sp_u_fdbg_symbol_t);
        cursor += sizeof(sp_u_fdbg_arraydim_t) * sym->dimcount;
        continue;
      }

      cursor += sizeof(sp_u_fdbg_symbol_t);
    }

    return SP_ERROR_NOT_FOUND;
  }
}

int
DebugInfo::LookupLine(ucell_t addr, uint32_t *line)
{
  int high, low, mid;

  high = m_pPlugin->debug.lines_num;
  low = -1;

  while (high - low > 1) {
    mid = USHR(low + high);
    if (m_pPlugin->debug.lines[mid].addr <= addr)
      low = mid;
    else
      high = mid;
  }

  if (low == -1)
    return SP_ERROR_NOT_FOUND;

  /* Since the CIP occurs BEFORE the line, we have to add one */
  *line = m_pPlugin->debug.lines[low].line + 1;

  return SP_ERROR_NONE;
}

#undef USHR

int
PluginContext::GetLastNativeError()
{
  return native_error_;
}

cell_t *
PluginContext::GetLocalParams()
{
  return (cell_t *)(m_pRuntime->plugin()->memory + m_ctx.frm + (2 * sizeof(cell_t)));
}

void
PluginContext::SetKey(int k, void *value)
{
  if (k < 1 || k > 4)
    return;

  m_keys[k - 1] = value;
  m_keys_set[k - 1] = true;
}

bool
PluginContext::GetKey(int k, void **value)
{
  if (k < 1 || k > 4 || m_keys_set[k - 1] == false)
    return false;

  *value = m_keys[k - 1];
  return true;
}

void
PluginContext::ClearLastNativeError()
{
  native_error_ = SP_ERROR_NONE;
}

int
PluginContext::popTrackerAndSetHeap()
{
  assert(tracker_.pCur > tracker_.pBase);

  tracker_.pCur--;
  if (tracker_.pCur < tracker_.pBase)
    return SP_ERROR_TRACKER_BOUNDS;

  ucell_t amt = *tracker_.pCur;
  if (amt > (m_ctx.hp - m_pRuntime->plugin()->data_size))
    return SP_ERROR_HEAPMIN;

  m_ctx.hp -= amt;
  return SP_ERROR_NONE;
}

int
PluginContext::pushTracker(uint32_t amount)
{
  if ((size_t)(tracker_.pCur - tracker_.pBase) >= tracker_.size)
    return SP_ERROR_TRACKER_BOUNDS;

  if (tracker_.pCur + 1 - (tracker_.pBase + tracker_.size) == 0) {
    size_t disp = tracker_.size - 1;
    tracker_.size *= 2;
    tracker_.pBase = (ucell_t *)realloc(tracker_.pBase, tracker_.size * sizeof(cell_t));

    if (!tracker_.pBase)
      return SP_ERROR_TRACKER_BOUNDS;

    tracker_.pCur = tracker_.pBase + disp;
  }

  *tracker_.pCur++ = amount;
  return SP_ERROR_NONE;
}

cell_t
PluginContext::invokeNative(ucell_t native_idx, cell_t *params)
{
  cell_t save_sp = m_ctx.sp;
  cell_t save_hp = m_ctx.hp;

  // Note: Invoke() saves the last native, so we don't need to here.
  last_native_ = native_idx;

  sp_native_t *native = &m_pRuntime->plugin()->natives[native_idx];

  if (native->status == SP_NATIVE_UNBOUND) {
    native_error_ = SP_ERROR_INVALID_NATIVE;
    return 0;
  }

  cell_t result = native->pfn(m_ctx.basecx, params);

  if (native_error_ != SP_ERROR_NONE)
    return result;

  if (save_sp != m_ctx.sp) {
    native_error_ = SP_ERROR_STACKLEAK;
    return result;
  }
  if (save_hp != m_ctx.hp) {
    native_error_ = SP_ERROR_HEAPLEAK;
    return result;
  }

  return result;
}

cell_t
PluginContext::invokeBoundNative(SPVM_NATIVE_FUNC pfn, cell_t *params)
{
  cell_t save_sp = m_ctx.sp;
  cell_t save_hp = m_ctx.hp;

  cell_t result = pfn(this, params);

  if (native_error_ != SP_ERROR_NONE)
    return result;

  if (save_sp != m_ctx.sp) {
    native_error_ = SP_ERROR_STACKLEAK;
    return result;
  }
  if (save_hp != m_ctx.hp) {
    native_error_ = SP_ERROR_HEAPLEAK;
    return result;
  }

  return result;
}
