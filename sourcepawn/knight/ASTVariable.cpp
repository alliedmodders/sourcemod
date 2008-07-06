#include "ASTVariable.h"

using namespace Knight;

ASTVariable::ASTVariable(const Token_t & ident, const Token_t & tag, VarDeclType decl) : 
	m_Ident(ident), m_Tag(tag), m_VarDeclType(decl)
{
}

void ASTVariable::Accept(IASTVisitor *visit, void *data)
{
	visit->Visit(this, data);
}
