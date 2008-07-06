#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include "Lexer.h"

using namespace Knight;

#define CF_WHITESPACE		(1<<0)
#define CF_IDSTART			(1<<1)
#define CF_IDTOK			(1<<2)
#define CF_NUMBER			(1<<3)
#define CF_OPSTART			(1<<4)
#define LEX_STATE_NONE			0
#define LEX_STATE_IDENT			1
#define LEX_STATE_SL_COMMENT	2
#define LEX_STATE_ML_COMMENT	3
#define LEX_STATE_NUMBER		4

static const char *g_LexTokenNames[] = 
{
	"<eof>", 
	"<char>",
	"<identifier>", 
	"<integer>",
	"<label>",
	"'const'",
	"'public'",
	"'native'",
	"'stock'",
	"'forward'",
	"'new'",
	"'enum'",
	"'true'",
	"'false'",
	"'if'",
	"'else'",
	"'while'",
	"'for'",
	"'do'",
	"'*='",
	"'/='",
	"'%='",
	"'+='",
	"'-='",
	"'<<='",
	"'>>='",
	"'&='",
	"'^='",
	"'|='",
	"'|'",
	"'&'",
	"'=='",
	"'!='",
	"'<='",
	"'>='",
	"'<<'",
	"'>>'",
	"'++'",
	"'--'",
	"'...'",
};

Lexer::Lexer(StringTable *pTable, const char *input) : 
	m_pStrTab(pTable), m_pInput(input)
{
	m_CurPos.ch = 1;
	m_CurPos.line = 1;
	m_CurPos.pos = m_pInput;

	m_InputSize = strlen(m_pInput);
	
	InitCharTable();
}

Lexer::~Lexer()
{
}

const char *Lexer::GetTokenName(TokenType t)
{
	if (t < 0 || t >= KTOKENS_TOTAL)
	{
		return NULL;
	}

	return g_LexTokenNames[t];
}

const char *Lexer::GetString(LexString_t idx)
{
	if (idx == LEX_NULL_STRING)
	{
		return NULL;
	}

	return m_pStrTab->GetString(idx);
}

TokenType Lexer::NextToken(Token_t *info)
{
	Token_t tok;

	tok = Lex();
	if (info != NULL)
	{
		*info = tok;
	}

	return tok.type;
}

bool Lexer::MatchToken(TokenType t, Token_t *info)
{
	Token_t tok;

	tok = Lex();

	if (tok.type == t)
	{
		if (info != NULL)
		{
			*info = tok;
		}

		return true;
	}
	else
	{
		PushBack(tok);

		return false;
	}
}

void Lexer::PushBack(const Token_t & tok)
{
	m_LexStack.push(tok);
}

#define LOOKAHEAD(n) *(m_CurPos.pos + n)

Token_t Lexer::Lex()
{
	int state;
	Token_t tok;
	LexState_t begin;

	if (!m_LexStack.empty())
	{
		tok = m_LexStack.front();
		m_LexStack.pop();
		return tok;
	}

	begin = m_CurPos;
	state = LEX_STATE_NONE;

	do
	{
		if (state == LEX_STATE_NONE)
		{
			int flags;

			if (*m_CurPos.pos == '\0')
			{
				tok.image = LEX_NULL_STRING;
				tok.type = KTOK_EOF;
				break;
			}

			flags = GetCharFlags(*m_CurPos.pos);

			if (flags & CF_IDSTART)
			{
				state = LEX_STATE_IDENT;
				begin = m_CurPos;
			}
			else if (flags & CF_NUMBER)
			{
				state = LEX_STATE_NUMBER;
				begin = m_CurPos;
			}
			else if (flags & CF_WHITESPACE)
			{
				if (*m_CurPos.pos == '\n')
				{
					m_CurPos.ch = 0;
					m_CurPos.line++;
				}
			}
			else if (*m_CurPos.pos == '/' 
					 && NumCharsLeft() >= 2
					 && (LOOKAHEAD(1) == '/' || LOOKAHEAD(1) == '*'))
			{
				if (LOOKAHEAD(1) == '/')
				{
					state = LEX_STATE_SL_COMMENT;
					begin = m_CurPos;
				}
				else if (LOOKAHEAD(1) == '*')
				{
					state = LEX_STATE_ML_COMMENT;
					begin = m_CurPos;
				}
			}
			else
			{
				TokenType t;
				unsigned int eaten;

				/* Check for operators */
				if (NumCharsLeft() >= 2 && (t = FindOperator(&eaten)) != KTOK_EOF)
				{
					tok.image = -1;
					tok.line = m_CurPos.line;
					tok.ch = m_CurPos.ch;
					tok.type = t;

					m_CurPos.ch += eaten;
					m_CurPos.pos += eaten;

					return tok;
				}
				else if (((unsigned)*m_CurPos.pos) & 0x80)
				{
					/* This is an invalid character! Attempt to consume 
					 * tokens until we get something ASCII again.
					 * :TODO:
					 */
				}
				else
				{
					/* General character, return it. */
					tok.ch = m_CurPos.ch;
					tok.image = *m_CurPos.pos;
					tok.line = m_CurPos.line;
					tok.type = KTOK_CHAR;

					m_CurPos.pos++;
					m_CurPos.ch++;

					return tok;
				}
			}
		}
		else if (state == LEX_STATE_IDENT)
		{
			int flags;

			flags = GetCharFlags(*m_CurPos.pos);
			
			/* If we don't have an identifier anymore, we create a token and jump back. */
			if ((flags & CF_IDTOK) != CF_IDTOK)
			{
				tok.type = FindKeyword(begin.pos, m_CurPos.pos - begin.pos, &tok.image);
				tok.ch = begin.ch;
				tok.line = begin.line;

				/* We have the token... if it's not a keyword, we can detect a label */
				if (tok.type == KTOK_IDENTIFIER && *(m_CurPos.pos) == ':')
				{
					tok.type = KTOK_LABEL;
					m_CurPos.pos++;
					m_CurPos.ch++;
				}

				return tok;
			}
		}
		else if (state == LEX_STATE_NUMBER)
		{
			int flags;

			flags = GetCharFlags(*m_CurPos.pos);

			/* If we don't have a number anymore, we create a token and jump back. */
			if ((flags & CF_NUMBER) != CF_NUMBER)
			{
				tok.type = KTOK_INTEGER_LITERAL;
				tok.ch = begin.ch;
				tok.line = begin.line;
				tok.image = m_pStrTab->CreateStringEntry(begin.pos, m_CurPos.pos - begin.pos);

				return tok;
			}
		}
		else if (state == LEX_STATE_SL_COMMENT)
		{
			if (*m_CurPos.pos == '\n')
			{
				state = LEX_STATE_NONE;
				m_CurPos.ch = 0;
				m_CurPos.line++;
			}
		}
		else if (state == LEX_STATE_ML_COMMENT)
		{
			if (*m_CurPos.pos == '\n')
			{
				m_CurPos.ch = 0;
				m_CurPos.line++;
			}
			else if (*m_CurPos.pos == '*'
					 && NumCharsLeft() >= 2 
					 && LOOKAHEAD(1) == '/')
			{
				m_CurPos.pos += 2;
				m_CurPos.ch += 2;
				state = LEX_STATE_NONE;
				continue;
			}
		}

		/* Advance input stream */
		m_CurPos.pos++;
		m_CurPos.ch++;

	} while (true);

	/* Fallthrough from EOF break */

	return tok;
}

size_t Lexer::NumCharsLeft()
{
	return m_InputSize - (m_CurPos.pos - m_pInput);
}

int Lexer::GetCharFlags(char c)
{
	return m_CharTable[(unsigned)c];
}

struct lex_keyword_t
{
	const char *begin;
	size_t size;
	TokenType type;
};

/* Yes, these are sorted! */
static lex_keyword_t LEX_TWOCHAR_OPERATORS[] = 
{
	{"!=",		2,	KTOK_OP_TEST_NEQ},		/**< "!=" */
	{"%=",		2,	KTOK_OP_MODEQ},			/**< "%=" */
	{"&&",		2,	KTOK_OP_TEST_AND},		/**< "&&" */
	{"&=",		2,	KTOK_OP_ANDEQ},			/**< "&=" */
	{"*=",		2,	KTOK_OP_MULEQ},			/**< "*=" */
	{"++",		2,	KTOK_OP_INCREMENT},		/**< "++" */
	{"+=",		2,	KTOK_OP_ADDEQ},			/**< "+=" */
	{"--",		2,	KTOK_OP_DECREMENT},		/**< "--" */
	{"-=",		2,	KTOK_OP_SUBEQ},			/**< "-=" */
	{"/=",		2,	KTOK_OP_DIVEQ},			/**< "/=" */
	{"<<",		2,	KTOK_OP_SHL},			/**< "<<" */
	{"<=",		2,	KTOK_OP_TEST_LTE},		/**< "<=" */
	{"==",		2,	KTOK_OP_TEST_EQ},		/**< "==" */
	{">=",		2,	KTOK_OP_TEST_GRE},		/**< ">=" */
	{">>",		2,	KTOK_OP_SHR},			/**< ">>" */
	{"^=",		2,	KTOK_OP_XOREQ},			/**< "^=" */
	{"|=",		2,	KTOK_OP_OREQ},			/**< "|=" */
	{"||",		2,	KTOK_OP_TEST_OR},		/**< "||" */
};

/* These are NOT sorted! */
static lex_keyword_t LEX_THREECHAR_OPERATORS[] = 
{
	{"<<=",		3,	KTOK_OP_SHLEQ},
	{">>=",		3,	KTOK_OP_SHREQ},
	{"...",		3,	KTOK_OP_ELIPSES}
};

static lex_keyword_t LEX_KEYWORDS[] = 
{
	{"const",		5,		KTOK_CONST},
	{"else",		4,		KTOK_ELSE},
	{"enum",		4,		KTOK_ENUM},
	{"false",		5,		KTOK_FALSE},
	{"forward",		7,		KTOK_FORWARD},
	{"if",			2,		KTOK_IF},
	{"native",		6,		KTOK_NATIVE},
	{"new",			3,		KTOK_NEW},
	{"public",		6,		KTOK_PUBLIC},
	{"stock",		5,		KTOK_STOCK},
	{"true",		4,		KTOK_TRUE},
	{"while",		5,		KTOK_WHILE},
	{"do",			2,		KTOK_DO},
	{"for",			3,		KTOK_FOR},
};

int keyword_comparator(const void *item1, const void *item2)
{
	int val;
	lex_keyword_t *key1, *key2;

	key1 = (lex_keyword_t *)item1;
	key2 = (lex_keyword_t *)item2;

	/* Compare the first N characters are the same, where N is the 
	 * size of the smaller string.
	 */
	val = strncmp(key1->begin, 
				key2->begin, 
				(key1->size > key2->size) ? key2->size : key1->size);

	/* If they were equal, return the size difference. */
	if (val == 0)
	{
		if (key1->size > key2->size)
		{
			return 1;
		}
		else if (key1->size < key2->size)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return val;
	}
}

TokenType Lexer::FindOperator(unsigned int *eaten)
{
	size_t num;
	lex_keyword_t item;
	lex_keyword_t *found;

	num = NumCharsLeft();

	if (num >= 3)
	{
		int i;

		for (i = 0; i < sizeof(LEX_THREECHAR_OPERATORS) / sizeof(LEX_THREECHAR_OPERATORS[0]); i++)
		{
			if (strncmp(LEX_THREECHAR_OPERATORS[i].begin, m_CurPos.pos, 3) == 0)
			{
				*eaten = 3;
				return LEX_THREECHAR_OPERATORS[i].type;
			}
		}
	}

	item.begin = m_CurPos.pos;
	item.size = 2;

	found = (lex_keyword_t *)bsearch(&item, 
		LEX_TWOCHAR_OPERATORS, 
		sizeof(LEX_TWOCHAR_OPERATORS) / sizeof(LEX_TWOCHAR_OPERATORS[0]), 
		sizeof(LEX_TWOCHAR_OPERATORS[0]),
		keyword_comparator);

	if (found != NULL)
	{
		*eaten = 2;
		return found->type;
	}

	*eaten = 0;

	return KTOK_EOF;
}

TokenType Lexer::FindKeyword(const char *begin, size_t len, LexString_t *image)
{
	lex_keyword_t item;
	lex_keyword_t *found;

	item.begin = begin;
	item.size = len ? len : strlen(item.begin);

	found = (lex_keyword_t *)bsearch(&item, 
		LEX_KEYWORDS, 
		sizeof(LEX_KEYWORDS) / sizeof(LEX_KEYWORDS[0]), 
		sizeof(LEX_KEYWORDS[0]),
		keyword_comparator);

	if (found == NULL)
	{
		*image = m_pStrTab->CreateStringEntry(begin, len);
		return KTOK_IDENTIFIER;
	}

	*image = LEX_NULL_STRING;

	return found->type;
}

LexString_t StringTable::CreateStringEntry(const char *begin, size_t len)
{
	size_t i;
	int position;
	char *oldBase;

	if (len == 0)
	{
		len = strlen(begin);
	}

	/**
	 * We account for the null terminator, which means >=
	 * (For example, if pos is 3, len is 2, and size is 5, 
	 *  5 >= 5 will fail because there is no room for NUL).
	 */
	if (m_StrTabPos + len >= m_StrTabSize)
	{
		do
		{
			m_StrTabSize *= 2;
		} while (m_StrTabPos + len >= m_StrTabSize);

		m_pStringTab = (char *)realloc(m_pStringTab, sizeof(char) * m_StrTabSize);
	}

	position = (int)m_StrTabPos;
	oldBase = m_pStringTab + m_StrTabPos;
	for (i = 0; i < len; i++)
	{
		oldBase[i] = begin[i];
	}
	oldBase[len] = '\0';
	m_StrTabPos += (len + 1);

	return position;
}

void Lexer::InitCharTable()
{
	int i;

	memset(m_CharTable, 0, sizeof(m_CharTable));

	/* \t, \n, \v, \r, ' ' */
	m_CharTable[0x09] |= CF_WHITESPACE;
	m_CharTable[0x0A] |= CF_WHITESPACE;
	m_CharTable[0x0B] |= CF_WHITESPACE;
	m_CharTable[0x0D] |= CF_WHITESPACE;
	m_CharTable[0x20] |= CF_WHITESPACE;

	/* Lowercase letters */
	for (i = 0x41; i <= 0x5A; i++)
	{
		m_CharTable[i] |= (CF_IDSTART|CF_IDTOK);
	}

	/* Uppercase letters */
	for (i = 0x61; i <= 0x7A; i++)
	{
		m_CharTable[i] |= (CF_IDSTART|CF_IDTOK);
	}

	/* Underscore */
	m_CharTable[0x5F] |= (CF_IDSTART|CF_IDTOK);

	/* Numbers */
	for (i = 0x30; i <= 0x39; i++)
	{
		m_CharTable[i] |= (CF_IDTOK|CF_NUMBER);
	}

	/* ! % & * + - . / < = > ^ */
	m_CharTable[0x21] |= CF_OPSTART;
	m_CharTable[0x25] |= CF_OPSTART;
	m_CharTable[0x26] |= CF_OPSTART;
	m_CharTable[0x2A] |= CF_OPSTART;
	m_CharTable[0x2B] |= CF_OPSTART;
	m_CharTable[0x2E] |= CF_OPSTART;
	m_CharTable[0x2F] |= CF_OPSTART;
	m_CharTable[0x3C] |= CF_OPSTART;
	m_CharTable[0x3D] |= CF_OPSTART;
	m_CharTable[0x3E] |= CF_OPSTART;
	m_CharTable[0x5E] |= CF_OPSTART;
	m_CharTable[0x7C] |= CF_OPSTART;
}

const char *StringTable::GetString(LexString_t idx)
{
	if (idx == LEX_NULL_STRING)
	{
		return NULL;
	}

	return m_pStringTab + idx;
}

StringTable::StringTable()
{
	m_StrTabPos = 0;
	m_StrTabSize = 512;
	m_pStringTab = (char *)malloc(sizeof(char) * m_StrTabSize);
}

StringTable::~StringTable()
{
	free(m_pStringTab);
}
