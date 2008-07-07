/**
 * vim: set ts=4 :
 * =============================================================================
 * SourcePawn
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_BASEFUNCTION_H_
#define _INCLUDE_SOURCEMOD_BASEFUNCTION_H_

#include <sp_vm_api.h>

class BaseRuntime;

using namespace SourcePawn;

struct ParamInfo
{
	int flags;			/* Copy-back flags */
	bool marked;		/* Whether this is marked as being used */
	cell_t local_addr;	/* Local address to free */
	cell_t *phys_addr;	/* Physical address of our copy */
	cell_t *orig_addr;	/* Original address to copy back to */
	ucell_t size;		/* Size of array in bytes */
	struct
	{
		bool is_sz;		/* is a string */
		int sz_flags;	/* has sz flags */
	} str;
};

class CPlugin;

class CFunction : public IPluginFunction
{
	friend class SourcePawnEngine;
public:
	CFunction(uint32_t code_addr,
			  BaseRuntime *pRuntime, 
			  funcid_t fnid,
			  uint32_t pub_id);
public:
	virtual int PushCell(cell_t cell);
	virtual int PushCellByRef(cell_t *cell, int flags);
	virtual int PushFloat(float number);
	virtual int PushFloatByRef(float *number, int flags);
	virtual int PushArray(cell_t *inarray, unsigned int cells, int copyback);
	virtual int PushString(const char *string);
	virtual int PushStringEx(char *buffer, size_t length, int sz_flags, int cp_flags);
	virtual int Execute(cell_t *result);
	virtual void Cancel();
	virtual int CallFunction(const cell_t *params, unsigned int num_params, cell_t *result);
	virtual IPluginContext *GetParentContext();
	bool IsInvalidated();
	void Invalidate();
	bool IsRunnable();
	funcid_t GetFunctionID();
	int Execute(IPluginContext *ctx, cell_t *result);
	int CallFunction(IPluginContext *ctx, 
		const cell_t *params, 
		unsigned int num_params, 
		cell_t *result);
public:
	void Set(uint32_t code_addr, BaseRuntime *runtime, funcid_t fnid, uint32_t pub_id);
private:
	int _PushString(const char *string, int sz_flags, int cp_flags, size_t len);
	int SetError(int err);
private:
	uint32_t m_codeaddr;
	BaseRuntime *m_pRuntime;
	cell_t m_params[SP_MAX_EXEC_PARAMS];
	ParamInfo m_info[SP_MAX_EXEC_PARAMS];
	unsigned int m_curparam;
	int m_errorstate;
	CFunction *m_pNext;
	bool m_Invalid;
	funcid_t m_FnId;
};

#endif //_INCLUDE_SOURCEMOD_BASEFUNCTION_H_
