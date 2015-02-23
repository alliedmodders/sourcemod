// vim: set sts=2 ts=8 sw=2 tw=99 noet:
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
#ifndef _INCLUDE_SOURCEPAWN_ENGINE_2_H_
#define _INCLUDE_SOURCEPAWN_ENGINE_2_H_

#include <sp_vm_api.h>

namespace SourcePawn {

/** 
 * @brief Outlines the interface a Virtual Machine (JIT) must expose
 */
class SourcePawnEngine2 : public ISourcePawnEngine2
{
 public:
  SourcePawnEngine2();

 public:
  unsigned int GetAPIVersion();
  const char *GetEngineName();
  const char *GetVersionString();
  IPluginRuntime *LoadPlugin(ICompilation *co, const char *file, int *err);
  SPVM_NATIVE_FUNC CreateFakeNative(SPVM_FAKENATIVE_FUNC callback, void *pData);
  void DestroyFakeNative(SPVM_NATIVE_FUNC func);
  IDebugListener *SetDebugListener(IDebugListener *listener);
  ICompilation *StartCompilation();
  const char *GetErrorString(int err);
  bool Initialize();
  void Shutdown();
  IPluginRuntime *CreateEmptyRuntime(const char *name, uint32_t memory);
  bool InstallWatchdogTimer(size_t timeout_ms);

  bool SetJitEnabled(bool enabled) {
    jit_enabled_ = enabled;
    return true;
    }

  bool IsJitEnabled() {
    return jit_enabled_;
  }

  void SetProfiler(IProfiler *profiler) {
    // Deprecated.
  }

  void EnableProfiling() {
    profiling_enabled_ = !!profiler_;
  }
  void DisableProfiling() {
    profiling_enabled_ = false;
  }
  bool IsProfilingEnabled() {
    return profiling_enabled_;
  }
  void SetProfilingTool(IProfilingTool *tool) {
    profiler_ = tool;
  }

 public:
  IProfilingTool *GetProfiler() {
    return profiler_;
  }

 private:
  IProfilingTool *profiler_;
  bool jit_enabled_;
  bool profiling_enabled_;
};

} // namespace SourcePawn

extern SourcePawn::SourcePawnEngine2 g_engine2;

class EnterProfileScope
{
 public:
  EnterProfileScope(const char *group, const char *name)
  {
    if (g_engine2.IsProfilingEnabled())
      g_engine2.GetProfiler()->EnterScope(group, name);
  }

  ~EnterProfileScope()
  {
    if (g_engine2.IsProfilingEnabled())
      g_engine2.GetProfiler()->LeaveScope();
  }
};

#endif //_INCLUDE_SOURCEPAWN_ENGINE_2_H_
