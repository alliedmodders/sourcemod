#ifndef _INCLUDE_KNIGHT_AST_VNAME_H_
#define _INCLUDE_KNIGHT_AST_VNAME_H_

#include "ASTBaseNode.h"
#include "Lexer.h"
#include <sh_vector.h>

namespace Knight
{
	class ASTVName : public ASTBaseNode
	{
	public:
		ASTVName(Token_t ident);
	public: /* ASTBaseNode */
		void Accept(IASTVisitor *visit, void *data);
	public:
		void AddAccessor(ASTBaseNode *pNode);
	private:
		Token_t m_Ident;
		SourceHook::CVector<ASTBaseNode *> m_Accessors;
	};
}

#endif //_INCLUDE_KNIGHT_AST_VNAME_H_
