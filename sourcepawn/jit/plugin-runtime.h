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
#ifndef _INCLUDE_SOURCEPAWN_JIT_RUNTIME_H_
#define _INCLUDE_SOURCEPAWN_JIT_RUNTIME_H_

#include <sp_vm_api.h>
#include <am-vector.h>
#include <am-inlinelist.h>
#include "jit_shared.h"
#include "compiled-function.h"
#include "scripted-invoker.h"

class BaseContext;

class DebugInfo : public IPluginDebugInfo
{
public:
  DebugInfo(sp_plugin_t *plugin);
public:
  int LookupFile(ucell_t addr, const char **filename);
  int LookupFunction(ucell_t addr, const char **name);
  int LookupLine(ucell_t addr, uint32_t *line);
private:
  sp_plugin_t *m_pPlugin;
};

struct floattbl_t
{
  floattbl_t() {
    found = false;
    index = 0;
  }
  bool found;
  unsigned int index;
};

/* Jit wants fast access to this so we expose things as public */
class PluginRuntime
  : public SourcePawn::IPluginRuntime,
    public ke::InlineListNode<PluginRuntime>
{
 public:
  PluginRuntime();
  ~PluginRuntime();

 public:
  virtual int CreateBlank(uint32_t heastk);
  virtual int CreateFromMemory(sp_file_hdr_t *hdr, uint8_t *base);
  virtual bool IsDebugging();
  virtual IPluginDebugInfo *GetDebugInfo();
  virtual int FindNativeByName(const char *name, uint32_t *index);
  virtual int GetNativeByIndex(uint32_t index, sp_native_t **native);
  virtual sp_native_t *GetNativeByIndex(uint32_t index);
  virtual uint32_t GetNativesNum();
  virtual int FindPublicByName(const char *name, uint32_t *index);
  virtual int GetPublicByIndex(uint32_t index, sp_public_t **publicptr);
  virtual uint32_t GetPublicsNum();
  virtual int GetPubvarByIndex(uint32_t index, sp_pubvar_t **pubvar);
  virtual int FindPubvarByName(const char *name, uint32_t *index);
  virtual int GetPubvarAddrs(uint32_t index, cell_t *local_addr, cell_t **phys_addr);
  virtual uint32_t GetPubVarsNum();
  virtual IPluginFunction *GetFunctionByName(const char *public_name);
  virtual IPluginFunction *GetFunctionById(funcid_t func_id);
  virtual IPluginContext *GetDefaultContext();
  virtual int ApplyCompilationOptions(ICompilation *co);
  virtual void SetPauseState(bool paused);
  virtual bool IsPaused();
  virtual size_t GetMemUsage();
  virtual unsigned char *GetCodeHash();
  virtual unsigned char *GetDataHash();
  CompiledFunction *GetJittedFunctionByOffset(cell_t pcode_offset);
  void AddJittedFunction(CompiledFunction *fn);
  void SetName(const char *name);
  unsigned GetNativeReplacement(size_t index);
  ScriptedInvoker *GetPublicFunction(size_t index);

  BaseContext *GetBaseContext();
  const sp_plugin_t *plugin() const {
    return &m_plugin;
  }

  size_t NumJitFunctions() const {
    return m_JitFunctions.length();
  }
  CompiledFunction *GetJitFunction(size_t i) const {
    return m_JitFunctions[i];
  }

 private:
  void SetupFloatNativeRemapping();

 private:
  sp_plugin_t m_plugin;
  uint8_t *alt_pcode_;
  unsigned int m_NumFuncs;
  unsigned int m_MaxFuncs;
  floattbl_t *float_table_;
  CompiledFunction **function_map_;
  size_t function_map_size_;
  ke::Vector<CompiledFunction *> m_JitFunctions;

 public:
  DebugInfo m_Debug;
  BaseContext *m_pCtx;
  ScriptedInvoker **m_PubFuncs;
  CompiledFunction **m_PubJitFuncs;

 public:
  unsigned int m_CompSerial;
  
  unsigned char m_CodeHash[16];
  unsigned char m_DataHash[16];
};

#endif //_INCLUDE_SOURCEPAWN_JIT_RUNTIME_H_

