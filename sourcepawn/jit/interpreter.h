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
#ifndef _include_sourcepawn_interpreter_h_
#define _include_sourcepawn_interpreter_h_

#include <sp_vm_types.h>
#include <sp_vm_api.h>
#include "BaseRuntime.h"
#include "sp_vm_basecontext.h"

struct tracker_t
{
  size_t size; 
  ucell_t *pBase; 
  ucell_t *pCur;
};

int Interpret(BaseRuntime *rt, uint32_t aCodeStart, cell_t *rval);

int GenerateFullArray(BaseRuntime *rt, uint32_t argc, cell_t *argv, int autozero);
cell_t NativeCallback(sp_context_t *ctx, ucell_t native_idx, cell_t *params);
int PopTrackerAndSetHeap(BaseRuntime *rt);
int PushTracker(sp_context_t *ctx, size_t amount);

#endif // _include_sourcepawn_interpreter_h_
