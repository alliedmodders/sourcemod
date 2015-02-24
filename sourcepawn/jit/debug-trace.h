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
#ifndef _include_sourcepawn_vm_debug_trace_h_
#define _include_sourcepawn_vm_debug_trace_h_

#include <sp_vm_api.h>

class PluginRuntime;

namespace sp {

using namespace SourcePawn;

class CContextTrace : public IContextTrace
{
 public:
  CContextTrace(PluginRuntime *pRuntime, int err, const char *errstr, cell_t start_rp);

 public:
  int GetErrorCode();
  const char *GetErrorString();
  bool DebugInfoAvailable();
  const char *GetCustomErrorString();
  bool GetTraceInfo(CallStackInfo *trace);
  void ResetTrace();
  const char *GetLastNative(uint32_t *index);

 private:
  PluginRuntime *m_pRuntime;
  sp_context_t *m_ctx;
  int m_Error;
  const char *m_pMsg;
  cell_t m_StartRp;
  cell_t m_Level;
  IPluginDebugInfo *m_pDebug;
};

}

#endif // _include_sourcepawn_vm_debug_trace_h_
