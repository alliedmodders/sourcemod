/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
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

#ifndef _INCLUDE_SOURCEPAWN_JIT_X86_H_
#define _INCLUDE_SOURCEPAWN_JIT_X86_H_

#include <sp_vm_types.h>
#include <sp_vm_api.h>
#include <KeCodeAllocator.h>
#include "jit_helpers.h"
#include "jit_shared.h"
#include "BaseRuntime.h"
#include "jit_function.h"

using namespace SourcePawn;

#define JIT_INLINE_ERRORCHECKS		(1<<0)
#define JIT_INLINE_NATIVES			(1<<1)
#define STACK_MARGIN				64			//8 parameters of safety, I guess
#define JIT_FUNCMAGIC				0x214D4148	//magic function offset

#define JITVARS_TRACKER				0		//important: don't change this to avoid trouble
#define JITVARS_BASECTX				1		//important: don't change this aWOAWOGJQG I LIKE HAM
#define JITVARS_PROFILER			2		//profiler
#define JITVARS_PLUGIN				3		//sp_plugin_t

#define sDIMEN_MAX					5		//this must mirror what the compiler has.

#define GET_CONTEXT(c)  ((IPluginContext *)c->vm[JITVARS_BASECTX])

typedef struct tracker_s
{
	size_t size; 
	ucell_t *pBase; 
	ucell_t *pCur;
} tracker_t;

typedef struct funcinfo_s
{
	unsigned int magic;
	unsigned int index;
} funcinfo_t;

typedef struct functracker_s
{
	unsigned int num_functions;
	unsigned int code_size;
} functracker_t;

struct floattbl_t
{
	floattbl_t()
	{
		found = false;
		index = 0;
	}
	bool found;
	unsigned int index;
};

struct call_thunk_t
{
	jitoffs_t patch_addr;
	cell_t pcode_offs;
	jitoffs_t thunk_addr;
};

class CompData : public ICompilation
{
public:
	CompData() 
	:	runtime(NULL),
		plugin(NULL),
		rebase(NULL),
		jit_float_table(NULL),
		profile(0),
		inline_level(0),
		error_set(SP_ERROR_NONE),
		num_thunks(0),
		max_thunks(0),
		thunks(NULL)
	{
	};
	bool SetOption(const char *key, const char *val);
	void SetRuntime(BaseRuntime *runtime);
	void Abort();
public:
	BaseRuntime *runtime;			/* runtime handle */
	sp_plugin_t *plugin;			/* plugin handle */
	uint8_t *rebase;				/* relocation map */
	floattbl_t *jit_float_table;
	cell_t cur_func;				/* current func pcode offset */
	/* Options */
	int profile;					/* profiling flags */
	int inline_level;				/* inline optimization level */
	/* Per-compilation properties */
	int error_set;					/* error code to halt process */
	unsigned int func_idx;			/* current function index */
	jitoffs_t jit_error_bounds;
	jitoffs_t jit_error_divzero;
	jitoffs_t jit_error_stacklow;
	jitoffs_t jit_error_stackmin;
	jitoffs_t jit_error_memaccess;
	jitoffs_t jit_error_heaplow;
	jitoffs_t jit_error_heapmin;
	jitoffs_t jit_extern_error;		/* returning generic error */
	jitoffs_t jit_sysreq_c;			/* old version! */
	uint32_t num_thunks;			/* number of thunks needed */
	uint32_t max_thunks;			/* maximum number of thunks */
	call_thunk_t *thunks;			/* thunk array */
};

class JITX86
{
public:
	JITX86();
public:
	bool InitializeJIT();
	void ShutdownJIT();
	ICompilation *StartCompilation(BaseRuntime *runtime);
	ICompilation *StartCompilation();
	void SetupContextVars(BaseRuntime *runtime, BaseContext *pCtx, sp_context_t *ctx);
	void FreeContextVars(sp_context_t *ctx);
	SPVM_NATIVE_FUNC CreateFakeNative(SPVM_FAKENATIVE_FUNC callback, void *pData);
	void DestroyFakeNative(SPVM_NATIVE_FUNC func);
	JitFunction *CompileFunction(BaseRuntime *runtime, cell_t pcode_offs, int *err);
	ICompilation *ApplyOptions(ICompilation *_IN, ICompilation *_OUT);
	int InvokeFunction(BaseRuntime *runtime, JitFunction *fn, cell_t *result);
public:
	void *GetGenArrayIntrinsic();
	void *GetReturnPoint();
	void *GetRoundingTable();
	void *AllocCode(size_t size);
	void FreeCode(void *code);
private:
	void *m_pJitEntry;				/* Entry function */
	void *m_pJitReturn;				/* Return point for errors */
	int m_RoundTable[3];			/* [-1, 0, 1] rounding table */
	void *m_pJitGenArray;			/* Generates an array */
};

cell_t NativeCallback(sp_context_t *ctx, ucell_t native_idx, cell_t *params);
cell_t NativeCallback_Profile(sp_context_t *ctx, ucell_t native_idx, cell_t *params);
uint32_t FuncLookup(CompData *data, cell_t pcode_offs);
jitoffs_t RelocLookup(JitWriter *jit, cell_t pcode_offs, bool relative=false);
void *CompileThunk(BaseRuntime *runtime, cell_t pcode_ffs, void *jmploc_addr);

#define AMX_REG_PRI		REG_EAX
#define AMX_REG_ALT		REG_EDX
#define AMX_REG_STK		REG_EDI
#define AMX_REG_DAT		REG_EBP
#define AMX_REG_TMP		REG_ECX
#define AMX_REG_INFO	REG_ESI
#define AMX_REG_FRM		REG_EBX

#define AMX_NUM_INFO_VARS	9

#define AMX_INFO_FRM		AMX_REG_INFO	//not relocated
#define AMX_INFO_FRAME		0				//(same thing as above) 
#define AMX_INFO_HEAP		4				//not relocated
#define AMX_INFO_RETVAL		8				//physical
#define AMX_INFO_CONTEXT	12				//physical
#define AMX_INFO_STACKTOP	16				//relocated
#define AMX_INFO_CIP		20				//pcode CIP
#define AMX_INFO_DATASIZE	24				//plugin->data_size
#define AMX_INFO_MEMORY		28				//plugin->memory
#define AMX_INFO_NSTACK		32				//native stack

extern Knight::KeCodeCache *g_pCodeCache;
extern JITX86 g_Jit;

#endif //_INCLUDE_SOURCEPAWN_JIT_X86_H_
