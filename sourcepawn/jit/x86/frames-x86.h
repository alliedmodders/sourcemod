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
#ifndef _include_sourcepawn_jit_frames_x86_h_
#define _include_sourcepawn_jit_frames_x86_h_

#include <sp_vm_types.h>

namespace sp {

using namespace SourcePawn;

class PluginContext;

// This is the layout of the stack in between each scripted function call.
struct JitFrame
{
  intptr_t align0;
  intptr_t align1;
  ucell_t function_cip;
  void *return_address;

  static inline const JitFrame *FromSp(const intptr_t *sp) {
    return reinterpret_cast<const JitFrame *>(sp);
  }
};

// When we're about to call a native, the stack pointer we store in the exit
// frame is such that (sp + sizeof(JitExitFrameForNative)) conforms to this
// structure.
//
// Note that it looks reversed compared to JitFrame because we capture the sp
// before saving registers and pushing arguments.
struct JitExitFrameForNative
{
  void *return_address;
  PluginContext *cx;
  union {
    uint32_t native_index;
    SPVM_NATIVE_FUNC fn;
  } arg;
  const cell_t *params;
  cell_t saved_alt;

  static inline const JitExitFrameForNative *FromExitSp(const intptr_t *exit_sp) {
    return reinterpret_cast<const JitExitFrameForNative *>(
      reinterpret_cast<const uint8_t *>(exit_sp) - sizeof(JitExitFrameForNative));
  }
};

// Unlke native frames, the exit_sp for these is created at the base address.
struct JitExitFrameForHelper
{
  void *return_address;

  static inline const JitExitFrameForHelper *FromExitSp(const intptr_t *exit_sp) {
    return reinterpret_cast<const JitExitFrameForHelper *>(exit_sp);
  }

  bool isCompileFunction() const {
    return !!return_address;
  }
  const JitFrame *prev() const {
    return reinterpret_cast<const JitFrame *>(this + 1);
  }
};

} // namespace sp

#endif // _include_sourcepawn_jit_frames_x86_h_
