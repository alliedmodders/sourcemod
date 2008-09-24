#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "decompiler.h"
#include "code_analyzer.h"
#include "platform_util.h"

struct Block;

#define PCM_BRANCH_TARGET		(1<<0)
#define PCM_OPCODE				(1<<1)
#define PCM_HAS_BLOCK			(1<<2)
#define PCM_NUM_BITS			3

#define NEXT_OP(cip, op)		cip += (dc->opdef[op].params + 1) * sizeof(cell_t)

struct Edge
{
	Edge *next;
	Block *target;
	uint32_t ip;
};

struct Block
{
	cell_t ip_start;
	cell_t ip_end;
	Edge *edge_list;
	Edge *final_edge;
	Block *next_in_table;
};

struct ControlFlowGraph
{
	int err;
	unsigned int max_blocks;
	unsigned int max_edges;
	unsigned int used_blocks;
	unsigned int used_edges;
	Block *blocks;
	Edge *edges;
	Block *entry;
	FunctionInfo *func;
};

static FunctionInfo *sp_InferFuncPrototype(sp_decomp_t *dc, uint32_t code_addr, bool is_public)
{
	sp_tag_t *pTag;
	sp_file_t *plugin;
	FunctionInfo *info;
	sp_fdbg_symbol_t *sym;

	plugin = dc->plugin;
	info = new FunctionInfo;
	memset(info, 0, sizeof(FunctionInfo));

	info->code_addr = code_addr;
	
	if ((sym = Sp_FindFunctionSym(plugin, code_addr)) != NULL)
	{
		Sp_Format(info->name, sizeof(info->name), "%s", plugin->debug.stringbase + sym->name);
		if (sym->tagid != 0)
		{
			info->tag = Sp_FindTag(plugin, sym->tagid);
		}
	}
	else
	{
		Sp_Format(info->name, sizeof(info->name), "function_%x", code_addr);
		for (uint32_t i = 0; i < plugin->num_publics; i++)
		{
			if (plugin->publics[i].code_offs == code_addr)
			{
				Sp_Format(info->name,
					sizeof(info->name),
					"%s",
					plugin->publics[i].name);
				break;
			}
		}
	}
	info->sym = sym;
	info->is_public = is_public;
	

	/* Search for all locals */
	{
		uint32_t i;
		uint8_t *cursor;

		cursor = (uint8_t *)plugin->debug.symbols;
		for (i = 0; i < plugin->debug.syms_num; i++)
		{
			sym = (sp_fdbg_symbol_t *)cursor;

			if (sym->ident == SP_SYM_VARIABLE
				|| sym->ident == SP_SYM_REFARRAY
				|| sym->ident == SP_SYM_REFERENCE
				|| sym->ident == SP_SYM_ARRAY)
			{
				if (code_addr >= sym->codestart
					&& code_addr < sym->codeend
					&& sym->addr >= 0xC
					&& (sym->addr & 0x3) == 0
					&& ((sym->addr - 0xC) >> 2) < SP_MAX_EXEC_PARAMS
					&& sym->vclass == 1)
				{
					unsigned int num;

					num = (sym->addr - 0xC) >> 2;
					info->known_args[num].sym = sym;
					if (num + 1 > info->num_known_args)
					{
						info->num_known_args = num + 1;
					}
				}
			}

			cursor += sizeof(sp_fdbg_symbol_t);
			if (sym->dimcount > 0)
			{
				cursor += sizeof(sp_fdbg_arraydim_t) * sym->dimcount;
			}
		}
	}

	/* Map all arguments into place. */
	for (unsigned int i = 0; i < info->num_known_args; i++)
	{
		if ((sym = info->known_args[i].sym) == NULL)
		{
			continue;
		}
		if (sym->tagid != 0 && (pTag = Sp_FindTag(plugin, sym->tagid)) != NULL)
		{
			info->known_args[i].tag = pTag;
		}
		info->known_args[i].name = plugin->debug.stringbase + sym->name;
		
		if (sym->dimcount > 0)
		{
			info->known_args[i].dims = 
				(sp_fdbg_arraydim_t *)((char *)sym + sizeof(sp_fdbg_symbol_t));
		}
	}

	return info;
}

static FunctionInfo *sp_GetFuncPrototype(sp_decomp_t *dc,
										 uint32_t code_addr,
										 bool known,
										 bool is_public = false);
static FunctionInfo *sp_GetFuncPrototype(sp_decomp_t *dc,
										 uint32_t code_addr,
										 bool known,
										 bool is_public)
{
	FunctionInfo *func;

	func = dc->funcs;
	while (func != NULL)
	{
		if (func->code_addr == code_addr)
		{
			return func;
		}
		func = func->next;
	}

	/* See if this is public. */
	if (!known)
	{
		is_public = false;
		for (unsigned int i = 0; i < dc->plugin->num_publics; i++)
		{
			if (dc->plugin->publics[i].code_offs == code_addr)
			{
				is_public = true;
				break;
			}
		}
	}

	func = sp_InferFuncPrototype(dc, code_addr, is_public);
	func->next = dc->funcs;
	dc->funcs = func;

	return func;
}

static void sp_CacheLocals(sp_decomp_t *dc, FunctionInfo *info)
{
	sp_file_t *plugin;
	sp_fdbg_symbol_t *sym;

	plugin = dc->plugin;

	/* Search for all locals */
	{
		uint32_t i;
		uint8_t *cursor;

		cursor = (uint8_t *)plugin->debug.symbols;
		for (i = 0; i < plugin->debug.syms_num; i++)
		{
			sym = (sp_fdbg_symbol_t *)cursor;

			if ((sym->codestart >= info->code_addr && sym->codestart < info->code_end)
				&& (sym->codeend <= info->code_end && sym->codeend >= info->code_addr)
				&& sym->vclass == 1)
			{
				info->num_known_vars++;
			}

			cursor += sizeof(sp_fdbg_symbol_t);
			if (sym->dimcount > 0)
			{
				cursor += sizeof(sp_fdbg_arraydim_t) * sym->dimcount;
			}
		}
	}

	/* Map all locals into place. */
	if (info->num_known_vars)
	{
		uint32_t i, num;
		uint8_t *cursor;

		num = 0;
		info->known_vars = new FuncVar[info->num_known_vars];
		memset(info->known_vars, 0, sizeof(FuncVar) * info->num_known_vars);

		cursor = (uint8_t *)plugin->debug.symbols;
		for (i = 0; i < plugin->debug.syms_num; i++)
		{
			sym = (sp_fdbg_symbol_t *)cursor;

			if (sym->ident == SP_SYM_VARIABLE
				|| sym->ident == SP_SYM_REFARRAY
				|| sym->ident == SP_SYM_REFERENCE
				|| sym->ident == SP_SYM_ARRAY)
			{
				/* Detect all function variables. */
				if ((sym->codestart >= info->code_addr && sym->codestart < info->code_end)
					&& (sym->codeend <= info->code_end && sym->codeend >= info->code_addr)
					&& sym->vclass == 1)
				{
					assert(num < info->num_known_vars);
					info->known_vars[num].sym = sym;
					info->known_vars[num].name = plugin->debug.stringbase + sym->name;
					info->known_vars[num].tag = 
						(sym->tagid == 0) ? NULL : Sp_FindTag(plugin, sym->tagid);
					info->known_vars[num].dims = 
						(sym->dimcount == 0)
						? NULL
						: (sp_fdbg_arraydim_t *)(cursor + sizeof(sp_fdbg_symbol_t));
					num++;
				}
			}

			cursor += sizeof(sp_fdbg_symbol_t);
			if (sym->dimcount > 0)
			{
				cursor += sizeof(sp_fdbg_arraydim_t) * sym->dimcount;
			}
		}

		assert(num == info->num_known_vars);
	}
}

static void sp_PrintFunctionPrototype(PrintBuffer *printer, sp_decomp_t *dc, FunctionInfo *func)
{
	FuncVar *var;
	
	if (func->is_public)
	{
		printer->Append("public ");
	}
	if (func->tag != NULL)
	{
		printer->Append("%s:", func->tag->name);
	}
	printer->Append("%s(", func->name);

	for (unsigned int i = 0; i < func->num_known_args; i++)
	{
		var = &func->known_args[i];
		if (var->sym == NULL)
		{
			printer->Append("unknown_arg_%d%s, ", i, i == func->num_known_args - 1 ? "" : ",");
			continue;
		}
		if (var->sym->ident == SP_SYM_REFERENCE)
		{
			printer->Append("&");
		}
		if (var->tag != NULL)
		{
			printer->Append("%s:", var->tag->name);
		}
		printer->Append("%s", var->name);

		if (var->sym->dimcount > 0)
		{
			sp_fdbg_arraydim_t *dims = 
				(sp_fdbg_arraydim_t *)((char *)var->sym + sizeof(sp_fdbg_symbol_t));

			for (uint16_t j = 0; j < var->sym->dimcount; j++)
			{
				if (dims[j].size == 0)
				{
					printer->Append("[]");
				}
				else
				{
					printer->Append("[%d]", dims[j].size);
				}
			}
		}

		if (i != func->num_known_args - 1)
		{
			printer->Append(", ");
		}
	}

	printer->Append(")");
}

void sp_DebugExprTree(PrintBuffer *buffer, BaseNode *node, bool is_stmt=false)
{
	switch (node->op)
	{
	case OP_CALL:
		{
			CallNode *call = (CallNode *)node;

			buffer->Append("%s(", call->func->name);
			for (unsigned int i = 0; i < call->argc; i++)
			{
				sp_DebugExprTree(buffer, call->args[i], false);
			}
			buffer->Append(")");
			break;
		}
	case _OP_VAR:
		{
			VarNode *var_node = (VarNode *)node;
			buffer->Append("%s", var_node->var->name);
			break;
		}
	case _OP_NEW:
		{
			DeclNode *d_node = (DeclNode *)node;

			if (d_node->expr == NULL)
			{
				buffer->Append("decl ");
				sp_DebugExprTree(buffer, d_node->var, true);
			}
			else
			{
				buffer->Append("new ");
				sp_DebugExprTree(buffer, d_node->var, true);
				buffer->Append(" = ");
				sp_DebugExprTree(buffer, d_node->expr, true);
			}

			break;
		}
	case _OP_STMT:
	case OP_RETN:
		{
			StmtNode *u_node = (StmtNode *)node;

			while (u_node != NULL)
			{
				buffer->Append("\t");
				if (u_node->op == OP_RETN)
				{
					buffer->Append("return ");
				}
				sp_DebugExprTree(buffer, u_node->node, true);
				buffer->Append(";\n");
				u_node = u_node->next;
			}
			break;
		}
	case _OP_CONST:
		{
			ConstNode *c_node = (ConstNode *)node;

			buffer->Append("%d", c_node->val);

			break;
		}
	case OP_ADD:
	case OP_ADD_C:
	case OP_SMUL:
	case OP_SMUL_C:
	case OP_SDIV:
	case OP_SUB_ALT:
	case _OP_STORE:
	case _OP_MODULO:
		{
			BinOpNode *bin_node = (BinOpNode *)node;

			if (!is_stmt)
			{
				buffer->Append("(");
			}
			sp_DebugExprTree(buffer, bin_node->left);

			switch (node->op)
			{
			case OP_ADD:
			case OP_ADD_C:
				{
					buffer->Append(" + ");
					break;
				}
			case OP_SMUL:
			case OP_SMUL_C:
				{
					buffer->Append(" * ");
					break;
				}
			case _OP_STORE:
				{
					buffer->Append(" = ");
					break;
				}
			case OP_SUB_ALT:
				{
					buffer->Append(" - ");
					break;
				}
			case OP_SDIV:
				{
					buffer->Append(" / ");
					break;
				}
			case _OP_MODULO:
				{
					buffer->Append(" % ");
					break;
				}
			}

			sp_DebugExprTree(buffer, bin_node->right, node->op == _OP_STORE);
			if (!is_stmt)
			{
				buffer->Append(")");
			}

			break;
		}
	}
}

void sp_DebugPrint(FunctionInfo *info, sp_decomp_t *dc, BaseNode *node)
{
	PrintBuffer buffer;

	sp_PrintFunctionPrototype(&buffer, dc, info);
	buffer.Append("\n{\n");
	sp_DebugExprTree(&buffer, node, true);
	buffer.Append("}\n");
	buffer.Dump(stdout);
}

#define PCODE_VAL(pcode, offs)			*(cell_t *)((char *)(pcode)+(offs))
#define PCODE_PARAM(pcode, offs, num)	*(cell_t *)((char *)(pcode)+(offs)+((num)*sizeof(cell_t)))

FuncVar *sp_FindGraphVar(FunctionInfo *func, ucell_t cip, cell_t sp, cell_t stack)
{
	if (stack >= 0xC)
	{
		if ((stack & 0x3) != 0)
		{
			return NULL;
		}
		stack -= 0xC;
		stack >>= 2;
		if (stack >= SP_MAX_EXEC_PARAMS)
		{
			return NULL;
		}
		if (func->known_args[stack].sym == NULL)
		{
			return NULL;
		}
		return &func->known_args[stack];
	}
	else if (stack < 0)
	{
		FuncVar *var;

		if (stack < sp)
		{
			return NULL;
		}

		for (unsigned int i = 0; i < func->num_known_vars; i++)
		{
			var = &func->known_vars[i];

			if (cip >= var->sym->codestart
				&& cip <= var->sym->codeend
				&& stack == var->sym->addr)
			{
				return var;
			}
		}

		return NULL;
	}
	else
	{
		return NULL;
	}
}

#define CHECK_VALUE(v)							\
	assert(v != NULL);							\
	if (v == NULL) {							\
		err = SP_ERROR_INVALID_INSTRUCTION;		\
		goto return_error;						\
	}

void *operator new(size_t size, ControlFlowGraph *graph)
{
	return malloc(size);
}

void *operator new[](size_t size, ControlFlowGraph *graph)
{
	return malloc(size);
}

void operator delete(void *mem, ControlFlowGraph *graph)
{
	free(mem);
}

void operator delete[](void *mem, ControlFlowGraph *graph)
{
	free(mem);
}

#define MAX_STACK_ENTRIES		256

struct StackEntry
{
	StackEntry()
	{
	}
	StackEntry(cell_t d, BaseNode *e) : depth(d), expr(e)
	{
	}
	cell_t depth;
	BaseNode *expr;
};

int sp_AnalyzeGraph(sp_decomp_t *dc, ControlFlowGraph *graph)
{
	int err;
	cell_t sp;
	Block *block;
	cell_t *pcode;
	cell_t cip, cip_end;
	StmtNode *root, *rtemp;
	StmtNode *root_tail;
	BaseNode *pri, *alt;
	FunctionInfo *func, *tfunc;
	cell_t p1, p2, op;
	FuncVar *v1, *v2;
	unsigned int stack_entries = 0;
	StackEntry eval_stack[MAX_STACK_ENTRIES];

	/* Init everything.
	 * Stack pointer starts off at 0.
	 */
	sp = 0;
	pri = NULL;
	alt = NULL;
	root = NULL;
	func = graph->func;
	block = graph->entry;
	pcode = (cell_t *)dc->plugin->pcode;

	cip = block->ip_start;
	cip_end = block->ip_end;

	root_tail = root = new (graph) StmtNode(_OP_STMT, NULL, NULL);

	while (cip < cip_end)
	{
		op = PCODE_VAL(pcode, cip);

		switch (op)
		{
		case OP_PROC:
		case OP_BREAK:
			{
				break;
			}
		case OP_CALL:
			{
				p1 = PCODE_PARAM(pcode, cip, 1);
				tfunc = sp_GetFuncPrototype(dc, p1, false);
				CHECK_VALUE(tfunc);

				/* Sanity checks, :TODO: runtime checks later. */
				assert(stack_entries >= 1);

				ConstNode *cnode;
				cnode = (ConstNode *)eval_stack[stack_entries-1].expr;
				stack_entries--;

				/* Sanity checks, :TODO: runtime checks later. */
				assert(cnode->op == _OP_CONST);
				assert(cnode->val >= 0);
				assert(cnode->val == tfunc->num_known_args);
				assert(unsigned int(cnode->val) <= stack_entries);
				
				cell_t argc = cnode->val;
				BaseNode **nodes = new (graph) BaseNode *[cnode->val];
				for (cell_t argno = 0; argno < cnode->val; argno++, stack_entries--)
				{
					nodes[argno] = eval_stack[stack_entries-1].expr;
				}

				sp += (cnode->val + 1) * sizeof(cell_t);
				assert(sp <= 0);

				pri = new (graph) CallNode(tfunc, nodes, cnode->val);
				
				break;
			}
		case OP_CONST_PRI:
			{
				p1 = PCODE_PARAM(pcode, cip, 1);
				pri = new (graph) ConstNode(p1);
				break;
			}
		case OP_PUSH_C:
			{
				p1 = PCODE_PARAM(pcode, cip, 1);

				/* Adjust stack pointer. */
				sp -= sizeof(cell_t);

				if ((v1 = sp_FindGraphVar(func, cip, sp, sp)) == NULL)
				{
					/* Add a new stack entry. */
					assert(stack_entries < MAX_STACK_ENTRIES);
					eval_stack[stack_entries++] = StackEntry(sp, new ConstNode(p1));
				}
				else
				{
					assert(v1->new_stmt == NULL);
					if (v1->new_stmt == NULL)
					{
						v1->new_stmt = new (graph) DeclNode(
							new (graph) VarNode(v1),
							new (graph) ConstNode(p1));
						rtemp = new (graph) StmtNode(
							_OP_STMT,
							v1->new_stmt,
							NULL);
						root_tail->next = rtemp;
						root_tail = rtemp;
					}
				}
				break;
			}
		case OP_PUSH_S:
			{
				p1 = PCODE_PARAM(pcode, cip, 1);
				v1 = sp_FindGraphVar(func, cip, sp, p1);
				CHECK_VALUE(v1);

				/* Adjust stack pointer. */
				sp -= sizeof(cell_t);

				/* Right now, don't allow aliasing or overflows. */
				assert(sp_FindGraphVar(func, cip, sp, sp) == NULL);
				assert(stack_entries < MAX_STACK_ENTRIES);

				/* Add a new stack entry. */
				eval_stack[stack_entries++] = StackEntry(sp, new (graph) VarNode(v1));
				break;
			}
		case OP_PUSH_PRI:
			{
				CHECK_VALUE(pri);

				/* Adjust stack pointer. */
				sp -= sizeof(cell_t);
				
				/* Right now, don't allow aliasing or overflows. */
				assert(sp_FindGraphVar(func, cip, sp, sp) == NULL);
				assert(stack_entries < MAX_STACK_ENTRIES);

				/* Add a new stack entry. */
				eval_stack[stack_entries++] = StackEntry(sp, pri);
				break;
			}
		case OP_POP_ALT:
			{
				assert(stack_entries > 0);
				assert(eval_stack[stack_entries - 1].depth == sp);

				alt = eval_stack[--stack_entries].expr;
				sp += sizeof(cell_t);
				assert(sp <= 0);
				break;
			}
		case OP_STACK:
			{
				p1 = PCODE_PARAM(pcode, cip, 1);
				sp += p1;
				assert(sp <= 0);
				if (p1 < 0)
				{
					/* Find if we should track a new variable declaration. */
					v1 = sp_FindGraphVar(func, cip, sp, sp);
					CHECK_VALUE(v1);
					assert(v1->new_stmt == NULL);
					if (v1->new_stmt == NULL)
					{
						v1->new_stmt = new (graph) DeclNode(
							new (graph) VarNode(v1),
							NULL);
						rtemp = new (graph) StmtNode(
							_OP_STMT,
							v1->new_stmt,
							NULL);
						root_tail->next = rtemp;
						root_tail = rtemp;
					}
					assert(abs(p1) >> 2 == 1);
				}
				else
				{
					/* Erase anything left over on the evaluation stack. */
					while (stack_entries > 0
						   && eval_stack[stack_entries - 1].depth < sp)
					{
						assert(0);
						stack_entries--;
					}
				}
				break;
			}
		case OP_ZERO_PRI:
			{
				pri = new (graph) ConstNode(0);
				break;
			}
		case OP_MOVE_ALT:
			{
				CHECK_VALUE(pri);
				alt = pri;
				break;
			}
		case OP_LOAD_S_PRI:
			{
				p1 = PCODE_PARAM(pcode, cip, 1);
				v1 = sp_FindGraphVar(func, cip, sp, p1);
				CHECK_VALUE(v1);
				pri = new (graph) VarNode(v1);
				break;
			}
		case OP_LOAD_S_ALT:
			{
				p1 = PCODE_PARAM(pcode, cip, 1);
				v1 = sp_FindGraphVar(func, cip, sp, p1);
				CHECK_VALUE(v1);
				alt = new (graph) VarNode(v1);
				break;
			}
		case OP_STOR_S_PRI:
			{
				p1 = PCODE_PARAM(pcode, cip, 1);
				v1 = sp_FindGraphVar(func, cip, sp, p1);
				CHECK_VALUE(v1);
				CHECK_VALUE(pri);
				rtemp = new (graph) StmtNode(
					_OP_STMT,
					new (graph) BinOpNode(_OP_STORE, new (graph) VarNode(v1), pri),
					NULL);
				root_tail->next = rtemp;
				root_tail = rtemp;

				/** 
				 * We create a varnode to simulate a load here.
				 * This helps accidental duplication of the expression tree.
				 */
				pri = new (graph) VarNode(v1);	
				break;
			}
		case OP_LOAD_S_BOTH:
			{
				p1 = PCODE_PARAM(pcode, cip, 1);
				p2 = PCODE_PARAM(pcode, cip, 2);
				v1 = sp_FindGraphVar(func, cip, sp, p1);
				v2 = sp_FindGraphVar(func, cip, sp, p2);
				CHECK_VALUE(v1);
				CHECK_VALUE(v2);
				pri = new (graph) VarNode(v1);
				alt = new (graph) VarNode(v2);
				break;
			}
		case OP_ADD:
		case OP_SMUL:
			{
				CHECK_VALUE(pri);
				CHECK_VALUE(alt);
				pri = new (graph) BinOpNode(op, pri, alt);
				break;
			}
		case OP_SUB_ALT:
			{
				CHECK_VALUE(pri);
				CHECK_VALUE(alt);
				pri = new (graph) BinOpNode(op, alt, pri);
				break;
			}
		case OP_SDIV:
			{
				CHECK_VALUE(pri);
				CHECK_VALUE(alt);
				pri = new (graph) BinOpNode(OP_SDIV, pri, alt);
				alt = new (graph) BinOpNode(_OP_MODULO, pri, alt);
				break;
			}
		case OP_SDIV_ALT:
			{
				CHECK_VALUE(pri);
				CHECK_VALUE(alt);
				pri = new (graph) BinOpNode(OP_SDIV, alt, pri);
				alt = new (graph) BinOpNode(_OP_MODULO, alt, pri);
				break;
			}
		case OP_ADD_C:
		case OP_SMUL_C:
			{
				p1 = PCODE_PARAM(pcode, cip, 1);
				CHECK_VALUE(pri);
				pri = new (graph) BinOpNode(op, pri, new (graph) ConstNode(p1));
				break;
			}
		case OP_RETN:
			{
				CHECK_VALUE(pri);
				rtemp = new (graph) StmtNode(op, pri, NULL);
				root_tail->next = rtemp;
				root_tail = rtemp;
				break;
			}
		default:
			{
				assert(0);
				break;
			}
		}

		NEXT_OP(cip, op);
	}

	sp_DebugPrint(graph->func, dc, root->next);

	return SP_ERROR_NONE;

return_error:
	return err;
}

#undef CHECK_FUNCVAR

inline Block *sp_BlockFromMap(sp_decomp_t *dc, ControlFlowGraph *graph, cell_t cip)
{
	cell_t val;

	assert(PCODE_VAL(dc->pcode_map, cip) & PCM_HAS_BLOCK);

	val = PCODE_VAL(dc->pcode_map, cip);
	val >>= PCM_NUM_BITS;

	assert(val >= 0 && val < (cell_t)graph->used_blocks);

	return &graph->blocks[val];
}

void sp_InsertBlock(sp_decomp_t *dc, unsigned int index, cell_t cip)
{
	cell_t val;

	val = PCODE_VAL(dc->pcode_map, cip);

	assert(!(val & PCM_HAS_BLOCK));
	assert(index < 0x20000000);

	val |= PCM_HAS_BLOCK;
	val |= index << 3;

	PCODE_VAL(dc->pcode_map, cip) = val;
}

static Block *sp_BuildBlocks(sp_decomp_t *dc, ControlFlowGraph *graph, cell_t cip)
{
	cell_t op;
	bool is_branch;
	cell_t target;
	Block *block;
	Edge *edge_tail;
	Block *outgoing;
	cell_t cip_end;
	cell_t *pcode;

	target = 0;
	cip_end = graph->func->code_end;
	pcode = (cell_t *)dc->plugin->pcode;

	if (PCODE_VAL(dc->pcode_map, cip) & PCM_HAS_BLOCK)
	{
		return sp_BlockFromMap(dc, graph, cip);
	}

	/* We can't add a new block if we hit our limit. */
	assert(graph->used_blocks < graph->max_blocks);
	if (graph->used_blocks >= graph->max_blocks)
	{
		return NULL;
	}

	edge_tail = NULL;

	/* Reserve a new block. */
	block = &graph->blocks[graph->used_blocks++];
	block->ip_start = cip;
	
	/* Enter us into the map. */
	sp_InsertBlock(dc, graph->used_blocks - 1, cip);

	/* Walk the code. */
	while (cip < cip_end)
	{
		/* Check whether this is the start of a new block. */
		if (cip != block->ip_start
			&& (PCODE_VAL(dc->pcode_map, cip) & PCM_BRANCH_TARGET))
		{
			block->ip_end = cip;
			if ((outgoing = sp_BuildBlocks(dc, graph, cip)) == NULL)
			{
				return NULL;
			}
			/* :TODO: add edge */
			return block;
		}

		is_branch = false;
		op = PCODE_VAL(pcode, cip);
		NEXT_OP(cip, op);

		if (is_branch)
		{
			/* Check that branch offsets go to valid instructions. */
			if (!(PCODE_VAL(dc->pcode_map, target) & PCM_OPCODE))
			{
				graph->err = SP_ERROR_INVALID_INSTRUCTION;
				return NULL;
			}

			/* Get our target block. */
			if ((outgoing = sp_BuildBlocks(dc, graph, target)) == NULL)
			{
				return NULL;
			}
			/* :TODO: add edge */
		}
		else if (op == OP_RETN /* :TODO: || op == OP_RET*/)
		{
			block->ip_end = cip;
			return block;
		}
	}

	graph->err = SP_ERROR_INVALID_INSTRUCTION;

	return NULL;
}


int Sp_DecompFunction(sp_decomp_t *dc, uint32_t code_addr, bool is_public)
{
	int err;
	cell_t op;
	ucell_t cip;
	cell_t *pcode;
	ucell_t cip_end;
	sp_file_t *plugin;
	FunctionInfo *func;
	ControlFlowGraph graph;
	
	func = NULL;
	plugin = dc->plugin;
	memset(&graph, 0, sizeof(graph));

	if (code_addr >= plugin->pcode_size)
	{
		err = SP_ERROR_INVALID_INSTRUCTION;
		goto return_error;
	}

	pcode = (cell_t *)plugin->pcode;

	if (PCODE_VAL(pcode, code_addr) != OP_PROC)
	{
		err = SP_ERROR_INVALID_INSTRUCTION;
		goto return_error;
	}

	cip = code_addr + sizeof(cell_t);

	/* See if we have an existing function stored. */
	func = sp_GetFuncPrototype(dc, code_addr, true, is_public);

	while (cip < plugin->pcode_size)
	{
		op = PCODE_VAL(pcode, cip);
		if (op >= OP_TOTAL || op < 0)
		{
			err = SP_ERROR_INVALID_INSTRUCTION;
		}
		if (dc->opdef[op].name == NULL)
		{
			err = SP_ERROR_INVALID_INSTRUCTION;
		}
		if (op == OP_PROC)
		{
			break;
		}
		NEXT_OP(cip, op);
	}
	cip_end = cip;

	func->code_end = cip_end;

	/* Now we can cache all local vars. */
	sp_CacheLocals(dc, func);

	/** 
	 * Make the entry point as the first block.
	 * It needs to be flagged as well so it doesn't get double counted.
	 */
	memset(&PCODE_VAL(dc->pcode_map, code_addr), 0, cip_end - code_addr);
		   
	graph.max_blocks = 1;
	graph.func = func;
	PCODE_VAL(dc->pcode_map, code_addr) |= PCM_BRANCH_TARGET;

	/* Walk the code. */
	{
		cell_t target;
		bool is_branch;

		target = 0;
		cip = code_addr;
		while (cip < cip_end)
		{
			/* Get opcode */
			op = PCODE_VAL(pcode, cip);
			/* Mark as not a branch */
			is_branch = false;
			/* Make sure we know this is a valid target */
			PCODE_VAL(dc->pcode_map, cip) |= PCM_OPCODE;

			if (is_branch)
			{
				/**
				 * Keep track of where branches are and how many blocks we need.
				 */
				if (!(PCODE_VAL(dc->pcode_map, target) & PCM_BRANCH_TARGET))
				{
					PCODE_VAL(dc->pcode_map, target) |= PCM_BRANCH_TARGET;
					graph.max_blocks++;

					/**
					 * If we're creating a new block, estimate that we need to create a new edge 
					 * as well.
					 */
					graph.max_edges++;
				}

                /**
                 * In this algorithm, jumps do not terminate a block unless they are unconditional.
                 * This reduces the number of blocks and edges.
                 *
                 * A property of this is that jumps are edges, so the number of edges is identical 
				 * to the number of jumps.
                 */
				graph.max_edges++;
			}

			NEXT_OP(cip, op);
		}
	}

	/* Initialize memory needed for the graph. */
	graph.blocks = new Block[graph.max_blocks];
	memset(graph.blocks, 0, sizeof(Block) * graph.max_blocks);
	
	if (graph.max_edges != 0)
	{
		graph.edges = new Edge[graph.max_edges];
		memset(graph.edges, 0, sizeof(Edge) * graph.max_edges);
	}

	if ((graph.entry = sp_BuildBlocks(dc, &graph, code_addr)) == NULL)
	{
		if ((err = graph.err) == SP_ERROR_NONE)
		{
			err = SP_ERROR_ABORTED;
		}
		goto return_error;
	}

	if ((err = sp_AnalyzeGraph(dc, &graph)) == NULL)
	{
		goto return_error;
	}

	return SP_ERROR_NONE;

return_error:
	return err;
}
