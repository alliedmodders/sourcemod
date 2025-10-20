#include <sourcemod>
#include <yyjson>

#pragma newdecls required
#pragma semicolon 1

public Plugin myinfo =
{
	name = "YYJSON Test Suite",
	author = "ProjectSky",
	description = "test suite for YYJSON functions",
	version = "1.0.0",
	url = "https://github.com/ProjectSky/sm-ext-yyjson"
};

// Test statistics
int g_iTotalTests = 0;
int g_iPassedTests = 0;
int g_iFailedTests = 0;
char g_sCurrentTest[128];
bool g_bCurrentTestFailed = false;

public void OnPluginStart()
{
	RegServerCmd("test_yyjson", Command_RunTests, "Run YYJSON test suite");
}

// ============================================================================
// Test Framework - Assertion Functions
// ============================================================================

/**
 * Start a new test case
 */
void TestStart(const char[] name)
{
	g_iTotalTests++;
	strcopy(g_sCurrentTest, sizeof(g_sCurrentTest), name);
	g_bCurrentTestFailed = false;
}

/**
 * End current test case
 */
void TestEnd()
{
	if (!g_bCurrentTestFailed)
	{
		g_iPassedTests++;
		PrintToServer("[PASS] %s", g_sCurrentTest);
	}
	else
	{
		g_iFailedTests++;
	}
}

/**
 * Assert a condition is true
 */
void Assert(bool condition, const char[] message = "")
{
	if (!condition)
	{
		g_bCurrentTestFailed = true;
		PrintToServer("[FAIL] %s - %s", g_sCurrentTest, message);
	}
}

/**
 * Assert two integers are equal
 */
void AssertEq(int a, int b, const char[] message = "")
{
	if (a != b)
	{
		g_bCurrentTestFailed = true;
		char buffer[256];
		if (message[0] != '\0')
		{
			Format(buffer, sizeof(buffer), "%s (expected %d, got %d)", message, b, a);
		}
		else
		{
			Format(buffer, sizeof(buffer), "Expected %d, got %d", b, a);
		}
		PrintToServer("[FAIL] %s - %s", g_sCurrentTest, buffer);
	}
}

/**
 * Assert two integers are not equal
 */
stock void AssertNeq(int a, int b, const char[] message = "")
{
	if (a == b)
	{
		g_bCurrentTestFailed = true;
		char buffer[256];
		if (message[0] != '\0')
		{
			Format(buffer, sizeof(buffer), "%s (values should not be equal: %d)", message, a);
		}
		else
		{
			Format(buffer, sizeof(buffer), "Values should not be equal: %d", a);
		}
		PrintToServer("[FAIL] %s - %s", g_sCurrentTest, buffer);
	}
}

/**
 * Assert condition is true
 */
void AssertTrue(bool condition, const char[] message = "")
{
	Assert(condition, message[0] != '\0' ? message : "Expected true, got false");
}

/**
 * Assert condition is false
 */
void AssertFalse(bool condition, const char[] message = "")
{
	Assert(!condition, message[0] != '\0' ? message : "Expected false, got true");
}

/**
 * Assert two strings are equal
 */
void AssertStrEq(const char[] a, const char[] b, const char[] message = "")
{
	if (!StrEqual(a, b))
	{
		g_bCurrentTestFailed = true;
		char buffer[512];
		if (message[0] != '\0')
		{
			Format(buffer, sizeof(buffer), "%s (expected \"%s\", got \"%s\")", message, b, a);
		}
		else
		{
			Format(buffer, sizeof(buffer), "Expected \"%s\", got \"%s\"", b, a);
		}
		PrintToServer("[FAIL] %s - %s", g_sCurrentTest, buffer);
	}
}

/**
 * Assert two floats are equal within epsilon
 */
void AssertFloatEq(float a, float b, const char[] message = "", float epsilon = 0.0001)
{
	if (FloatAbs(a - b) > epsilon)
	{
		g_bCurrentTestFailed = true;
		char buffer[256];
		if (message[0] != '\0')
		{
			Format(buffer, sizeof(buffer), "%s (expected %.4f, got %.4f)", message, b, a);
		}
		else
		{
			Format(buffer, sizeof(buffer), "Expected %.4f, got %.4f", b, a);
		}
		PrintToServer("[FAIL] %s - %s", g_sCurrentTest, buffer);
	}
}

/**
 * Assert handle is valid
 */
void AssertValidHandle(Handle handle, const char[] message = "")
{
	Assert(handle != null, message[0] != '\0' ? message : "Handle should not be null");
}

/**
 * Assert handle is null
 */
void AssertNullHandle(Handle handle, const char[] message = "")
{
	Assert(handle == null, message[0] != '\0' ? message : "Handle should be null");
}

// ============================================================================
// Test Command
// ============================================================================

public Action Command_RunTests(int args)
{
	PrintToServer("========================================");
	PrintToServer("YYJSON Test Suite");
	PrintToServer("========================================");
	
	// Reset statistics
	g_iTotalTests = 0;
	g_iPassedTests = 0;
	g_iFailedTests = 0;
	
	// Run all test categories
	Test_BasicValues();
	Test_ObjectOperations();
	Test_ArrayOperations();
	Test_ParseAndSerialize();
	Test_Iterators();
	Test_JSONPointer();
	Test_AdvancedFeatures();
	Test_EdgeCases();
	
	// Print results
	PrintToServer("========================================");
	PrintToServer("YYJSON Test Suite Results");
	PrintToServer("========================================");
	PrintToServer("Total Tests: %d", g_iTotalTests);
	PrintToServer("Passed: %d", g_iPassedTests);
	PrintToServer("Failed: %d", g_iFailedTests);
	
	if (g_iTotalTests > 0)
	{
		float successRate = (float(g_iPassedTests) / float(g_iTotalTests)) * 100.0;
		PrintToServer("Success Rate: %.2f%%", successRate);
	}
	
	PrintToServer("========================================");
	
	return Plugin_Handled;
}

// ============================================================================
// 2.1 Basic Value Tests
// ============================================================================

void Test_BasicValues()
{
	PrintToServer("\n[Category] Basic Value Tests");
	
	// Test CreateBool & GetBool
	TestStart("BasicValues_CreateBool_True");
	{
		YYJSON val = YYJSON.CreateBool(true);
		AssertValidHandle(val);
		AssertTrue(YYJSON.GetBool(val));
		AssertEq(val.Type, YYJSON_TYPE_BOOL);
		AssertTrue(val.IsBool);
		AssertTrue(val.IsTrue);
		AssertFalse(val.IsFalse);
		delete val;
	}
	TestEnd();
	
	TestStart("BasicValues_CreateBool_False");
	{
		YYJSON val = YYJSON.CreateBool(false);
		AssertValidHandle(val);
		AssertFalse(YYJSON.GetBool(val));
		AssertTrue(val.IsBool);
		AssertFalse(val.IsTrue);
		AssertTrue(val.IsFalse);
		delete val;
	}
	TestEnd();
	
	// Test CreateInt & GetInt
	TestStart("BasicValues_CreateInt_Positive");
	{
		YYJSON val = YYJSON.CreateInt(42);
		AssertValidHandle(val);
		AssertEq(YYJSON.GetInt(val), 42);
		AssertEq(val.Type, YYJSON_TYPE_NUM);
		AssertTrue(val.IsInt);
		AssertTrue(val.IsNum);
		delete val;
	}
	TestEnd();
	
	TestStart("BasicValues_CreateInt_Negative");
	{
		YYJSON val = YYJSON.CreateInt(-123);
		AssertValidHandle(val);
		AssertEq(YYJSON.GetInt(val), -123);
		AssertTrue(val.IsSint);
		delete val;
	}
	TestEnd();
	
	TestStart("BasicValues_CreateInt_Zero");
	{
		YYJSON val = YYJSON.CreateInt(0);
		AssertValidHandle(val);
		AssertEq(YYJSON.GetInt(val), 0);
		delete val;
	}
	TestEnd();
	
	// Test CreateFloat & GetFloat
	TestStart("BasicValues_CreateFloat_Positive");
	{
		YYJSON val = YYJSON.CreateFloat(3.14159);
		AssertValidHandle(val);
		AssertFloatEq(YYJSON.GetFloat(val), 3.14159);
		AssertTrue(val.IsFloat);
		AssertTrue(val.IsNum);
		delete val;
	}
	TestEnd();
	
	TestStart("BasicValues_CreateFloat_Negative");
	{
		YYJSON val = YYJSON.CreateFloat(-2.71828);
		AssertValidHandle(val);
		AssertFloatEq(YYJSON.GetFloat(val), -2.71828);
		delete val;
	}
	TestEnd();
	
	TestStart("BasicValues_CreateFloat_Zero");
	{
		YYJSON val = YYJSON.CreateFloat(0.0);
		AssertValidHandle(val);
		AssertFloatEq(YYJSON.GetFloat(val), 0.0);
		delete val;
	}
	TestEnd();
	
	// Test CreateString & GetString
	TestStart("BasicValues_CreateString_Regular");
	{
		YYJSON val = YYJSON.CreateString("Hello, World!");
		AssertValidHandle(val);
		char buffer[64];
		AssertTrue(YYJSON.GetString(val, buffer, sizeof(buffer)));
		AssertStrEq(buffer, "Hello, World!");
		AssertEq(val.Type, YYJSON_TYPE_STR);
		AssertTrue(val.IsStr);
		delete val;
	}
	TestEnd();
	
	TestStart("BasicValues_CreateString_Empty");
	{
		YYJSON val = YYJSON.CreateString("");
		AssertValidHandle(val);
		char buffer[64];
		AssertTrue(YYJSON.GetString(val, buffer, sizeof(buffer)));
		AssertStrEq(buffer, "");
		delete val;
	}
	TestEnd();
	
	TestStart("BasicValues_CreateString_Unicode");
	{
		YYJSON val = YYJSON.CreateString("测试字符串");
		AssertValidHandle(val);
		char buffer[64];
		AssertTrue(YYJSON.GetString(val, buffer, sizeof(buffer)));
		AssertStrEq(buffer, "测试字符串");
		delete val;
	}
	TestEnd();
	
	// Test CreateInt64 & GetInt64
	TestStart("BasicValues_CreateInt64_Large");
	{
		YYJSON val = YYJSON.CreateInt64("9223372036854775807");
		AssertValidHandle(val);
		char buffer[32];
		AssertTrue(YYJSON.GetInt64(val, buffer, sizeof(buffer)));
		AssertStrEq(buffer, "9223372036854775807");
		delete val;
	}
	TestEnd();
	
	TestStart("BasicValues_CreateInt64_Negative");
	{
		YYJSON val = YYJSON.CreateInt64("-9223372036854775808");
		AssertValidHandle(val);
		char buffer[32];
		AssertTrue(YYJSON.GetInt64(val, buffer, sizeof(buffer)));
		AssertStrEq(buffer, "-9223372036854775808");
		delete val;
	}
	TestEnd();
	
	// Test CreateNull
	TestStart("BasicValues_CreateNull");
	{
		YYJSON val = YYJSON.CreateNull();
		AssertValidHandle(val);
		AssertEq(val.Type, YYJSON_TYPE_NULL);
		AssertTrue(val.IsNull);
		delete val;
	}
	TestEnd();
	
	// Test GetTypeDesc
	TestStart("BasicValues_GetTypeDesc");
	{
		YYJSON boolVal = YYJSON.CreateBool(true);
		YYJSON intVal = YYJSON.CreateInt(42);
		YYJSON floatVal = YYJSON.CreateFloat(3.14);
		YYJSON strVal = YYJSON.CreateString("test");
		YYJSON nullVal = YYJSON.CreateNull();

		char buffer[32];

		YYJSON.GetTypeDesc(boolVal, buffer, sizeof(buffer));
		AssertStrEq(buffer, "true");

		YYJSON.GetTypeDesc(intVal, buffer, sizeof(buffer));
		AssertTrue(StrEqual(buffer, "uint") || StrEqual(buffer, "sint"));

		YYJSON.GetTypeDesc(floatVal, buffer, sizeof(buffer));
		AssertStrEq(buffer, "real");

		YYJSON.GetTypeDesc(strVal, buffer, sizeof(buffer));
		AssertStrEq(buffer, "string");

		YYJSON.GetTypeDesc(nullVal, buffer, sizeof(buffer));
		AssertStrEq(buffer, "null");

		delete boolVal;
		delete intVal;
		delete floatVal;
		delete strVal;
		delete nullVal;
	}
	TestEnd();
}

// ============================================================================
// 2.2 Object Operations Tests
// ============================================================================

void Test_ObjectOperations()
{
	PrintToServer("\n[Category] Object Operations Tests");
	
	// Test object creation
	TestStart("Object_Constructor");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertValidHandle(obj);
		AssertTrue(obj.IsObject);
		AssertEq(obj.Size, 0);
		delete obj;
	}
	TestEnd();
	
	// Test Set/Get methods
	TestStart("Object_SetGetInt");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertTrue(obj.SetInt("number", 42));
		AssertEq(obj.GetInt("number"), 42);
		AssertEq(obj.Size, 1);
		delete obj;
	}
	TestEnd();
	
	TestStart("Object_SetGetFloat");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertTrue(obj.SetFloat("pi", 3.14159));
		AssertFloatEq(obj.GetFloat("pi"), 3.14159);
		delete obj;
	}
	TestEnd();
	
	TestStart("Object_SetGetBool");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertTrue(obj.SetBool("flag", true));
		AssertTrue(obj.GetBool("flag"));
		delete obj;
	}
	TestEnd();
	
	TestStart("Object_SetGetString");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertTrue(obj.SetString("name", "test"));
		char buffer[64];
		AssertTrue(obj.GetString("name", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "test");
		delete obj;
	}
	TestEnd();
	
	TestStart("Object_SetGetInt64");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertTrue(obj.SetInt64("bignum", "9223372036854775807"));
		char buffer[32];
		AssertTrue(obj.GetInt64("bignum", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "9223372036854775807");
		delete obj;
	}
	TestEnd();
	
	TestStart("Object_SetGetNull");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertTrue(obj.SetNull("nullable"));
		AssertTrue(obj.IsNull("nullable"));
		delete obj;
	}
	TestEnd();
	
	// Test HasKey
	TestStart("Object_HasKey");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.SetInt("exists", 1);
		AssertTrue(obj.HasKey("exists"));
		AssertFalse(obj.HasKey("notexists"));
		delete obj;
	}
	TestEnd();
	
	// Test GetKey and GetValueAt
	TestStart("Object_GetKeyAndValueAt");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.SetInt("first", 1);
		obj.SetInt("second", 2);

		char key[32];
		AssertTrue(obj.GetKey(0, key, sizeof(key)));
		AssertStrEq(key, "first");

		YYJSON val = obj.GetValueAt(0);
		AssertValidHandle(val);
		AssertEq(YYJSON.GetInt(val), 1);

		delete val;
		delete obj;
	}
	TestEnd();
	
	// Test Remove
	TestStart("Object_Remove");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.SetInt("remove_me", 1);
		obj.SetInt("keep_me", 2);
		AssertEq(obj.Size, 2);

		AssertTrue(obj.Remove("remove_me"));
		AssertEq(obj.Size, 1);
		AssertFalse(obj.HasKey("remove_me"));
		AssertTrue(obj.HasKey("keep_me"));

		delete obj;
	}
	TestEnd();
	
	// Test Clear
	TestStart("Object_Clear");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.SetInt("a", 1);
		obj.SetInt("b", 2);
		obj.SetInt("c", 3);
		AssertEq(obj.Size, 3);

		AssertTrue(obj.Clear());
		AssertEq(obj.Size, 0);

		delete obj;
	}
	TestEnd();
	
	// Test RenameKey
	TestStart("Object_RenameKey");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.SetInt("oldname", 42);

		AssertTrue(obj.RenameKey("oldname", "newname"));
		AssertFalse(obj.HasKey("oldname"));
		AssertTrue(obj.HasKey("newname"));
		AssertEq(obj.GetInt("newname"), 42);

		delete obj;
	}
	TestEnd();
	
	// Test FromString
	TestStart("Object_FromString");
	{
		YYJSONObject obj = YYJSONObject.FromString("{\"key\":\"value\",\"num\":123}");
		AssertValidHandle(obj);

		char buffer[64];
		AssertTrue(obj.GetString("key", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "value");
		AssertEq(obj.GetInt("num"), 123);

		delete obj;
	}
	TestEnd();
	
	// Test FromStrings
	TestStart("Object_FromStrings");
	{
		char pairs[][] = {"name", "test", "type", "demo", "version", "1.0"};
		YYJSONObject obj = YYJSONObject.FromStrings(pairs, sizeof(pairs));
		AssertValidHandle(obj);
		AssertEq(obj.Size, 3);

		char buffer[64];
		AssertTrue(obj.GetString("name", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "test");

		delete obj;
	}
	TestEnd();
	
	// Test Sort
	TestStart("Object_Sort_Ascending");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.SetInt("zebra", 1);
		obj.SetInt("alpha", 2);
		obj.SetInt("beta", 3);

		AssertTrue(obj.Sort(YYJSON_SORT_ASC));

		char key[32];
		obj.GetKey(0, key, sizeof(key));
		AssertStrEq(key, "alpha");

		delete obj;
	}
	TestEnd();
	
	TestStart("Object_Sort_Descending");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.SetInt("alpha", 1);
		obj.SetInt("beta", 2);
		obj.SetInt("gamma", 3);

		AssertTrue(obj.Sort(YYJSON_SORT_DESC));

		char key[32];
		obj.GetKey(0, key, sizeof(key));
		AssertStrEq(key, "gamma");

		delete obj;
	}
	TestEnd();
	
	// Test Set with handle
	TestStart("Object_SetWithHandle");
	{
		YYJSONObject obj = new YYJSONObject();
		YYJSON val = YYJSON.CreateInt(999);

		AssertTrue(obj.Set("nested", val));
		AssertEq(obj.GetInt("nested"), 999);

		delete val;
		delete obj;
	}
	TestEnd();
	
	// Test Get with handle
	TestStart("Object_GetHandle");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.SetInt("value", 42);

		YYJSON val = obj.Get("value");
		AssertValidHandle(val);
		AssertEq(YYJSON.GetInt(val), 42);

		delete val;
		delete obj;
	}
	TestEnd();
}

// ============================================================================
// 2.3 Array Operations Tests
// ============================================================================

void Test_ArrayOperations()
{
	PrintToServer("\n[Category] Array Operations Tests");
	
	// Test array creation
	TestStart("Array_Constructor");
	{
		YYJSONArray arr = new YYJSONArray();
		AssertValidHandle(arr);
		AssertTrue(arr.IsArray);
		AssertEq(arr.Length, 0);
		delete arr;
	}
	TestEnd();
	
	// Test Push methods
	TestStart("Array_PushInt");
	{
		YYJSONArray arr = new YYJSONArray();
		AssertTrue(arr.PushInt(42));
		AssertEq(arr.Length, 1);
		AssertEq(arr.GetInt(0), 42);
		delete arr;
	}
	TestEnd();
	
	TestStart("Array_PushFloat");
	{
		YYJSONArray arr = new YYJSONArray();
		AssertTrue(arr.PushFloat(3.14));
		AssertFloatEq(arr.GetFloat(0), 3.14);
		delete arr;
	}
	TestEnd();
	
	TestStart("Array_PushBool");
	{
		YYJSONArray arr = new YYJSONArray();
		AssertTrue(arr.PushBool(true));
		AssertTrue(arr.GetBool(0));
		delete arr;
	}
	TestEnd();
	
	TestStart("Array_PushString");
	{
		YYJSONArray arr = new YYJSONArray();
		AssertTrue(arr.PushString("test"));
		char buffer[64];
		AssertTrue(arr.GetString(0, buffer, sizeof(buffer)));
		AssertStrEq(buffer, "test");
		delete arr;
	}
	TestEnd();
	
	TestStart("Array_PushInt64");
	{
		YYJSONArray arr = new YYJSONArray();
		AssertTrue(arr.PushInt64("9223372036854775807"));
		char buffer[32];
		arr.GetInt64(0, buffer, sizeof(buffer));
		AssertStrEq(buffer, "9223372036854775807");
		delete arr;
	}
	TestEnd();
	
	TestStart("Array_PushNull");
	{
		YYJSONArray arr = new YYJSONArray();
		AssertTrue(arr.PushNull());
		AssertTrue(arr.IsNull(0));
		delete arr;
	}
	TestEnd();
	
	// Test Set/Get methods
	TestStart("Array_SetGetInt");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushInt(0);
		AssertTrue(arr.SetInt(0, 100));
		AssertEq(arr.GetInt(0), 100);
		delete arr;
	}
	TestEnd();
	
	TestStart("Array_SetGetFloat");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushFloat(0.0);
		AssertTrue(arr.SetFloat(0, 2.718));
		AssertFloatEq(arr.GetFloat(0), 2.718);
		delete arr;
	}
	TestEnd();
	
	// Test Remove methods
	TestStart("Array_Remove");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushInt(1);
		arr.PushInt(2);
		arr.PushInt(3);
		AssertEq(arr.Length, 3);

		AssertTrue(arr.Remove(1));
		AssertEq(arr.Length, 2);
		AssertEq(arr.GetInt(0), 1);
		AssertEq(arr.GetInt(1), 3);

		delete arr;
	}
	TestEnd();
	
	TestStart("Array_RemoveFirst");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushInt(1);
		arr.PushInt(2);
		arr.PushInt(3);

		AssertTrue(arr.RemoveFirst());
		AssertEq(arr.Length, 2);
		AssertEq(arr.GetInt(0), 2);

		delete arr;
	}
	TestEnd();
	
	TestStart("Array_RemoveLast");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushInt(1);
		arr.PushInt(2);
		arr.PushInt(3);

		AssertTrue(arr.RemoveLast());
		AssertEq(arr.Length, 2);
		AssertEq(arr.GetInt(1), 2);

		delete arr;
	}
	TestEnd();
	
	TestStart("Array_RemoveRange");
	{
		YYJSONArray arr = new YYJSONArray();
		for (int i = 0; i < 10; i++)
		{
			arr.PushInt(i);
		}

		AssertTrue(arr.RemoveRange(2, 5));
		AssertEq(arr.Length, 5);

		delete arr;
	}
	TestEnd();
	
	TestStart("Array_Clear");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushInt(1);
		arr.PushInt(2);
		arr.PushInt(3);

		AssertTrue(arr.Clear());
		AssertEq(arr.Length, 0);

		delete arr;
	}
	TestEnd();
	
	// Test First/Last properties
	TestStart("Array_FirstLast");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushInt(10);
		arr.PushInt(20);
		arr.PushInt(30);

		YYJSON first = arr.First;
		YYJSON last = arr.Last;

		AssertValidHandle(first);
		AssertValidHandle(last);
		AssertEq(YYJSON.GetInt(first), 10);
		AssertEq(YYJSON.GetInt(last), 30);

		delete first;
		delete last;
		delete arr;
	}
	TestEnd();
	
	// Test IndexOf methods
	TestStart("Array_IndexOfInt");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushInt(10);
		arr.PushInt(20);
		arr.PushInt(30);

		AssertEq(arr.IndexOfInt(20), 1);
		AssertEq(arr.IndexOfInt(999), -1);

		delete arr;
	}
	TestEnd();
	
	TestStart("Array_IndexOfBool");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushBool(false);
		arr.PushBool(true);
		arr.PushBool(false);

		AssertEq(arr.IndexOfBool(true), 1);

		delete arr;
	}
	TestEnd();
	
	TestStart("Array_IndexOfString");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushString("apple");
		arr.PushString("banana");
		arr.PushString("cherry");

		AssertEq(arr.IndexOfString("banana"), 1);
		AssertEq(arr.IndexOfString("orange"), -1);

		delete arr;
	}
	TestEnd();
	
	TestStart("Array_IndexOfFloat");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushFloat(1.1);
		arr.PushFloat(2.2);
		arr.PushFloat(3.3);

		AssertEq(arr.IndexOfFloat(2.2), 1);

		delete arr;
	}
	TestEnd();
	
	TestStart("Array_IndexOfInt64");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushInt64("1000000000000");
		arr.PushInt64("2000000000000");

		AssertEq(arr.IndexOfInt64("2000000000000"), 1);
		AssertEq(arr.IndexOfInt64("3000000000000"), -1);

		delete arr;
	}
	TestEnd();
	
	// Test FromString
	TestStart("Array_FromString");
	{
		YYJSONArray arr = YYJSONArray.FromString("[1,2,3,4,5]");
		AssertValidHandle(arr);
		AssertEq(arr.Length, 5);
		AssertEq(arr.GetInt(0), 1);
		AssertEq(arr.GetInt(4), 5);
		delete arr;
	}
	TestEnd();
	
	// Test FromStrings
	TestStart("Array_FromStrings");
	{
		char strings[][] = {"apple", "banana", "cherry"};
		YYJSONArray arr = YYJSONArray.FromStrings(strings, sizeof(strings));
		AssertValidHandle(arr);
		AssertEq(arr.Length, 3);

		char buffer[64];
		arr.GetString(1, buffer, sizeof(buffer));
		AssertStrEq(buffer, "banana");

		delete arr;
	}
	TestEnd();
	
	// Test Sort
	TestStart("Array_Sort_Ascending");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushInt(3);
		arr.PushInt(1);
		arr.PushInt(4);
		arr.PushInt(1);
		arr.PushInt(5);

		AssertTrue(arr.Sort(YYJSON_SORT_ASC));
		AssertEq(arr.GetInt(0), 1);
		AssertEq(arr.GetInt(4), 5);

		delete arr;
	}
	TestEnd();
	
	TestStart("Array_Sort_Descending");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushInt(1);
		arr.PushInt(2);
		arr.PushInt(3);
		arr.PushInt(4);
		arr.PushInt(5);

		AssertTrue(arr.Sort(YYJSON_SORT_DESC));
		AssertEq(arr.GetInt(0), 5);
		AssertEq(arr.GetInt(4), 1);

		delete arr;
	}
	TestEnd();
	
	// Test Push with handle
	TestStart("Array_PushHandle");
	{
		YYJSONArray arr = new YYJSONArray();
		YYJSON val = YYJSON.CreateInt(999);

		AssertTrue(arr.Push(val));
		AssertEq(arr.GetInt(0), 999);

		delete val;
		delete arr;
	}
	TestEnd();
	
	// Test Get with handle
	TestStart("Array_GetHandle");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushInt(42);

		YYJSON val = arr.Get(0);
		AssertValidHandle(val);
		AssertEq(YYJSON.GetInt(val), 42);

		delete val;
		delete arr;
	}
	TestEnd();
}

// ============================================================================
// 2.4 Parse & Serialize Tests
// ============================================================================

void Test_ParseAndSerialize()
{
	PrintToServer("\n[Category] Parse & Serialize Tests");
	
	// Test Parse string
	TestStart("Parse_SimpleObject");
	{
		YYJSON json = YYJSON.Parse("{\"key\":\"value\",\"num\":42}");
		AssertValidHandle(json);
		AssertTrue(json.IsObject);
		delete json;
	}
	TestEnd();
	
	TestStart("Parse_SimpleArray");
	{
		YYJSON json = YYJSON.Parse("[1,2,3,4,5]");
		AssertValidHandle(json);
		AssertTrue(json.IsArray);
		delete json;
	}
	TestEnd();
	
	TestStart("Parse_NestedStructure");
	{
		YYJSON json = YYJSON.Parse("{\"user\":{\"name\":\"test\",\"age\":25},\"items\":[1,2,3]}");
		AssertValidHandle(json);
		delete json;
	}
	TestEnd();
	
	// Test mutable/immutable
	TestStart("Parse_ImmutableDocument");
	{
		YYJSON json = YYJSON.Parse("{\"key\":\"value\"}");
		AssertValidHandle(json);
		AssertTrue(json.IsImmutable);
		AssertFalse(json.IsMutable);
		delete json;
	}
	TestEnd();
	
	TestStart("Parse_MutableDocument");
	{
		YYJSON json = YYJSON.Parse("{\"key\":\"value\"}", false, true);
		AssertValidHandle(json);
		AssertTrue(json.IsMutable);
		AssertFalse(json.IsImmutable);
		delete json;
	}
	TestEnd();
	
	// Test ToString
	TestStart("Serialize_ToString");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.SetInt("num", 42);
		obj.SetString("str", "test");

		char buffer[256];
		int len = obj.ToString(buffer, sizeof(buffer));
		AssertTrue(len > 0);
		Assert(StrContains(buffer, "num") != -1);
		Assert(StrContains(buffer, "test") != -1);

		delete obj;
	}
	TestEnd();
	
	TestStart("Serialize_ToString_Pretty");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.SetInt("a", 1);
		obj.SetInt("b", 2);

		char buffer[256];
		int len = obj.ToString(buffer, sizeof(buffer), YYJSON_WRITE_PRETTY);
		AssertTrue(len > 0);
		Assert(StrContains(buffer, "\n") != -1, "Pretty output should contain newlines");

		delete obj;
	}
	TestEnd();
	
	// Test GetSerializedSize
	TestStart("Serialize_GetSerializedSize");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.SetInt("test", 123);

		int size = obj.GetSerializedSize();
		AssertTrue(size > 0);

		char[] buffer = new char[size];
		int written = obj.ToString(buffer, size);
		AssertEq(written, size);

		delete obj;
	}
	TestEnd();
	
	// Test read flags
	TestStart("Parse_WithTrailingCommas");
	{
		YYJSON json = YYJSON.Parse("[1,2,3,]", false, false, YYJSON_READ_ALLOW_TRAILING_COMMAS);
		AssertValidHandle(json);
		delete json;
	}
	TestEnd();
	
	TestStart("Parse_WithComments");
	{
		YYJSON json = YYJSON.Parse("/* comment */ {\"key\":\"value\"}", false, false, YYJSON_READ_ALLOW_COMMENTS);
		AssertValidHandle(json);
		delete json;
	}
	TestEnd();
	
	// Test file operations (create temporary test file)
	TestStart("Parse_ToFile_FromFile");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.SetInt("filetest", 999);
		obj.SetString("name", "testfile");

		// Write to file
		AssertTrue(obj.ToFile("yyjson_test_temp.json"));

		// Read from file
		YYJSONObject loaded = YYJSON.Parse("yyjson_test_temp.json", true);
		AssertValidHandle(loaded);

		// Verify content
		YYJSONObject loadedObj = loaded;
		AssertEq(loadedObj.GetInt("filetest"), 999);

		char buffer[64];
		loadedObj.GetString("name", buffer, sizeof(buffer));
		AssertStrEq(buffer, "testfile");

		delete obj;
		delete loaded;

		// Cleanup
		DeleteFile("yyjson_test_temp.json");
	}
	TestEnd();
	
	// Test round-trip serialization
	TestStart("Parse_RoundTrip");
	{
		char original[] = "{\"int\":42,\"float\":3.14,\"bool\":true,\"str\":\"test\",\"null\":null}";
		YYJSON json1 = YYJSON.Parse(original);

		char buffer[256];
		json1.ToString(buffer, sizeof(buffer));

		YYJSON json2 = YYJSON.Parse(buffer);
		AssertTrue(YYJSON.Equals(json1, json2));

		delete json1;
		delete json2;
	}
	TestEnd();
}

// ============================================================================
// 2.5 Iterator Tests
// ============================================================================

void Test_Iterators()
{
	PrintToServer("\n[Category] Iterator Tests");
	
	// Test ForeachObject
	TestStart("Iterator_ForeachObject");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.SetInt("a", 1);
		obj.SetInt("b", 2);
		obj.SetInt("c", 3);

		int count = 0;
		char key[32];
		YYJSON value;

		while (obj.ForeachObject(key, sizeof(key), value))
		{
			count++;
			AssertValidHandle(value);
			delete value;
		}

		AssertEq(count, 3);
		delete obj;
	}
	TestEnd();
	
	// Test ForeachArray
	TestStart("Iterator_ForeachArray");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushInt(10);
		arr.PushInt(20);
		arr.PushInt(30);

		int count = 0;
		int index;
		YYJSON value;

		while (arr.ForeachArray(index, value))
		{
			AssertEq(index, count);
			AssertValidHandle(value);
			delete value;
			count++;
		}

		AssertEq(count, 3);
		delete arr;
	}
	TestEnd();
	
	// Test ForeachKey
	TestStart("Iterator_ForeachKey");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.SetInt("key1", 1);
		obj.SetInt("key2", 2);
		obj.SetInt("key3", 3);

		int count = 0;
		char key[32];

		while (obj.ForeachKey(key, sizeof(key)))
		{
			AssertTrue(strlen(key) > 0);
			count++;
		}

		AssertEq(count, 3);
		delete obj;
	}
	TestEnd();
	
	// Test ForeachIndex
	TestStart("Iterator_ForeachIndex");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushInt(1);
		arr.PushInt(2);
		arr.PushInt(3);

		int count = 0;
		int index;

		while (arr.ForeachIndex(index))
		{
			AssertEq(index, count);
			count++;
		}

		AssertEq(count, 3);
		delete arr;
	}
	TestEnd();
	
	// Test empty iterations
	TestStart("Iterator_EmptyObject");
	{
		YYJSONObject obj = new YYJSONObject();

		char key[32];
		YYJSON value;
		AssertFalse(obj.ForeachObject(key, sizeof(key), value));

		delete obj;
	}
	TestEnd();
	
	TestStart("Iterator_EmptyArray");
	{
		YYJSONArray arr = new YYJSONArray();

		int index;
		YYJSON value;
		AssertFalse(arr.ForeachArray(index, value));

		delete arr;
	}
	TestEnd();
}

// ============================================================================
// 2.6 JSON Pointer Tests
// ============================================================================

void Test_JSONPointer()
{
	PrintToServer("\n[Category] JSON Pointer Tests");
	
	// Test PtrSet methods
	TestStart("Pointer_PtrSetInt");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertTrue(obj.PtrSetInt("/number", 42));
		AssertEq(obj.PtrGetInt("/number"), 42);
		delete obj;
	}
	TestEnd();
	
	TestStart("Pointer_PtrSetFloat");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertTrue(obj.PtrSetFloat("/pi", 3.14159));
		AssertFloatEq(obj.PtrGetFloat("/pi"), 3.14159);
		delete obj;
	}
	TestEnd();
	
	TestStart("Pointer_PtrSetBool");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertTrue(obj.PtrSetBool("/flag", true));
		AssertTrue(obj.PtrGetBool("/flag"));
		delete obj;
	}
	TestEnd();
	
	TestStart("Pointer_PtrSetString");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertTrue(obj.PtrSetString("/name", "test"));

		char buffer[64];
		AssertTrue(obj.PtrGetString("/name", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "test");

		delete obj;
	}
	TestEnd();
	
	TestStart("Pointer_PtrSetInt64");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertTrue(obj.PtrSetInt64("/bignum", "9223372036854775807"));

		char buffer[32];
		AssertTrue(obj.PtrGetInt64("/bignum", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "9223372036854775807");

		delete obj;
	}
	TestEnd();
	
	TestStart("Pointer_PtrSetNull");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertTrue(obj.PtrSetNull("/nullable"));
		AssertTrue(obj.PtrGetIsNull("/nullable"));
		delete obj;
	}
	TestEnd();
	
	// Test nested path creation
	TestStart("Pointer_NestedPathCreation");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertTrue(obj.PtrSetInt("/a/b/c/d", 123));
		AssertEq(obj.PtrGetInt("/a/b/c/d"), 123);
		delete obj;
	}
	TestEnd();
	
	// Test PtrGet with handle
	TestStart("Pointer_PtrGet");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.PtrSetInt("/test", 999);

		YYJSON val = obj.PtrGet("/test");
		AssertValidHandle(val);
		AssertEq(YYJSON.GetInt(val), 999);

		delete val;
		delete obj;
	}
	TestEnd();
	
	// Test PtrAdd methods
	TestStart("Pointer_PtrAddInt");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.PtrSetInt("/arr/0", 1);
		AssertTrue(obj.PtrAddInt("/arr/1", 2));
		AssertEq(obj.PtrGetInt("/arr/1"), 2);
		delete obj;
	}
	TestEnd();
	
	TestStart("Pointer_PtrAddString");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.PtrSetString("/items/0", "first");
		AssertTrue(obj.PtrAddString("/items/1", "second"));

		char buffer[64];
		obj.PtrGetString("/items/1", buffer, sizeof(buffer));
		AssertStrEq(buffer, "second");

		delete obj;
	}
	TestEnd();
	
	// Test PtrRemove
	TestStart("Pointer_PtrRemove");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.PtrSetInt("/remove_me", 1);
		obj.PtrSetInt("/keep_me", 2);

		AssertTrue(obj.PtrRemove("/remove_me"));

		YYJSON val;
		obj.PtrTryGetVal("/remove_me", val);
		AssertNullHandle(val);

		delete obj;
	}
	TestEnd();
	
	// Test PtrGetLength
	TestStart("Pointer_PtrGetLength");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.PtrSetString("/text", "hello");

		int len = obj.PtrGetLength("/text");
		AssertEq(len, 6); // Including null terminator

		delete obj;
	}
	TestEnd();
	
	// Test PtrTryGet methods
	TestStart("Pointer_PtrTryGetInt");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.PtrSetInt("/value", 42);

		int value;
		AssertTrue(obj.PtrTryGetInt("/value", value));
		AssertEq(value, 42);

		AssertFalse(obj.PtrTryGetInt("/nonexistent", value));

		delete obj;
	}
	TestEnd();
	
	TestStart("Pointer_PtrTryGetBool");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.PtrSetBool("/flag", true);

		bool value;
		AssertTrue(obj.PtrTryGetBool("/flag", value));
		AssertTrue(value);

		delete obj;
	}
	TestEnd();
	
	TestStart("Pointer_PtrTryGetFloat");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.PtrSetFloat("/pi", 3.14);

		float value;
		AssertTrue(obj.PtrTryGetFloat("/pi", value));
		AssertFloatEq(value, 3.14);

		delete obj;
	}
	TestEnd();
	
	TestStart("Pointer_PtrTryGetString");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.PtrSetString("/name", "test");

		char buffer[64];
		AssertTrue(obj.PtrTryGetString("/name", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "test");

		delete obj;
	}
	TestEnd();
	
	TestStart("Pointer_PtrTryGetInt64");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.PtrSetInt64("/bignum", "123456789012345");

		char buffer[32];
		AssertTrue(obj.PtrTryGetInt64("/bignum", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "123456789012345");

		delete obj;
	}
	TestEnd();
	
	TestStart("Pointer_PtrTryGetVal");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.PtrSetInt("/test", 42);

		YYJSON value;
		AssertTrue(obj.PtrTryGetVal("/test", value));
		AssertValidHandle(value);
		AssertEq(YYJSON.GetInt(value), 42);

		delete value;
		delete obj;
	}
	TestEnd();
}

// ============================================================================
// 2.7 Advanced Features Tests
// ============================================================================

void Test_AdvancedFeatures()
{
	PrintToServer("\n[Category] Advanced Features Tests");
	
	// Test DeepCopy
	TestStart("Advanced_DeepCopy_Object");
	{
		YYJSONObject original = new YYJSONObject();
		original.SetInt("num", 42);
		original.SetString("str", "test");

		YYJSONObject target = new YYJSONObject();
		YYJSONObject copy = YYJSON.DeepCopy(target, original);

		AssertValidHandle(copy);
		AssertEq(copy.GetInt("num"), 42);

		char buffer[64];
		copy.GetString("str", buffer, sizeof(buffer));
		AssertStrEq(buffer, "test");

		delete original;
		delete target;
		delete copy;
	}
	TestEnd();
	
	TestStart("Advanced_DeepCopy_Array");
	{
		YYJSONArray original = new YYJSONArray();
		original.PushInt(1);
		original.PushInt(2);
		original.PushInt(3);

		YYJSONArray target = new YYJSONArray();
		YYJSONArray copy = YYJSON.DeepCopy(target, original);

		AssertValidHandle(copy);
		AssertEq(copy.Length, 3);
		AssertEq(copy.GetInt(0), 1);
		AssertEq(copy.GetInt(2), 3);

		delete original;
		delete target;
		delete copy;
	}
	TestEnd();
	
	// Test Equals
	TestStart("Advanced_Equals_True");
	{
		YYJSONObject obj1 = new YYJSONObject();
		obj1.SetInt("a", 1);
		obj1.SetString("b", "test");

		YYJSONObject obj2 = new YYJSONObject();
		obj2.SetInt("a", 1);
		obj2.SetString("b", "test");

		AssertTrue(YYJSON.Equals(obj1, obj2));

		delete obj1;
		delete obj2;
	}
	TestEnd();
	
	TestStart("Advanced_Equals_False");
	{
		YYJSONObject obj1 = new YYJSONObject();
		obj1.SetInt("a", 1);

		YYJSONObject obj2 = new YYJSONObject();
		obj2.SetInt("a", 2);

		AssertFalse(YYJSON.Equals(obj1, obj2));

		delete obj1;
		delete obj2;
	}
	TestEnd();
	
	// Test ToMutable/ToImmutable
	TestStart("Advanced_ToMutable");
	{
		YYJSON immutable = YYJSON.Parse("{\"key\":\"value\"}");
		AssertTrue(immutable.IsImmutable);

		YYJSON mutable = immutable.ToMutable();
		AssertValidHandle(mutable);
		AssertTrue(mutable.IsMutable);

		delete immutable;
		delete mutable;
	}
	TestEnd();
	
	TestStart("Advanced_ToImmutable");
	{
		YYJSONObject mutable = new YYJSONObject();
		mutable.SetInt("key", 42);
		AssertTrue(mutable.IsMutable);

		YYJSON immutable = mutable.ToImmutable();
		AssertValidHandle(immutable);
		AssertTrue(immutable.IsImmutable);

		delete mutable;
		delete immutable;
	}
	TestEnd();
	
	// Test Pack
	TestStart("Advanced_Pack_SimpleObject");
	{
		YYJSONObject json = YYJSON.Pack("{s:i,s:s,s:b}", 
			"num", 42,
			"str", "test",
			"bool", true
		);

		AssertValidHandle(json);
		AssertTrue(json.IsObject);

		YYJSONObject obj = json;
		AssertEq(obj.GetInt("num"), 42);

		char buffer[64];
		obj.GetString("str", buffer, sizeof(buffer));
		AssertStrEq(buffer, "test");

		AssertTrue(obj.GetBool("bool"));

		delete json;
	}
	TestEnd();
	
	TestStart("Advanced_Pack_Array");
	{
		YYJSONArray json = YYJSON.Pack("[i,i,i]", 1, 2, 3);

		AssertValidHandle(json);
		AssertTrue(json.IsArray);

		YYJSONArray arr = json;
		AssertEq(arr.Length, 3);
		AssertEq(arr.GetInt(0), 1);
		AssertEq(arr.GetInt(2), 3);

		delete json;
	}
	TestEnd();
	
	TestStart("Advanced_Pack_Nested");
	{
		YYJSONObject json = YYJSON.Pack("{s:{s:s,s:i}}", 
			"user",
				"name", "test",
				"age", 25
		);

		AssertValidHandle(json);

		char buffer[64];
		YYJSONObject obj = json;
		obj.PtrGetString("/user/name", buffer, sizeof(buffer));
		AssertStrEq(buffer, "test");
		AssertEq(obj.PtrGetInt("/user/age"), 25);

		delete json;
	}
	TestEnd();
	
	// Test mixed type array sorting
	TestStart("Advanced_MixedTypeSort");
	{
		YYJSONArray json = YYJSON.Parse("[true, 42, \"hello\", 1.5, false]", false, true);
		YYJSONArray arr = json;

		AssertTrue(arr.Sort(YYJSON_SORT_ASC));
		AssertEq(arr.Length, 5);

		delete json;
	}
	TestEnd();
}

// ============================================================================
// 2.8 Edge Cases & Error Handling Tests
// ============================================================================

void Test_EdgeCases()
{
	PrintToServer("\n[Category] Edge Cases & Error Handling Tests");
	
	// Test empty containers
	TestStart("EdgeCase_EmptyObject");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertEq(obj.Size, 0);
		AssertFalse(obj.HasKey("anything"));

		char buffer[64];
		int len = obj.ToString(buffer, sizeof(buffer));
		AssertTrue(len > 0);

		delete obj;
	}
	TestEnd();
	
	TestStart("EdgeCase_EmptyArray");
	{
		YYJSONArray arr = new YYJSONArray();
		AssertEq(arr.Length, 0);
		delete arr;
	}
	TestEnd();
	
	// Test nonexistent keys/indices
	TestStart("EdgeCase_NonexistentKey");
	{
		YYJSONObject obj = new YYJSONObject();
		obj.SetInt("exists", 1);

		AssertTrue(obj.HasKey("exists"));
		AssertFalse(obj.HasKey("notexists"));

		delete obj;
	}
	TestEnd();
	
	TestStart("EdgeCase_ArrayOutOfBounds");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushInt(1);

		// Verify array bounds
		AssertEq(arr.Length, 1);
		AssertEq(arr.GetInt(0), 1);

		delete arr;
	}
	TestEnd();
	
	// Test very long strings
	TestStart("EdgeCase_LongString");
	{
		char longStr[1024];
		for (int i = 0; i < sizeof(longStr) - 1; i++)
		{
			longStr[i] = 'A' + (i % 26);
		}
		longStr[sizeof(longStr) - 1] = '\0';

		YYJSON val = YYJSON.CreateString(longStr);
		AssertValidHandle(val);

		char retrieved[1024];
		AssertTrue(YYJSON.GetString(val, retrieved, sizeof(retrieved)));
		AssertStrEq(retrieved, longStr);

		delete val;
	}
	TestEnd();
	
	// Test deep nesting
	TestStart("EdgeCase_DeepNesting");
	{
		YYJSONObject obj = new YYJSONObject();

		// Create deeply nested structure
		obj.PtrSetInt("/a/b/c/d/e/f/g/h/i/j", 42);
		AssertEq(obj.PtrGetInt("/a/b/c/d/e/f/g/h/i/j"), 42);

		delete obj;
	}
	TestEnd();
	
	// Test large arrays
	TestStart("EdgeCase_LargeArray");
	{
		YYJSONArray arr = new YYJSONArray();

		for (int i = 0; i < 1000; i++)
		{
			arr.PushInt(i);
		}

		AssertEq(arr.Length, 1000);
		AssertEq(arr.GetInt(0), 0);
		AssertEq(arr.GetInt(999), 999);

		delete arr;
	}
	TestEnd();
	
	// Test large objects
	TestStart("EdgeCase_LargeObject");
	{
		YYJSONObject obj = new YYJSONObject();

		char key[32];
		for (int i = 0; i < 100; i++)
		{
			Format(key, sizeof(key), "key_%d", i);
			obj.SetInt(key, i);
		}

		AssertEq(obj.Size, 100);
		AssertEq(obj.GetInt("key_0"), 0);
		AssertEq(obj.GetInt("key_99"), 99);

		delete obj;
	}
	TestEnd();
	
	// Test int64 boundaries
	TestStart("EdgeCase_Int64_MaxValue");
	{
		YYJSON val = YYJSON.CreateInt64("9223372036854775807");
		AssertValidHandle(val);

		char buffer[32];
		YYJSON.GetInt64(val, buffer, sizeof(buffer));
		AssertStrEq(buffer, "9223372036854775807");

		delete val;
	}
	TestEnd();
	
	TestStart("EdgeCase_Int64_MinValue");
	{
		YYJSON val = YYJSON.CreateInt64("-9223372036854775808");
		AssertValidHandle(val);

		char buffer[32];
		YYJSON.GetInt64(val, buffer, sizeof(buffer));
		AssertStrEq(buffer, "-9223372036854775808");

		delete val;
	}
	TestEnd();
	
	// Test special float values
	TestStart("EdgeCase_Float_VerySmall");
	{
		YYJSON val = YYJSON.CreateFloat(0.000001);
		AssertValidHandle(val);
		AssertFloatEq(YYJSON.GetFloat(val), 0.000001);
		delete val;
	}
	TestEnd();
	
	TestStart("EdgeCase_Float_VeryLarge");
	{
		YYJSON val = YYJSON.CreateFloat(999999.999999);
		AssertValidHandle(val);
		AssertFloatEq(YYJSON.GetFloat(val), 999999.999999, "", 0.001);
		delete val;
	}
	TestEnd();
	
	// Test special characters in strings
	TestStart("EdgeCase_SpecialCharacters");
	{
		YYJSON val = YYJSON.CreateString("Line1\nLine2\tTabbed\"Quoted\"");
		AssertValidHandle(val);

		char buffer[128];
		YYJSON.GetString(val, buffer, sizeof(buffer));
		Assert(StrContains(buffer, "Line1") != -1);

		delete val;
	}
	TestEnd();
	
	// Test removing from empty array
	TestStart("EdgeCase_RemoveFromEmptyArray");
	{
		YYJSONArray arr = new YYJSONArray();
		AssertFalse(arr.RemoveFirst());
		AssertFalse(arr.RemoveLast());
		delete arr;
	}
	TestEnd();
	
	// Test clearing already empty containers
	TestStart("EdgeCase_ClearEmpty");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertTrue(obj.Clear());
		AssertEq(obj.Size, 0);

		YYJSONArray arr = new YYJSONArray();
		AssertTrue(arr.Clear());
		AssertEq(arr.Length, 0);

		delete obj;
		delete arr;
	}
	TestEnd();
	
	// Test IndexOf on empty array
	TestStart("EdgeCase_IndexOfEmpty");
	{
		YYJSONArray arr = new YYJSONArray();
		AssertEq(arr.IndexOfInt(42), -1);
		AssertEq(arr.IndexOfString("test"), -1);
		AssertEq(arr.IndexOfBool(true), -1);
		delete arr;
	}
	TestEnd();
	
	// Test pointer to nonexistent path
	TestStart("EdgeCase_PointerNonexistentPath");
	{
		YYJSONObject obj = new YYJSONObject();

		// Note: PtrGet throws exception for nonexistent paths (expected behavior)
		// Use PtrTryGet methods for safe access
		int intVal;
		AssertFalse(obj.PtrTryGetInt("/does/not/exist", intVal));

		delete obj;
	}
	TestEnd();
	
	// Test sorting empty containers
	TestStart("EdgeCase_SortEmpty");
	{
		YYJSONObject obj = new YYJSONObject();
		AssertTrue(obj.Sort());

		YYJSONArray arr = new YYJSONArray();
		AssertTrue(arr.Sort());

		delete obj;
		delete arr;
	}
	TestEnd();
	
	// Test single element operations
	TestStart("EdgeCase_SingleElement");
	{
		YYJSONArray arr = new YYJSONArray();
		arr.PushInt(42);

		AssertTrue(arr.Sort());
		AssertEq(arr.GetInt(0), 42);

		AssertEq(arr.IndexOfInt(42), 0);

		YYJSON first = arr.First;
		YYJSON last = arr.Last;
		AssertEq(YYJSON.GetInt(first), YYJSON.GetInt(last));

		delete first;
		delete last;
		delete arr;
	}
	TestEnd();
}