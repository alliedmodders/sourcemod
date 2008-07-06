#ifndef _INCLUDE_KNIGHT_AST_IF_STATEMENT_H_
#define _INCLUDE_KNIGHT_AST_IF_STATEMENT_H_

#include "ASTBaseNode.h"
#include "Lexer.h"
#include <sh_vector.h>

namespace Knight
{
	struct IfBranch_t
	{
		ASTBaseNode *pExpr;
		ASTBaseNode *pStmt;
	};

	class ASTIfStatement : public ASTBaseNode
	{
	public:
		ASTIfStatement();
	public: /* ASTBaseNode */
		void Accept(IASTVisitor *visit, void *data);
	public:
		void AddBranch(ASTBaseNode *expr, ASTBaseNode *stmt);
		void SetFalseBranch(ASTBaseNode *expr, ASTBaseNode *stmt);
	private:
		SourceHook::CVector<IfBranch_t> m_Branches;
		IfBranch_t m_Default;
	};
}

#endif //_INCLUDE_KNIGHT_AST_IF_STATEMENT_H_
