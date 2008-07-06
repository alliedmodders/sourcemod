#ifndef _INCLUDE_KNIGHT_LEXER_H_
#define _INCLUDE_KNIGHT_LEXER_H_

#include <string.h>
#include <sh_stack.h>

namespace Knight
{
	typedef int LexString_t;

	/**
	 * Null entry in the lex string table.
	 */
	static const LexString_t LEX_NULL_STRING = -1;

	/**
	 * Lexical tokens.  Unless otherwise noted, tokens do not 
	 * have image text (their image field will be NULL_LEX_STRING).
	 */
	enum TokenType
	{
		KTOK_EOF = 0,			/**< End of input */

		/* Literals */
		KTOK_CHAR,				/**< Any ASCII character (image = char #) */
		KTOK_IDENTIFIER,		/**< An identifier (image = string idx for text) */
		KTOK_INTEGER_LITERAL,	/**< An integer literal (image = string idx for text) */
		KTOK_LABEL,				/**< An identifier with a : (image does not have the :) */

		/* Keywords */
		KTOK_CONST,				/**< "const" */
		KTOK_PUBLIC,			/**< "public" */
		KTOK_NATIVE,			/**< "native" */
		KTOK_STOCK,				/**< "stock" */
		KTOK_FORWARD,			/**< "forward" */
		KTOK_NEW,				/**< "new" */
		KTOK_ENUM,				/**< "enum" */
		KTOK_TRUE,				/**< "true" */
		KTOK_FALSE,				/**< "false" */
		KTOK_IF,				/**< "if" */
		KTOK_ELSE,				/**< "else" */
		KTOK_WHILE,				/**< "while" */
		KTOK_DO,				/**< "do" */
		KTOK_FOR,				/**< "for" */

		/* Operators */
		KTOK_OP_MULEQ,			/**< "*=" */
		KTOK_OP_DIVEQ,			/**< "/=" */
		KTOK_OP_MODEQ,			/**< "%=" */
		KTOK_OP_ADDEQ,			/**< "+=" */
		KTOK_OP_SUBEQ,			/**< "-=" */
		KTOK_OP_SHLEQ,			/**< "<<=" */
		KTOK_OP_SHREQ,			/**< ">>=" */
		KTOK_OP_ANDEQ,			/**< "&=" */
		KTOK_OP_XOREQ,			/**< "^=" */
		KTOK_OP_OREQ,			/**< "|=" */
		KTOK_OP_TEST_OR,		/**< "||" */
		KTOK_OP_TEST_AND,		/**< "&&" */
		KTOK_OP_TEST_EQ,		/**< "==" */
		KTOK_OP_TEST_NEQ,		/**< "!=" */
		KTOK_OP_TEST_LTE,		/**< "<=" */
		KTOK_OP_TEST_GRE,		/**< ">=" */
		KTOK_OP_SHL,			/**< "<<" */
		KTOK_OP_SHR,			/**< ">>" */
		KTOK_OP_INCREMENT,		/**< "++" */
		KTOK_OP_DECREMENT,		/**< "--" */
		KTOK_OP_ELIPSES,		/**< "..." */

		KTOKENS_TOTAL
	};

	struct Token_t
	{
		TokenType type;			/**< Token type. */
		LexString_t image;		/**< Index into the string table for image 
								 * text, or -1 for none.
								 */
		unsigned int line;		/**< Line it starts at. */
		unsigned int ch;		/**< Character it starts at. */
	};

	struct LexState_t
	{
		unsigned int line;
		unsigned int ch;
		const char *pos;
	};

	class StringTable
	{
	public:
		StringTable();
		~StringTable();
	public:
		const char *GetString(LexString_t idx);
		LexString_t CreateStringEntry(const char *begin, size_t len);
	private:
		char *m_pStringTab;
		size_t m_StrTabPos;
		size_t m_StrTabSize;
	};

	class Lexer
	{
	public:
		Lexer(StringTable *pStrTab, const char *input);
		~Lexer();
	public:
		TokenType NextToken(Token_t *info);
		bool MatchToken(TokenType t, Token_t *info=NULL);
		void PushBack(const Token_t & tok);
		const char *GetString(LexString_t idx);
		const char *GetTokenName(TokenType t);
	private:
		Token_t Lex();
		int GetCharFlags(char c);
		void InitCharTable();
		TokenType FindKeyword(const char *begin, size_t len, LexString_t *image);
		TokenType FindOperator(unsigned int *eaten);
		size_t NumCharsLeft();
	private:
		StringTable *m_pStrTab;
		const char *m_pInput;
		LexState_t m_CurPos;
		int m_CharTable[255];
		size_t m_InputSize;
		SourceHook::CStack<Token_t> m_LexStack;
	};
};

#endif //_INCLUDE_KNIGHT_LEXER_H_
