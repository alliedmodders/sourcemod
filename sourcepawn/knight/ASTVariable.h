#ifndef _INCLUDE_KNIGHT_AST_VARIABLE_H_
#define _INCLUDE_KNIGHT_AST_VARIABLE_H_

#include "ASTBaseNode.h"
#include "Lexer.h"

namespace Knight
{
	enum VarDeclType
	{
		VarDecl_Global,
		VarDecl_Local,
	};

	class ASTVariable : public ASTBaseNode
	{
	public:
		ASTVariable(const Token_t & ident, const Token_t & tag, VarDeclType decl);
	public: /* ASTBaseNode */
		void Accept(IASTVisitor *visit, void *data);
	private:
		Token_t m_Ident;
		Token_t m_Tag;
		VarDeclType m_VarDeclType;
	};
}

#endif //_INCLUDE_KNIGHT_AST_VARIABLE_H_
