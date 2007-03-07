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

	return PushArray(cell, 1, NULL, flags);
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

int CFunction::PushArray(cell_t *inarray, unsigned int cells, cell_t **phys_addr, int copyback)
{
	if (m_curparam >= SP_MAX_EXEC_PARAMS)
	{
		return SetError(SP_ERROR_PARAMS_MAX);
	}

	ParamInfo *info = &m_info[m_curparam];
	int err;

	if ((err=m_pContext->HeapAlloc(cells, &info->local_addr, &info->phys_addr)) != SP_ERROR_NONE)
	{
		return SetError(err);
	}

	info->flags = inarray ? copyback : 0;
	info->marked = true;
	info->size = cells * sizeof(cell_t);
	m_params[m_curparam] = info->local_addr;
	m_curparam++;

	if (inarray)
	{
		memcpy(info->phys_addr, inarray, sizeof(cell_t) * cells);
		info->orig_addr = inarray;
	} else {
		info->orig_addr = info->phys_addr;
	}

	if (phys_addr)
	{
		*phys_addr = info->phys_addr;
	}

	return true;
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
	size_t cells = (len + sizeof(cell_t) - 1) / sizeof(cell_t);
	int err;

	if ((err=m_pContext->HeapAlloc(cells, &info->local_addr, &info->phys_addr)) != SP_ERROR_NONE)
	{
		return SetError(err);
	}

	info->marked = true;
	m_params[m_curparam] = info->local_addr;
	m_curparam++;	/* Prevent a leak */

	if (!(sz_flags & SM_PARAM_STRING_COPY))
	{
		goto skip_localtostr;
	}

	if (sz_flags & SM_PARAM_STRING_UTF8)
	{
		if ((err=m_pContext->StringToLocalUTF8(info->local_addr, len, string, NULL)) != SP_ERROR_NONE)
		{
			return SetError(err);
		}
	} else {
		if ((err=m_pContext->StringToLocal(info->local_addr, len, string)) != SP_ERROR_NONE)
		{
			return SetError(err);
		}
	}

skip_localtostr:
	info->flags = cp_flags;
	info->orig_addr = (cell_t *)string;
	info->size = len;

	return SP_ERROR_NONE;
}

void CFunction::Cancel()
{
	if (!m_curparam)
	{
		return;
	}

	while (m_curparam--)
	{
		if (m_info[m_curparam].marked)
		{
			m_pContext->HeapRelease(m_info[m_curparam].local_addr);
			m_info[m_curparam].marked = false;
		}
	}

	m_errorstate = SP_ERROR_NONE;
}

int CFunction::Execute(cell_t *result)
{
	int err;
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
	bool docopies = true;

	if (numparams)
	{
		//Save the info locally, then reset it for re-entrant calls.
		memcpy(temp_params, m_params, numparams * sizeof(cell_t));
		memcpy(temp_info, m_info, numparams * sizeof(ParamInfo));
	}
	m_curparam = 0;

	if ((err = CallFunction(temp_params, numparams, result)) != SP_ERROR_NONE)
	{
		docopies = false;
	}

	while (numparams--)
	{
		if (!temp_info[numparams].marked)
		{
			continue;
		}
		if (docopies && temp_info[numparams].flags)
		{
			if (temp_info[numparams].orig_addr)
			{
				if (temp_info[numparams].size == sizeof(cell_t))
				{
					*temp_info[numparams].orig_addr = *temp_info[numparams].phys_addr;
				} else {
					memcpy(temp_info[numparams].orig_addr, 
							temp_info[numparams].phys_addr, 
							temp_info[numparams].size);
				}
			}
		}
		m_pContext->HeapPop(temp_info[numparams].local_addr);
		temp_info[numparams].marked = false;
	}

	return err;
}

cell_t *CFunction::GetAddressOfPushedParam(unsigned int param)
{
	if (m_errorstate != SP_ERROR_NONE
		|| param >= m_curparam
		|| !m_info[param].marked)
	{
		return NULL;
	}

	return m_info[param].phys_addr;
}
