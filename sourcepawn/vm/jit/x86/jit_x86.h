#ifndef _INCLUDE_SOURCEPAWN_JIT_X86_H_
#define _INCLUDE_SOURCEPAWN_JIT_X86_H_

#include <sp_vm_types.h>
#include <sp_vm_api.h>
#include "..\jit_helpers.h"

using namespace SourcePawn;

#define JIT_INLINE_ERRORCHECKS		(1<<0)
#define JIT_INLINE_NATIVES			(1<<1)

class CompData : public ICompilation
{
public:
	CompData() : plugin(NULL), 
		debug(false), inline_level(3), checks(true)
	{
	};
public:
	sp_plugin_t *plugin;
	jitoffs_t jit_return;
	jitoffs_t jit_verify_addr_eax;
	jitoffs_t jit_verify_addr_edx;
	int inline_level;
	bool checks;
	bool debug;
};

class JITX86 : public IVirtualMachine
{
public:
	JITX86();
public:
	const char *GetVMName() =0;
	ICompilation *StartCompilation(sp_plugin_t *plugin);
	bool SetCompilationOption(ICompilation *co, const char *key, const char *val) ;
	IPluginContext *CompileToContext(ICompilation *co, int *err);
	void AbortCompilation(ICompilation *co);
	void FreeContextVars(sp_context_t *ctx);
	int ContextExecute(sp_context_t *ctx, uint32_t code_idx, cell_t *result);
};

#define AMX_REG_PRI		REG_EAX
#define AMX_REG_ALT		REG_EDX
#define AMX_REG_STK		REG_EBP
#define AMX_REG_DAT		REG_EDI
#define AMX_REG_TMP		REG_ECX
#define AMX_REG_INFO	REG_ESI
#define AMX_REG_FRM		REG_EBX

#define AMX_INFO_FRM		AMX_REG_INFO
#define AMX_INFO_HEAP		4
#define AMX_INFO_RETVAL		8
#define AMX_INFO_CONTEXT	12
#define AMX_INFO_STACKTOP	16

#endif //_INCLUDE_SOURCEPAWN_JIT_X86_H_
