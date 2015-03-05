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
#ifndef _include_sourcepawn_vm_api_h_
#define _include_sourcepawn_vm_api_h_

#include <sp_vm_api.h>
#include <am-utility.h> // Replace with am-cxx later.

namespace sp {

using namespace SourcePawn;

class SourcePawnEngine : public ISourcePawnEngine
{
 public:
  SourcePawnEngine();

  sp_plugin_t *LoadFromFilePointer(FILE *fp, int *err) KE_OVERRIDE;
  sp_plugin_t *LoadFromMemory(void *base, sp_plugin_t *plugin, int *err) KE_OVERRIDE;
  int FreeFromMemory(sp_plugin_t *plugin) KE_OVERRIDE;
  void *BaseAlloc(size_t size) KE_OVERRIDE;
  void BaseFree(void *memory) KE_OVERRIDE;
  void *ExecAlloc(size_t size) KE_OVERRIDE;
  void ExecFree(void *address) KE_OVERRIDE;
  IDebugListener *SetDebugListener(IDebugListener *pListener) KE_OVERRIDE;
  unsigned int GetContextCallCount() KE_OVERRIDE;
  unsigned int GetEngineAPIVersion() KE_OVERRIDE;
  void *AllocatePageMemory(size_t size) KE_OVERRIDE;
  void SetReadWrite(void *ptr) KE_OVERRIDE;
  void SetReadExecute(void *ptr) KE_OVERRIDE;
  void FreePageMemory(void *ptr) KE_OVERRIDE;
  void SetReadWriteExecute(void *ptr);
  const char *GetErrorString(int err);
};

class SourcePawnEngine2 : public ISourcePawnEngine2
{
 public:
  SourcePawnEngine2();

  unsigned int GetAPIVersion() KE_OVERRIDE;
  const char *GetEngineName() KE_OVERRIDE;
  const char *GetVersionString() KE_OVERRIDE;
  IPluginRuntime *LoadPlugin(ICompilation *co, const char *file, int *err) KE_OVERRIDE;
  SPVM_NATIVE_FUNC CreateFakeNative(SPVM_FAKENATIVE_FUNC callback, void *pData) KE_OVERRIDE;
  void DestroyFakeNative(SPVM_NATIVE_FUNC func) KE_OVERRIDE;
  IDebugListener *SetDebugListener(IDebugListener *listener) KE_OVERRIDE;
  ICompilation *StartCompilation() KE_OVERRIDE;
  const char *GetErrorString(int err) KE_OVERRIDE;
  bool Initialize() KE_OVERRIDE;
  void Shutdown() KE_OVERRIDE;
  IPluginRuntime *CreateEmptyRuntime(const char *name, uint32_t memory) KE_OVERRIDE;
  bool InstallWatchdogTimer(size_t timeout_ms) KE_OVERRIDE;
  bool SetJitEnabled(bool enabled) KE_OVERRIDE;
  bool IsJitEnabled() KE_OVERRIDE;
  void SetProfiler(IProfiler *profiler) KE_OVERRIDE;
  void EnableProfiling() KE_OVERRIDE;
  void DisableProfiling() KE_OVERRIDE;
  void SetProfilingTool(IProfilingTool *tool) KE_OVERRIDE;
  IPluginRuntime *LoadBinaryFromFile(const char *file, char *error, size_t maxlength) KE_OVERRIDE;
  ISourcePawnEnvironment *Environment() KE_OVERRIDE;
};

extern size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...);
extern size_t UTIL_FormatVA(char *buffer, size_t maxlength, const char *fmt, va_list ap);

} // namespace sp

#endif // _include_sourcepawn_vm_api_h_
