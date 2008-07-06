#include "ASTIfStatement.h"

using namespace Knight;

ASTIfStatement::ASTIfStatement()
{
	m_Default.pExpr = NULL;
	m_Default.pStmt = NULL;
}

void ASTIfStatement::Accept(IASTVisitor *visit, void *data)
{
	visit->Visit(this, data);
}

void ASTIfStatement::AddBranch(ASTBaseNode *expr, ASTBaseNode *stmt)
{
	IfBranch_t branch;

	branch.pExpr = expr;
	branch.pStmt = stmt;

	m_Branches.push_back(branch);
}

void ASTIfStatement::SetFalseBranch(ASTBaseNode *expr, ASTBaseNode *stmt)
{
	m_Default.pExpr = expr;
	m_Default.pStmt = stmt;
}
