#ifndef _INCLUDE_SOURCEPAWN_JIT_X86_H_
#define _INCLUDE_SOURCEPAWN_JIT_X86_H_

#include <sp_vm_types.h>
#include <sp_vm_api.h>
#include "..\jit_helpers.h"

using namespace SourcePawn;

#define JIT_INLINE_ERRORCHECKS		(1<<0)
#define JIT_INLINE_NATIVES			(1<<1)
#define STACK_MARGIN				64			//8 parameters of safety, I guess
#define JIT_FUNCMAGIC				0x214D4148	//magic function offset

#define JITVARS_TRACKER				0		//important: don't change this to avoid trouble
#define JITVARS_FUNCINFO			1		//important: don't change this aWOAWOGJQG I LIKE HAM

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

class CompData : public ICompilation
{
public:
	CompData() : plugin(NULL), 
		debug(false), inline_level(0), rebase(NULL),
		error_set(SP_ERROR_NONE), func_idx(0)
	{
	};
public:
	sp_plugin_t *plugin;			/* plugin handle */
	jitcode_t rebase;				/* relocation map */
	jitoffs_t jit_return;			/* point in main call to return to */
	jitoffs_t jit_verify_addr_eax;
	jitoffs_t jit_verify_addr_edx;
	jitoffs_t jit_break;			/* call to op.break */
	jitoffs_t jit_sysreq_n;			/* call version of op.sysreq.n */
	jitoffs_t jit_genarray;			/* call to genarray intrinsic */
	jitoffs_t jit_error_bounds;
	jitoffs_t jit_error_divzero;
	jitoffs_t jit_error_stacklow;
	jitoffs_t jit_error_stackmin;
	jitoffs_t jit_error_memaccess;
	jitoffs_t jit_error_heaplow;
	jitoffs_t jit_error_heapmin;
	jitoffs_t jit_error_array_too_big;
	jitoffs_t jit_error_tracker_bounds;
	jitoffs_t jit_extern_error;		/* returning generic error */
	jitoffs_t jit_sysreq_c;			/* old version! */
	uint32_t codesize;				/* total codesize */
	unsigned int func_idx;			/* current function index */
	int inline_level;				/* inline optimization level */
	int error_set;					/* error code to halt process */
	bool debug;						/* whether to compile debug mode */
};

class JITX86 : public IVirtualMachine
{
public:
	const char *GetVMName();
	ICompilation *StartCompilation(sp_plugin_t *plugin);
	bool SetCompilationOption(ICompilation *co, const char *key, const char *val);
	sp_context_t *CompileToContext(ICompilation *co, int *err);
	void AbortCompilation(ICompilation *co);
	void FreeContext(sp_context_t *ctx);
	int ContextExecute(sp_context_t *ctx, uint32_t code_idx, cell_t *result);
	unsigned int GetAPIVersion();
	bool FunctionLookup(const sp_context_t *ctx, uint32_t code_addr, unsigned int *result);
	unsigned int FunctionCount(const sp_context_t *ctx);
};

cell_t NativeCallback(sp_context_t *ctx, ucell_t native_idx, cell_t *params);
cell_t NativeCallback_Debug(sp_context_t *ctx, ucell_t native_idx, cell_t *params);
jitoffs_t RelocLookup(JitWriter *jit, cell_t pcode_offs, bool relative=false);

#define AMX_REG_PRI		REG_EAX
#define AMX_REG_ALT		REG_EDX
#define AMX_REG_STK		REG_EDI
#define AMX_REG_DAT		REG_EBP
#define AMX_REG_TMP		REG_ECX
#define AMX_REG_INFO	REG_ESI
#define AMX_REG_FRM		REG_EBX

#define AMX_INFO_FRM		AMX_REG_INFO	//not relocated
#define AMX_INFO_FRAME		0				//(same thing as above) 
#define AMX_INFO_HEAP		4				//not relocated
#define AMX_INFO_RETVAL		8				//physical
#define AMX_INFO_CONTEXT	12				//physical
#define AMX_INFO_STACKTOP	16				//relocated

extern ISourcePawnEngine *engine;

#endif //_INCLUDE_SOURCEPAWN_JIT_X86_H_
