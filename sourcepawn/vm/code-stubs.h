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
#ifndef _include_sourcepawn_vm_code_stubs_h_
#define _include_sourcepawn_vm_code_stubs_h_

#include <stdint.h>
#include <sp_vm_api.h>

namespace sp {

class PluginContext;
class Environment;

typedef int (*InvokeStubFn)(PluginContext *cx, void *code, cell_t *rval);

class CodeStubs
{
 public:
  CodeStubs(Environment *env);

 public:
  bool Initialize();
  void Shutdown();

  SPVM_NATIVE_FUNC CreateFakeNativeStub(SPVM_FAKENATIVE_FUNC callback, void *userData);

  InvokeStubFn InvokeStub() const {
    return (InvokeStubFn)invoke_stub_;
  }
  void *ReturnStub() const {
    return return_stub_;
  }

 private:
  bool InitializeFeatureDetection();
  bool CompileInvokeStub();

 private:
  Environment *env_;
  void *invoke_stub_;
  void *return_stub_;   // Owned by invoke_stub_.
};

}

#endif // _include_sourcepawn_vm_code_stubs_h_
