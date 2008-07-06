#ifndef _INCLUDE_KUNIT_TEST_BASE_H_
#define _INCLUDE_KUNIT_TEST_BASE_H_

/**
 * by David "BAILOPAN" Anderson.
 * I hereby release this file into the public domain.
 */

namespace Knight
{
	/**
	 * @brief Encapsulates an exception from a test.
	 */
	class KUnitException
	{
	public:
		/**
		 * @brief Instantiates the exception.
		 * 
		 * @param f			File (__FILE__).
		 * @param l			Line (__LINE__).
		 * @param e			Expression text.
		 */
		KUnitException(const char *f, unsigned int l, const char *e) : 
		  expr(e), file(f), line(l)
		  {
		  }
	public:
		const char *expr;			/**< Expression that failed. */
		const char *file;			/**< File that failed. */
		unsigned int line;			/**< Line that failed. */
	};

	/**
	 * @brief Callbacks for test reports.
	 */
	class ITestReporter
	{
	public:
		/**
		 * @brief Called when tests are started on a class.
		 *
		 * @param module		Class name.
		 */
		virtual void StartTests(const char *module) =0;

		/**
		 * @brief Called when a test succeeds.
		 *
		 * @param num			Test number (starts at 1).
		 * @param name			Test name.
		 */
		virtual void TestSucceeded(unsigned int num, const char *name) =0;

		/**
		 * @brief Called when a test fails.
		 *
		 * @param num			Test number (starts at 1).
		 * @param name			Test name.
		 * @param e				Exception information.
		 */
		virtual void TestFailed(unsigned int num, const char *name, KUnitException e) =0;

		/**
		 * @brief Called when tests are ended on a class.
		 *
		 * @param module		Class name.
		 */
		virtual void EndTests(const char *module) =0;
	};

	/**
	 * @brief Base class for unit tests.
	 */
	class KUnitTestBase
	{
	public:
		/**
		 * @brief Runs all contained tests.
		 *
		 * @param reporter		Reporter instance.
		 */
		virtual void RunTests(ITestReporter *reporter) = 0;

		/**
		 * @brief Called before tests are run.
		 */
		virtual void SetupTests()
		{
		}

		/**
		 * @brief Called after tests are run.
		 */
		virtual void TeardownTests()
		{
		}
	};

	/**
	 * @brief Throws an exception if expr is false.
	 */
	#define KASSERT(expr) \
		if (!(expr)) throw Knight::KUnitException(__FILE__, __LINE__, #expr);

	/**
	 * @brief Helper macro to set up a class inheriting from 
	 * KUnitTestBase.
	 */
	#define KUNIT_DECLARE_BASE() \
		public: \
			virtual void RunTests(ITestReporter *reporter);

	/**
	 * @brief Helper macro for implementing the top of a RunTests function.
	 */
	#define KUNIT_RUNNER_BEGIN(cls) \
		void cls::RunTests(ITestReporter *reporter) \
		{ \
			typedef cls KUnitTester; \
			struct kunit_test_t \
			{ \
				void (cls::*func)(); \
				const char *name; \
			}; \
			const char *module = #cls; \
			static kunit_test_t TESTS[] = {

	/**
	 * @brief Used in between KUNIT_RUNNER_BEGIN and KUNIT_RUNNER_END, 
	 * adds a member function to the test list.
	 */
	#define KUNIT_ADD_TEST(func) \
				{&KUnitTester::func, #func},

	/**
	 * @brief Used to complement KUNIT_RUNNER_END, implements the rest of 
	 * the RunTests() function.
	 */
	#define KUNIT_RUNNER_END() \
			}; \
			unsigned int total_tests = (sizeof(TESTS) / sizeof(TESTS[0])); \
			reporter->StartTests(module); \
			SetupTests(); \
			for (unsigned int i = 0; i < total_tests; i++) \
			{ \
				try \
				{ \
					(this->*TESTS[i].func)(); \
					reporter->TestSucceeded(i+1, TESTS[i].name); \
				} \
				catch (Knight::KUnitException e) \
				{ \
					reporter->TestFailed(i+1, TESTS[i].name, e); \
				} \
			} \
			TeardownTests(); \
			reporter->EndTests(module); \
		}
}

#endif //_INCLUDE_KUNIT_TEST_BASE_H_
