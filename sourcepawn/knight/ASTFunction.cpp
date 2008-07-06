#include "ASTFunction.h"

using namespace Knight;

ASTFunction::ASTFunction(ASTPrototype *proto, ASTBaseNode *body) :
	m_pProto(proto), m_pBody(body)
{
	
}

void ASTFunction::Accept(IASTVisitor *visit, void *data)
{
	visit->Visit(this, data);
}
