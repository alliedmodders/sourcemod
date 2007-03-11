/**
 * ================================================================
 * SourcePawn (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ================================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#include <stdio.h>
#include <string.h>
#include "PluginSys.h"

/********************
* FUNCTION CALLING *
********************/

void CFunction::Set(uint32_t code_addr, IPluginContext *plugin)
{
	m_codeaddr = code_addr;
	m_pContext = plugin;
	m_curparam = 0;
	m_errorstate = SP_ERROR_NONE;
	m_Invalid = false;
	m_pCtx = plugin ? plugin->GetContext() : NULL;
}

bool CFunction::IsRunnable()
{
	return ((m_pCtx->flags & SPFLAG_PLUGIN_PAUSED) != SPFLAG_PLUGIN_PAUSED);
}

int CFunction::CallFunction(const cell_t *params, unsigned int num_params, cell_t *result)
{
	if (!IsRunnable())
	{
		return SP_ERROR_NOT_RUNNABLE;
	}

	while (num_params--)
	{
		m_pContext->PushCell(params[num_params]);
	}

	return m_pContext->Execute(m_codeaddr, result);
}

IPluginContext *CFunction::GetParentContext()
{
	return m_pContext;
}

CFunction::CFunction(uint32_t code_addr, IPluginContext *plugin) : 
	m_codeaddr(code_addr), m_pContext(plugin), m_curparam(0), 
	m_errorstate(SP_ERROR_NONE)
{
	m_Invalid = false;
	if (plugin)
	{
		m_pCtx = plugin->GetContext();
	}
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
	if (m_curparam >= SP_MAX_EXEC_PARAMS)
	{
		return SetError(SP_ERROR_PARAMS_MAX);
	}

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
				if ((err=m_pContext->HeapAlloc(temp_info[i].size, 
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
			} else {
				/* Calculate cells required for the string */
				size_t cells = (temp_info[i].size + sizeof(cell_t) - 1) / sizeof(cell_t);

				/* Allocate the buffer */
				if ((err=m_pContext->HeapAlloc(cells,
												&(temp_info[i].local_addr),
												&(temp_info[i].phys_addr)))
					!= SP_ERROR_NONE)
				{
					break;
				}
				/* Copy original string if necessary */
				if ((temp_info[i].str.sz_flags & SM_PARAM_STRING_COPY) && (temp_info[i].orig_addr != NULL))
				{
					if (temp_info[i].str.sz_flags & SM_PARAM_STRING_UTF8)
					{
						if ((err=m_pContext->StringToLocalUTF8(temp_info[i].local_addr, 
																temp_info[i].size, 
																(const char *)temp_info[i].orig_addr,
																NULL))
							!= SP_ERROR_NONE)
						{
							break;
						}
					} else {
						if ((err=m_pContext->StringToLocal(temp_info[i].local_addr,
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
		} else {
			/* Just copy the value normally */
			temp_params[i] = m_params[i];
		}
	}

	/* Make the call if we can */
	if (err == SP_ERROR_NONE)
	{
		if ((err = CallFunction(temp_params, numparams, result)) != SP_ERROR_NONE)
		{
			docopies = false;
		}
	} else {
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
				} else {
					if (temp_info[i].size == 1)
					{
						*temp_info[i].orig_addr = *(temp_info[i].phys_addr);
					} else {
						memcpy(temp_info[i].orig_addr, 
								temp_info[i].phys_addr, 
								temp_info[i].size * sizeof(cell_t));
					}
				}
			}
		}
		if ((err=m_pContext->HeapPop(temp_info[i].local_addr)) != SP_ERROR_NONE)
		{
			return err;
		}
	}

	return err;
}
