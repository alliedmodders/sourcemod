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
#ifndef _INCLUDE_SOURCEPAWN_VM_ENGINE_H_
#define _INCLUDE_SOURCEPAWN_VM_ENGINE_H_

#include <sp_vm_api.h>
#include <am-utility.h> // Replace with am-cxx.h later.
#include "scripted-invoker.h"

class BaseContext;

class SourcePawnEngine : public ISourcePawnEngine
{
 public:
  SourcePawnEngine();
  ~SourcePawnEngine();

 public: //ISourcePawnEngine
  sp_plugin_t *LoadFromFilePointer(FILE *fp, int *err) KE_OVERRIDE;
  sp_plugin_t *LoadFromMemory(void *base, sp_plugin_t *plugin, int *err) KE_OVERRIDE;
  int FreeFromMemory(sp_plugin_t *plugin) KE_OVERRIDE;
  void *BaseAlloc(size_t size) KE_OVERRIDE;
  void BaseFree(void *memory) KE_OVERRIDE;
  void *ExecAlloc(size_t size) KE_OVERRIDE;
  void ExecFree(void *address) KE_OVERRIDE;
  IDebugListener *SetDebugListener(IDebugListener *pListener) KE_OVERRIDE;
  unsigned int GetContextCallCount() KE_OVERRIDE;
  unsigned int GetEngineAPIVersion() KE_OVERRIDE;
  void *AllocatePageMemory(size_t size) KE_OVERRIDE;
  void SetReadWrite(void *ptr) KE_OVERRIDE;
  void SetReadExecute(void *ptr) KE_OVERRIDE;
  void FreePageMemory(void *ptr) KE_OVERRIDE;
  void SetReadWriteExecute(void *ptr);
  const char *GetErrorString(int err);
};

#endif //_INCLUDE_SOURCEPAWN_VM_ENGINE_H_
