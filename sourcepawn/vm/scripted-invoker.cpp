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

#include <stdio.h>
#include <string.h>
#include "scripted-invoker.h"
#include "plugin-runtime.h"
#include "environment.h"
#include "plugin-context.h"

/********************
* FUNCTION CALLING *
********************/

using namespace sp;
using namespace SourcePawn;

ScriptedInvoker::ScriptedInvoker(PluginRuntime *runtime, funcid_t id, uint32_t pub_id)
 : env_(Environment::get()),
   context_(runtime->GetBaseContext()),
   m_curparam(0),
   m_errorstate(SP_ERROR_NONE),
   m_FnId(id),
   cc_function_(nullptr)
{
  runtime->GetPublicByIndex(pub_id, &public_);

  size_t rt_len = strlen(runtime->Name());
  size_t len = rt_len + strlen("::") + strlen(public_->name);

  full_name_ = new char[len + 1];
  strcpy(full_name_, runtime->Name());
  strcpy(&full_name_[rt_len], "::");
  strcpy(&full_name_[rt_len + 2], public_->name);
}

ScriptedInvoker::~ScriptedInvoker()
{
}

bool
ScriptedInvoker::IsRunnable()
{
  return !context_->runtime()->IsPaused();
}

int
ScriptedInvoker::CallFunction(const cell_t *params, unsigned int num_params, cell_t *result)
{
  Environment::get()->ReportError(SP_ERROR_ABORTED);
  return SP_ERROR_ABORTED;
}

int
ScriptedInvoker::CallFunction2(IPluginContext *pContext, const cell_t *params, unsigned int num_params, cell_t *result)
{
  Environment::get()->ReportError(SP_ERROR_ABORTED);
  return SP_ERROR_ABORTED;
}

IPluginContext *
ScriptedInvoker::GetParentContext()
{
  return context_;
}

int ScriptedInvoker::PushCell(cell_t cell)
{
  if (m_curparam >= SP_MAX_EXEC_PARAMS)
    return SetError(SP_ERROR_PARAMS_MAX);

  m_info[m_curparam].marked = false;
  m_params[m_curparam] = cell;
  m_curparam++;

  return SP_ERROR_NONE;
}

int
ScriptedInvoker::PushCellByRef(cell_t *cell, int flags)
{
  return PushArray(cell, 1, flags);
}

int
ScriptedInvoker::PushFloat(float number)
{
  cell_t val = *(cell_t *)&number;

  return PushCell(val);
}

int
ScriptedInvoker::PushFloatByRef(float *number, int flags)
{
  return PushCellByRef((cell_t *)number, flags);
}

int
ScriptedInvoker::PushArray(cell_t *inarray, unsigned int cells, int copyback)
{
  if (m_curparam >= SP_MAX_EXEC_PARAMS)
  {
    return SetError(SP_ERROR_PARAMS_MAX);
  }

  ParamInfo *info = &m_info[m_curparam];

  info->flags = inarray ? copyback : 0;
  info->marked = true;
  info->size = cells;
  info->str.is_sz = false;
  info->orig_addr = inarray;

  m_curparam++;

  return SP_ERROR_NONE;
}

int
ScriptedInvoker::PushString(const char *string)
{
  return _PushString(string, SM_PARAM_STRING_COPY, 0, strlen(string)+1);
}

int
ScriptedInvoker::PushStringEx(char *buffer, size_t length, int sz_flags, int cp_flags)
{
  return _PushString(buffer, sz_flags, cp_flags, length);
}

int
ScriptedInvoker::_PushString(const char *string, int sz_flags, int cp_flags, size_t len)
{
  if (m_curparam >= SP_MAX_EXEC_PARAMS)
    return SetError(SP_ERROR_PARAMS_MAX);

  ParamInfo *info = &m_info[m_curparam];

  info->marked = true;
  info->orig_addr = (cell_t *)string;
  info->flags = cp_flags;
  info->size = len;
  info->str.sz_flags = sz_flags;
  info->str.is_sz = true;

  m_curparam++;

  return SP_ERROR_NONE;
}

void
ScriptedInvoker::Cancel()
{
  if (!m_curparam)
    return;

  m_errorstate = SP_ERROR_NONE;
  m_curparam = 0;
}

int
ScriptedInvoker::Execute(cell_t *result)
{
  Environment *env = Environment::get();
  env->clearPendingException();

  // For backward compatibility, we have to clear the exception state.
  // Otherwise code like this:
  //   
  // static cell_t native(cx, params) {
  //   for (auto callback : callbacks) {
  //     callback->Execute();
  //   }
  // }
  //
  // Could unintentionally leak a pending exception back to the caller,
  // which wouldn't have happened before the Great Exception Refactoring.
  ExceptionHandler eh(context_);
  if (!Invoke(result)) {
    assert(env->hasPendingException());
    return env->getPendingExceptionCode();
  }

  return SP_ERROR_NONE;
}

bool
ScriptedInvoker::Invoke(cell_t *result)
{
  if (!IsRunnable()) {
    Cancel();
    env_->ReportError(SP_ERROR_NOT_RUNNABLE);
    return false;
  }
  if (int err = m_errorstate) {
    Cancel();
    env_->ReportError(err);
    return false;
  }

  //This is for re-entrancy!
  cell_t temp_params[SP_MAX_EXEC_PARAMS];
  ParamInfo temp_info[SP_MAX_EXEC_PARAMS];
  unsigned int numparams = m_curparam;
  unsigned int i;

  if (numparams)
  {
    //Save the info locally, then reset it for re-entrant calls.
    memcpy(temp_info, m_info, numparams * sizeof(ParamInfo));
  }
  m_curparam = 0;

  /* Browse the parameters and build arrays */
  bool ok = true;
  for (i=0; i<numparams; i++) {
    /* Is this marked as an array? */
    if (temp_info[i].marked) {
      if (!temp_info[i].str.is_sz) {
        /* Allocate a normal/generic array */
        int err = context_->HeapAlloc(
          temp_info[i].size, 
          &(temp_info[i].local_addr),
          &(temp_info[i].phys_addr));
        if (err != SP_ERROR_NONE) {
          env_->ReportError(err);
          ok = false;
          break;
        }
        if (temp_info[i].orig_addr)
        {
          memcpy(temp_info[i].phys_addr, temp_info[i].orig_addr, sizeof(cell_t) * temp_info[i].size);
        }
      } else {
        /* Calculate cells required for the string */
        size_t cells = (temp_info[i].size + sizeof(cell_t) - 1) / sizeof(cell_t);

        /* Allocate the buffer */
        int err = context_->HeapAlloc(
          cells,
          &(temp_info[i].local_addr),
          &(temp_info[i].phys_addr));
        if (err != SP_ERROR_NONE) {
          env_->ReportError(err);
          ok = false;
          break;
        }

        /* Copy original string if necessary */
        if ((temp_info[i].str.sz_flags & SM_PARAM_STRING_COPY) && (temp_info[i].orig_addr != NULL))
        {
          /* Cut off UTF-8 properly */
          if (temp_info[i].str.sz_flags & SM_PARAM_STRING_UTF8) {
            context_->StringToLocalUTF8(
              temp_info[i].local_addr, 
              temp_info[i].size, 
              (const char *)temp_info[i].orig_addr,
              NULL);
          }
          /* Copy a binary blob */
          else if (temp_info[i].str.sz_flags & SM_PARAM_STRING_BINARY)
          {
            memmove(temp_info[i].phys_addr, temp_info[i].orig_addr, temp_info[i].size);
          }
          /* Copy ASCII characters */
          else
          {
            context_->StringToLocal(
              temp_info[i].local_addr,
              temp_info[i].size,
              (const char *)temp_info[i].orig_addr);
          }
        }
      } /* End array/string calculation */
      /* Update the pushed parameter with the byref local address */
      temp_params[i] = temp_info[i].local_addr;
    } else {
      /* Just copy the value normally */
      temp_params[i] = m_params[i];
    }
  }

  /* Make the call if we can */
  if (ok)
    ok = context_->Invoke(m_FnId, temp_params, numparams, result);

  /* i should be equal to the last valid parameter + 1 */
  bool docopies = ok;
  while (i--) {
    if (!temp_info[i].marked)
      continue;

    if (docopies && (temp_info[i].flags & SM_PARAM_COPYBACK)) {
      if (temp_info[i].orig_addr) {
        if (temp_info[i].str.is_sz) {
          memcpy(temp_info[i].orig_addr, temp_info[i].phys_addr, temp_info[i].size);
        
        } else {
          if (temp_info[i].size == 1) {
            *temp_info[i].orig_addr = *(temp_info[i].phys_addr);
          } else {
            memcpy(temp_info[i].orig_addr, 
                temp_info[i].phys_addr, 
                temp_info[i].size * sizeof(cell_t));
          }
        }
      }
    }

    if (int err = context_->HeapPop(temp_info[i].local_addr))
      env_->ReportError(err);
  }

  return !env_->hasPendingException();
}

int
ScriptedInvoker::Execute2(IPluginContext *ctx, cell_t *result)
{
  Environment::get()->ReportError(SP_ERROR_ABORTED);
  return SP_ERROR_ABORTED;
}

IPluginRuntime *
ScriptedInvoker::GetParentRuntime()
{
  return context_->runtime();
}

funcid_t
ScriptedInvoker::GetFunctionID()
{
  return m_FnId;
}

int
ScriptedInvoker::SetError(int err)
{
  m_errorstate = err;

  return err;
}

