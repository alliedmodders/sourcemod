#ifndef _INCLUDE_KNIGHT_PARSER_H_
#define _INCLUDE_KNIGHT_PARSER_H_

#include "Lexer.h"
#include "ErrorManager.h"
#include "DefineASTs.h"

namespace Knight
{
	#define KNIGHT_MAX_DIMS			4

	class Parser
	{
	public:
		Parser(Lexer *pLexer, ErrorManager *pErrors);
		~Parser();
	public:
		ASTProgram *ParseProgram();
		ASTBaseNode *ParseGlobalStatement();
		ASTBaseNode *ParseExpression();
		ASTBaseNode *ParsePrimaryExpression();
		ASTBaseNode *ParseStatement();
	private: /* Private parsers that cannot fail */
		ASTPrototype *ParsePrototype();
		ASTEnumeration *ParseEnumeration();
	private:
		void NeedToken(TokenType t, Token_t *info = NULL);
		bool MatchChar(char c);
		void NeedChar(char c);
		bool MatchToken(TokenType t, Token_t *info = NULL);
		Token_t ENextToken();
		void CheckSemicolon();
		void Err_UnexpectedToken(const char *expected);
	private:
		bool m_bRequireSemicolon;
		Lexer *m_pLexer;
		ErrorManager *m_pErrMan;
	};
}

#endif //_INCLUDE_KNIGHT_PARSER_H_
