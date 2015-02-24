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
#include "compiled-function.h"
#include "sp_vm_engine.h"
#include "jit_x86.h"

CompiledFunction::CompiledFunction(void *entry_addr, cell_t pcode_offs, FixedArray<LoopEdge> *edges)
  : entry_(entry_addr),
    code_offset_(pcode_offs),
    edges_(edges)
{
}

CompiledFunction::~CompiledFunction()
{
  g_Jit.FreeCode(entry_);
}
