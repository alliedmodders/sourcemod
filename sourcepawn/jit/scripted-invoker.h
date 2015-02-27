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
#ifndef _INCLUDE_SOURCEMOD_BASEFUNCTION_H_
#define _INCLUDE_SOURCEMOD_BASEFUNCTION_H_

#include <sp_vm_api.h>
#include <am-utility.h>

namespace sp {

using namespace SourcePawn;

class PluginRuntime;
class PluginContext;
class CompiledFunction;

struct ParamInfo
{
  int flags;      /* Copy-back flags */
  bool marked;    /* Whether this is marked as being used */
  cell_t local_addr;  /* Local address to free */
  cell_t *phys_addr;  /* Physical address of our copy */
  cell_t *orig_addr;  /* Original address to copy back to */
  ucell_t size;    /* Size of array in bytes */
  struct {
    bool is_sz;    /* is a string */
    int sz_flags;  /* has sz flags */
  } str;
};

class ScriptedInvoker : public IPluginFunction
{
 public:
  ScriptedInvoker(PluginRuntime *pRuntime, funcid_t fnid, uint32_t pub_id);
  ~ScriptedInvoker();

 public:
  int PushCell(cell_t cell);
  int PushCellByRef(cell_t *cell, int flags);
  int PushFloat(float number);
  int PushFloatByRef(float *number, int flags);
  int PushArray(cell_t *inarray, unsigned int cells, int copyback);
  int PushString(const char *string);
  int PushStringEx(char *buffer, size_t length, int sz_flags, int cp_flags);
  int Execute(cell_t *result);
  void Cancel();
  int CallFunction(const cell_t *params, unsigned int num_params, cell_t *result);
  IPluginContext *GetParentContext();
  bool Invoke(cell_t *result);
  bool IsRunnable();
  funcid_t GetFunctionID();
  int Execute2(IPluginContext *ctx, cell_t *result);
  int CallFunction2(IPluginContext *ctx, 
    const cell_t *params, 
    unsigned int num_params, 
    cell_t *result);
  IPluginRuntime *GetParentRuntime();

 public:
  const char *FullName() const {
    return full_name_;
  }
  sp_public_t *Public() const {
    return public_;
  }

  CompiledFunction *cachedCompiledFunction() const {
    return cc_function_;
  }
  void setCachedCompiledFunction(CompiledFunction *fn) {
    cc_function_ = fn;
  }

 private:
  int _PushString(const char *string, int sz_flags, int cp_flags, size_t len);
  int SetError(int err);

 private:
  Environment *env_;
  PluginRuntime *m_pRuntime;
  PluginContext *context_;
  cell_t m_params[SP_MAX_EXEC_PARAMS];
  ParamInfo m_info[SP_MAX_EXEC_PARAMS];
  unsigned int m_curparam;
  int m_errorstate;
  funcid_t m_FnId;
  ke::AutoArray<char> full_name_;
  sp_public_t *public_;
  CompiledFunction *cc_function_;
};

} // namespace sp

#endif //_INCLUDE_SOURCEMOD_BASEFUNCTION_H_
