#ifndef _INCLUDE_KNIGHT_ASTBASENODE_H_
#define _INCLUDE_KNIGHT_ASTBASENODE_H_

#include "IASTVisitor.h"

namespace Knight
{
	class ASTBaseNode
	{
	public:
		virtual void Accept(IASTVisitor *visit, void *data) =0;
	};
}

#endif //_INCLUDE_KNIGHT_ASTBASENODE_H_
