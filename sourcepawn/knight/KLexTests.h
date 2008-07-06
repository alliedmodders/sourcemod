#ifndef _INCLUDE_KUNIT_LEX_TESTS_H_
#define _INCLUDE_KUNIT_LEX_TESTS_H_

#include "KUnitTest.h"

using namespace Knight;

class KLexTests : public KUnitTestBase
{
	KUNIT_DECLARE_BASE();
private:
	void TestKeywords();
	void TestOperators1();
	void TestOperators2();
	void TestPushBack();
};

#endif //_INCLUDE_KUNIT_LEX_TESTS_H_
