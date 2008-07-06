#include "ASTEnumeration.h"

using namespace Knight;

ASTEnumeration::ASTEnumeration(Token_t *name)
{
	if (name == NULL)
	{
		m_Ident.type = KTOK_EOF;
	}
	else
	{
		m_Ident = *name;
	}
}

void ASTEnumeration::AddEntry(const Token_t & tok)
{
	m_Entries.push_back(tok);
}

void ASTEnumeration::Accept(IASTVisitor *visit, void *data)
{
	visit->Visit(this, data);
}
