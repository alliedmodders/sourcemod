#include "ASTVName.h"

using namespace Knight;

ASTVName::ASTVName(Token_t ident) : m_Ident(ident)
{
}

void ASTVName::Accept(IASTVisitor *visit, void *data)
{
	visit->Visit(this, data);
}

void ASTVName::AddAccessor(ASTBaseNode *pNode)
{
	m_Accessors.push_back(pNode);
}
