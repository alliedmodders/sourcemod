/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_NATIVE_INVOKER_H_
#define _INCLUDE_SOURCEMOD_NATIVE_INVOKER_H_

#include "sm_globals.h"
#include <INativeInvoker.h>

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

class NativeInvoker : public INativeInvoker
{
public:
	NativeInvoker();
	~NativeInvoker();
public: /* ICallable */
	int PushCell(cell_t cell);
	int PushCellByRef(cell_t *cell, int flags=SM_PARAM_COPYBACK);
	int PushFloat(float number);
	int PushFloatByRef(float *number, int flags=SM_PARAM_COPYBACK);
	int PushArray(cell_t *inarray, unsigned int cells, int flags=0);
	int PushString(const char *string);
	int PushStringEx(char *buffer, size_t length, int sz_flags, int cp_flags);
	void Cancel();
public: /* INativeInvoker */
	bool Start(IPluginContext *pContext, const char *name);
	int Invoke(cell_t *result);
private:
	int _PushString(const char *string, int sz_flags, int cp_flags, size_t len);
	int SetError(int err);
private:
	IPluginContext *pContext;
	SPVM_NATIVE_FUNC native;
	cell_t m_params[SP_MAX_EXEC_PARAMS];
	ParamInfo m_info[SP_MAX_EXEC_PARAMS];
	unsigned int m_curparam;
	int m_errorstate;
};

class NativeInterface : 
	public INativeInterface,
	public SMGlobalClass
{
public: /* SMGlobalClass */
	void OnSourceModAllInitialized();
public: /* SMInterface */
	unsigned int GetInterfaceVersion();
	const char *GetInterfaceName();
public: /* INativeInvoker */
	IPluginRuntime *CreateRuntime(const char *name, size_t bytes);
	INativeInvoker *CreateInvoker();
};

extern NativeInterface g_NInvoke;

#endif /* _INCLUDE_SOURCEMOD_NATIVE_INVOKER_H_ */
