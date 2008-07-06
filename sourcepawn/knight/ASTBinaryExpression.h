#ifndef _INCLUDE_KNIGHT_AST_BINARY_H_
#define _INCLUDE_KNIGHT_AST_BINARY_H_

#include "ASTBaseNode.h"
#include "Lexer.h"

namespace Knight
{
	class ASTBinaryExpression : public ASTBaseNode
	{
	public:
		ASTBinaryExpression(ASTBaseNode *lft, const Token_t & op, ASTBaseNode *rght);
	public: /* ASTBaseNode */
		void Accept(IASTVisitor *visit, void *data);
	private:
		ASTBaseNode *m_LeftExpression;
		ASTBaseNode *m_RightExpression;
		Token_t m_Op;
	};
}

#endif //_INCLUDE_KNIGHT_AST_BINARY_H_
