#include "ASTBinaryExpression.h"

using namespace Knight;

ASTBinaryExpression::ASTBinaryExpression(ASTBaseNode *lft, const Token_t & op, ASTBaseNode *rght)
{
	m_RightExpression = rght;
	m_LeftExpression = lft;
	m_Op = op;
}

void ASTBinaryExpression::Accept(IASTVisitor *visit, void *data)
{
	visit->Visit(this, data);
}
