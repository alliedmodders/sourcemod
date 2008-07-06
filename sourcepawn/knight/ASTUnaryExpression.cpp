#include "ASTUnaryExpression.h"

using namespace Knight;

ASTUnaryExpression::ASTUnaryExpression(Token_t op, ASTBaseNode *expr)
{
	m_Op = op;
	m_Expression = expr;
}

void ASTUnaryExpression::Accept(IASTVisitor *visit, void *data)
{
	visit->Visit(this, data);
}
