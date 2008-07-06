#include <string.h>
#include "KLexTests.h"
#include "Lexer.h"

using namespace Knight;

KUNIT_RUNNER_BEGIN(KLexTests)
	KUNIT_ADD_TEST(TestKeywords)
	KUNIT_ADD_TEST(TestOperators1)
	KUNIT_ADD_TEST(TestOperators2)
	KUNIT_ADD_TEST(TestPushBack)
KUNIT_RUNNER_END()

void KLexTests::TestKeywords()
{
	StringTable strtab;
	Lexer k(&strtab, "public   \n stock \t\t\n crab forward native /* crab */ adsf //asdf\nblah");
	Token_t tok;

	KASSERT(k.MatchToken(KTOK_PUBLIC, NULL));
	KASSERT(k.MatchToken(KTOK_STOCK, NULL));
	KASSERT(k.MatchToken(KTOK_IDENTIFIER, &tok));
	KASSERT(strcmp(strtab.GetString(tok.image), "crab") == 0);
	KASSERT(k.MatchToken(KTOK_FORWARD, NULL));
	KASSERT(k.MatchToken(KTOK_NATIVE, NULL));
	KASSERT(k.MatchToken(KTOK_IDENTIFIER, &tok));
	KASSERT(strcmp(strtab.GetString(tok.image), "adsf") == 0);
	KASSERT(k.MatchToken(KTOK_IDENTIFIER, &tok));
	KASSERT(strcmp(strtab.GetString(tok.image), "blah") == 0);
	KASSERT(k.MatchToken(KTOK_EOF, NULL));
}

void KLexTests::TestOperators1()
{
	StringTable strtab;
	Lexer k(&strtab, "++ += <<= <<<<= / $ + ++");
	Token_t tok;

	KASSERT(k.MatchToken(KTOK_OP_INCREMENT, NULL));
	KASSERT(k.MatchToken(KTOK_OP_ADDEQ, NULL));
	KASSERT(k.MatchToken(KTOK_OP_SHLEQ, NULL));
	KASSERT(k.MatchToken(KTOK_OP_SHL, NULL));
	KASSERT(k.MatchToken(KTOK_OP_SHLEQ, NULL));
	KASSERT(k.MatchToken(KTOK_CHAR, &tok));
	KASSERT(tok.image == '/');
	KASSERT(k.MatchToken(KTOK_CHAR, &tok));
	KASSERT(tok.image == '$');
	KASSERT(k.MatchToken(KTOK_CHAR, &tok));
	KASSERT(tok.image == '+');
	KASSERT(k.MatchToken(KTOK_OP_INCREMENT, NULL));
}

void KLexTests::TestOperators2()
{
	StringTable strtab;
	Lexer k(&strtab, "....>>>>=");
	Token_t tok;

	KASSERT(k.MatchToken(KTOK_OP_ELIPSES, NULL));
	KASSERT(k.MatchToken(KTOK_CHAR, &tok));
	KASSERT(tok.image == '.');
	KASSERT(k.MatchToken(KTOK_OP_SHR, NULL));
	KASSERT(k.MatchToken(KTOK_OP_SHREQ, NULL));
}

void KLexTests::TestPushBack()
{
	StringTable strtab;
	Lexer k(&strtab, "....>>>>=");

	KASSERT(!k.MatchToken(KTOK_OP_SHR));
	KASSERT(!k.MatchToken(KTOK_OP_SHL));
	KASSERT(k.MatchToken(KTOK_OP_ELIPSES));
}
