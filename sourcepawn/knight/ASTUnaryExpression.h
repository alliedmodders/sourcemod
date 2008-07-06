#ifndef _INCLUDE_AST_UNARY_EXPRESSION_H_
#define _INCLUDE_AST_UNARY_EXPRESSION_H_

#include "Lexer.h"
#include "ASTBaseNode.h"

namespace Knight
{
	class ASTUnaryExpression : public ASTBaseNode
	{
	public:
		ASTUnaryExpression(Token_t op, ASTBaseNode *expr);
	public: /* ASTBaseNode */
		void Accept(IASTVisitor *visit, void *data);
	private:
		Token_t m_Op;
		ASTBaseNode *m_Expression;
	};
}

#endif //_INCLUDE_AST_UNARY_EXPRESSION_H_
