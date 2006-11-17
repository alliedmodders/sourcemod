#include <stdio.h>
#include "PluginSys.h"

/********************
* FUNCTION CALLING *
********************/

void CFunction::Set(funcid_t funcid, CPlugin *plugin)
{
	m_funcid = funcid;
	m_pPlugin = plugin;
	m_curparam = 0;
	m_errorstate = SP_ERROR_NONE;
}

int CFunction::CallFunction(const cell_t *params, unsigned int num_params, cell_t *result)
{
	IPluginContext *ctx = m_pPlugin->m_ctx_current.base;

	while (num_params--)
	{
		ctx->PushCell(params[num_params]);
	}

	return ctx->Execute(m_funcid, result);
}

IPlugin *CFunction::GetParentPlugin()
{
	return m_pPlugin;
}

CFunction::CFunction(funcid_t funcid, CPlugin *plugin) : 
	m_funcid(funcid), m_pPlugin(plugin), m_curparam(0), 
	m_errorstate(SP_ERROR_NONE)
{
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

	IPluginContext *ctx = m_pPlugin->m_ctx_current.base;
	ParamInfo *info = &m_info[m_curparam];
	int err;

	if ((err=ctx->HeapAlloc(cells, &info->local_addr, &info->phys_addr)) != SP_ERROR_NONE)
	{
		return SetError(err);
	}

	info->flags = inarray ? copyback : 0;
	info->marked = true;
	info->size = cells;
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
	return _PushString(string, 0);
}

int CFunction::PushStringEx(char *string, int flags)
{
	return _PushString(string, flags);
}

int CFunction::_PushString(const char *string, int flags)
{
	if (m_curparam >= SP_MAX_EXEC_PARAMS)
	{
		return SetError(SP_ERROR_PARAMS_MAX);
	}

	IPluginContext *base = m_pPlugin->m_ctx_current.base;
	ParamInfo *info = &m_info[m_curparam];
	size_t len = strlen(string);
	size_t cells = (len + sizeof(cell_t)) / sizeof(cell_t);
	int err;

	if ((err=base->HeapAlloc(cells, &info->local_addr, &info->phys_addr)) != SP_ERROR_NONE)
	{
		return SetError(err);
	}

	info->marked = true;
	m_params[m_curparam] = info->local_addr;
	m_curparam++;	/* Prevent a leak */

	if ((err=base->StringToLocalUTF8(info->local_addr, len+1, string, NULL)) != SP_ERROR_NONE)
	{
		return SetError(err);
	}

	info->flags = flags;
	info->orig_addr = (cell_t *)string;
	info->size = cells;

	return SP_ERROR_NONE;
}

void CFunction::Cancel()
{
	if (!m_curparam)
	{
		return;
	}

	IPluginContext *base = m_pPlugin->m_ctx_current.base;

	while (m_curparam--)
	{
		if (m_info[m_curparam].marked)
		{
			base->HeapRelease(m_info[m_curparam].local_addr);
			m_info[m_curparam].marked = false;
		}
	}

	m_errorstate = SP_ERROR_NONE;
}

int CFunction::Execute(cell_t *result)
{
	int err;
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

	IPluginContext *base = m_pPlugin->m_ctx_current.base;

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
				if (temp_info[numparams].size == 1)
				{
					*temp_info[numparams].orig_addr = *temp_info[numparams].phys_addr;
				} else {
					memcpy(temp_info[numparams].orig_addr, 
							temp_info[numparams].phys_addr, 
							temp_info[numparams].size * sizeof(cell_t));
				}
			}
		}
		base->HeapPop(temp_info[numparams].local_addr);
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
