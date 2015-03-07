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
#ifndef _include_sourcepawn_vm_stack_frames_h_
#define _include_sourcepawn_vm_stack_frames_h_

#include <sp_vm_api.h>
#include <assert.h>

namespace sp {

using namespace SourcePawn;

class PluginContext;
class PluginRuntime;

// An ExitFrame represents the state of the most recent exit from VM state to
// the outside world. Because this transition is on a critical path, we declare
// exactly one ExitFrame and save/restore it in InvokeFrame(). Anytime we're in
// the VM, we are guaranteed to have an ExitFrame for each InvokeFrame().
class ExitFrame
{
 public:
  ExitFrame()
   : exit_sp_(nullptr),
     exit_native_(-1)
  {}

 public:
  const intptr_t *exit_sp() const {
    return exit_sp_;
  }
  bool has_exit_native() const {
    return exit_native_ != -1;
  }
  uint32_t exit_native() const {
    assert(has_exit_native());
    return exit_native_;
  }

 public:
  static inline size_t offsetOfExitSp() {
    return offsetof(ExitFrame, exit_sp_);
  }
  static inline size_t offsetOfExitNative() {
    return offsetof(ExitFrame, exit_native_);
  }

 private:
  const intptr_t *exit_sp_;
  int exit_native_;
};

// An InvokeFrame represents one activation of Execute2().
class InvokeFrame
{
 public:
  InvokeFrame(PluginContext *cx, ucell_t cip);
  ~InvokeFrame();

  InvokeFrame *prev() const {
    return prev_;
  }
  PluginContext *cx() const {
    return cx_;
  }

  const ExitFrame &prev_exit_frame() const {
    return prev_exit_frame_;
  }
  const intptr_t *entry_sp() const {
    return entry_sp_;
  }
  ucell_t entry_cip() const {
    return entry_cip_;
  }

 public:
  static inline size_t offsetOfEntrySp() {
    return offsetof(InvokeFrame, entry_sp_);
  }

 private:
  InvokeFrame *prev_;
  PluginContext *cx_;
  ExitFrame prev_exit_frame_;
  ucell_t entry_cip_;
  const intptr_t *entry_sp_;
};

class FrameIterator : public SourcePawn::IFrameIterator
{
 public:
  FrameIterator();

  bool Done() const KE_OVERRIDE {
    return !ivk_;
  }
  void Next() KE_OVERRIDE;
  void Reset() KE_OVERRIDE;

  bool IsNativeFrame() const KE_OVERRIDE;
  bool IsScriptedFrame() const KE_OVERRIDE;
  const char *FunctionName() const KE_OVERRIDE;
  const char *FilePath() const KE_OVERRIDE;
  unsigned LineNumber() const KE_OVERRIDE;
  IPluginContext *Context() const KE_OVERRIDE;

 private:
  void nextInvokeFrame();
  cell_t findCip() const;

 private:
  InvokeFrame *ivk_;
  ExitFrame exit_frame_;
  PluginRuntime *runtime_;
  const intptr_t *sp_iter_;
  const intptr_t *sp_stop_;
  ucell_t function_cip_;
  mutable ucell_t cip_;
  void *pc_;
};

} // namespace sp

#endif // _include_sourcepawn_vm_stack_frames_h_
