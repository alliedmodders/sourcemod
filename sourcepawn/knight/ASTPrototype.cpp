#include "ASTPrototype.h"

using namespace Knight;

ASTPrototype::ASTPrototype(Token_t ident, Token_t *tag)
{
	m_Ident = ident;
	
	if (tag == NULL)
	{
		m_bHasReturnTag = false;
	}
	else
	{
		m_bHasReturnTag = true;
		m_Tag = *tag;
	}

	m_FuncType = KTOK_EOF;
}

void ASTPrototype::AddArgument(const proto_arg_t & arg)
{
	m_Arguments.push_back(arg);
}

void ASTPrototype::SetFuncType(TokenType t)
{
	m_FuncType = t;
}

void ASTPrototype::Accept(IASTVisitor *visitor, void *data)
{
	visitor->Visit(this, data);
}

