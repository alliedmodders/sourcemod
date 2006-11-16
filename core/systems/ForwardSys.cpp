#include <assert.h>
#include "ForwardSys.h"
#include "PluginSys.h"

CForwardManager g_Forwards;

/**
 * Gensis turns to its source, reduction occurs stepwise although the essence is all one.
 * End of line.  FTL system check.
 *
 * :TODO: Implement the manager.  ho ho ho
 *
 * :TODO: WHAT NEEDS TO BE TESTED IN THIS BEAST (X=done, -=TODO)
 * NORMAL FUNCTIONS:
 * X Push cells
 * X Push cells byref (copyback tested = yes)
 * X Push floats
 * X Push floats byref (copyback tested = yes)
 * X Push arrays  (copyback tested = yes)
 * - Push strings (copyback tested = ??)
 * VARARG FUNCTIONS:
 * - Pushing no varargs
 * - Push vararg cells (copyback should be verified to not happen = ??)
 * - Push vararg cells byref (copyback tested = ??)
 * - Push vararg floats (copyback should be verified to not happen = ??)
 * - Push vararg floats byref (copyback tested = ??)
 * - Push vararg arrays  (copyback tested = ??)
 * - Push vararg strings (copyback tested = ??)
 */

IForward *CForwardManager::CreateForward(const char *name, ExecType et, unsigned int num_params, ParamType *types, ...)
{
	CForward *fwd;
	va_list ap;
	va_start(ap, types);
	
	fwd = CForward::CreateForward(name, et, num_params, types, ap);

	va_end(ap);

	return fwd;
}

IChangeableForward *CForwardManager::CreateForwardEx(const char *name, ExecType et, int num_params, ParamType *types, ...)
{
	CForward *fwd;
	va_list ap;
	va_start(ap, types);

	fwd = CForward::CreateForward(name, et, num_params, types, ap);

	va_end(ap);

	return fwd;
}

IForward *CForwardManager::FindForward(const char *name, IChangeableForward **ifchng)
{
	List<CForward *>::iterator iter;
	CForward *fwd;

	for (iter=m_managed.begin(); iter!=m_managed.end(); iter++)
	{
		fwd = (*iter);
		if (strcmp(fwd->GetForwardName(), name) == 0)
		{
			if (ifchng)
			{
				*ifchng = NULL;
			}
			return fwd;
		}
	}

	for (iter=m_unmanaged.begin(); iter!=m_unmanaged.end(); iter++)
	{
		fwd = (*iter);
		if (strcmp(fwd->GetForwardName(), name) == 0)
		{
			if (ifchng)
			{
				*ifchng = fwd;
			}
			return fwd;
		}
	}

	if (ifchng)
	{
		*ifchng = NULL;
	}

	return NULL;
}

void CForwardManager::ReleaseForward(IForward *forward)
{
	ForwardFree(static_cast<CForward *>(forward));
}

void CForwardManager::ForwardFree(CForward *fwd)
{
	m_FreeForwards.push(fwd);
}

CForward *CForwardManager::ForwardMake()
{
	CForward *fwd;
	if (m_FreeForwards.empty())
	{
		fwd = new CForward;
	} else {
		fwd = m_FreeForwards.front();
		m_FreeForwards.pop();
	}
	return fwd;
}

/*************************************
 * ACTUAL FORWARD API IMPLEMENTATION *
 *************************************/

CForward *CForward::CreateForward(const char *name, ExecType et, unsigned int num_params, ParamType *types, va_list ap)
{
	ParamType _types[SP_MAX_EXEC_PARAMS];

	if (num_params > SP_MAX_EXEC_PARAMS)
	{
		return NULL;
	}
	
	if (types == NULL && num_params)
	{
		for (unsigned int i=0; i<num_params; i++)
		{
			_types[i] = va_arg(ap, ParamType);
			if (_types[i] == Param_VarArgs && (i != num_params - 1))
			{
				return NULL;
			}
		}
		types = _types;
	} else {
		for (unsigned int i=0; i<num_params; i++)
		{
			if (types[i] == Param_VarArgs && (i != num_params - 1))
			{
				return NULL;
			}
		}
	}

	/* First parameter can never be varargs */
	if (types[0] == Param_VarArgs)
	{
		return NULL;
	}

	CForward *pForward = g_Forwards.ForwardMake();
	pForward->m_curparam = 0;
	pForward->m_ExecType = et;
	snprintf(pForward->m_name, FORWARDS_NAME_MAX, "%s", name ? name : "");
	
	for (unsigned int i=0; i<num_params; i++)
	{
		pForward->m_types[i] = types[i];
	}

	if (num_params && types[num_params-1] == Param_VarArgs)
	{
		pForward->m_varargs = num_params--;
	} else {
		pForward->m_varargs = false;
	}

	pForward->m_numparams = num_params;
	pForward->m_errstate = SP_ERROR_NONE;

	return pForward;
}

int CForward::Execute(cell_t *result, IForwardFilter *filter)
{
	if (m_errstate)
	{
		int err = m_errstate;
		Cancel();
		return err;
	}

	if (filter)
	{
		filter->OnExecuteBegin();
	}

	FuncIter iter = m_functions.begin();
	IPluginFunction *func;
	cell_t cur_result = 0;
	cell_t high_result = 0;
	int err;
	unsigned int failed=0, success=0;
	unsigned int save_numcopy = 0;
	unsigned int num_params = m_curparam;
	FwdParamInfo temp_info[SP_MAX_EXEC_PARAMS];
	FwdParamInfo *param;
	ParamType type;

	/* Save local, reset */
	memcpy(temp_info, m_params, sizeof(m_params));
	m_curparam = 0;

	for (iter=m_functions.begin(); iter!=m_functions.end(); iter++)
	{
		func = (*iter);

		for (unsigned int i=0; i<num_params; i++)
		{
			param = &temp_info[i];
			if (i >= m_numparams || m_types[i] == Param_Any)
			{
				type = param->pushedas;
			} else {
				type = m_types[i];
			}
			if ((i >= m_numparams) || (type & SP_PARAMFLAG_BYREF))
			{
				/* If we're byref or we're vararg, we always push everything by ref.
				 * Even if they're byval, we must push them byref.
				 */
				if (type == Param_String) 
				{
					func->PushStringEx((char *)param->byref.orig_addr, param->byref.flags);
				} else if (type == Param_Float || type == Param_Cell) {
					func->PushCellByRef(&param->val, 0); 
				} else {
					func->PushArray(param->byref.orig_addr, param->byref.cells, NULL, param->byref.flags);
					assert(type == Param_Array || type == Param_FloatByRef || type == Param_CellByRef);
				}
			} else {
				/* If we're not byref or not vararg, our job is a bit easier. */
				assert(type == Param_Cell || type == Param_Float);
				func->PushCell(param->val);
			}
		}
		
		/* Call the function and deal with the return value.
		 * :TODO: only pass reader if we know we have an array in the list
		 */
		if ((err=func->Execute(&cur_result)) != SP_ERROR_NONE)
		{
			bool handled = false;
			if (filter)
			{
				handled = filter->OnErrorReport(this, func, err);
			}
			if (!handled)
			{
				/* :TODO: invoke global error reporting here */
			}
			failed++;
		} else {
			success++;
			switch (m_ExecType)
			{
			case ET_Event:
				{
					if (cur_result > high_result)
					{
						high_result = cur_result;
					}
					break;
				}
			case ET_Hook:
				{
					if (cur_result > high_result)
					{
						high_result = cur_result;
						if ((ResultType)high_result == Pl_Stop)
						{
							iter = m_functions.end();
							break;
						}
					}
					break;
				}
			case ET_Custom:
				{
					if (filter)
					{
						if (filter->OnFunctionReturn(this, func, &cur_result) == Pl_Stop)
						{
							iter = m_functions.end();
						}
					}
					break;
				}
			}
		}
	}

	if (m_ExecType == ET_Event || m_ExecType == ET_Hook)
	{
		cur_result = high_result;
	} else if (m_ExecType == ET_Ignore) {
		cur_result = 0;
	}

	*result = cur_result;

	if (filter)
	{
		filter->OnExecuteEnd(&cur_result, success, failed);
	}

	return SP_ERROR_NONE;
}

int CForward::PushCell(cell_t cell)
{
	if (m_curparam < m_numparams)
	{
		if (m_types[m_curparam] == Param_Any)
		{
			m_params[m_curparam].pushedas = Param_Cell;
		} else if (m_types[m_curparam] != Param_Cell) {
			return SetError(SP_ERROR_PARAM);
		}
	} else {
		if (!m_varargs || m_numparams > SP_MAX_EXEC_PARAMS)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		}
		m_params[m_curparam].pushedas = Param_Cell;
	}

	m_params[m_curparam++].val = cell;

	return SP_ERROR_NONE;
}

int CForward::PushFloat(float number)
{
	if (m_curparam < m_numparams)
	{
		if (m_types[m_curparam] == Param_Any)
		{
			m_params[m_curparam].pushedas = Param_Float;
		} else if (m_types[m_curparam] != Param_Float) {
			return SetError(SP_ERROR_PARAM);
		}
	} else {
		if (!m_varargs || m_numparams > SP_MAX_EXEC_PARAMS)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		}
		m_params[m_curparam].pushedas = Param_Float;
	}

	m_params[m_curparam++].val = *(cell_t *)&number;

	return SP_ERROR_NONE;
}

int CForward::PushCellByRef(cell_t *cell, int flags)
{
	if (m_curparam < m_numparams)
	{
		if (m_types[m_curparam] != Param_CellByRef && m_types[m_curparam] != Param_Any)
		{
			return SetError(SP_ERROR_PARAM);
		}
	} else {
		if (!m_varargs || m_numparams > SP_MAX_EXEC_PARAMS)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		}
	}

	_Int_PushArray(cell, 1, flags);
	m_curparam++;

	return SP_ERROR_NONE;
}

int CForward::PushFloatByRef(float *num, int flags)
{
	if (m_curparam < m_numparams)
	{
		if (m_types[m_curparam] != Param_FloatByRef && m_types[m_curparam] != Param_Any)
		{
			return SetError(SP_ERROR_PARAM);
		}
	} else {
		if (!m_varargs || m_numparams > SP_MAX_EXEC_PARAMS)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		}
	}

	_Int_PushArray((cell_t *)num, 1, flags);
	m_curparam++;

	return SP_ERROR_NONE;
}

void CForward::_Int_PushArray(cell_t *inarray, unsigned int cells, int flags)
{
	m_params[m_curparam].byref.cells = cells;
	m_params[m_curparam].byref.flags = flags;
	m_params[m_curparam].byref.orig_addr = inarray;
}

int CForward::PushArray(cell_t *inarray, unsigned int cells, cell_t **phys_addr, int flags)
{
	/* We don't allow this here */
	if (!inarray)
	{
		return SetError(SP_ERROR_PARAM);
	}

	if (m_curparam < m_numparams)
	{
		if (m_types[m_curparam] == Param_Any)
		{
			m_params[m_curparam].pushedas = Param_Array;
		} else if (m_types[m_curparam] != Param_Array) {
			return SetError(SP_ERROR_PARAM);
		}
	} else {
		if (!m_varargs || m_curparam > SP_MAX_EXEC_PARAMS)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		}
		m_params[m_curparam].pushedas = Param_Array;
	}

	if (phys_addr)
	{
		*phys_addr = NULL;
	}

	_Int_PushArray(inarray, cells, flags);

	m_curparam++;

	return SP_ERROR_NONE;
}

int CForward::PushString(const char *string)
{
	if (m_curparam < m_numparams)
	{
		if (m_types[m_curparam] == Param_Any)
		{
			m_params[m_curparam].pushedas = Param_String;
		} else if (m_types[m_curparam] == Param_String) {
			return SetError(SP_ERROR_PARAM);
		}
	} else {
		if (!m_varargs || m_curparam > SP_MAX_EXEC_PARAMS)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		}
		m_params[m_curparam].pushedas = Param_String;
	}

	_Int_PushArray((cell_t *)string, 0, 0);
	m_curparam++;

	return SP_ERROR_NONE;
}

int CForward::PushStringEx(char *string, int flags)
{
	if (m_curparam < m_numparams)
	{
		if (m_types[m_curparam] == Param_Any)
		{
			m_params[m_curparam].pushedas = Param_String;
		} else if (m_types[m_curparam] == Param_String) {
			return SetError(SP_ERROR_PARAM);
		}
	} else {
		if (!m_varargs || m_curparam > SP_MAX_EXEC_PARAMS)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		}
		m_params[m_curparam].pushedas = Param_String;
	}

	_Int_PushArray((cell_t *)string, 0, flags);
	m_curparam++;

	return SP_ERROR_NONE;
}

void CForward::Cancel()
{
	if (!m_curparam)
	{
		return;
	}

	m_curparam = 0;
	m_errstate = SP_ERROR_NONE;
}

bool CForward::AddFunction(sp_context_t *ctx, funcid_t index)
{
	IPlugin *pPlugin = g_PluginMngr.FindPluginByContext(ctx);
	if (!pPlugin)
	{
		return false;
	}

	IPluginFunction *pFunc = pPlugin->GetFunctionById(index);
	if (!pFunc)
	{
		return false;
	}

	return AddFunction(pFunc);
}

bool CForward::RemoveFunction(IPluginFunction *func)
{
	bool found = false;
	FuncIter iter;
	IPluginFunction *pNext = NULL;

	for (iter=m_functions.begin(); iter!=m_functions.end();)
	{
		if ((*iter) == func)
		{
			found = true;
			/* If this iterator is being used, swap in a new one !*/
			iter = m_functions.erase(iter);
		} else {
			iter++;
		}
	}

	/* Cancel a call, if any */
	if (found || m_curparam)
	{
		func->Cancel();
	}

	return found;
}

unsigned int CForward::RemoveFunctionsOfPlugin(IPlugin *plugin)
{
	FuncIter iter;
	IPluginFunction *func;
	unsigned int removed = 0;
	for (iter=m_functions.begin(); iter!=m_functions.end();)
	{
		func = (*iter);
		if (func->GetParentPlugin() == plugin)
		{
			iter = m_functions.erase(iter);
			removed++;
		} else {
			iter++;
		}
	}

	return removed;
}

bool CForward::AddFunction(IPluginFunction *func)
{
	if (m_curparam)
	{
		return false;
	}

	//:TODO: eventually we will tell the plugin we're using it
	m_functions.push_back(func);

	return true;
}

const char *CForward::GetForwardName()
{
	return m_name;
}

unsigned int CForward::GetFunctionCount()
{
	return m_functions.size();
}

ExecType CForward::GetExecType()
{
	return m_ExecType;
}
