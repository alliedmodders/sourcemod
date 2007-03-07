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

#ifndef _INCLUDE_SOURCEMOD_FORWARDSYSTEM_H_
#define _INCLUDE_SOURCEMOD_FORWARDSYSTEM_H_

#include <IForwardSys.h>
#include <IPluginSys.h>
#include "sm_globals.h"
#include <sh_list.h>
#include <sh_stack.h>
#include "sourcemod.h"

using namespace SourceHook;

typedef List<IPluginFunction *>::iterator FuncIter;

//:TODO: a global name max define for sourcepawn, should mirror compiler's sNAMEMAX
#define FORWARDS_NAME_MAX		64

struct ByrefInfo
{
	unsigned int cells;
	cell_t *orig_addr;
	int flags;
	int sz_flags;
};

struct FwdParamInfo
{
	cell_t val;
	ByrefInfo byref;
	ParamType pushedas;
};

class CForward : public IChangeableForward
{
public: //ICallable
	virtual int PushCell(cell_t cell);
	virtual int PushCellByRef(cell_t *cell, int flags);
	virtual int PushFloat(float number);
	virtual int PushFloatByRef(float *number, int flags);
	virtual int PushArray(cell_t *inarray, unsigned int cells, cell_t **phys_addr, int flags);
	virtual int PushString(const char *string);
	virtual int PushStringEx(char *buffer, size_t length, int sz_flags, int cp_flags);
	virtual void Cancel();
public: //IForward
	virtual const char *GetForwardName() const;
	virtual unsigned int GetFunctionCount() const;
	virtual ExecType GetExecType() const;
	virtual int Execute(cell_t *result, IForwardFilter *filter);
public: //IChangeableForward
	virtual bool RemoveFunction(IPluginFunction *func);
	virtual unsigned int RemoveFunctionsOfPlugin(IPlugin *plugin);
	virtual bool AddFunction(IPluginFunction *func);
	virtual bool AddFunction(IPluginContext *ctx, funcid_t index);
public:
	static CForward *CreateForward(const char *name, 
								   ExecType et, 
								   unsigned int num_params, 
								   const ParamType *types, 
								   va_list ap);
private:
	void _Int_PushArray(cell_t *inarray, unsigned int cells, int flags);
	void _Int_PushString(cell_t *inarray, unsigned int cells, int sz_flags, int cp_flags);
	inline int SetError(int err)
	{
		m_errstate = err;
		return err;
	}
protected:
	/* :TODO: I want a caching list type here.
	 * Destroying these things and using new/delete for their members feels bad.
	 */
	mutable List<IPluginFunction *> m_functions;
	mutable List<IPluginFunction *> m_paused;

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
	void OnSourceModShutdown();
protected:
	CForward *ForwardMake();
	void ForwardFree(CForward *fwd);
private:
	CStack<CForward *> m_FreeForwards;
	List<CForward *> m_managed;
	List<CForward *> m_unmanaged;
};

extern CForwardManager g_Forwards;

#endif //_INCLUDE_SOURCEMOD_FORWARDSYSTEM_H_
