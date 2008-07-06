#ifndef _INCLUDE_AST_CONST_LITERAL_H_
#define _INCLUDE_AST_CONST_LITERAL_H_

#include "ASTBaseNode.h"
#include "Lexer.h"

namespace Knight
{
	class ASTConstLiteral : public ASTBaseNode
	{
	public:
		ASTConstLiteral(Token_t tok);
	public: /* ASTBaseNode */
		void Accept(IASTVisitor *visit, void *data);
	private:
		Token_t m_Token;
	};
}

#endif //_INCLUDE_AST_CONST_LITERAL_H_
