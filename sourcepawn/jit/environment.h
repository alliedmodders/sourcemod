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
#ifndef _include_sourcepawn_vm_environment_h_
#define _include_sourcepawn_vm_environment_h_

#include <sp_vm_api.h>
#include <am-utility.h> // Replace with am-cxx later.
#include <am-inlinelist.h>
#include <am-thread-utils.h>
#include "code-allocator.h"
#include "plugin-runtime.h"
#include "stack-frames.h"

namespace sp {

using namespace SourcePawn;

class PluginRuntime;
class CodeStubs;
class WatchdogTimer;

// An Environment encapsulates everything that's needed to load and run
// instances of plugins on a single thread. There can be at most one
// environment per thread.
//
// Currently, the VM is single threaded in that no more than one
// Environment can be created per process.
class Environment : public ISourcePawnEnvironment
{
 public:
  Environment();
  ~Environment();

  static Environment *New();

  void Shutdown() KE_OVERRIDE;
  ISourcePawnEngine *APIv1() KE_OVERRIDE;
  ISourcePawnEngine2 *APIv2() KE_OVERRIDE;
  int ApiVersion() KE_OVERRIDE {
    return SOURCEPAWN_API_VERSION;
  }

  // Access the current Environment.
  static Environment *get();

  bool InstallWatchdogTimer(int timeout_ms);

  void EnterExceptionHandlingScope(ExceptionHandler *handler) KE_OVERRIDE;
  void LeaveExceptionHandlingScope(ExceptionHandler *handler) KE_OVERRIDE;
  bool HasPendingException(const ExceptionHandler *handler) KE_OVERRIDE;
  const char *GetPendingExceptionMessage(const ExceptionHandler *handler) KE_OVERRIDE;

  // Runtime functions.
  const char *GetErrorString(int err);
  void ReportError(int code);
  void ReportError(int code, const char *message);
  void ReportErrorFmt(int code, const char *message, ...);
  void ReportErrorVA(const char *fmt, va_list ap);
  void ReportErrorVA(int code, const char *fmt, va_list ap);

  // Allocate and free executable memory.
  void *AllocateCode(size_t size);
  void FreeCode(void *code);
  CodeStubs *stubs() {
    return code_stubs_;
  }

  // Runtime management.
  void RegisterRuntime(PluginRuntime *rt);
  void DeregisterRuntime(PluginRuntime *rt);
  void PatchAllJumpsForTimeout();
  void UnpatchAllJumpsFromTimeout();
  ke::Mutex *lock() {
    return &mutex_;
  }
  int Invoke(PluginRuntime *runtime, CompiledFunction *fn, cell_t *result);

  // Helpers.
  void SetProfiler(IProfilingTool *profiler) {
    profiler_ = profiler;
  }
  IProfilingTool *profiler() const {
    return profiler_;
  }
  bool IsProfilingEnabled() const {
    return profiling_enabled_;
  }
  void EnableProfiling();
  void DisableProfiling();

  void SetJitEnabled(bool enabled) {
  }
  bool IsJitEnabled() const {
    return jit_enabled_;
  }
  void SetDebugger(IDebugListener *debugger) {
    debugger_ = debugger;
  }
  IDebugListener *debugger() const {
    return debugger_;
  }

  WatchdogTimer *watchdog() const {
    return watchdog_timer_;
  }

  bool hasPendingException() const;
  void clearPendingException();
  int getPendingExceptionCode() const;

  // These are indicators used for the watchdog timer.
  uintptr_t FrameId() const {
    return frame_id_;
  }
  bool RunningCode() const {
    return !!top_;
  }
  void enterInvoke(InvokeFrame *frame) {
    if (!top_)
      frame_id_++;
    top_ = frame;
  }
  void leaveInvoke() {
    exit_frame_ = top_->prev_exit_frame();
    top_ = top_->prev();
  }

  InvokeFrame *top() const {
    return top_;
  }
  const ExitFrame &exit_frame() const {
    return exit_frame_;
  }

 public:
  static inline size_t offsetOfTopFrame() {
    return offsetof(Environment, top_);
  }
  static inline size_t offsetOfExceptionCode() {
    return offsetof(Environment, exception_code_);
  }

 private:
  bool Initialize();

 private:
  ke::AutoPtr<ISourcePawnEngine> api_v1_;
  ke::AutoPtr<ISourcePawnEngine2> api_v2_;
  ke::AutoPtr<WatchdogTimer> watchdog_timer_;
  ke::Mutex mutex_;

  IDebugListener *debugger_;
  ExceptionHandler *eh_top_;
  int exception_code_;
  char exception_message_[1024];

  IProfilingTool *profiler_;
  bool jit_enabled_;
  bool profiling_enabled_;

  Knight::KeCodeCache *code_pool_;
  ke::InlineList<PluginRuntime> runtimes_;

  uintptr_t frame_id_;

  ke::AutoPtr<CodeStubs> code_stubs_;

  InvokeFrame *top_;
  ExitFrame exit_frame_;
};

class EnterProfileScope
{
 public:
  EnterProfileScope(const char *group, const char *name)
  {
    if (Environment::get()->IsProfilingEnabled())
      Environment::get()->profiler()->EnterScope(group, name);
  }

  ~EnterProfileScope()
  {
    if (Environment::get()->IsProfilingEnabled())
      Environment::get()->profiler()->LeaveScope();
  }
};

} // namespace sp

#endif // _include_sourcepawn_vm_environment_h_
