#include "ASTForStatement.h"

using namespace Knight;

ASTForStatement::ASTForStatement(ASTBaseNode *pStart, ASTBaseNode *pCond, ASTBaseNode *pIter, ASTBaseNode *pStmt)
:m_pStart(pStart), m_pCondition(pCond), m_pIterator(pIter), m_pStatement(pStmt)
{
}

void ASTForStatement::Accept(IASTVisitor *visit, void *data)
{
	visit->Visit(this, data);
}
