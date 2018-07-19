// vim: set ts=4 sw=4 tw=99 noet :
// =============================================================================
// SourceMod
// Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_FORWARDSYSTEM_H_
#define _INCLUDE_SOURCEMOD_FORWARDSYSTEM_H_

#include <IForwardSys.h>
#include <IPluginSys.h>
#include "common_logic.h"
#include "ISourceMod.h"
#include "ReentrantList.h"

typedef ReentrantList<IPluginFunction *>::iterator FuncIter;

/* :TODO: a global name max define for sourcepawn, should mirror compiler's sNAMEMAX */
#define FORWARDS_NAME_MAX		64

class CForward : public IChangeableForward
{
public: //ICallable
	virtual int PushCell(cell_t cell);
	virtual int PushCellByRef(cell_t *cell, int flags);
	virtual int PushFloat(float number);
	virtual int PushFloatByRef(float *number, int flags);
	virtual int PushArray(cell_t *inarray, unsigned int cells, int flags);
	virtual int PushString(const char *string);
	virtual int PushStringEx(char *buffer, size_t length, int sz_flags, int cp_flags);
	virtual void Cancel();
public: //IForward
	virtual const char *GetForwardName();
	virtual unsigned int GetFunctionCount();
	virtual ExecType GetExecType();
	virtual int Execute(cell_t *result, IForwardFilter *filter);
public: //IChangeableForward
	virtual bool RemoveFunction(IPluginFunction *func);
	virtual unsigned int RemoveFunctionsOfPlugin(IPlugin *plugin);
	virtual bool AddFunction(IPluginFunction *func);
	virtual bool AddFunction(IPluginContext *ctx, funcid_t index);
	virtual bool RemoveFunction(IPluginContext *ctx, funcid_t index);
public:
	static CForward *CreateForward(const char *name, 
								   ExecType et, 
								   unsigned int num_params, 
								   const ParamType *types, 
								   va_list ap);
	bool IsFunctionRegistered(IPluginFunction *func);
private:
	CForward(ExecType et, const char *name,
	         const ParamType *types, unsigned num_params);

	int PushNullString();
	int PushNullVector();
	int _ExecutePushRef(IPluginFunction *func, ParamType type, FwdParamInfo *param);
	void _Int_PushArray(cell_t *inarray, unsigned int cells, int flags);
	void _Int_PushString(cell_t *inarray, unsigned int cells, int sz_flags, int cp_flags);
	inline int SetError(int err)
	{
		m_errstate = err;
		return err;
	}
protected:
	mutable ReentrantList<IPluginFunction *> m_functions;
	mutable ReentrantList<IPluginFunction *> m_paused;

	/* Type and name information */
	FwdParamInfo m_params[SP_MAX_EXEC_PARAMS];
	ParamType m_types[SP_MAX_EXEC_PARAMS];
	char m_name[FORWARDS_NAME_MAX+1];
	unsigned int m_numparams;
	unsigned int m_varargs;
	ExecType m_ExecType;

	/* State information */
	unsigned int m_curparam;
	int m_errstate;
};

class CForwardManager : 
	public IForwardManager,
	public IPluginsListener,
	public SMGlobalClass
{
	friend class CForward;
public: //IForwardManager
	IForward *CreateForward(const char *name, 
		ExecType et, 
		unsigned int num_params, 
		const ParamType *types, 
		...);
	IChangeableForward *CreateForwardEx(const char *name, 
		ExecType et, 
		int num_params, 
		const ParamType *types, 
		...);
	IForward *FindForward(const char *name, IChangeableForward **ifchng);
	void ReleaseForward(IForward *forward);
public: //IPluginsListener
	void OnPluginLoaded(IPlugin *plugin);
	void OnPluginUnloaded(IPlugin *plugin);
	void OnPluginPauseChange(IPlugin *plugin, bool paused);
public: //SMGlobalClass
	void OnSourceModAllInitialized();
private:
	ReentrantList<CForward *> m_managed;
	ReentrantList<CForward *> m_unmanaged;

	typedef ReentrantList<CForward *>::iterator ForwardIter;
};

extern CForwardManager g_Forwards;

#endif //_INCLUDE_SOURCEMOD_FORWARDSYSTEM_H_
