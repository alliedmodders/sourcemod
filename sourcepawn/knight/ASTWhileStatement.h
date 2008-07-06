#ifndef _INCLUDE_KNIGHT_AST_WHILE_STATEMENT_H_
#define _INCLUDE_KNIGHT_AST_WHILE_STATEMENT_H_

#include "ASTBaseNode.h"

namespace Knight
{
	enum WhileLoopType
	{
		WhileLoop_While,
		WhileLoop_Do,
	};

	class ASTWhileStatement : public ASTBaseNode
	{
	public:
		ASTWhileStatement(ASTBaseNode *pExpr, ASTBaseNode *pStmt, WhileLoopType type);
	public: /* ASTBaseNode */
		void Accept(IASTVisitor *visit, void *data);
	private:
		ASTBaseNode *m_pExpression;
		ASTBaseNode *m_pStatement;
		WhileLoopType m_Type;
	};
}

#endif //_INCLUDE_KNIGHT_AST_WHILE_STATEMENT_H_
