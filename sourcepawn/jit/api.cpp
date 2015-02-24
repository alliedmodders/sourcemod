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
#include <string.h>
#include <assert.h>
#include <KeCodeAllocator.h>
#include "x86/jit_x86.h"
#include "environment.h"
#include "api.h"
#include "zlib/zlib.h"
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
  return g_Jit.AllocCode(size);
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
  g_Jit.FreeCode(ptr);
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

IPluginRuntime *
SourcePawnEngine2::LoadPlugin(ICompilation *co, const char *file, int *err)
{
  sp_file_hdr_t hdr;
  uint8_t *base;
  int z_result;
  int error;
  size_t ignore;
  PluginRuntime *pRuntime;

  FILE *fp = fopen(file, "rb");

  if (!fp) {
    error = SP_ERROR_NOT_FOUND;
    goto return_error;
  }

  /* Rewind for safety */
  ignore = fread(&hdr, sizeof(sp_file_hdr_t), 1, fp);

  if (hdr.magic != SmxConsts::FILE_MAGIC) {
    error = SP_ERROR_FILE_FORMAT;
    goto return_error;
  }

  switch (hdr.compression)
  {
  case SmxConsts::FILE_COMPRESSION_GZ:
    {
      uint32_t uncompsize = hdr.imagesize - hdr.dataoffs;
      uint32_t compsize = hdr.disksize - hdr.dataoffs;
      uint32_t sectsize = hdr.dataoffs - sizeof(sp_file_hdr_t);
      uLongf destlen = uncompsize;

      char *tempbuf = (char *)malloc(compsize);
      void *uncompdata = malloc(uncompsize);
      void *sectheader = malloc(sectsize);

      ignore = fread(sectheader, sectsize, 1, fp);
      ignore = fread(tempbuf, compsize, 1, fp);

      z_result = uncompress((Bytef *)uncompdata, &destlen, (Bytef *)tempbuf, compsize);
      free(tempbuf);
      if (z_result != Z_OK)
      {
        free(sectheader);
        free(uncompdata);
        error = SP_ERROR_DECOMPRESSOR;
        goto return_error;
      }

      base = (uint8_t *)malloc(hdr.imagesize);
      memcpy(base, &hdr, sizeof(sp_file_hdr_t));
      memcpy(base + sizeof(sp_file_hdr_t), sectheader, sectsize);
      free(sectheader);
      memcpy(base + hdr.dataoffs, uncompdata, uncompsize);
      free(uncompdata);
      break;
    }
  case SmxConsts::FILE_COMPRESSION_NONE:
    {
      base = (uint8_t *)malloc(hdr.imagesize);
      rewind(fp);
      ignore = fread(base, hdr.imagesize, 1, fp);
      break;
    }
  default:
    {
      error = SP_ERROR_DECOMPRESSOR;
      goto return_error;
    }
  }

  pRuntime = new PluginRuntime();
  if ((error = pRuntime->CreateFromMemory(&hdr, base)) != SP_ERROR_NONE) {
    delete pRuntime;
    goto return_error;
  }

  size_t len;
  
  len = strlen(file);
  for (size_t i = len - 1; i < len; i--)
  {
    if (file[i] == '/' 
    #if defined WIN32
      || file[i] == '\\'
    #endif
    )
    {
      pRuntime->SetName(&file[i+1]);
      break;
    }
  }

  (void)ignore;

  if (!pRuntime->plugin()->name)
    pRuntime->SetName(file);

  pRuntime->ApplyCompilationOptions(co);

  fclose(fp);

  return pRuntime;

return_error:
  *err = error;
  if (fp != NULL)
  {
      fclose(fp);
  }

  return NULL;
}

SPVM_NATIVE_FUNC
SourcePawnEngine2::CreateFakeNative(SPVM_FAKENATIVE_FUNC callback, void *pData)
{
  return g_Jit.CreateFakeNative(callback, pData);
}

void
SourcePawnEngine2::DestroyFakeNative(SPVM_NATIVE_FUNC func)
{
  g_Jit.DestroyFakeNative(func);
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
  return g_Jit.StartCompilation();
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

  PluginRuntime *rt = new PluginRuntime();
  if ((err = rt->CreateBlank(memory)) != SP_ERROR_NONE) {
    delete rt;
    return NULL;
  }

  rt->SetName(name != NULL ? name : "<anonymous>");

  rt->ApplyCompilationOptions(NULL);
  
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
