#include <assert.h>
#include "ForwardSys.h"
#include "PluginSys.h"

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

	CForward *pForward = new CForward;
	pForward->m_curparam = 0;
	pForward->m_ExecType = et;
	snprintf(pForward->m_name, FORWARDS_NAME_MAX, "%s", name ? name : "");
	
	for (unsigned int i=0; i<num_params; i++)
	{
		pForward->m_types[i] = types[i];
	}

	if (num_params && types[num_params-1] == Param_VarArgs)
	{
		pForward->m_varargs = true;
	} else {
		pForward->m_varargs = false;
	}

	pForward->m_numparams = num_params;
	pForward->m_errstate = SP_ERROR_NONE;
	pForward->m_CopyBacks.numrecopy = 0;

	return pForward;
}

bool CForward::OnCopybackArray(unsigned int param, 
							   unsigned int cells, 
							   cell_t *source_addr, 
							   cell_t *orig_addr, 
							   int flags)
{
	/* Check if the stack is empty, this should be an assertion */
	if (m_NextStack.empty())
	{
		/* This should never happen! */
		assert(!m_NextStack.empty());
		return true;
	}

	/* Check if we even want to copy to the next plugin */
	if (!(flags & SMFUNC_COPYBACK_ALWAYS))
	{
		return true;
	}

	/* Keep track of the copy back and save the info */
	NextCallInfo &info = m_NextStack.front();
	info.recopy[info.numrecopy++] = param;
	info.orig_addrs[param] = orig_addr;
	info.sizes[param] = cells;

	/* We don't want to override the copy. */
	return true;
}

int CForward::Execute(cell_t *result, IForwardFilter *filter)
{
	if (m_errstate)
	{
		int err = m_errstate;
		Cancel();
		return err;
	}

	/* Reset marker */
	m_curparam = 0;

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

	/** 
	 * Save copyback into to start the chain, 
	 * then reset it for re-entrancy
	 */
	m_NextStack.push(m_CopyBacks);
	NextCallInfo &info = m_NextStack.front();
	m_CopyBacks.numrecopy = 0;
	
	for (iter=m_functions.begin(); iter!=m_functions.end(); iter++)
	{
		func = (*iter);
		
		/**
		 * Check if we need to copy a new array back into the plugin.
		 */
		if (info.numrecopy)
		{
			/* The last plugin has a chained copyback, we must redirect it here. */
			unsigned int param;
			cell_t *orig_addr, *targ_addr;
			for (unsigned int i=0; i<info.numrecopy; i++)
			{
				/* Get the parameter info to copy */
				param = info.recopy[i];
				targ_addr = func->GetAddressOfPushedParam(param);
				orig_addr = info.orig_addrs[param];
				/* Only do the copy for valid targets */
				if (targ_addr && orig_addr)
				{
					if (info.sizes[param] == 1)
					{
						*targ_addr = *orig_addr;
					} else {
						memcpy(targ_addr, orig_addr, info.sizes[param] * sizeof(cell_t));
					}
				}
				/* If this failed, the plugin will most likely be failing as well. */
			}
			save_numcopy = info.numrecopy;
			info.numrecopy = 0;
		}

		/* Call the function and deal with the return value.
		 * :TODO: only pass reader if we know we have an array in the list
		 */
		if ((err=func->Execute(&cur_result, this)) != SP_ERROR_NONE)
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
			/* If we failed, we're not quite done.  The copy chain has been broken.
			 * We have to restore it so past changes will continue to get mirrored.
			 */
			info.numrecopy = save_numcopy;
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

	m_NextStack.pop();

	if (m_ExecType == ET_Event || m_ExecType == ET_Hook)
	{
		cur_result = high_result;
	} else if (m_ExecType == ET_Ignore) {
		cur_result = 0;
	}

	*result = cur_result;

	DumpAdditionQueue();

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
		if (m_types[m_curparam] != Param_Cell && m_types[m_curparam] != Param_Any)
		{
			return SetError(SP_ERROR_PARAM);
		}
	} else {
		if (!m_varargs || m_numparams > SP_MAX_EXEC_PARAMS)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		}
	}

	FuncIter iter;
	IPluginFunction *func;
	for (iter=m_functions.begin(); iter!=m_functions.end(); iter++)
	{
		func = (*iter);
		func->PushCell(cell);
	}

	m_curparam++;

	return SP_ERROR_NONE;
}

int CForward::PushFloat(float number)
{
	if (m_curparam < m_numparams)
	{
		if (m_types[m_curparam] != Param_Float && m_types[m_curparam] != Param_Any)
		{
			return SetError(SP_ERROR_PARAM);
		}
	} else {
		if (!m_varargs || m_numparams > SP_MAX_EXEC_PARAMS)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		}
	}

	FuncIter iter;
	IPluginFunction *func;
	for (iter=m_functions.begin(); iter!=m_functions.end(); iter++)
	{
		func = (*iter);
		func->PushFloat(number);
	}

	m_curparam++;

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

	if (flags & SMFUNC_COPYBACK_ALWAYS)
	{
		_Int_PushArray(cell, 1, flags);
	} else {
		FuncIter iter;
		IPluginFunction *func;
		for (iter=m_functions.begin(); iter!=m_functions.end(); iter++)
		{
			func = (*iter);
			func->PushCellByRef(cell, flags);
		}
	}

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

	if (flags & SMFUNC_COPYBACK_ALWAYS)
	{
		_Int_PushArray((cell_t *)num, 1, flags);
	} else {
		FuncIter iter;
		IPluginFunction *func;
		for (iter=m_functions.begin(); iter!=m_functions.end(); iter++)
		{
			func = (*iter);
			func->PushFloatByRef(num, flags);
		}
	}

	m_curparam++;

	return SP_ERROR_NONE;
}

int CForward::PushCells(cell_t array[], unsigned int numcells, bool each)
{
	if (each)
	{
		/* Type check each cell if we need to! */
		if (m_curparam + numcells >= m_numparams && !m_varargs)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		} else {
			for (unsigned int i=m_curparam; i<m_numparams; i++)
			{
				if (m_types[i] != Param_Any || m_types[i] != Param_Cell)
				{
					return SetError(SP_ERROR_PARAM);
				}
			}
		}
	} else {
		if (m_curparam < m_numparams)
		{
			if (m_types[m_curparam] != Param_Any || m_types[m_curparam] == Param_Array)
			{
				return SetError(SP_ERROR_PARAM);
			}
		} else {
			if (!m_varargs || m_curparam > SP_MAX_EXEC_PARAMS)
			{
				return SetError(SP_ERROR_PARAMS_MAX);
			}
		}
	}

	FuncIter iter;
	IPluginFunction *func;
	for (iter=m_functions.begin(); iter!=m_functions.end(); iter++)
	{
		func = (*iter);
		func->PushCells(array, numcells, each);
	}

	m_curparam += each ? numcells : 1;
	
	return SP_ERROR_NONE;
}

void CForward::_Int_PushArray(cell_t *inarray, unsigned int cells, int flags)
{
	FuncIter iter;
	IPluginFunction *func;
	if (flags & SMFUNC_COPYBACK_ALWAYS)
	{
		/* As a special optimization, we create blank default arrays because they will be 
		* copied over anyway!
		*/
		for (iter=m_functions.begin(); iter!=m_functions.end(); iter++)
		{
			func = (*iter);
			func->PushArray(NULL, cells, NULL, flags);
		}
		m_CopyBacks.recopy[m_CopyBacks.numrecopy++] = m_curparam;
		m_CopyBacks.orig_addrs[m_curparam] = inarray;
		m_CopyBacks.sizes[m_curparam] = cells;
	} else {
		for (iter=m_functions.begin(); iter!=m_functions.end(); iter++)
		{
			func = (*iter);
			func->PushArray(inarray, cells, NULL, flags);
		}
	}
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
		if (m_types[m_curparam] != Param_Any || m_types[m_curparam] == Param_Array)
		{
			return SetError(SP_ERROR_PARAM);
		}
	} else {
		if (!m_varargs || m_curparam > SP_MAX_EXEC_PARAMS)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		}
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
		if (m_types[m_curparam] != Param_Any || m_types[m_curparam] == Param_String)
		{
			return SetError(SP_ERROR_PARAM);
		}
	} else {
		if (!m_varargs || m_curparam > SP_MAX_EXEC_PARAMS)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		}
	}

	FuncIter iter;
	IPluginFunction *func;
	for (iter=m_functions.begin(); iter!=m_functions.end(); iter++)
	{
		func = (*iter);
		func->PushString(string);
	}

	m_curparam++;

	return SP_ERROR_NONE;
}

int CForward::PushStringByRef(char *string, int flags)
{
	if (m_curparam < m_numparams)
	{
		if (m_types[m_curparam] != Param_Any || m_types[m_curparam] == Param_String)
		{
			return SetError(SP_ERROR_PARAM);
		}
	} else {
		if (!m_varargs || m_curparam > SP_MAX_EXEC_PARAMS)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		}
	}

	FuncIter iter;
	IPluginFunction *func;
	for (iter=m_functions.begin(); iter!=m_functions.end(); iter++)
	{
		func = (*iter);
		func->PushStringByRef(string, flags);
	}

	m_curparam++;

	return SP_ERROR_NONE;
}

void CForward::Cancel()
{
	if (!m_curparam)
	{
		return;
	}

	FuncIter iter;
	IPluginFunction *func;
	for (iter=m_functions.begin(); iter!=m_functions.end(); iter++)
	{
		func = (*iter);
		func->Cancel();
	}

	m_CopyBacks.numrecopy = 0;

	DumpAdditionQueue();

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

	/* Just in case */
	m_AddQueue.remove(func);
	
	/* Cancel a call, if any */
	if (found && (!m_NextStack.empty() || m_curparam))
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

	for (iter=m_AddQueue.begin(); iter!=m_AddQueue.end();)
	{
		func = (*iter);
		if (func->GetParentPlugin() == plugin)
		{
			/* Don't count these toward the total */
			iter = m_functions.erase(iter);
		} else {
			iter++;
		}
	}

	return removed;
}

void CForward::DumpAdditionQueue()
{
	FuncIter iter = m_AddQueue.begin();
	while (iter != m_AddQueue.end())
	{
		m_functions.push_back((*iter));
		//:TODO: eventually we will tell the plugin we're using it
		iter = m_AddQueue.erase(iter);
	}
}

bool CForward::AddFunction(IPluginFunction *func)
{
	if (m_curparam)
	{
		return false;
	}

	if (!m_NextStack.empty())
	{
		m_AddQueue.push_back(func);
	} else {
		//:TODO: eventually we will tell the plugin we're using it
		m_functions.push_back(func);
	}

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
