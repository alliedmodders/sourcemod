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
#include "debug-trace.h"
#include "plugin-context.h"
#include "environment.h"
#include "plugin-runtime.h"

using namespace ke;
using namespace sp;
using namespace SourcePawn;

CContextTrace::CContextTrace(PluginRuntime *pRuntime, int err, const char *errstr, cell_t start_rp) 
 : m_pRuntime(pRuntime),
   context_(pRuntime->GetBaseContext()),
   m_Error(err),
   m_pMsg(errstr),
   m_StartRp(start_rp),
   m_Level(0)
{
  m_pDebug = m_pRuntime->GetDebugInfo();
}

bool
CContextTrace::DebugInfoAvailable()
{
  return (m_pDebug != NULL);
}

const char *
CContextTrace::GetCustomErrorString()
{
  return m_pMsg;
}

int
CContextTrace::GetErrorCode()
{
  return m_Error;
}

const char *
CContextTrace::GetErrorString()
{
  return Environment::get()->GetErrorString(m_Error);
}

void
CContextTrace::ResetTrace()
{
  m_Level = 0;
}

bool
CContextTrace::GetTraceInfo(CallStackInfo *trace)
{
  cell_t cip;

  if (m_Level == 0) {
    cip = context_->cip();
  } else if (context_->rp() > 0) {
    /* Entries go from ctx.rp - 1 to m_StartRp */
    cell_t offs, start, end;

    offs = m_Level - 1;
    start = context_->rp() - 1;
    end = m_StartRp;

    if (start - offs < end)
      return false;

    cip = context_->getReturnStackCip(start - offs);
  } else {
    return false;
  }

  if (trace == NULL) {
    m_Level++;
    return true;
  }

  if (m_pDebug->LookupFile(cip, &(trace->filename)) != SP_ERROR_NONE)
    trace->filename = NULL;

  if (m_pDebug->LookupFunction(cip, &(trace->function)) != SP_ERROR_NONE)
    trace->function = NULL;

  if (m_pDebug->LookupLine(cip, &(trace->line)) != SP_ERROR_NONE)
    trace->line = 0;

  m_Level++;

  return true;
}

const char *
CContextTrace::GetLastNative(uint32_t *index)
{
  if (context_->GetLastNativeError() == SP_ERROR_NONE)
    return NULL;

  int lastNative = context_->lastNative();
  if (lastNative < 0)
    return NULL;

  const sp_native_t *native = m_pRuntime->GetNative(lastNative);
  if (!native)
    return NULL;

  if (index)
    *index = lastNative;

  return native->name;
}
