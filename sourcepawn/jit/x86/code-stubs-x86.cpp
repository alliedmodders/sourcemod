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
#include <sp_vm_api.h>
#include "code-stubs.h"
#include "x86-utils.h"
#include "jit_x86.h"
#include "environment.h"

using namespace sp;
using namespace SourcePawn;

#define __ masm.

bool
CodeStubs::InitializeFeatureDetection()
{
  MacroAssemblerX86 masm;
  MacroAssemblerX86::GenerateFeatureDetection(masm);
  void *code = LinkCode(env_, masm);
  if (!code)
    return false;
  MacroAssemblerX86::RunFeatureDetection(code);
  return true;
}


bool
CodeStubs::CompileInvokeStub()
{
  AssemblerX86 masm;

  __ push(ebp);
  __ movl(ebp, esp);

  __ push(esi);   // ebp - 4
  __ push(edi);   // ebp - 8
  __ push(ebx);   // ebp - 12
  __ push(esp);   // ebp - 16

  // ebx = cx
  __ movl(ebx, Operand(ebp, 8 + 4 * 0));

  // ecx = code
  __ movl(ecx, Operand(ebp, 8 + 4 * 1));

  // eax = cx->memory
  __ movl(eax, Operand(ebx, PluginContext::offsetOfMemory()));

  // Set up run-time registers.
  __ movl(edi, Operand(ebx, PluginContext::offsetOfSp()));
  __ addl(edi, eax);
  __ movl(esi, eax);
  __ movl(ebx, edi);

  // Align the stack.
  __ andl(esp, 0xfffffff0);

  // Set up the last piece of the invoke frame. This lets us find the bounds
  // of the call stack.
  __ movl(eax, intptr_t(Environment::get()));
  __ movl(eax, Operand(eax, Environment::offsetOfTopFrame()));
  __ movl(Operand(eax, InvokeFrame::offsetOfEntrySp()), esp);

  // Call into plugin (align the stack first).
  __ call(ecx);

  // Get input context, store rval.
  __ movl(ecx, Operand(ebp, 8 + 4 * 2));
  __ movl(Operand(ecx, 0), pri);

  // Set no error.
  __ movl(eax, SP_ERROR_NONE);

  // Store latest stk. If we have an error code, we'll jump directly to here,
  // so eax will already be set.
  Label ret;
  __ bind(&ret);
  __ subl(stk, dat);
  __ movl(ecx, Operand(ebp, 8 + 4 * 0));
  __ movl(Operand(ecx, PluginContext::offsetOfSp()), stk);

  // Restore stack.
  __ movl(esp, Operand(ebp, -16));

  // Restore registers and gtfo.
  __ pop(ebx);
  __ pop(edi);
  __ pop(esi);
  __ pop(ebp);
  __ ret();

  // The universal emergency return will jump to here.
  Label error;
  __ bind(&error);
  __ movl(ecx, Operand(ebp, 8 + 4 * 0)); // ret-path expects ecx = ctx
  __ jmp(&ret);

  invoke_stub_ = LinkCode(env_, masm);
  if (!invoke_stub_)
    return false;

  return_stub_ = reinterpret_cast<uint8_t *>(invoke_stub_) + error.offset();
  return true;
}

SPVM_NATIVE_FUNC
CodeStubs::CreateFakeNativeStub(SPVM_FAKENATIVE_FUNC callback, void *pData)
{
  AssemblerX86 masm;

  __ push(ebx);
  __ push(edi);
  __ push(esi);
  __ movl(edi, Operand(esp, 16)); // store ctx
  __ movl(esi, Operand(esp, 20)); // store params
  __ movl(ebx, esp);
  __ andl(esp, 0xfffffff0);
  __ subl(esp, 4);

  __ push(intptr_t(pData));
  __ push(esi);
  __ push(edi);
  __ call(ExternalAddress((void *)callback));
  __ movl(esp, ebx);
  __ pop(esi);
  __ pop(edi);
  __ pop(ebx);
  __ ret();

  return (SPVM_NATIVE_FUNC)LinkCode(env_, masm);
}
