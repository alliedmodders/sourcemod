/**
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include "ForwardSys.h"
#include "PluginSys.h"
#include "ShareSys.h"

CForwardManager g_Forwards;

/**
 * Gensis turns to its source, reduction occurs stepwise although the essence is all one.
 * End of line.  FTL system check.
 *
 * :TODO: WHAT NEEDS TO BE TESTED IN THIS BEAST
 * VARARG FUNCTIONS:
 * - Pushing no varargs
 */

// :TODO: IMPORTANT!!! The result pointer arg in the execute function maybe invalid if the forward fails
// so later evaluation of this result may cause problems on higher levels of abstraction. DOCUMENT OR FIX ALL FORWARDS!

void CForwardManager::OnSourceModAllInitialized()
{
	g_PluginSys.AddPluginsListener(this);
	g_ShareSys.AddInterface(NULL, this);
}

void CForwardManager::OnSourceModShutdown()
{
	g_PluginSys.RemovePluginsListener(this);
}

IForward *CForwardManager::CreateForward(const char *name, ExecType et, unsigned int num_params, ParamType *types, ...)
{
	CForward *fwd;
	va_list ap;
	va_start(ap, types);
	
	fwd = CForward::CreateForward(name, et, num_params, types, ap);

	va_end(ap);

	if (fwd)
	{
		m_managed.push_back(fwd);
	}

	return fwd;
}

IChangeableForward *CForwardManager::CreateForwardEx(const char *name, ExecType et, int num_params, ParamType *types, ...)
{
	CForward *fwd;
	va_list ap;
	va_start(ap, types);

	fwd = CForward::CreateForward(name, et, num_params, types, ap);

	va_end(ap);

	if (fwd)
	{
		m_unmanaged.push_back(fwd);
	}

	return fwd;
}

void CForwardManager::OnPluginLoaded(IPlugin *plugin)
{
	/* Attach any globally managed forwards */
	List<CForward *>::iterator iter;
	CForward *fwd;

	for (iter=m_managed.begin(); iter!=m_managed.end(); iter++)
	{
		fwd = (*iter);
		IPluginFunction *pFunc = plugin->GetBaseContext()->GetFunctionByName(fwd->GetForwardName());
		if (pFunc)
		{
			fwd->AddFunction(pFunc);
		}
	}
}

void CForwardManager::OnPluginUnloaded(IPlugin *plugin)
{
	List<CForward *>::iterator iter;
	CForward *fwd;

	for (iter=m_managed.begin(); iter!=m_managed.end(); iter++)
	{
		fwd = (*iter);
		fwd->RemoveFunctionsOfPlugin(plugin);
	}

	for (iter=m_unmanaged.begin(); iter!=m_unmanaged.end(); iter++)
	{
		fwd = (*iter);
		fwd->RemoveFunctionsOfPlugin(plugin);
	}
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
	m_managed.remove(fwd);
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

void CForwardManager::OnPluginPauseChange(IPlugin *plugin, bool paused)
{
	List<CForward *>::iterator iter;
	CForward *fwd;

	if (paused)
	{
		for (iter=m_managed.begin(); iter!=m_managed.end(); iter++)
		{
			fwd = (*iter);
			fwd->PushPausedFunctions(plugin);
		}
		for (iter=m_unmanaged.begin(); iter!=m_unmanaged.end(); iter++)
		{
			fwd = (*iter);
			fwd->PushPausedFunctions(plugin);
		}
	} else {
		for (iter=m_managed.begin(); iter!=m_managed.end(); iter++)
		{
			fwd = (*iter);
			fwd->PopPausedFunctions(plugin);
		}
		for (iter=m_unmanaged.begin(); iter!=m_unmanaged.end(); iter++)
		{
			fwd = (*iter);
			fwd->PopPausedFunctions(plugin);
		}
	}
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
			_types[i] = (ParamType)va_arg(ap, int);
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
					func->PushStringEx((char *)param->byref.orig_addr, param->byref.cells, param->byref.sz_flags, param->byref.flags);
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
		
		/* Call the function and deal with the return value. */
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
			default:
				{
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
		if (m_types[m_curparam] == Param_Any)
		{
			m_params[m_curparam].pushedas = Param_CellByRef;
		} else if (m_types[m_curparam] != Param_CellByRef) {
			return SetError(SP_ERROR_PARAM);
		}
	} else {
		if (!m_varargs || m_numparams > SP_MAX_EXEC_PARAMS)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		}
		m_params[m_curparam].pushedas = Param_CellByRef;
	}

	_Int_PushArray(cell, 1, flags);
	m_curparam++;

	return SP_ERROR_NONE;
}

int CForward::PushFloatByRef(float *num, int flags)
{
	if (m_curparam < m_numparams)
	{
		if (m_types[m_curparam] == Param_Any)
		{
			m_params[m_curparam].pushedas = Param_FloatByRef;
		} else if (m_types[m_curparam] != Param_FloatByRef) {
			return SetError(SP_ERROR_PARAM);
		}
	} else {
		if (!m_varargs || m_numparams > SP_MAX_EXEC_PARAMS)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		}
		m_params[m_curparam].pushedas = Param_FloatByRef;
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

void CForward::_Int_PushString(cell_t *inarray, unsigned int cells, int sz_flags, int cp_flags)
{
	m_params[m_curparam].byref.cells = cells;	/* Notice this contains the char count not cell count */
	m_params[m_curparam].byref.flags = cp_flags;
	m_params[m_curparam].byref.orig_addr = inarray;
	m_params[m_curparam].byref.sz_flags = sz_flags;
}

int CForward::PushString(const char *string)
{
	if (m_curparam < m_numparams)
	{
		if (m_types[m_curparam] == Param_Any)
		{
			m_params[m_curparam].pushedas = Param_String;
		} else if (m_types[m_curparam] != Param_String) {
			return SetError(SP_ERROR_PARAM);
		}
	} else {
		if (!m_varargs || m_curparam > SP_MAX_EXEC_PARAMS)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		}
		m_params[m_curparam].pushedas = Param_String;
	}

	_Int_PushString((cell_t *)string, strlen(string)+1, SM_PARAM_STRING_COPY, 0);
	m_curparam++;

	return SP_ERROR_NONE;
}

int CForward::PushStringEx(char *buffer, size_t length, int sz_flags, int cp_flags)
{
	if (m_curparam < m_numparams)
	{
		if (m_types[m_curparam] == Param_Any)
		{
			m_params[m_curparam].pushedas = Param_String;
		} else if (m_types[m_curparam] != Param_String) {
			return SetError(SP_ERROR_PARAM);
		}
	} else {
		if (!m_varargs || m_curparam > SP_MAX_EXEC_PARAMS)
		{
			return SetError(SP_ERROR_PARAMS_MAX);
		}
		m_params[m_curparam].pushedas = Param_String;
	}

	_Int_PushString((cell_t *)buffer, length, sz_flags, cp_flags);
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

bool CForward::AddFunction(IPluginContext *pContext, funcid_t index)
{
	IPluginFunction *pFunc = pContext->GetFunctionById(index);

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
	List<IPluginFunction *> *lst;

	if (func->GetParentContext()->IsRunnable())
	{
		lst = &m_functions;
	} else {
		lst = &m_paused;
	}

	for (iter=lst->begin(); iter!=lst->end(); iter++)
	{
		if ((*iter) == func)
		{
			found = true;
			lst->erase(iter);
			break;
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
	IPluginContext *pContext = plugin->GetBaseContext();
	for (iter=m_functions.begin(); iter!=m_functions.end();)
	{
		func = (*iter);
		if (func->GetParentContext() == pContext)
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

	//:IDEA: eventually we will tell the plugin we're using it [?]
	if (func->GetParentContext()->IsRunnable())
	{
		m_functions.push_back(func);
	} else {
		m_paused.push_back(func);
	}

	return true;
}

const char *CForward::GetForwardName() const
{
	return m_name;
}

unsigned int CForward::GetFunctionCount() const
{
	return m_functions.size();
}

ExecType CForward::GetExecType() const
{
	return m_ExecType;
}

void CForward::PushPausedFunctions(IPlugin *plugin)
{
	FuncIter iter;
	IPluginFunction *func;
	IPluginContext *pContext = plugin->GetBaseContext();

	for (iter=m_functions.begin(); iter!=m_functions.end();)
	{
		func = (*iter);
		if (func->GetParentContext() == pContext)
		{
			m_paused.push_back(func);
			iter = m_functions.erase(iter);
		} else {
			iter++;
		}
	}
}

void CForward::PopPausedFunctions(IPlugin *plugin)
{
	FuncIter iter;
	IPluginFunction *func;
	IPluginContext *pContext = plugin->GetBaseContext();

	for (iter=m_paused.begin(); iter!=m_paused.end();)
	{
		func = (*iter);
		if (func->GetParentContext() == pContext)
		{
			m_functions.push_back(func);
			iter = m_paused.erase(iter);
		} else {
			iter++;
		}
	}
}
