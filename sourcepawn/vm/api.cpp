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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "x86/jit_x86.h"
#include "environment.h"
#include "api.h"
#include <zlib/zlib.h>
#if defined __GNUC__
#include <unistd.h>
#endif

#if defined WIN32
 #define WIN32_LEAN_AND_MEAN
 #include <windows.h>
#elif defined __GNUC__
 #include <sys/mman.h>
#endif

#if defined __linux__
#include <malloc.h>
#endif

#include <sourcemod_version.h>
#include "code-stubs.h"
#include "smx-v1-image.h"

using namespace sp;
using namespace SourcePawn;

// ////// //
// API v1
// ////// //

SourcePawnEngine::SourcePawnEngine()
{
}

const char *
SourcePawnEngine::GetErrorString(int error)
{
  return Environment::get()->GetErrorString(error);
}

void *
SourcePawnEngine::ExecAlloc(size_t size)
{
#if defined WIN32
  return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#elif defined __GNUC__
# if defined __APPLE__
  void *base = valloc(size);
# else
  void *base = memalign(sysconf(_SC_PAGESIZE), size);
# endif
  if (mprotect(base, size, PROT_READ|PROT_WRITE|PROT_EXEC) != 0) {
    free(base);
    return NULL;
  }
  return base;
#endif
}

void *
SourcePawnEngine::AllocatePageMemory(size_t size)
{
  return Environment::get()->AllocateCode(size);
}

void
SourcePawnEngine::SetReadExecute(void *ptr)
{
  /* already re */
}

void
SourcePawnEngine::SetReadWrite(void *ptr)
{
  /* already rw */
}

void
SourcePawnEngine::FreePageMemory(void *ptr)
{
  Environment::get()->FreeCode(ptr);
}

void
SourcePawnEngine::ExecFree(void *address)
{
#if defined WIN32
  VirtualFree(address, 0, MEM_RELEASE);
#elif defined __GNUC__
  free(address);
#endif
}

void
SourcePawnEngine::SetReadWriteExecute(void *ptr)
{
  //:TODO:  g_ExeMemory.SetRWE(ptr);
  SetReadExecute(ptr);
}

void *
SourcePawnEngine::BaseAlloc(size_t size)
{
  return malloc(size);
}

void
SourcePawnEngine::BaseFree(void *memory)
{
  free(memory);
}

sp_plugin_t *
SourcePawnEngine::LoadFromFilePointer(FILE *fp, int *err)
{
  if (err != NULL)
    *err = SP_ERROR_ABORTED;

  return NULL;
}

sp_plugin_t *
SourcePawnEngine::LoadFromMemory(void *base, sp_plugin_t *plugin, int *err)
{
  if (err != NULL)
    *err = SP_ERROR_ABORTED;

  return NULL;
}

int
SourcePawnEngine::FreeFromMemory(sp_plugin_t *plugin)
{
  return SP_ERROR_ABORTED;
}

IDebugListener *
SourcePawnEngine::SetDebugListener(IDebugListener *pListener)
{
  IDebugListener *old = Environment::get()->debugger();
  Environment::get()->SetDebugger(pListener);
  return old;
}

unsigned int
SourcePawnEngine::GetEngineAPIVersion()
{
  return 4;
}

unsigned int
SourcePawnEngine::GetContextCallCount()
{
  return 0;
}

// ////// //
// API v2
// ////// //

SourcePawnEngine2::SourcePawnEngine2()
{
}

size_t
sp::UTIL_FormatVA(char *buffer, size_t maxlength, const char *fmt, va_list ap)
{
  size_t len = vsnprintf(buffer, maxlength, fmt, ap);

  if (len >= maxlength) {
    buffer[maxlength - 1] = '\0';
    return maxlength - 1;
  }
  return len;
}

size_t
sp::UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  size_t len = UTIL_FormatVA(buffer, maxlength, fmt, ap);
  va_end(ap);

  return len;
}

IPluginRuntime *
SourcePawnEngine2::LoadPlugin(ICompilation *co, const char *file, int *err)
{
  if (co) {
    if (err)
      *err = SP_ERROR_PARAM;
    return nullptr;
  }

  IPluginRuntime *rt = LoadBinaryFromFile(file, nullptr, 0);
  if (!rt) {
    if (err) {
      if (FILE *fp = fopen(file, "rb")) {
        fclose(fp);
        *err = SP_ERROR_FILE_FORMAT;
      } else {
        *err = SP_ERROR_NOT_FOUND;
      }
    }
    return nullptr;
  }

  return rt;
}

IPluginRuntime *
SourcePawnEngine2::LoadBinaryFromFile(const char *file, char *error, size_t maxlength)
{
  FILE *fp = fopen(file, "rb");

  if (!fp) {
    UTIL_Format(error, maxlength, "file not found");
    return nullptr;
  }

  ke::AutoPtr<SmxV1Image> image(new SmxV1Image(fp));
  fclose(fp);

  if (!image->validate()) {
    const char *errorMessage = image->errorMessage();
    if (!errorMessage)
      errorMessage = "file parse error";
    UTIL_Format(error, maxlength, "%s", errorMessage);
    return nullptr;
  }

  PluginRuntime *pRuntime = new PluginRuntime(image.take());
  if (!pRuntime->Initialize()) {
    delete pRuntime;

    UTIL_Format(error, maxlength, "out of memory");
    return nullptr;
  }

  size_t len = strlen(file);
  for (size_t i = len - 1; i < len; i--) {
    if (file[i] == '/' 
# if defined WIN32
      || file[i] == '\\'
# endif
    )
    {
      pRuntime->SetNames(file, &file[i + 1]);
      break;
    }
  }

  if (!pRuntime->Name())
    pRuntime->SetNames(file, file);

  return pRuntime;
}

SPVM_NATIVE_FUNC
SourcePawnEngine2::CreateFakeNative(SPVM_FAKENATIVE_FUNC callback, void *pData)
{
  return Environment::get()->stubs()->CreateFakeNativeStub(callback, pData);
}

void
SourcePawnEngine2::DestroyFakeNative(SPVM_NATIVE_FUNC func)
{
  return Environment::get()->FreeCode((void *)func);
}

const char *
SourcePawnEngine2::GetEngineName()
{
  return "SourcePawn 1.7, jit-x86";
}

const char *
SourcePawnEngine2::GetVersionString()
{
  return SOURCEMOD_VERSION;
}

IDebugListener *
SourcePawnEngine2::SetDebugListener(IDebugListener *listener)
{
  IDebugListener *old = Environment::get()->debugger();
  Environment::get()->SetDebugger(listener);
  return old;
}

unsigned int
SourcePawnEngine2::GetAPIVersion()
{
  return SOURCEPAWN_ENGINE2_API_VERSION;
}

ICompilation *
SourcePawnEngine2::StartCompilation()
{
  return nullptr;
}

const char *
SourcePawnEngine2::GetErrorString(int err)
{
  return Environment::get()->GetErrorString(err);
}

bool
SourcePawnEngine2::Initialize()
{
  return true;
}

void
SourcePawnEngine2::Shutdown()
{
}

IPluginRuntime *
SourcePawnEngine2::CreateEmptyRuntime(const char *name, uint32_t memory)
{
  int err;

  ke::AutoPtr<EmptyImage> image(new EmptyImage(memory));

  PluginRuntime *rt = new PluginRuntime(image.take());
  if (!rt->Initialize()) {
    delete rt;
    return NULL;
  }

  if (!name)
    name = "<anonymous>";
  rt->SetNames(name, name);
  return rt;
}

bool
SourcePawnEngine2::InstallWatchdogTimer(size_t timeout_ms)
{
  return Environment::get()->InstallWatchdogTimer(timeout_ms);
}

bool
SourcePawnEngine2::SetJitEnabled(bool enabled)
{
  Environment::get()->SetJitEnabled(enabled);
  return Environment::get()->IsJitEnabled() == enabled;
}

bool
SourcePawnEngine2::IsJitEnabled()
{
  return Environment::get()->IsJitEnabled();
}

void
SourcePawnEngine2::SetProfiler(IProfiler *profiler)
{
  // Deprecated.
}

void
SourcePawnEngine2::EnableProfiling()
{
  Environment::get()->EnableProfiling();
}

void
SourcePawnEngine2::DisableProfiling()
{
  Environment::get()->DisableProfiling();
}

void
SourcePawnEngine2::SetProfilingTool(IProfilingTool *tool)
{
  Environment::get()->SetProfiler(tool);
}

ISourcePawnEnvironment *
SourcePawnEngine2::Environment()
{
  return Environment::get();
}
