// vim: set ts=8 sw=2 sts=2 tw=99 et:
#ifndef _INCLUDE_SOURCEPAWN_JIT_RUNTIME_H_
#define _INCLUDE_SOURCEPAWN_JIT_RUNTIME_H_

#include <sp_vm_api.h>
#include <am-vector.h>
#include <am-inlinelist.h>
#include "jit_shared.h"
#include "sp_vm_function.h"

class BaseContext;
class JitFunction;

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
class BaseRuntime
  : public SourcePawn::IPluginRuntime,
    public ke::InlineListNode<BaseRuntime>
{
 public:
  BaseRuntime();
  ~BaseRuntime();

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
  JitFunction *GetJittedFunctionByOffset(cell_t pcode_offset);
  void AddJittedFunction(JitFunction *fn);
  void SetName(const char *name);
  unsigned GetNativeReplacement(size_t index);
  CFunction *GetPublicFunction(size_t index);

  BaseContext *GetBaseContext();
  const sp_plugin_t *plugin() const {
    return &m_plugin;
  }

  size_t NumJitFunctions() const {
    return m_JitFunctions.length();
  }
  JitFunction *GetJitFunction(size_t i) const {
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
  JitFunction **function_map_;
  size_t function_map_size_;
  ke::Vector<JitFunction *> m_JitFunctions;

 public:
  DebugInfo m_Debug;
  BaseContext *m_pCtx;
  CFunction **m_PubFuncs;
  JitFunction **m_PubJitFuncs;

 private:
  ICompilation *co_;

 public:
  unsigned int m_CompSerial;
  
  unsigned char m_CodeHash[16];
  unsigned char m_DataHash[16];
};

#endif //_INCLUDE_SOURCEPAWN_JIT_RUNTIME_H_

