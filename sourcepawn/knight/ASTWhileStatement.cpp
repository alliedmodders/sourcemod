#include "ASTWhileStatement.h"

using namespace Knight;

ASTWhileStatement::ASTWhileStatement(ASTBaseNode *pExpr, ASTBaseNode *pStmt, WhileLoopType type)
: m_pExpression(pExpr), m_pStatement(pStmt), m_Type(type)
{
}

void ASTWhileStatement::Accept(IASTVisitor *visit, void *data)
{
	visit->Visit(this, data);
}
