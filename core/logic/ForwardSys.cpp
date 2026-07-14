// vim: set ts=4 sw=4 tw=99 noet :
// =============================================================================
// SourceMod
// Copyright (C) 2004-2015 AlliedModders LLC.  All rights reserved.
// =============================================================================
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License, version 3.0, as published by the
// Free Software Foundation.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
// As a special exception, AlliedModders LLC gives you permission to link the
// code of this program (as well as its derivative works) to "Half-Life 2," the
// "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
// by the Valve Corporation.  You must obey the GNU General Public License in
// all respects for all other code used.  Additionally, AlliedModders LLC grants
// this exception to all derivative works.  AlliedModders LLC defines further
// exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
// or <http://www.sourcemod.net/license.php>.

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include "ForwardSys.h"
#include "DebugReporter.h"
#include "common_logic.h"
#include <bridge/include/IScriptManager.h>
#include <amtl/am-string.h>
#include <ReentrantList.h>

using namespace ke;
using sp::CallArgs;

CForwardManager g_Forwards;

// Genesis turns to its source, reduction occurs stepwise although the essence
// is all one. End of line.  FTL system check.

void CForwardManager::OnSourceModAllInitialized()
{
	scripts->AddPluginsListener(this);
	sharesys->AddInterface(NULL, this);
}

IForward *CForwardManager::CreateForward(const char *name, ExecType et, unsigned int num_params, const ParamType *types, ...)
{
	va_list ap;
	va_start(ap, types);
	
	CForward *fwd = CForward::CreateForward(name, et, num_params, types, ap);

	va_end(ap);

	if (fwd)
	{
		scripts->AddFunctionsToForward(name, fwd);

		m_managed.push_back(fwd);
	}

	return fwd;
}

IChangeableForward *CForwardManager::CreateForwardEx(const char *name, ExecType et, int num_params, const ParamType *types, ...)
{
	va_list ap;
	va_start(ap, types);

	CForward *fwd = CForward::CreateForward(name, et, num_params, types, ap);

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
	for (ForwardIter iter(m_managed); !iter.done(); iter.next()) {
		CForward *fwd = (*iter);
		IPluginFunction *pFunc = plugin->GetBaseContext()->GetFunctionByName(fwd->GetForwardName());
		if (pFunc)
			fwd->AddFunction(pFunc);
	}
}

void CForwardManager::OnPluginUnloaded(IPlugin *plugin)
{
	for (ForwardIter iter(m_managed); !iter.done(); iter.next()) {
		CForward *fwd = (*iter);
		fwd->RemoveFunctionsOfPlugin(plugin);
	}

	for (ForwardIter iter(m_unmanaged); !iter.done(); iter.next()) {
		CForward *fwd = (*iter);
		fwd->RemoveFunctionsOfPlugin(plugin);
	}
}

IForward *CForwardManager::FindForward(const char *name, IChangeableForward **ifchng)
{
	for (ForwardIter iter(m_managed); !iter.done(); iter.next()) {
		CForward *fwd = (*iter);
		if (strcmp(fwd->GetForwardName(), name) == 0) {
			if (ifchng)
				*ifchng = NULL;
			return fwd;
		}
	}

	for (ForwardIter iter(m_unmanaged); !iter.done(); iter.next()) {
		CForward *fwd = (*iter);
		if (strcmp(fwd->GetForwardName(), name) == 0) {
			if (ifchng)
				*ifchng = fwd;
			return fwd;
		}
	}

	if (ifchng)
		*ifchng = NULL;

	return NULL;
}

void CForwardManager::ReleaseForward(IForward *aForward)
{
	CForward *fwd = static_cast<CForward *>(aForward);
	m_managed.remove(fwd);
	m_unmanaged.remove(fwd);
	delete fwd;
}

void CForwardManager::OnPluginPauseChange(IPlugin *plugin, bool paused)
{
	if (paused)
		return;

	/* Attach any globally managed forwards */
	for (ForwardIter iter(m_managed); !iter.done(); iter.next()) {
		CForward *fwd = (*iter);
		IPluginFunction *pFunc = plugin->GetBaseContext()->GetFunctionByName(fwd->GetForwardName());
		// Only add functions, if they aren't registered yet!
		if (pFunc && !fwd->IsFunctionRegistered(pFunc))
		{
			fwd->AddFunction(pFunc);
		}
	}
}

/*************************************
 * ACTUAL FORWARD API IMPLEMENTATION *
 *************************************/

CForward::CForward(ExecType et, const char *name, const ParamType *types, unsigned num_params)
	: m_numparams(0),
	  m_ExecType(et),
	  m_errstate(SP_ERROR_NONE)
{
	ke::SafeStrcpy(m_name, sizeof(m_name), name ? name : "");

	for (unsigned i = 0; i < num_params; i++)
		m_types[i] = types[i];

	m_numparams = num_params;
}

CForward *CForward::CreateForward(const char *name, ExecType et, unsigned int num_params, const ParamType *types, va_list ap)
{
	ParamType _types[SP_MAX_EXEC_PARAMS];

	if (num_params > SP_MAX_EXEC_PARAMS)
	{
		return NULL;
	}
	
	if (types == NULL && num_params)
	{
		for (unsigned int i=0; i<num_params; i++)
			_types[i] = (ParamType)va_arg(ap, int);
		types = _types;
	} else {
		for (unsigned int i=0; i<num_params; i++)
			_types[i] = types[i];
	}

	return new CForward(et, name, _types, num_params);
}

bool CForward::ProcessArguments(const CallArgs& args) {
	if (args.error)
		return !!SetError(SP_ERROR_PARAMS_MAX);

	for (uint32_t i = 0; i < args.argc; i++) {
		if (i >= m_numparams)
			return !!SetError(SP_ERROR_PARAM);

		ParamType pt = m_types[i];
		if (pt == Param_Any)
			continue;

		auto& arg = args.argv[i];
		switch (arg.type) {
			case CallArgs::ARG_CELL:
				if (arg.flags & CallArgs::FLAG_CELL_TYPE_FLOAT) {
					if (pt != Param_Float)
						return !!SetError(SP_ERROR_PARAM);
				} else {
					if (pt != Param_Cell)
						return !!SetError(SP_ERROR_PARAM);
				}
				break;
			case CallArgs::ARG_CELL_BY_REF:
				if (arg.flags & CallArgs::FLAG_CELL_TYPE_FLOAT) {
					if (pt != Param_FloatByRef)
						return !!SetError(SP_ERROR_PARAM);
				} else {
					if (pt != Param_CellByRef)
						return !!SetError(SP_ERROR_PARAM);
				}
				break;
			case CallArgs::ARG_ARRAY:
				if (pt != Param_Array)
					return !!SetError(SP_ERROR_PARAM);
				break;
			case CallArgs::ARG_CHAR_ARRAY:
				if (pt != Param_String)
					return !!SetError(SP_ERROR_PARAM);
				break;
		}
	}

	return true;
}

int CForward::Execute(cell_t *result, IForwardFilter *filter)
{
	auto args = std::move(default_args_);
	default_args_.Reset();

	return Execute(args, result, filter);
}

int CForward::Execute(const sp::CallArgs& in_args, cell_t *result, IForwardFilter *filter)
{
	if (m_errstate) {
		int err = m_errstate;
		Cancel();
		return err;
	}

	auto args = in_args;
	unsigned int success = 0;
	cell_t high_result = 0;
	cell_t low_result = 0;
	cell_t cur_result = 0;

	if (!ProcessArguments(args)) {
		int err = m_errstate;
		Cancel();
		return err;
	}

	for (FuncIter iter(m_functions); !iter.done(); iter.next())
	{
		IPluginFunction *func = (*iter);

		if (filter)
			filter->Preprocess(func, args);

		if (func->GetParentRuntime()->IsPaused())
			continue;

		ExceptionHandler eh(func->GetParentRuntime());

		if (!func->Invoke(args, &cur_result))
			continue;

		success++;
		switch (m_ExecType)
		{
			case ET_Event:
				if (cur_result > high_result)
					high_result = cur_result;
				break;
			case ET_Hook:
				if (cur_result > high_result)
					high_result = cur_result;
				break;
			case ET_LowEvent:
				/* Check if the current result is the lowest so far (or if it's the first result) */
				if (cur_result < low_result || success == 1)
					low_result = cur_result;
				break;
			default:
				break;
		}

		if (m_ExecType == ET_Hook && (ResultType)high_result == Pl_Stop)
			break;
	}

	switch (m_ExecType)
	{
		case ET_Ignore:
			cur_result = 0;
			break;
		case ET_Event:
		case ET_Hook:
			cur_result = high_result;
			break;
		case ET_LowEvent:
			cur_result = low_result;
			break;
		default:
			break;
	}

	if (result)
		*result = cur_result;

	return SP_ERROR_NONE;
}

int CForward::PushCell(cell_t cell)
{
	default_args_.PushCell(cell);
	if (default_args_.error)
		return SetError(SP_ERROR_PARAMS_MAX);
	return SP_ERROR_NONE;
}

int CForward::PushInt64(int64_t value)
{
	default_args_.PushInt64(value);
	if (default_args_.error)
		return SetError(SP_ERROR_PARAMS_MAX);
	return SP_ERROR_NONE;
}

int CForward::PushFloat(float number)
{
	default_args_.PushFloat(number);
	if (default_args_.error)
		return SetError(SP_ERROR_PARAMS_MAX);
	return SP_ERROR_NONE;
}

int CForward::PushCellByRef(cell_t *cell, int flags)
{
	default_args_.PushCellByRef(cell, flags);
	if (default_args_.error)
		return SetError(SP_ERROR_PARAMS_MAX);
	return SP_ERROR_NONE;
}

int CForward::PushFloatByRef(float *num, int flags)
{
	default_args_.PushFloatByRef(num, flags);
	if (default_args_.error)
		return SetError(SP_ERROR_PARAMS_MAX);
	return SP_ERROR_NONE;
}

int CForward::PushArray(cell_t *inarray, unsigned int cells, int flags)
{
	if (!inarray) {
		return SetError(SP_ERROR_PARAM);
	}
	default_args_.PushArray(inarray, cells, flags);
	if (default_args_.error)
		return SetError(SP_ERROR_PARAMS_MAX);
	return SP_ERROR_NONE;
}

int CForward::PushString(const char *string)
{
	if (!string) {
		return SetError(SP_ERROR_PARAM);
	}
	default_args_.PushString(string);
	if (default_args_.error)
		return SetError(SP_ERROR_PARAMS_MAX);
	return SP_ERROR_NONE;
}

int CForward::PushStringEx(char *buffer, size_t length, int sz_flags, int cp_flags)
{
	default_args_.PushString(buffer, length, sz_flags | cp_flags);
	if (default_args_.error)
		return SetError(SP_ERROR_PARAMS_MAX);
	return SP_ERROR_NONE;
}

void CForward::Cancel()
{
	default_args_.Reset();
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

bool CForward::RemoveFunction(IPluginContext *pContext, funcid_t index)
{
	IPluginFunction *pFunc = pContext->GetFunctionById(index);

	if (!pFunc)
	{
		return false;
	}

	return RemoveFunction(pFunc);
}

bool CForward::RemoveFunction(IPluginFunction *func)
{
	ReentrantList<IPluginFunction *> *lst;
	if (func->IsRunnable())
		lst = &m_functions;
	else
		lst = &m_paused;

	bool found = false;
	for (FuncIter iter(lst); !iter.done(); iter.next()) {
		if (*iter == func) {
			iter.remove();
			found = true;
			break;
		}
	}

	/* Cancel a call, if any */
	if (found || default_args_.argc)
		default_args_.Reset();

	return found;
}

unsigned int CForward::RemoveFunctionsOfPlugin(IPlugin *plugin)
{
	unsigned int removed = 0;
	for (FuncIter iter(m_functions); !iter.done(); iter.next()) {
		IPluginFunction *func = *iter;
		if (func->GetParentContext() == plugin->GetBaseContext()) {
			iter.remove();
			removed++;
		}
	}
	return removed;
}

bool CForward::AddFunction(IPluginFunction *func)
{
	if (default_args_.argc)
		return false;

	if (func->IsRunnable())
		m_functions.push_back(func);
	else
		m_paused.push_back(func);

	return true;
}

bool CForward::IsFunctionRegistered(IPluginFunction *func)
{
	ReentrantList<IPluginFunction *> *lst;
	if (func->IsRunnable())
		lst = &m_functions;
	else
		lst = &m_paused;

	for (FuncIter iter(lst); !iter.done(); iter.next()) {
		if ((*iter) == func)
			return true;
	}
	return false;
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
