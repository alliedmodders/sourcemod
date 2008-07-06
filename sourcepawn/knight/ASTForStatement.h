#ifndef _INCLUDE_KNIGHT_AST_FOR_STATEMENT_H_
#define _INCLUDE_KNIGHT_AST_FOR_STATEMENT_H_

#include "ASTBaseNode.h"

namespace Knight
{
	class ASTForStatement : public ASTBaseNode
	{
	public:
		ASTForStatement(ASTBaseNode *pStart,
			ASTBaseNode *pCond,
			ASTBaseNode *pIter,
			ASTBaseNode *pStmt);
	public: /* ASTBaseNode */
		void Accept(IASTVisitor *visit, void *data);
	private:
		ASTBaseNode *m_pStart;
		ASTBaseNode *m_pCondition;
		ASTBaseNode *m_pIterator;
		ASTBaseNode *m_pStatement;
	};
}

#endif //_INCLUDE_KNIGHT_AST_FOR_STATEMENT_H_
