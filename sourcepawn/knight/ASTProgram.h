#ifndef _INCLUDE_KNIGHT_AST_PROGRAM_H_
#define _INCLUDE_KNIGHT_AST_PROGRAM_H_

#include "ASTBaseNode.h"
#include <sh_vector.h>

namespace Knight
{
	class ASTProgram : public ASTBaseNode
	{
	public:
		ASTProgram();
	public: /* ASTBaseNode */
		void Accept(IASTVisitor *visit, void *data);
	public:
		void AddStatement(ASTBaseNode *node);
	private:
		SourceHook::CVector<ASTBaseNode *> m_Statements;
	};
}

#endif //_INCLUDE_KNIGHT_AST_PROGRAM_H_
