#ifndef _INCLUDE_KNIGHT_AST_PROTOTYPE_H_
#define _INCLUDE_KNIGHT_AST_PROTOTYPE_H_

#include "Parser.h"
#include "ASTBaseNode.h"
#include <sh_vector.h>

namespace Knight
{
	struct proto_arg_t
	{
		bool is_ref;
		bool is_const;
		bool has_tag;
		Token_t tag;
		Token_t ident;
		int dims[KNIGHT_MAX_DIMS];
		unsigned int dim_count;
	};

	class ASTPrototype : public ASTBaseNode
	{
	public:
		ASTPrototype(Token_t ident, Token_t *tag);
	public: /* ASTBaseNode */
		void Accept(IASTVisitor *visit, void *data);
	public:
		void AddArgument(const proto_arg_t & arg);
		void SetFuncType(TokenType t);
	private:
		Token_t m_Ident;
		Token_t m_Tag;
		SourceHook::CVector<proto_arg_t> m_Arguments;
		bool m_bHasReturnTag;
		TokenType m_FuncType;
	};
}

#endif //_INCLUDE_KNIGHT_AST_PROTOTYPE_H_
