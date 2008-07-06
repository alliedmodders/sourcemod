#ifndef _INCLUDE_KNIGHT_AST_ENUMERATION_H_
#define _INCLUDE_KNIGHT_AST_ENUMERATION_H_

#include <sh_vector.h>
#include "ASTBaseNode.h"
#include "Lexer.h"

namespace Knight
{
	class ASTEnumeration : public ASTBaseNode
	{
	public:
		ASTEnumeration(Token_t *name);
	public: /* ASTBaseNode */
		void Accept(IASTVisitor *visit, void *data);
	public:
		void AddEntry(const Token_t & tok);
	private:
		Token_t m_Ident;
		SourceHook::CVector<Token_t> m_Entries;
	};
}

#endif //_INCLUDE_KNIGHT_AST_ENUMERATION_H_
