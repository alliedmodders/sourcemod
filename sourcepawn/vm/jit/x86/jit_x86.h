#ifndef _INCLUDE_SOURCEPAWN_JIT_X86_H_
#define _INCLUDE_SOURCEPAWN_JIT_X86_H_

#include <sp_vm_types.h>
#include <sp_vm_api.h>
#include "..\jit_helpers.h"

using namespace SourcePawn;

#define JIT_INLINE_ERRORCHECKS		(1<<0)
#define JIT_INLINE_NATIVES			(1<<1)
#define STACK_MARGIN				64		//8 parameters of safety, I guess

class CompData : public ICompilation
{
public:
	CompData() : plugin(NULL), 
		debug(false), inline_level(0), rebase(NULL)
	{
	};
public:
	sp_plugin_t *plugin;
	jitcode_t rebase;
	jitoffs_t jit_return;
	jitoffs_t jit_verify_addr_eax;
	jitoffs_t jit_verify_addr_edx;
	jitoffs_t jit_break;
	jitoffs_t jit_sysreq_n;
	jitoffs_t jit_error_bounds;
	jitoffs_t jit_error_divzero;
	jitoffs_t jit_error_stacklow;
	jitoffs_t jit_error_stackmin;
	jitoffs_t jit_error_memaccess;
	jitoffs_t jit_error_heaplow;
	jitoffs_t jit_error_heapmin;
	uint32_t codesize;
	int inline_level;
	bool debug;
};

class JITX86 : public IVirtualMachine
{
public:
	JITX86();
public:
	const char *GetVMName();
	ICompilation *StartCompilation(sp_plugin_t *plugin);
	bool SetCompilationOption(ICompilation *co, const char *key, const char *val);
	sp_context_t *CompileToContext(ICompilation *co, int *err);
	void AbortCompilation(ICompilation *co);
	void FreeContext(sp_context_t *ctx);
	int ContextExecute(sp_context_t *ctx, uint32_t code_idx, cell_t *result);
};

cell_t NativeCallback(sp_context_t *ctx, ucell_t native_idx, cell_t *params);
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
#define AMX_INFO_HEAPLOW	20				//not relocated
#define AMX_INFO_STACKTOP_U	24				//not relocated

extern ISourcePawnEngine *engine;

#endif //_INCLUDE_SOURCEPAWN_JIT_X86_H_
