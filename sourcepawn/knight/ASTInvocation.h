#ifndef _INCLUDE_KNIGHT_AST_INVOCATION_H_
#define _INCLUDE_KNIGHT_AST_INVOCATION_H_

#include "ASTBaseNode.h"
#include "Lexer.h"
#include <sh_vector.h>

namespace Knight
{
	class ASTInvocation : public ASTBaseNode
	{
	public:
		ASTInvocation(Token_t ident);
	public: /* ASTBaseNode */
		void Accept(IASTVisitor *visit, void *data);
	public:
		void AddParameter(ASTBaseNode *expression);
	private:
		Token_t m_Ident;
		SourceHook::CVector<ASTBaseNode *> m_Parameters;
	};
}

#endif //_INCLUDE_KNIGHT_AST_INVOCATION_H_
