#ifndef _INCLUDE_KNIGHT_AST_COMPOUND_STATEMENT_H_
#define _INCLUDE_KNIGHT_AST_COMPOUND_STATEMENT_H_

#include "ASTBaseNode.h"
#include <sh_vector.h>

namespace Knight
{
	class ASTCompoundStmt : public ASTBaseNode
	{
	public: /*ASTBaseNode */
		void Accept(IASTVisitor *visit, void *data);
	public:
		void AddStatement(ASTBaseNode *stmt);
	private:
		SourceHook::CVector<ASTBaseNode *> m_pStatements;
	};
}

#endif //_INCLUDE_KNIGHT_AST_COMPOUND_STATEMENT_H_
