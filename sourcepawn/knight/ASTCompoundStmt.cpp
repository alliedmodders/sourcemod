#include "ASTCompoundStmt.h"

using namespace Knight;

void ASTCompoundStmt::AddStatement(ASTBaseNode *stmt)
{
	m_pStatements.push_back(stmt);
}

void ASTCompoundStmt::Accept(IASTVisitor *visit, void *data)
{
	visit->Visit(this, data);
}
