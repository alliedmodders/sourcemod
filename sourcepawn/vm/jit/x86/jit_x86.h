#ifndef _INCLUDE_SOURCEPAWN_JIT_X86_H_
#define _INCLUDE_SOURCEPAWN_JIT_X86_H_

#include <sp_vm_api.h>

using namespace SourcePawn;

class CompData : public ICompilation
{
public:
	CompData() : plugin(NULL), debug(false)
	{
	};
public:
	sp_plugin_t *plugin;
	bool debug;
};

class JITX86 : public IVirtualMachine
{
public:
	const char *GetVMName() =0;
	ICompilation *StartCompilation(sp_plugin_t *plugin);
	bool SetCompilationOption(ICompilation *co, const char *key, const char *val) ;
	IPluginContext *CompileToContext(ICompilation *co);
	void AbortCompilation(ICompilation *co);
	void FreeContextVars(sp_context_t *ctx);
	int ContextExecute(sp_context_t *ctx, uint32_t code_idx, cell_t *result);
};

#endif //_INCLUDE_SOURCEPAWN_JIT_X86_H_
