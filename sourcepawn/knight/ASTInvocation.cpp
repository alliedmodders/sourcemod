#include "ASTInvocation.h"

using namespace Knight;

ASTInvocation::ASTInvocation(Token_t ident) : m_Ident(ident)
{
}

void ASTInvocation::Accept(IASTVisitor *visit, void *data)
{
	visit->Visit(this, data);
}

void ASTInvocation::AddParameter(ASTBaseNode *expression)
{
	m_Parameters.push_back(expression);
}
