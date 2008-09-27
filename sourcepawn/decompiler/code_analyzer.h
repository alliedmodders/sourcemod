#ifndef _INCLUDE_SP_DECOMP_CODE_ANALYZER_H_
#define _INCLUDE_SP_DECOMP_CODE_ANALYZER_H_

#include <sp_vm_types.h>

enum Opcode
{
	_OP_VAR		= -1,		/* Variable container */
	_OP_STMT	= -2,		/* Statement contained */
	_OP_CONST	= -3,		/* Constant value */
	_OP_STORE	= -4,		/* Store operation (binary assignment) */
	_OP_NEW		= -5,		/* Variable declaration */
	_OP_MODULO	= -6,		/* Binary modulo operator */
	_OP_DEFINE	= -7,		/* "define" node for SSA */
	_OP_USE		= -8,		/* "use" node for SSA */
#define OPDEF(num,name,str,params) \
	OP_##name = num,
#include "opcodes.tbl"
#undef OPDEF
};

struct sp_opdef_t
{
	const char *name;
	int params;
};

struct FuncVar;

struct BaseNode
{
	BaseNode(cell_t _op) : op(_op)
	{
	}
	cell_t op;
};

struct ConstNode : public BaseNode
{
	ConstNode(cell_t _val) : BaseNode(_OP_CONST), val(_val)
	{
	}
	cell_t val;
};

struct StmtNode : public BaseNode
{
	StmtNode(cell_t _op, BaseNode *_node, StmtNode *_next) : BaseNode(_op)
		, node(_node)
		, next(_next)
	{
	}
	BaseNode *node;
	StmtNode *next;
};

struct FunctionInfo;

struct CallNode : public BaseNode
{
	CallNode(FunctionInfo *f, BaseNode **t, unsigned int c) : BaseNode(OP_CALL)
		, func(f)
		, args(t)
		, argc(c)
	{
	}
	FunctionInfo *func;
	BaseNode **args;
	unsigned int argc;
};

struct DefineNode : public BaseNode
{
	DefineNode(BaseNode *o, const char *fmt, ...);
	char name[32];
	BaseNode *other;
};

struct UseNode : public BaseNode
{
	UseNode(DefineNode *n) : BaseNode(_OP_USE), def(n)
	{
	}
	DefineNode *def;
};

struct UnaryNode : public BaseNode
{
	UnaryNode(cell_t _op, BaseNode *_node) : BaseNode(_op), node(_node)
	{
	}
	BaseNode *node;
};

struct VarNode : public BaseNode
{
	VarNode(FuncVar *_var) : BaseNode(_OP_VAR), var(_var)
	{
	}
	FuncVar *var;
};

struct BinOpNode : public BaseNode
{
	BinOpNode(cell_t _op, BaseNode *_left, BaseNode *_right) : BaseNode(_op)
		,left(_left)
		,right(_right)
	{
	}
	BaseNode *left;
	BaseNode *right;
};

struct DeclNode : public BaseNode
{
	DeclNode(VarNode *_var, BaseNode *_expr) : BaseNode(_OP_NEW), var(_var), expr(_expr)
	{
	}
	VarNode *var;
	BaseNode *expr;
};

struct FuncVar
{
	sp_fdbg_symbol_t *sym;
	sp_fdbg_arraydim_t *dims;
	sp_tag_t *tag;
	const char *name;
	unsigned int dimcount;
	DeclNode *new_stmt;
};

struct FunctionInfo
{
	char name[64];
	sp_fdbg_symbol_t *sym;
	FuncVar known_args[SP_MAX_EXEC_PARAMS];
	unsigned int num_known_args;
	sp_tag_t *tag;
	uint32_t code_addr;
	uint32_t code_end;
	bool is_public;
	FunctionInfo *next;
	FuncVar *known_vars;
	unsigned int num_known_vars;
};

struct sp_decomp_t;

int Sp_DecompFunction(sp_decomp_t *dc, uint32_t code_addr, bool is_public);

#endif //_INCLUDE_SP_DECOMP_CODE_ANALYZER_H_
