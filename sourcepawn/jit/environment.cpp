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
#include "api.h"
#include "code-stubs.h"
#include "watchdog_timer.h"
#include <stdarg.h>

using namespace sp;
using namespace SourcePawn;

static Environment *sEnvironment = nullptr;

Environment::Environment()
 : debugger_(nullptr),
   exception_code_(SP_ERROR_NONE),
   profiler_(nullptr),
   jit_enabled_(true),
   profiling_enabled_(false),
   code_pool_(nullptr),
   top_(nullptr)
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
  "Script execution timed out",
  "Custom error",
  "Fatal error"
};

const char *
Environment::GetErrorString(int error)
{
  if (error < 1 || error > int(sizeof(sErrorMsgTable) / sizeof(sErrorMsgTable[0])))
    return NULL;
  return sErrorMsgTable[error];
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

static inline void
SwapLoopEdge(uint8_t *code, LoopEdge &e)
{
  int32_t *loc = reinterpret_cast<int32_t *>(code + e.offset - 4);
  int32_t new_disp32 = e.disp32;
  e.disp32 = *loc;
  *loc = new_disp32;
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

      for (size_t j = 0; j < fun->NumLoopEdges(); j++)
        SwapLoopEdge(base, fun->GetLoopEdge(j));
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

      for (size_t j = 0; j < fun->NumLoopEdges(); j++)
        SwapLoopEdge(base, fun->GetLoopEdge(j));
    }
  }
}

int
Environment::Invoke(PluginRuntime *runtime, CompiledFunction *fn, cell_t *result)
{
  // Must be in an invoke frame.
  assert(top_ && top_->cx() == runtime->GetBaseContext());

  PluginContext *cx = runtime->GetBaseContext();

  InvokeStubFn invoke = code_stubs_->InvokeStub();
  return invoke(cx, fn->GetEntryAddress(), result);
}

void
Environment::ReportError(int code)
{
  const char *message = GetErrorString(code);
  if (!message) {
    char buffer[255];
    UTIL_Format(buffer, sizeof(buffer), "Unknown error code %d", code);
    ReportError(code, buffer);
  } else {
    ReportError(code, message);
  }
}

class ErrorReport : public SourcePawn::IErrorReport
{
 public:
  ErrorReport(int code, const char *message, PluginContext *cx)
   : code_(code),
     message_(message),
     context_(cx)
  {}

  const char *Message() const KE_OVERRIDE {
    return message_;
  }
  bool IsFatal() const KE_OVERRIDE {
    switch (code_) {
      case SP_ERROR_HEAPLOW:
      case SP_ERROR_INVALID_ADDRESS:
      case SP_ERROR_STACKLOW:
      case SP_ERROR_INVALID_INSTRUCTION:
      case SP_ERROR_MEMACCESS:
      case SP_ERROR_STACKMIN:
      case SP_ERROR_HEAPMIN:
      case SP_ERROR_INSTRUCTION_PARAM:
      case SP_ERROR_STACKLEAK:
      case SP_ERROR_HEAPLEAK:
      case SP_ERROR_TRACKER_BOUNDS:
      case SP_ERROR_PARAMS_MAX:
      case SP_ERROR_ABORTED:
      case SP_ERROR_OUT_OF_MEMORY:
      case SP_ERROR_FATAL:
        return true;
      default:
        return false;
    }
  }
  IPluginContext *Context() const KE_OVERRIDE {
    return context_;
  }

 private:
  int code_;
  const char *message_;
  PluginContext *context_;
};

void
Environment::ReportErrorVA(const char *fmt, va_list ap)
{
  ReportErrorVA(SP_ERROR_USER, fmt, ap);
}

void
Environment::ReportErrorVA(int code, const char *fmt, va_list ap)
{
  // :TODO: right-size the string rather than rely on this buffer.
  char buffer[1024];
  UTIL_FormatVA(buffer, sizeof(buffer), fmt, ap);
  ReportError(code, buffer);
}

void
Environment::ReportErrorFmt(int code, const char *message, ...)
{
  va_list ap;
  va_start(ap, message);
  ReportErrorVA(code, message, ap);
  va_end(ap);
}

void
Environment::ReportError(int code, const char *message)
{
  FrameIterator iter;
  ErrorReport report(code, message, top_ ? top_->cx() : nullptr);

  // If this fires, someone forgot to propagate an error.
  assert(!hasPendingException());

  // Save the exception state.
  if (eh_top_) {
    exception_code_ = code;
    UTIL_Format(exception_message_, sizeof(exception_message_), "%s", message);
  }

  // For now, we always report exceptions even if they might be handled.
  if (debugger_)
    debugger_->ReportError(report, iter);
}

void
Environment::EnterExceptionHandlingScope(ExceptionHandler *handler)
{
  handler->next_ = eh_top_;
  eh_top_ = handler;
}

void
Environment::LeaveExceptionHandlingScope(ExceptionHandler *handler)
{
  assert(handler == eh_top_);
  eh_top_ = eh_top_->next_;

  // To preserve compatibility with older API, we clear the exception state
  // when there is no EH handler.
  if (!eh_top_ || handler->catch_)
    exception_code_ = SP_ERROR_NONE;
}

bool
Environment::HasPendingException(const ExceptionHandler *handler)
{
  // Note here and elsewhere - this is not a sanity assert. In the future, the
  // API may need to query the handler.
  assert(handler == eh_top_);
  return hasPendingException();
}

const char *
Environment::GetPendingExceptionMessage(const ExceptionHandler *handler)
{
  // Note here and elsewhere - this is not a sanity assert. In the future, the
  // API may need to query the handler.
  assert(handler == eh_top_);
  assert(HasPendingException(handler));
  return exception_message_;
}

bool
Environment::hasPendingException() const
{
  return exception_code_ != SP_ERROR_NONE;
}

void
Environment::clearPendingException()
{
  exception_code_ = SP_ERROR_NONE;
}

int
Environment::getPendingExceptionCode() const
{
  return exception_code_;
}
