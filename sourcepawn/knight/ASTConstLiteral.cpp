#include "ASTConstLiteral.h"

using namespace Knight;

ASTConstLiteral::ASTConstLiteral(Token_t tok)
{
	m_Token = tok;
}

void ASTConstLiteral::Accept(IASTVisitor *visit, void *data)
{
	visit->Visit(this, data);
}
