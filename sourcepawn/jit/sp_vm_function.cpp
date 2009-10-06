/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourcePawn
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 */

#include <stdio.h>
#include <string.h>
#include "sp_vm_function.h"
#include "BaseRuntime.h"

/********************
* FUNCTION CALLING *
********************/

void CFunction::Set(BaseRuntime *runtime, funcid_t fnid, uint32_t pub_id)
{
	m_pRuntime = runtime;
	m_curparam = 0;
	m_errorstate = SP_ERROR_NONE;
	m_FnId = fnid;
}

bool CFunction::IsRunnable()
{
	return !m_pRuntime->IsPaused();
}

int CFunction::CallFunction(const cell_t *params, unsigned int num_params, cell_t *result)
{
	return CallFunction2(m_pRuntime->GetDefaultContext(), params, num_params, result);
}

int CFunction::CallFunction2(IPluginContext *pContext, const cell_t *params, unsigned int num_params, cell_t *result)
{
	return pContext->Execute2(this, params, num_params, result);
}

IPluginContext *CFunction::GetParentContext()
{
	return m_pRuntime->GetDefaultContext();
}

CFunction::CFunction(BaseRuntime *runtime, funcid_t id, uint32_t pub_id) : 
	m_curparam(0), m_errorstate(SP_ERROR_NONE), m_FnId(id)
{
	m_pRuntime = runtime;
}

int CFunction::PushCell(cell_t cell)
{
	if (m_curparam >= SP_MAX_EXEC_PARAMS)
	{
		return SetError(SP_ERROR_PARAMS_MAX);
	}

	m_info[m_curparam].marked = false;
	m_params[m_curparam] = cell;
	m_curparam++;

	return SP_ERROR_NONE;
}

int CFunction::PushCellByRef(cell_t *cell, int flags)
{
	return PushArray(cell, 1, flags);
}

int CFunction::PushFloat(float number)
{
	cell_t val = *(cell_t *)&number;

	return PushCell(val);
}

int CFunction::PushFloatByRef(float *number, int flags)
{
	return PushCellByRef((cell_t *)number, flags);
}

int CFunction::PushArray(cell_t *inarray, unsigned int cells, int copyback)
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

int CFunction::PushString(const char *string)
{
	return _PushString(string, SM_PARAM_STRING_COPY, 0, strlen(string)+1);
}

int CFunction::PushStringEx(char *buffer, size_t length, int sz_flags, int cp_flags)
{
	return _PushString(buffer, sz_flags, cp_flags, length);
}

int CFunction::_PushString(const char *string, int sz_flags, int cp_flags, size_t len)
{
	if (m_curparam >= SP_MAX_EXEC_PARAMS)
	{
		return SetError(SP_ERROR_PARAMS_MAX);
	}

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

void CFunction::Cancel()
{
	if (!m_curparam)
	{
		return;
	}

	m_errorstate = SP_ERROR_NONE;
	m_curparam = 0;
}

int CFunction::Execute(cell_t *result)
{
	return Execute2(m_pRuntime->GetDefaultContext(), result);
}

int CFunction::Execute2(IPluginContext *ctx, cell_t *result)
{
	int err = SP_ERROR_NONE;

	if (!IsRunnable())
	{
		m_errorstate = SP_ERROR_NOT_RUNNABLE;
	}

	if (m_errorstate != SP_ERROR_NONE)
	{
		err = m_errorstate;
		Cancel();
		return err;
	}

	//This is for re-entrancy!
	cell_t temp_params[SP_MAX_EXEC_PARAMS];
	ParamInfo temp_info[SP_MAX_EXEC_PARAMS];
	unsigned int numparams = m_curparam;
	unsigned int i;
	bool docopies = true;

	if (numparams)
	{
		//Save the info locally, then reset it for re-entrant calls.
		memcpy(temp_info, m_info, numparams * sizeof(ParamInfo));
	}
	m_curparam = 0;

	/* Browse the parameters and build arrays */
	for (i=0; i<numparams; i++)
	{
		/* Is this marked as an array? */
		if (temp_info[i].marked)
		{
			if (!temp_info[i].str.is_sz)
			{
				/* Allocate a normal/generic array */
				if ((err=ctx->HeapAlloc(temp_info[i].size, 
										   &(temp_info[i].local_addr),
										   &(temp_info[i].phys_addr)))
					!= SP_ERROR_NONE)
				{
					break;
				}
				if (temp_info[i].orig_addr)
				{
					memcpy(temp_info[i].phys_addr, temp_info[i].orig_addr, sizeof(cell_t) * temp_info[i].size);
				}
			}
			else
			{
				/* Calculate cells required for the string */
				size_t cells = (temp_info[i].size + sizeof(cell_t) - 1) / sizeof(cell_t);

				/* Allocate the buffer */
				if ((err=ctx->HeapAlloc(cells,
										&(temp_info[i].local_addr),
										&(temp_info[i].phys_addr)))
					!= SP_ERROR_NONE)
				{
					break;
				}
				/* Copy original string if necessary */
				if ((temp_info[i].str.sz_flags & SM_PARAM_STRING_COPY) && (temp_info[i].orig_addr != NULL))
				{
					/* Cut off UTF-8 properly */
					if (temp_info[i].str.sz_flags & SM_PARAM_STRING_UTF8)
					{
						if ((err=ctx->StringToLocalUTF8(temp_info[i].local_addr, 
															temp_info[i].size, 
															(const char *)temp_info[i].orig_addr,
															NULL))
							!= SP_ERROR_NONE)
						{
							break;
						}
					}
					/* Copy a binary blob */
					else if (temp_info[i].str.sz_flags & SM_PARAM_STRING_BINARY)
					{
						memmove(temp_info[i].phys_addr, temp_info[i].orig_addr, temp_info[i].size);
					}
					/* Copy ASCII characters */
					else
					{
						if ((err=ctx->StringToLocal(temp_info[i].local_addr,
														temp_info[i].size,
														(const char *)temp_info[i].orig_addr))
							!= SP_ERROR_NONE)
						{
							break;
						}
					}
				}
			} /* End array/string calculation */
			/* Update the pushed parameter with the byref local address */
			temp_params[i] = temp_info[i].local_addr;
		}
		else
		{
			/* Just copy the value normally */
			temp_params[i] = m_params[i];
		}
	}

	/* Make the call if we can */
	if (err == SP_ERROR_NONE)
	{
		if ((err = CallFunction2(ctx, temp_params, numparams, result)) != SP_ERROR_NONE)
		{
			docopies = false;
		}
	}
	else
	{
		docopies = false;
	}

	/* i should be equal to the last valid parameter + 1 */
	while (i--)
	{
		if (!temp_info[i].marked)
		{
			continue;
		}

		if (docopies && (temp_info[i].flags & SM_PARAM_COPYBACK))
		{
			if (temp_info[i].orig_addr)
			{
				if (temp_info[i].str.is_sz)
				{
					memcpy(temp_info[i].orig_addr, temp_info[i].phys_addr, temp_info[i].size);
				
				}
				else
				{
					if (temp_info[i].size == 1)
					{
						*temp_info[i].orig_addr = *(temp_info[i].phys_addr);
					}
					else
					{
						memcpy(temp_info[i].orig_addr, 
								temp_info[i].phys_addr, 
								temp_info[i].size * sizeof(cell_t));
					}
				}
			}
		}

		if ((err=ctx->HeapPop(temp_info[i].local_addr)) != SP_ERROR_NONE)
		{
			return err;
		}
	}

	return err;
}

IPluginRuntime *CFunction::GetParentRuntime()
{
	return m_pRuntime;
}

funcid_t CFunction::GetFunctionID()
{
	return m_FnId;
}

int CFunction::SetError(int err)
{
	m_errorstate = err;

	return err;
}

