#include <string.h>
#include <stdlib.h>
#include "Parser.h"
#include "IncludeASTs.h"
#include <sh_stack.h>

using namespace Knight;

Parser::Parser(Lexer *pLexer, ErrorManager *pErrors) 
	: m_bRequireSemicolon(false), m_pLexer(pLexer), m_pErrMan(pErrors)
{
	
}

Parser::~Parser()
{
}

ASTProgram *Parser::ParseProgram()
{
	Token_t tok;
	ASTProgram *prog;
	ASTBaseNode *pNode;

	prog = new ASTProgram();

	while (m_pLexer->NextToken(&tok) != KTOK_EOF)
	{
		m_pLexer->PushBack(tok);
		
		if ((pNode = ParseGlobalStatement()) == NULL)
		{
			Err_UnexpectedToken("<statement>");
			continue;
		}

		prog->AddStatement(pNode);
	}

	return prog;
}

ASTBaseNode *Parser::ParseGlobalStatement()
{
	Token_t tok;
	bool bRequireSemi;
	ASTBaseNode *pRetNode;

	bRequireSemi = true;
	m_pLexer->NextToken(&tok);

	if (tok.type == KTOK_NATIVE)
	{
		ASTPrototype *proto;

		proto = ParsePrototype();
		proto->SetFuncType(KTOK_NATIVE);
		pRetNode = proto;
	}
	else if (tok.type == KTOK_FORWARD)
	{
		ASTPrototype *proto;

		proto = ParsePrototype();
		proto->SetFuncType(KTOK_FORWARD);
		pRetNode = proto;
	}
	else if (tok.type == KTOK_ENUM)
	{
		pRetNode = ParseEnumeration();
	}
	else if (tok.type == KTOK_PUBLIC || tok.type == KTOK_STOCK || tok.type == KTOK_IDENTIFIER)
	{
		ASTPrototype *proto;

		/* An identifier at the middle of global scope must be a function definition.
		 * Push our token back so the prototype detector can succeed.
		 */
		if (tok.type == KTOK_IDENTIFIER)
		{
			m_pLexer->PushBack(tok);
		}

		proto = ParsePrototype();

		if (tok.type != KTOK_IDENTIFIER)
		{
			proto->SetFuncType(tok.type);
		}

		/* If we get a semicolon, it's a prototype ONLY and thus we terminate here.
		 * We allow this even if semicolon mode is optional.
		 */
		if (MatchChar(';'))
		{
			pRetNode = proto;
			bRequireSemi = false;
		}
		/* Compatibility warning: unlike AMXX/SM, we enforce braces on function bodies! */
		else
		{
			ASTCompoundStmt *comp;

			comp = new ASTCompoundStmt();

			NeedChar('{');
			while ((pRetNode = ParseStatement()) != NULL)
			{
				comp->AddStatement(pRetNode);
			}
			NeedChar('}');

			pRetNode = new ASTFunction(proto, comp);

			bRequireSemi = false;
		}
	}
	else if (tok.type == KTOK_NEW)
	{
		Token_t tag;

		m_pLexer->NextToken(&tok);

		if (!MatchToken(KTOK_LABEL, &tag))
		{
			tag.type = KTOK_EOF;
		}

		NeedToken(KTOK_IDENTIFIER, &tok);

		pRetNode = new ASTVariable(tok, tag, VarDecl_Global);
	}

	if (bRequireSemi)
	{
		CheckSemicolon();
	}

	return pRetNode;
}

ASTBaseNode *Parser::ParseStatement()
{
	Token_t tok;
	bool bRequireSemi;
	ASTBaseNode *pNode;

	pNode = NULL;

	m_pLexer->NextToken(&tok);

	bRequireSemi = true;

	if (tok.type == KTOK_CHAR && (char)tok.image == '{')
	{
		ASTCompoundStmt *comp;

		comp = new ASTCompoundStmt();
		while ((pNode = ParseStatement()) != NULL)
		{
			comp->AddStatement(pNode);
		}

		NeedChar('}');

		pNode = comp;

		bRequireSemi = false;
	}
	else if (tok.type == KTOK_IF)
	{
		ASTIfStatement *pIf;
		ASTBaseNode *pExpr, *pStmt;

		pIf = new ASTIfStatement();

		NeedChar('(');
		if ((pExpr = ParseExpression()) == NULL)
		{
			Err_UnexpectedToken("<expression>");
		}
		NeedChar(')');

		if ((pStmt = ParseStatement()) == NULL)
		{
			Err_UnexpectedToken("<statement>");
		}

		pIf->AddBranch(pExpr, pStmt);

		bool should_end = false;
		while (true)
		{
			if (!MatchToken(KTOK_ELSE, &tok))
			{
				break;
			}
			should_end = !MatchToken(KTOK_IF, &tok);
			NeedChar('(');
			if ((pExpr = ParseExpression()) == NULL)
			{
				Err_UnexpectedToken("<expression>");
			}
			NeedChar(')');
			if ((pStmt = ParseStatement()) == NULL)
			{
				Err_UnexpectedToken("<statement>");
			}
			if (should_end)
			{
				pIf->SetFalseBranch(pExpr, pStmt);
				break;
			}
			else
			{
				pIf->AddBranch(pExpr, pStmt);
			}
		}

		pNode = pIf;

		bRequireSemi = false;
	}
	else if (tok.type == KTOK_WHILE)
	{
		ASTBaseNode *pExpr, *pStmt;

		NeedChar('(');
		if ((pExpr = ParseExpression()) == NULL)
		{
			Err_UnexpectedToken("<expression>");
		}
		NeedChar(')');

		if ((pStmt = ParseStatement()) == NULL)
		{
			Err_UnexpectedToken("<statement>");
		}

		pNode = new ASTWhileStatement(pExpr, pStmt, WhileLoop_While);

		bRequireSemi = false;
	}
	else if (tok.type == KTOK_DO)
	{
		ASTBaseNode *pExpr, *pStmt;

		if ((pStmt = ParseStatement()) == NULL)
		{
			Err_UnexpectedToken("<statement>");
		}

		NeedToken(KTOK_WHILE);
		NeedChar('(');
		if ((pExpr = ParseExpression()) == NULL)
		{
			Err_UnexpectedToken("<expression>");
		}
		NeedChar(')');

		pNode = new ASTWhileStatement(pExpr, pStmt, WhileLoop_Do);

		/* In semicolon mode, we need a semicolon here! */
	}
	else if (tok.type == KTOK_FOR)
	{
		ASTBaseNode *pStart, *pCond, *pIter, *pStmt;

		NeedChar('(');
		pStart = ParseExpression();
		NeedChar(';');
		pCond = ParseExpression();
		NeedChar(';');
		pIter = ParseExpression();
		NeedChar(')');

		if ((pStmt = ParseStatement()) == NULL)
		{
			Err_UnexpectedToken("<statement>");
		}

		pNode = new ASTForStatement(pStart, pCond, pIter, pStmt);

		bRequireSemi = false;
	}
	else if (tok.type == KTOK_NEW)
	{
		Token_t tag;

		m_pLexer->NextToken(&tok);

		if (!MatchToken(KTOK_LABEL, &tag))
		{
			tag.type = KTOK_EOF;
		}

		NeedToken(KTOK_IDENTIFIER, &tok);

		pNode = new ASTVariable(tok, tag, VarDecl_Local);
	}
	else
	{
		m_pLexer->PushBack(tok);
		pNode = ParseExpression();
	}

	if (bRequireSemi && pNode != NULL)
	{
		CheckSemicolon();
	}

	return pNode;
}

ASTEnumeration *Parser::ParseEnumeration()
{
	Token_t tok;
	ASTEnumeration *pEnum;

	if (MatchToken(KTOK_ENUM, &tok))
	{
		pEnum = new ASTEnumeration(&tok);
	}
	else
	{
		pEnum = new ASTEnumeration(NULL);
	}

	NeedChar('{');
	
	do
	{
		NeedToken(KTOK_IDENTIFIER, &tok);
		pEnum->AddEntry(tok);
		if (!MatchChar(','))
		{
			NeedChar('}');
			break;
		}
	} while (true);

	return pEnum;
}

struct rpn_entry
{
	rpn_entry()
	{
	}
	rpn_entry(const Token_t & t, int p) : 
		tok(t), prec(p)
	{
	}
	Token_t tok;
	int prec;
};

#define OP_ORDER_L2R	0
#define OP_ORDER_R2L	1

inline bool GetOpPrecedence(const Token_t & tok, int *prec, int *order)
{
	switch (tok.type)
	{
	case KTOK_CHAR:
		{
			switch ((char)tok.image)
			{
			case '*':
			case '/':
			case '%':
				{
					*prec = 3;
					*order = OP_ORDER_L2R;
					return true;
				}
			case '+':
			case '-':
				{
					*prec = 4;
					*order = OP_ORDER_L2R;
					return true;
				}
			case '&':
				{
					*prec = 6;
					*order = OP_ORDER_L2R;
					return true;
				}
			case '^':
				{
					*prec = 7;
					*order = OP_ORDER_L2R;
					return true;
				}
			case '|':
				{
					*prec = 8;
					*order = OP_ORDER_L2R;
					return true;
				}
			case '>':
			case '<':
				{
					*prec = 9;
					*order = OP_ORDER_L2R;
					return true;
				}
			case '=':
				{
					*prec = 14;
					*order = OP_ORDER_R2L;
					return true;
				}
			default:
				{
					return false;
				}
			}
		}
	case KTOK_OP_SHL:
	case KTOK_OP_SHR:
		{
			*prec = 5;
			*order = OP_ORDER_L2R;
			return true;
		}
	case KTOK_OP_TEST_GRE:
	case KTOK_OP_TEST_LTE:
		{
			*prec = 9;
			*order = OP_ORDER_L2R;
			return true;
		}
	case KTOK_OP_TEST_EQ:
	case KTOK_OP_TEST_NEQ:
		{
			*prec = 10;
			*order = OP_ORDER_L2R;
			return true;
		}
	case KTOK_OP_TEST_AND:
		{
			*prec = 11;
			*order = OP_ORDER_L2R;
			return true;
		}
	case KTOK_OP_TEST_OR:
		{
			*prec = 12;
			*order = OP_ORDER_L2R;
			return true;
		}
	case KTOK_OP_ANDEQ:
	case KTOK_OP_OREQ:
	case KTOK_OP_MULEQ:
	case KTOK_OP_DIVEQ:
	case KTOK_OP_ADDEQ:
	case KTOK_OP_SUBEQ:
	case KTOK_OP_MODEQ:
	case KTOK_OP_SHLEQ:
	case KTOK_OP_SHREQ:
	case KTOK_OP_XOREQ:
		{
			*prec = 14;
			*order = OP_ORDER_R2L;
			return true;
		}
	}

	return false;
}

ASTBaseNode *Parser::ParseExpression()
{
	Token_t tok;
	int prec, order;
	ASTBaseNode *pNode;
	SourceHook::CStack<ASTBaseNode *> output;
	SourceHook::CStack<rpn_entry> ops;

	if ((pNode = ParsePrimaryExpression()) == NULL)
	{
		return NULL;
	}

	m_pLexer->NextToken(&tok);
	if (!GetOpPrecedence(tok, &prec, &order))
	{
		return pNode;
	}

	/* Rather than do a huge recursive mess, we're going to use the 
	 * shunting yard algorithm instead.  Instead of pushing operators 
	 * onto the output stack, we just build a binary tree instead.
	 */
	output.push(pNode);
	
	do
	{
		/* We always push the last operator onto the stack */
		ops.push(rpn_entry(tok, prec));

		/* We need to get an expression next */
		if ((pNode = ParsePrimaryExpression()) == NULL)
		{
			/* :TODO: figure out what to do so the stacked items do not leak */
			Err_UnexpectedToken("<expression>");
			return NULL;
		}

		output.push(pNode);

		m_pLexer->NextToken(&tok);
		if (!GetOpPrecedence(tok, &prec, &order))
		{
			m_pLexer->PushBack(tok);
			break;
		}

		while (!ops.empty())
		{
			rpn_entry & op = ops.front();

			if ((order == OP_ORDER_L2R && prec > op.prec)
				|| (order == OP_ORDER_R2L && prec >= op.prec))
			{
				/* Reduce the stack! Because we bail out on failures, 
				 * there will always be two items on the stack.
				 */
				ASTBaseNode *lft, *rght;

				/* Get the right and left sides off the stack */
				rght = output.front();
				output.pop();
				lft = output.front();
				output.pop();

				/* Create the combined binary expression */
				pNode = new ASTBinaryExpression(lft, op.tok, rght);

				/* Push the new expression back onto the output stack */
				output.push(pNode);

				/* Pop the operator off the operator stack */
				ops.pop();
			}
			else
			{
				break;
			}
		}
	} while (true);

	/* Now it's time to fully reduce the stack */
	while (!ops.empty())
	{
		ASTBaseNode *lft, *rght;
		rpn_entry & op = ops.front();

		/* Get the right and left sides off the stack */
		rght = output.front();
		output.pop();
		lft = output.front();
		output.pop();

		/* Create the combined binary expression */
		pNode = new ASTBinaryExpression(lft, op.tok, rght);

		/* Push the new expression back onto the output stack */
		output.push(pNode);

		/* Pop the operator off the operator stack */
		ops.pop();
	}

	/* We should be reduced to one item now! */
	assert(output.size() == 1);

	return output.front();
}

ASTBaseNode *Parser::ParsePrimaryExpression()
{
	Token_t tok;
	ASTBaseNode *pExpr;

	m_pLexer->NextToken(&tok);

	pExpr = NULL;

	/* Check for unary operations */
	if ((tok.type == KTOK_CHAR
		 && ((char)tok.image == '!'
			|| (char)tok.image == '~'
			|| (char)tok.image == '-'))
		|| tok.type == KTOK_OP_DECREMENT
		|| tok.type == KTOK_OP_INCREMENT
		|| tok.type == KTOK_LABEL)
	{
		if ((pExpr = ParsePrimaryExpression()) == NULL)
		{
			Err_UnexpectedToken("<expression>");
		}
		pExpr = new ASTUnaryExpression(tok, pExpr);
	}
	else if (tok.type == KTOK_INTEGER_LITERAL
				|| tok.type == KTOK_TRUE
				|| tok.type == KTOK_FALSE)
	{
		pExpr = new ASTConstLiteral(tok);
	}
	else if (tok.type == KTOK_CHAR && (char)tok.image == '(')
	{
		if ((pExpr = ParsePrimaryExpression()) == NULL)
		{
			/* Our parent will throw an error about this so we ignore it for now */
		}
		NeedChar(')');
	}
	else if (tok.type == KTOK_IDENTIFIER)
	{
		if (MatchChar('('))
		{
			ASTInvocation *pInvoke;

			pInvoke = new ASTInvocation(tok);

			/* This is a function invocation, whee */
			while (!MatchChar(')'))
			{
				if ((pExpr = ParseExpression()) == NULL)
				{
					Err_UnexpectedToken("<expression>");
				}

				pInvoke->AddParameter(pExpr);
				
				if (!MatchChar(','))
				{
					NeedChar(')');
					break;
				}
			}

			pExpr = pInvoke;
		}
		else
		{
			ASTVName *pVar;

			/* We have to assume this is some sort of VName.
			 * Luckily for us, the most complex vname is just an array, so we're good.
			 */
			pVar = new ASTVName(tok);

			while (MatchChar('['))
			{
				if ((pExpr = ParseExpression()) == NULL)
				{
					Err_UnexpectedToken("<expression>");
				}
				pVar->AddAccessor(pExpr);
				NeedChar(']');
			}

			pExpr = pVar;
		}
	}
	else if (tok.type == KTOK_LABEL)
	{
		
	}
	else
	{
		m_pLexer->PushBack(tok);
	}

	return pExpr;
}

ASTPrototype *Parser::ParsePrototype()
{
	Token_t tok;
	Token_t tag, *pTag;
	Token_t identifier;
	ASTPrototype *pPrototype;

	if (MatchToken(KTOK_LABEL, &tag))
	{
		pTag = &tag;
	}
	else
	{
		pTag = NULL;
	}
	
	NeedToken(KTOK_IDENTIFIER, &identifier);

	pPrototype = new ASTPrototype(identifier, pTag);

	NeedChar('(');

	do
	{
		proto_arg_t arg;

		if (MatchChar(')'))
		{
			break;
		}

		arg.is_const = MatchToken(KTOK_CONST);
		arg.is_ref = MatchChar('&');

		if (MatchToken(KTOK_LABEL, &arg.tag))
		{
			arg.has_tag = true;
		}
		else
		{
			arg.has_tag = false;
		}

		NeedToken(KTOK_IDENTIFIER, &arg.ident);

		arg.dim_count = 0;
		while (MatchChar('['))
		{
			if (MatchToken(KTOK_INTEGER_LITERAL, &tok))
			{
				arg.dims[arg.dim_count++] = atoi(m_pLexer->GetString(tok.image));
			}
			else
			{
				arg.dims[arg.dim_count++] = -1;
			}
			NeedChar(']');
		}

		if (!MatchChar(','))
		{
			NeedChar(')');
			break;
		}

		pPrototype->AddArgument(arg);

	} while (true);

	return pPrototype;
}

void Parser::NeedToken(TokenType t, Token_t *info)
{
	Token_t tok;

	tok = ENextToken();

	if (tok.type == t)
	{
		if (info != NULL)
		{
			*info = tok;
		}
		return;
	}

	m_pErrMan->AddError(ErrorMessage_UnexpectedToken,
		tok.line,
		m_pLexer->GetTokenName(t),
		m_pLexer->GetTokenName(tok.type));
}

Token_t Parser::ENextToken()
{
	Token_t tok;

	m_pLexer->NextToken(&tok);

	return tok;
}

bool Parser::MatchChar(char c)
{
	Token_t tok;

	if (!MatchToken(KTOK_CHAR, &tok))
	{
		return false;
	}

	if ((char)tok.image != c)
	{
		m_pLexer->PushBack(tok);
		return false;
	}

	return true;
}

bool Parser::MatchToken(TokenType t, Token_t *info /* = NULL */)
{
	return m_pLexer->MatchToken(t, info);
}

void Parser::NeedChar(char c)
{
	if (!MatchChar(c))
	{
		char msg[4];
		Token_t tok;
		
		msg[0] = '\'';
		msg[1] = c;
		msg[2] = '\'';
		msg[3] = '\0';

		tok = ENextToken();

		m_pErrMan->AddError(ErrorMessage_UnexpectedToken,
			tok.line,
			msg,
			m_pLexer->GetTokenName(KTOK_CHAR));
	}
}

void Parser::CheckSemicolon()
{
	if (m_bRequireSemicolon)
	{
		NeedChar(';');
	}
	else
	{
		MatchChar(';');
	}
}

void Parser::Err_UnexpectedToken(const char *expected)
{
	Token_t tok;

	tok = ENextToken();
	m_pErrMan->AddError(
		ErrorMessage_UnexpectedToken, 
		tok.line,
		"<expression>",
		m_pLexer->GetTokenName(tok.type));
}
