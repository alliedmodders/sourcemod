#ifndef _INCLUDE_SOURCEMOD_BASEFUNCTION_H_
#define _INCLUDE_SOURCEMOD_BASEFUNCTION_H_

#include <IPluginFunction.h>
#include "sm_globals.h"

struct ParamInfo
{
	int flags;			/* Copy-back flags */
	bool marked;		/* Whether this is marked as being used */
	cell_t local_addr;	/* Local address to free */
	cell_t *phys_addr;	/* Physical address of our copy */
	cell_t *orig_addr;	/* Original address to copy back to */
	ucell_t size;		/* Size of array in cells */
};

class CPlugin;

class CFunction : public IPluginFunction
{
public:
	CFunction(funcid_t funcid, CPlugin *plugin);
public:
	virtual int PushCell(cell_t cell);
	virtual int PushCellByRef(cell_t *cell, int flags);
	virtual int PushFloat(float number);
	virtual int PushFloatByRef(float *number, int flags);
	virtual int PushArray(cell_t *inarray, unsigned int cells, cell_t **phys_addr, int copyback);
	virtual int PushString(const char *string);
	virtual int PushStringEx(char *buffer, size_t length, int sz_flags, int cp_flags);
	virtual cell_t *GetAddressOfPushedParam(unsigned int param);
	virtual int Execute(cell_t *result);
	virtual void Cancel();
	virtual int CallFunction(const cell_t *params, unsigned int num_params, cell_t *result);
	virtual IPlugin *GetParentPlugin();
public:
	void Set(funcid_t funcid, CPlugin *plugin);
private:
	int _PushString(const char *string, int sz_flags, int cp_flags, size_t len);
	inline int SetError(int err)
	{
		m_errorstate = err;
		return err;
	}
private:
	funcid_t m_funcid;
	CPlugin *m_pPlugin;
	cell_t m_params[SP_MAX_EXEC_PARAMS];
	ParamInfo m_info[SP_MAX_EXEC_PARAMS];
	unsigned int m_curparam;
	int m_errorstate;
};

#endif //_INCLUDE_SOURCEMOD_BASEFUNCTION_H_
