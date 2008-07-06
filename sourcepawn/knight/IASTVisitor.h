#ifndef _INCLUDE_KNIGHT_IASTVISITOR_H_
#define _INCLUDE_KNIGHT_IASTVISITOR_H_

#include "DefineASTs.h"

namespace Knight
{
	class IASTVisitor
	{
	public:
		virtual void Visit(ASTPrototype *node, void *data) = 0;
		virtual void Visit(ASTEnumeration *node, void *data) = 0;
		virtual void Visit(ASTUnaryExpression *node, void *data) = 0;
		virtual void Visit(ASTConstLiteral *node, void *data) = 0;
		virtual void Visit(ASTBinaryExpression *node, void *data) = 0;
		virtual void Visit(ASTFunction *node, void *data) = 0;
		virtual void Visit(ASTCompoundStmt *node, void *data) = 0;
		virtual void Visit(ASTVariable *node, void *data) = 0;
		virtual void Visit(ASTProgram *node, void *data) = 0;
		virtual void Visit(ASTInvocation *node, void *data) = 0;
		virtual void Visit(ASTVName *node, void *data) = 0;
		virtual void Visit(ASTIfStatement *node, void *data) = 0;
		virtual void Visit(ASTWhileStatement *node, void *data) = 0;
		virtual void Visit(ASTForStatement *node, void *data) = 0;
	};
}

#endif //_INCLUDE_KNIGHT_IASTVISITOR_H_
