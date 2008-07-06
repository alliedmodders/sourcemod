#include "ASTProgram.h"

using namespace Knight;

ASTProgram::ASTProgram()
{
}

void ASTProgram::Accept(IASTVisitor *visit, void *data)
{
	visit->Visit(this, data);
}

void ASTProgram::AddStatement(ASTBaseNode *node)
{
	m_Statements.push_back(node);
}
