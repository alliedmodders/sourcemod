#ifndef _INCLUDE_SOURCEPAWN_JIT_X86_H_
#define _INCLUDE_SOURCEPAWN_JIT_X86_H_

#include <sp_vm_api.h>

using namespace SourcePawn;

class CompData : public ICompilation
{
public:
	CompData() : plugin(NULL), debug(false), always_inline(true)
	{
	};
public:
	sp_plugin_t *plugin;
	bool always_inline;
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

typedef enum
{
	OP_NONE,              /* invalid opcode */
	OP_LOAD_PRI,		//DONE
	OP_LOAD_ALT,		//DONE
	OP_LOAD_S_PRI,		//DONE
	OP_LOAD_S_ALT,		//DONE
	OP_LREF_PRI,		//DONE
	OP_LREF_ALT,		//DONE
	OP_LREF_S_PRI,		//DONE
	OP_LREF_S_ALT,		//DONE
	OP_LOAD_I,			//DONE
	OP_LODB_I,
	OP_CONST_PRI,		//DONE
	OP_CONST_ALT,		//DONE
	OP_ADDR_PRI,		//DONE
	OP_ADDR_ALT,		//DONE
	OP_STOR_PRI,		//DONE
	OP_STOR_ALT,		//DONE
	OP_STOR_S_PRI,		//DONE
	OP_STOR_S_ALT,		//DONE
	OP_SREF_PRI,		//DONE
	OP_SREF_ALT,		//DONE
	OP_SREF_S_PRI,		//DONE
	OP_SREF_S_ALT,		//DONE
	OP_STOR_I,			//DONE
	OP_STRB_I,
	OP_LIDX,			//DONE
	OP_LIDX_B,
	OP_IDXADDR,			//DONE
	OP_IDXADDR_B,
	OP_ALIGN_PRI,		//DONE
	OP_ALIGN_ALT,		//DONE
	OP_LCTRL,
	OP_SCTRL,
	OP_MOVE_PRI,		//DONE
	OP_MOVE_ALT,		//DONE
	OP_XCHG,			//DONE
	OP_PUSH_PRI,		//DONE
	OP_PUSH_ALT,		//DONE
	OP_PUSH_R,			//DONE
	OP_PUSH_C,			//DONE
	OP_PUSH,			//DONE
	OP_PUSH_S,			//DONE
	OP_POP_PRI,			//DONE
	OP_POP_ALT,			//DONE
	OP_STACK,			//DONE
	OP_HEAP,			//DONE
	OP_PROC,			//DONE
	OP_RET,
	OP_RETN,			//DONE
	OP_CALL,
	OP_CALL_PRI,
	OP_JUMP,			//DONE
	OP_JREL,			//DONE
	OP_JZER,			//DONE
	OP_JNZ,				//DONE
	OP_JEQ,				//DONE
	OP_JNEQ,			//DONE
	OP_JLESS,			//DONE
	OP_JLEQ,			//DONE
	OP_JGRTR,			//DONE
	OP_JGEQ,			//DONE
	OP_JSLESS,			//DONE
	OP_JSLEQ,			//DONE
	OP_JSGRTR,			//DONE
	OP_JSGEQ,			//DONE
	OP_SHL,				//DONE
	OP_SHR,				//DONE
	OP_SSHR,			//DONE
	OP_SHL_C_PRI,		//DONE
	OP_SHL_C_ALT,		//DONE
	OP_SHR_C_PRI,		//DONE
	OP_SHR_C_ALT,		//DONE
	OP_SMUL,			//DONE
	OP_SDIV,			//DONE
	OP_SDIV_ALT,		//DONE
	OP_UMUL,			//DONE
	OP_UDIV,			//DONE
	OP_UDIV_ALT,		//DONE
	OP_ADD,				//DONE
	OP_SUB,				//DONE
	OP_SUB_ALT,			//DONE
	OP_AND,				//DONE
	OP_OR,				//DONE
	OP_XOR,				//DONE
	OP_NOT,				//DONE
	OP_NEG,				//DONE
	OP_INVERT,			//DONE
	OP_ADD_C,			//DONE
	OP_SMUL_C,			//DONE
	OP_ZERO_PRI,		//DONE
	OP_ZERO_ALT,		//DONE
	OP_ZERO,			//DONE
	OP_ZERO_S,			//DONE
	OP_SIGN_PRI,		//DONE
	OP_SIGN_ALT,		//DONE
	OP_EQ,				//DONE
	OP_NEQ,				//DONE
	OP_LESS,			//DONE
	OP_LEQ,				//DONE
	OP_GRTR,			//DONE
	OP_GEQ,				//DONE
	OP_SLESS,			//DONE
	OP_SLEQ,			//DONE
	OP_SGRTR,			//DONE
	OP_SGEQ,			//DONE
	OP_EQ_C_PRI,		//DONE
	OP_EQ_C_ALT,		//DONE
	OP_INC_PRI,			//DONE
	OP_INC_ALT,			//DONE
	OP_INC,				//DONE
	OP_INC_S,			//DONE
	OP_INC_I,			//DONE
	OP_DEC_PRI,			//DONE
	OP_DEC_ALT,			//DONE
	OP_DEC,				//DONE
	OP_DEC_S,			//DONE
	OP_DEC_I,			//DONE
	OP_MOVS,			//DONE
	OP_CMPS,			//DONE
	OP_FILL,			//DONE
	OP_HALT,			//DONE
	OP_BOUNDS,
	OP_SYSREQ_PRI,
	OP_SYSREQ_C,
	OP_FILE,
	OP_LINE,
	OP_SYMBOL,
	OP_SRANGE,
	OP_JUMP_PRI,
	OP_SWITCH,
	OP_CASETBL,			//DONE
	OP_SWAP_PRI,		//DONE
	OP_SWAP_ALT,		//DONE
	OP_PUSH_ADR,		//DONE
	OP_NOP,				//DONE
	OP_SYSREQ_N,
	OP_SYMTAG, 
	OP_BREAK,
	OP_PUSH2_C,			//DONE
	OP_PUSH2,			//DONE
	OP_PUSH2_S,			//DONE
	OP_PUSH2_ADR,		//DONE
	OP_PUSH3_C,			//DONE
	OP_PUSH3,			//DONE
	OP_PUSH3_S,			//DONE
	OP_PUSH3_ADR,		//DONE
	OP_PUSH4_C,			//DONE
	OP_PUSH4,			//DONE
	OP_PUSH4_S,			//DONE
	OP_PUSH4_ADR,		//DONE
	OP_PUSH5_C,			//DONE
	OP_PUSH5,			//DONE
	OP_PUSH5_S,			//DONE
	OP_PUSH5_ADR,		//DONE
	OP_LOAD_BOTH,
	OP_LOAD_S_BOTH,
	OP_CONST,
	OP_CONST_S,
	/* ----- */
	OP_SYSREQ_D,
	OP_SYSREQ_ND,
	/* ----- */
	OP_HEAP_PRI,
	OP_PUSH_HEAP_C,
	OP_POP_HEAP_PRI,
	/* ----- */
	OP_NUM_OPCODES
} OPCODE;

#endif //_INCLUDE_SOURCEPAWN_JIT_X86_H_
