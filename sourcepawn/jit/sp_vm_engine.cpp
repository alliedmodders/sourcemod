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
#include "sp_vm_types.h"
#include <KeCodeAllocator.h>
#include "sp_vm_engine.h"
#include "jit_x86.h"
#include "zlib/zlib.h"
#include "sp_vm_basecontext.h"
#include "environment.h"
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

using namespace SourcePawn;

const char *
SourcePawnEngine::GetErrorString(int error)
{
  return Environment::get()->GetErrorString(error);
}

SourcePawnEngine::SourcePawnEngine()
{
}

SourcePawnEngine::~SourcePawnEngine()
{
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
