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
#include "watchdog_timer.h"

using namespace sp;
using namespace SourcePawn;

static Environment *sEnvironment = nullptr;

Environment::Environment()
 : debugger_(nullptr),
   profiler_(nullptr),
   jit_enabled_(true),
   profiling_enabled_(false)
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

  Environment *env = new Environment();
  if (!env->Initialize()) {
    delete env;
    return nullptr;
  }

  sEnvironment = env;
  return env;
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
  watchdog_timer_ = new WatchdogTimer();

  if (!g_Jit.InitializeJIT())
    return false;

  return true;
}

void
Environment::Shutdown()
{
  watchdog_timer_->Shutdown();
  g_Jit.ShutdownJIT();
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
  if (error < 1 || error > (sizeof(sErrorMsgTable) / sizeof(sErrorMsgTable[0])))
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
