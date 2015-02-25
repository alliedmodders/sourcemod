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
#include "environment.h"
#include "x86/jit_x86.h"
#include "watchdog_timer.h"
#include "debug-trace.h"
#include "api.h"
#include "code-stubs.h"
#include "watchdog_timer.h"

using namespace sp;
using namespace SourcePawn;

static Environment *sEnvironment = nullptr;

Environment::Environment()
 : debugger_(nullptr),
   profiler_(nullptr),
   jit_enabled_(true),
   profiling_enabled_(false),
   code_pool_(nullptr)
{
}

Environment::~Environment()
{
}

Environment *
Environment::New()
{
  assert(!sEnvironment);
  if (sEnvironment)
    return nullptr;

  sEnvironment = new Environment();
  if (!sEnvironment->Initialize()) {
    delete sEnvironment;
    sEnvironment = nullptr;
    return nullptr;
  }

  return sEnvironment;
}

Environment *
Environment::get()
{
  return sEnvironment;
}

bool
Environment::Initialize()
{
  api_v1_ = new SourcePawnEngine();
  api_v2_ = new SourcePawnEngine2();
  code_stubs_ = new CodeStubs(this);
  watchdog_timer_ = new WatchdogTimer(this);

  if ((code_pool_ = Knight::KE_CreateCodeCache()) == nullptr)
    return false;

  // Safe to initialize code now that we have the code cache.
  if (!code_stubs_->Initialize())
    return false;

  return true;
}

void
Environment::Shutdown()
{
  watchdog_timer_->Shutdown();
  code_stubs_->Shutdown();
  Knight::KE_DestroyCodeCache(code_pool_);

  assert(sEnvironment == this);
  sEnvironment = nullptr;
}

void
Environment::EnableProfiling()
{
  profiling_enabled_ = !!profiler_;
}

void
Environment::DisableProfiling()
{
  profiling_enabled_ = false;
}

bool
Environment::InstallWatchdogTimer(int timeout_ms)
{
  return watchdog_timer_->Initialize(timeout_ms);
}

ISourcePawnEngine *
Environment::APIv1()
{
  return api_v1_;
}

ISourcePawnEngine2 *
Environment::APIv2()
{
  return api_v2_;
}

static const char *sErrorMsgTable[] = 
{
  NULL,
  "Unrecognizable file format",
  "Decompressor was not found",
  "Not enough space on the heap",
  "Invalid parameter or parameter type",
  "Invalid plugin address",
  "Object or index not found",
  "Invalid index or index not found",
  "Not enough space on the stack",
  "Debug section not found or debug not enabled",
  "Invalid instruction",
  "Invalid memory access",
  "Stack went below stack boundary",
  "Heap went below heap boundary",
  "Divide by zero",
  "Array index is out of bounds",
  "Instruction contained invalid parameter",
  "Stack memory leaked by native",
  "Heap memory leaked by native",
  "Dynamic array is too big",
  "Tracker stack is out of bounds",
  "Native is not bound",
  "Maximum number of parameters reached",
  "Native detected error",
  "Plugin not runnable",
  "Call was aborted",
  "Plugin format is too old",
  "Plugin format is too new",
  "Out of memory",
  "Integer overflow",
  "Script execution timed out"
};

const char *
Environment::GetErrorString(int error)
{
  if (error < 1 || error > int(sizeof(sErrorMsgTable) / sizeof(sErrorMsgTable[0])))
    return NULL;
  return sErrorMsgTable[error];
}

void
Environment::ReportError(PluginRuntime *runtime, int err, const char *errstr, cell_t rp_start)
{
  if (!debugger_)
    return;

  CContextTrace trace(runtime, err, errstr, rp_start);

  debugger_->OnContextExecuteError(runtime->GetDefaultContext(), &trace);
}

void *
Environment::AllocateCode(size_t size)
{
  return Knight::KE_AllocCode(code_pool_, size);
}

void
Environment::FreeCode(void *code)
{
  Knight::KE_FreeCode(code_pool_, code);
}

void
Environment::RegisterRuntime(PluginRuntime *rt)
{
  mutex_.AssertCurrentThreadOwns();
  runtimes_.append(rt);
}

void
Environment::DeregisterRuntime(PluginRuntime *rt)
{
  mutex_.AssertCurrentThreadOwns();
  runtimes_.remove(rt);
}

void
Environment::PatchAllJumpsForTimeout()
{
  mutex_.AssertCurrentThreadOwns();
  for (ke::InlineList<PluginRuntime>::iterator iter = runtimes_.begin(); iter != runtimes_.end(); iter++) {
    PluginRuntime *rt = *iter;
    for (size_t i = 0; i < rt->NumJitFunctions(); i++) {
      CompiledFunction *fun = rt->GetJitFunction(i);
      uint8_t *base = reinterpret_cast<uint8_t *>(fun->GetEntryAddress());

      for (size_t j = 0; j < fun->NumLoopEdges(); j++) {
        const LoopEdge &e = fun->GetLoopEdge(j);
        int32_t diff = intptr_t(code_stubs_->TimeoutStub()) - intptr_t(base + e.offset);
        *reinterpret_cast<int32_t *>(base + e.offset - 4) = diff;
      }
    }
  }
}

void
Environment::UnpatchAllJumpsFromTimeout()
{
  mutex_.AssertCurrentThreadOwns();
  for (ke::InlineList<PluginRuntime>::iterator iter = runtimes_.begin(); iter != runtimes_.end(); iter++) {
    PluginRuntime *rt = *iter;
    for (size_t i = 0; i < rt->NumJitFunctions(); i++) {
      CompiledFunction *fun = rt->GetJitFunction(i);
      uint8_t *base = reinterpret_cast<uint8_t *>(fun->GetEntryAddress());

      for (size_t j = 0; j < fun->NumLoopEdges(); j++) {
        const LoopEdge &e = fun->GetLoopEdge(j);
        *reinterpret_cast<int32_t *>(base + e.offset - 4) = e.disp32;
      }
    }
  }
}

int
Environment::Invoke(PluginRuntime *runtime, CompiledFunction *fn, cell_t *result)
{
  PluginContext *cx = runtime->GetBaseContext();

  // Note that cip, hp, sp are saved and restored by Execute2().
  *cx->addressOfCip() = fn->GetCodeOffset();

  InvokeStubFn invoke = code_stubs_->InvokeStub();

  EnterInvoke();
  int err = invoke(cx, fn->GetEntryAddress(), result);
  LeaveInvoke();

  return err;
}
