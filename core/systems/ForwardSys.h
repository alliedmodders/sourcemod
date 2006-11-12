#ifndef _INCLUDE_SOURCEMOD_FORWARDSYSTEM_H_
#define _INCLUDE_SOURCEMOD_FORWARDSYSTEM_H_

#include <IForwardSys.h>
#include <IPluginSys.h>
#include "sm_globals.h"
#include <sh_list.h>
#include <sh_stack.h>

using namespace SourceHook;

typedef List<IPluginFunction *>::iterator FuncIter;

//:TODO: a global name max define for sourcepawn, should mirror compiler's sNAMEMAX
#define FORWARDS_NAME_MAX		64

struct NextCallInfo
{
	unsigned int recopy[SP_MAX_EXEC_PARAMS];
	unsigned int sizes[SP_MAX_EXEC_PARAMS];
	cell_t *orig_addrs[SP_MAX_EXEC_PARAMS];
	unsigned int numrecopy;
};

class CForward : public IChangeableForward, IFunctionCopybackReader
{
public: //ICallable
	virtual int PushCell(cell_t cell);
	virtual int PushCellByRef(cell_t *cell, int flags);
	virtual int PushFloat(float number);
	virtual int PushFloatByRef(float *number, int flags);
	virtual int PushCells(cell_t array[], unsigned int numcells, bool each);
	virtual int PushArray(cell_t *inarray, unsigned int cells, cell_t **phys_addr, int flags);
	virtual int PushString(const char *string);
	virtual int PushStringByRef(char *string, int flags);
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
	virtual bool AddFunction(sp_context_t *ctx, funcid_t index);
public: //IFunctionCopybackReader
	virtual bool OnCopybackArray(unsigned int param, 
								 unsigned int cells, 
								 cell_t *source_addr, 
								 cell_t *orig_addr, 
								 int flags);
public:
	static CForward *CreateForward(const char *name, 
								   ExecType et, 
								   unsigned int num_params, 
								   ParamType *types, 
								   va_list ap);
private:
	void DumpAdditionQueue();
	void _Int_PushArray(cell_t *inarray, unsigned int cells, int flags);
	inline int SetError(int err)
	{
		m_errstate = err;
		return err;
	}
protected:
	/* :TODO: I want a caching list type here.
	 * Destroying these things and using new/delete for their members feels bad.
	 */
	List<IPluginFunction *> m_functions;

	/* Type and name information */
	ParamType m_types[SP_MAX_EXEC_PARAMS];
	char m_name[FORWARDS_NAME_MAX+1];
	unsigned int m_numparams;
	bool m_varargs;
	ExecType m_ExecType;

	/* State information */
	unsigned int m_curparam;
	int m_errstate;
	CStack<NextCallInfo> m_NextStack;
	List<IPluginFunction *> m_AddQueue;
	NextCallInfo m_CopyBacks;
};

#endif //_INCLUDE_SOURCEMOD_FORWARDSYSTEM_H_
