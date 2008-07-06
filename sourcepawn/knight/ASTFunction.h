#ifndef _INCLUDE_KNIGHT_AST_FUNCTION_H_
#define _INCLUDE_KNIGHT_AST_FUNCTION_H_

#include "ASTBaseNode.h"

namespace Knight
{
	class ASTFunction : public ASTBaseNode
	{
	public:
		ASTFunction(ASTPrototype *proto, ASTBaseNode *body);
	public: /* ASTBaseNode */
		void Accept(IASTVisitor *visit, void *data);
	private:
		ASTPrototype *m_pProto;
		ASTBaseNode *m_pBody;
	};
}

#endif //_INCLUDE_KNIGHT_AST_FUNCTION_H_
