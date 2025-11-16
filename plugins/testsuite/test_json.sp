#include <sourcemod>
#include <json>

#pragma newdecls required
#pragma semicolon 1

public Plugin myinfo =
{
	name = "JSON Test Suite",
	author = "ProjectSky",
	description = "test suite for JSON functions",
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
	RegServerCmd("test_json", Command_RunTests, "Run JSON test suite");
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
			FormatEx(buffer, sizeof(buffer), "%s (expected %d, got %d)", message, b, a);
		}
		else
		{
			FormatEx(buffer, sizeof(buffer), "Expected %d, got %d", b, a);
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
			FormatEx(buffer, sizeof(buffer), "%s (values should not be equal: %d)", message, a);
		}
		else
		{
			FormatEx(buffer, sizeof(buffer), "Values should not be equal: %d", a);
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
	if (strcmp(a, b) != 0)
	{
		g_bCurrentTestFailed = true;
		char buffer[512];
		if (message[0] != '\0')
		{
			FormatEx(buffer, sizeof(buffer), "%s (expected \"%s\", got \"%s\")", message, b, a);
		}
		else
		{
			FormatEx(buffer, sizeof(buffer), "Expected \"%s\", got \"%s\"", b, a);
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
			FormatEx(buffer, sizeof(buffer), "%s (expected %.4f, got %.4f)", message, b, a);
		}
		else
		{
			FormatEx(buffer, sizeof(buffer), "Expected %.4f, got %.4f", b, a);
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
	PrintToServer("JSON Test Suite");
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
	Test_Int64Operations();
	Test_EdgeCases();

	// Print results
	PrintToServer("========================================");
	PrintToServer("JSON Test Suite Results");
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
		JSON val = JSON.CreateBool(true);
		AssertValidHandle(val);
		AssertTrue(val.GetBool());
		AssertEq(val.Type, JSON_TYPE_BOOL);
		AssertTrue(val.IsBool);
		AssertTrue(val.IsTrue);
		AssertFalse(val.IsFalse);
		delete val;
	}
	TestEnd();

	TestStart("BasicValues_CreateBool_False");
	{
		JSON val = JSON.CreateBool(false);
		AssertValidHandle(val);
		AssertFalse(val.GetBool());
		AssertTrue(val.IsBool);
		AssertFalse(val.IsTrue);
		AssertTrue(val.IsFalse);
		delete val;
	}
	TestEnd();

	// Test CreateInt & GetInt
	TestStart("BasicValues_CreateInt_Positive");
	{
		JSON val = JSON.CreateInt(42);
		AssertValidHandle(val);
		AssertEq(val.GetInt(), 42);
		AssertEq(val.Type, JSON_TYPE_NUM);
		AssertTrue(val.IsInt);
		AssertTrue(val.IsNum);
		delete val;
	}
	TestEnd();

	TestStart("BasicValues_CreateInt_Negative");
	{
		JSON val = JSON.CreateInt(-123);
		AssertValidHandle(val);
		AssertEq(val.GetInt(), -123);
		AssertTrue(val.IsSint);
		delete val;
	}
	TestEnd();

	TestStart("BasicValues_CreateInt_Zero");
	{
		JSON val = JSON.CreateInt(0);
		AssertValidHandle(val);
		AssertEq(val.GetInt(), 0);
		delete val;
	}
	TestEnd();

	// Test CreateFloat & GetFloat
	TestStart("BasicValues_CreateFloat_Positive");
	{
		JSON val = JSON.CreateFloat(3.14159);
		AssertValidHandle(val);
		AssertFloatEq(val.GetFloat(), 3.14159);
		AssertTrue(val.IsFloat);
		AssertTrue(val.IsNum);
		delete val;
	}
	TestEnd();

	TestStart("BasicValues_CreateFloat_Negative");
	{
		JSON val = JSON.CreateFloat(-2.71828);
		AssertValidHandle(val);
		AssertFloatEq(val.GetFloat(), -2.71828);
		delete val;
	}
	TestEnd();

	TestStart("BasicValues_CreateFloat_Zero");
	{
		JSON val = JSON.CreateFloat(0.0);
		AssertValidHandle(val);
		AssertFloatEq(val.GetFloat(), 0.0);
		delete val;
	}
	TestEnd();

	// Test CreateString & GetString
	TestStart("BasicValues_CreateString_Regular");
	{
		JSON val = JSON.CreateString("Hello, World!");
		AssertValidHandle(val);
		char buffer[64];
		AssertTrue(val.GetString(buffer, sizeof(buffer)));
		AssertStrEq(buffer, "Hello, World!");
		AssertEq(val.Type, JSON_TYPE_STR);
		AssertTrue(val.IsStr);
		delete val;
	}
	TestEnd();

	TestStart("BasicValues_CreateString_Empty");
	{
		JSON val = JSON.CreateString("");
		AssertValidHandle(val);
		char buffer[64];
		AssertTrue(val.GetString(buffer, sizeof(buffer)));
		AssertStrEq(buffer, "");
		delete val;
	}
	TestEnd();

	TestStart("BasicValues_CreateString_Unicode");
	{
		JSON val = JSON.CreateString("测试字符串");
		AssertValidHandle(val);
		char buffer[64];
		AssertTrue(val.GetString(buffer, sizeof(buffer)));
		AssertStrEq(buffer, "测试字符串");
		delete val;
	}
	TestEnd();

	// Test CreateInt64 & GetInt64
	TestStart("BasicValues_CreateInt64_Large");
	{
		JSON val = JSON.CreateInt64("9223372036854775807");
		AssertValidHandle(val);
		char buffer[32];
		AssertTrue(val.GetInt64(buffer, sizeof(buffer)));
		AssertStrEq(buffer, "9223372036854775807");
		delete val;
	}
	TestEnd();

	TestStart("BasicValues_CreateInt64_Negative");
	{
		JSON val = JSON.CreateInt64("-9223372036854775808");
		AssertValidHandle(val);
		char buffer[32];
		AssertTrue(val.GetInt64(buffer, sizeof(buffer)));
		AssertStrEq(buffer, "-9223372036854775808");
		delete val;
	}
	TestEnd();

	// Test CreateNull
	TestStart("BasicValues_CreateNull");
	{
		JSON val = JSON.CreateNull();
		AssertValidHandle(val);
		AssertEq(val.Type, JSON_TYPE_NULL);
		AssertTrue(val.IsNull);
		delete val;
	}
	TestEnd();

	// Test GetTypeDesc
	TestStart("BasicValues_GetTypeDesc");
	{
		JSON boolVal = JSON.CreateBool(true);
		JSON intVal = JSON.CreateInt(42);
		JSON floatVal = JSON.CreateFloat(3.14);
		JSON strVal = JSON.CreateString("test");
		JSON nullVal = JSON.CreateNull();

		char buffer[32];

		boolVal.GetTypeDesc(buffer, sizeof(buffer));
		AssertStrEq(buffer, "true");

		intVal.GetTypeDesc(buffer, sizeof(buffer));
		AssertTrue(strcmp(buffer, "uint") == 0 || strcmp(buffer, "sint") == 0);

		floatVal.GetTypeDesc(buffer, sizeof(buffer));
		AssertStrEq(buffer, "real");

		strVal.GetTypeDesc(buffer, sizeof(buffer));
		AssertStrEq(buffer, "string");

		nullVal.GetTypeDesc(buffer, sizeof(buffer));
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
		JSONObject obj = new JSONObject();
		AssertValidHandle(obj);
		AssertTrue(obj.IsObject);
		AssertEq(obj.Size, 0);
		delete obj;
	}
	TestEnd();

	// Test Set/Get methods
	TestStart("Object_SetGetInt");
	{
		JSONObject obj = new JSONObject();
		AssertTrue(obj.SetInt("number", 42));
		AssertEq(obj.GetInt("number"), 42);
		AssertEq(obj.Size, 1);
		delete obj;
	}
	TestEnd();

	TestStart("Object_SetGetFloat");
	{
		JSONObject obj = new JSONObject();
		AssertTrue(obj.SetFloat("pi", 3.14159));
		AssertFloatEq(obj.GetFloat("pi"), 3.14159);
		delete obj;
	}
	TestEnd();

	TestStart("Object_SetGetBool");
	{
		JSONObject obj = new JSONObject();
		AssertTrue(obj.SetBool("flag", true));
		AssertTrue(obj.GetBool("flag"));
		delete obj;
	}
	TestEnd();

	TestStart("Object_SetGetString");
	{
		JSONObject obj = new JSONObject();
		AssertTrue(obj.SetString("name", "test"));
		char buffer[64];
		AssertTrue(obj.GetString("name", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "test");
		delete obj;
	}
	TestEnd();

	TestStart("Object_SetGetInt64");
	{
		JSONObject obj = new JSONObject();
		AssertTrue(obj.SetInt64("bignum", "9223372036854775807"));
		char buffer[32];
		AssertTrue(obj.GetInt64("bignum", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "9223372036854775807");
		delete obj;
	}
	TestEnd();

	TestStart("Object_SetGetNull");
	{
		JSONObject obj = new JSONObject();
		AssertTrue(obj.SetNull("nullable"));
		AssertTrue(obj.IsNull("nullable"));
		delete obj;
	}
	TestEnd();

	// Test HasKey
	TestStart("Object_HasKey");
	{
		JSONObject obj = new JSONObject();
		obj.SetInt("exists", 1);
		AssertTrue(obj.HasKey("exists"));
		AssertFalse(obj.HasKey("notexists"));
		delete obj;
	}
	TestEnd();

	// Test GetKey and GetValueAt
	TestStart("Object_GetKeyAndValueAt");
	{
		JSONObject obj = new JSONObject();
		obj.SetInt("first", 1);
		obj.SetInt("second", 2);

		char key[32];
		AssertTrue(obj.GetKey(0, key, sizeof(key)));
		AssertStrEq(key, "first");

		JSON val = obj.GetValueAt(0);
		AssertValidHandle(val);
		AssertEq(val.GetInt(), 1);

		delete val;
		delete obj;
	}
	TestEnd();

	// Test Remove
	TestStart("Object_Remove");
	{
		JSONObject obj = new JSONObject();
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
		JSONObject obj = new JSONObject();
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
		JSONObject obj = new JSONObject();
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
		JSONObject obj = JSONObject.FromString("{\"key\":\"value\",\"num\":123}");
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
		JSONObject obj = JSONObject.FromStrings(pairs, sizeof(pairs));
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
		JSONObject obj = new JSONObject();
		obj.SetInt("zebra", 1);
		obj.SetInt("alpha", 2);
		obj.SetInt("beta", 3);

		AssertTrue(obj.Sort(JSON_SORT_ASC));

		char key[32];
		obj.GetKey(0, key, sizeof(key));
		AssertStrEq(key, "alpha");

		delete obj;
	}
	TestEnd();

	TestStart("Object_Sort_Descending");
	{
		JSONObject obj = new JSONObject();
		obj.SetInt("alpha", 1);
		obj.SetInt("beta", 2);
		obj.SetInt("gamma", 3);

		AssertTrue(obj.Sort(JSON_SORT_DESC));

		char key[32];
		obj.GetKey(0, key, sizeof(key));
		AssertStrEq(key, "gamma");

		delete obj;
	}
	TestEnd();

	// Test Set with handle
	TestStart("Object_SetWithHandle");
	{
		JSONObject obj = new JSONObject();
		JSON val = JSON.CreateInt(999);

		AssertTrue(obj.Set("nested", val));
		AssertEq(obj.GetInt("nested"), 999);

		delete val;
		delete obj;
	}
	TestEnd();

	// Test Get with handle
	TestStart("Object_GetHandle");
	{
		JSONObject obj = new JSONObject();
		obj.SetInt("value", 42);

		JSON val = obj.Get("value");
		AssertValidHandle(val);
		AssertEq(val.GetInt(), 42);

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
		JSONArray arr = new JSONArray();
		AssertValidHandle(arr);
		AssertTrue(arr.IsArray);
		AssertEq(arr.Length, 0);
		delete arr;
	}
	TestEnd();

	// Test Push methods
	TestStart("Array_PushInt");
	{
		JSONArray arr = new JSONArray();
		AssertTrue(arr.PushInt(42));
		AssertEq(arr.Length, 1);
		AssertEq(arr.GetInt(0), 42);
		delete arr;
	}
	TestEnd();

	TestStart("Array_PushFloat");
	{
		JSONArray arr = new JSONArray();
		AssertTrue(arr.PushFloat(3.14));
		AssertFloatEq(arr.GetFloat(0), 3.14);
		delete arr;
	}
	TestEnd();

	TestStart("Array_PushBool");
	{
		JSONArray arr = new JSONArray();
		AssertTrue(arr.PushBool(true));
		AssertTrue(arr.GetBool(0));
		delete arr;
	}
	TestEnd();

	TestStart("Array_PushString");
	{
		JSONArray arr = new JSONArray();
		AssertTrue(arr.PushString("test"));
		char buffer[64];
		AssertTrue(arr.GetString(0, buffer, sizeof(buffer)));
		AssertStrEq(buffer, "test");
		delete arr;
	}
	TestEnd();

	TestStart("Array_PushInt64");
	{
		JSONArray arr = new JSONArray();
		AssertTrue(arr.PushInt64("9223372036854775807"));
		char buffer[32];
		arr.GetInt64(0, buffer, sizeof(buffer));
		AssertStrEq(buffer, "9223372036854775807");
		delete arr;
	}
	TestEnd();

	TestStart("Array_PushNull");
	{
		JSONArray arr = new JSONArray();
		AssertTrue(arr.PushNull());
		AssertTrue(arr.IsNull(0));
		delete arr;
	}
	TestEnd();

	// Test Set/Get methods
	TestStart("Array_SetGetInt");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt(0);
		AssertTrue(arr.SetInt(0, 100));
		AssertEq(arr.GetInt(0), 100);
		delete arr;
	}
	TestEnd();

	TestStart("Array_SetGetFloat");
	{
		JSONArray arr = new JSONArray();
		arr.PushFloat(0.0);
		AssertTrue(arr.SetFloat(0, 2.718));
		AssertFloatEq(arr.GetFloat(0), 2.718);
		delete arr;
	}
	TestEnd();

	// Test Remove methods
	TestStart("Array_Remove");
	{
		JSONArray arr = new JSONArray();
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
		JSONArray arr = new JSONArray();
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
		JSONArray arr = new JSONArray();
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
		JSONArray arr = new JSONArray();
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
		JSONArray arr = new JSONArray();
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
		JSONArray arr = new JSONArray();
		arr.PushInt(10);
		arr.PushInt(20);
		arr.PushInt(30);

		JSON first = arr.First;
		JSON last = arr.Last;

		AssertValidHandle(first);
		AssertValidHandle(last);
		AssertEq(first.GetInt(), 10);
		AssertEq(last.GetInt(), 30);

		delete first;
		delete last;
		delete arr;
	}
	TestEnd();

	// Test IndexOf methods
	TestStart("Array_IndexOfInt");
	{
		JSONArray arr = new JSONArray();
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
		JSONArray arr = new JSONArray();
		arr.PushBool(false);
		arr.PushBool(true);
		arr.PushBool(false);

		AssertEq(arr.IndexOfBool(true), 1);

		delete arr;
	}
	TestEnd();

	TestStart("Array_IndexOfString");
	{
		JSONArray arr = new JSONArray();
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
		JSONArray arr = new JSONArray();
		arr.PushFloat(1.1);
		arr.PushFloat(2.2);
		arr.PushFloat(3.3);

		AssertEq(arr.IndexOfFloat(2.2), 1);

		delete arr;
	}
	TestEnd();

	TestStart("Array_IndexOfInt64");
	{
		JSONArray arr = new JSONArray();
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
		JSONArray arr = JSONArray.FromString("[1,2,3,4,5]");
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
		JSONArray arr = JSONArray.FromStrings(strings, sizeof(strings));
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
		JSONArray arr = new JSONArray();
		arr.PushInt(3);
		arr.PushInt(1);
		arr.PushInt(4);
		arr.PushInt(1);
		arr.PushInt(5);

		AssertTrue(arr.Sort(JSON_SORT_ASC));
		AssertEq(arr.GetInt(0), 1);
		AssertEq(arr.GetInt(4), 5);

		delete arr;
	}
	TestEnd();

	TestStart("Array_Sort_Descending");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt(1);
		arr.PushInt(2);
		arr.PushInt(3);
		arr.PushInt(4);
		arr.PushInt(5);

		AssertTrue(arr.Sort(JSON_SORT_DESC));
		AssertEq(arr.GetInt(0), 5);
		AssertEq(arr.GetInt(4), 1);

		delete arr;
	}
	TestEnd();

	// Test Push with handle
	TestStart("Array_PushHandle");
	{
		JSONArray arr = new JSONArray();
		JSON val = JSON.CreateInt(999);

		AssertTrue(arr.Push(val));
		AssertEq(arr.GetInt(0), 999);

		delete val;
		delete arr;
	}
	TestEnd();

	// Test Get with handle
	TestStart("Array_GetHandle");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt(42);

		JSON val = arr.Get(0);
		AssertValidHandle(val);
		AssertEq(val.GetInt(), 42);

		delete val;
		delete arr;
	}
	TestEnd();

	// Test FromInt
	TestStart("Array_FromInt");
	{
		int values[] = {10, 20, 30, 40, 50};
		JSONArray arr = JSONArray.FromInt(values, sizeof(values));

		AssertValidHandle(arr);
		AssertEq(arr.Length, 5);
		AssertEq(arr.GetInt(0), 10);
		AssertEq(arr.GetInt(2), 30);
		AssertEq(arr.GetInt(4), 50);

		delete arr;
	}
	TestEnd();

	TestStart("Array_FromInt_Negative");
	{
		int values[] = {-100, -50, 0, 50, 100};
		JSONArray arr = JSONArray.FromInt(values, sizeof(values));

		AssertValidHandle(arr);
		AssertEq(arr.Length, 5);
		AssertEq(arr.GetInt(0), -100);
		AssertEq(arr.GetInt(2), 0);
		AssertEq(arr.GetInt(4), 100);

		delete arr;
	}
	TestEnd();

	// Test FromInt64
	TestStart("Array_FromInt64_Mixed");
	{
		char values[][] = {"9223372036854775807", "-9223372036854775808", "0", "18446744073709551615"};
		JSONArray arr = JSONArray.FromInt64(values, sizeof(values));

		AssertValidHandle(arr);
		AssertEq(arr.Length, 4);

		char buffer[32];
		arr.GetInt64(0, buffer, sizeof(buffer));
		AssertStrEq(buffer, "9223372036854775807");

		arr.GetInt64(1, buffer, sizeof(buffer));
		AssertStrEq(buffer, "-9223372036854775808");

		arr.GetInt64(2, buffer, sizeof(buffer));
		AssertStrEq(buffer, "0");

		arr.GetInt64(3, buffer, sizeof(buffer));
		AssertStrEq(buffer, "18446744073709551615");

		delete arr;
	}
	TestEnd();

	TestStart("Array_FromInt64_LargeValues");
	{
		char values[][] = {"1234567890123456", "9876543210987654", "5555555555555555"};
		JSONArray arr = JSONArray.FromInt64(values, sizeof(values));

		AssertValidHandle(arr);
		AssertEq(arr.Length, 3);

		char buffer[32];
		arr.GetInt64(1, buffer, sizeof(buffer));
		AssertStrEq(buffer, "9876543210987654");

		delete arr;
	}
	TestEnd();

	// Test FromBool
	TestStart("Array_FromBool");
	{
		bool values[] = {true, false, true, true, false};
		JSONArray arr = JSONArray.FromBool(values, sizeof(values));

		AssertValidHandle(arr);
		AssertEq(arr.Length, 5);
		AssertTrue(arr.GetBool(0));
		AssertFalse(arr.GetBool(1));
		AssertTrue(arr.GetBool(2));
		AssertTrue(arr.GetBool(3));
		AssertFalse(arr.GetBool(4));

		delete arr;
	}
	TestEnd();

	TestStart("Array_FromBool_AllTrue");
	{
		bool values[] = {true, true, true};
		JSONArray arr = JSONArray.FromBool(values, sizeof(values));

		AssertValidHandle(arr);
		AssertEq(arr.Length, 3);
		AssertTrue(arr.GetBool(0));
		AssertTrue(arr.GetBool(1));
		AssertTrue(arr.GetBool(2));

		delete arr;
	}
	TestEnd();

	// Test FromFloat
	TestStart("Array_FromFloat");
	{
		float values[] = {1.1, 2.2, 3.3, 4.4, 5.5};
		JSONArray arr = JSONArray.FromFloat(values, sizeof(values));

		AssertValidHandle(arr);
		AssertEq(arr.Length, 5);
		AssertFloatEq(arr.GetFloat(0), 1.1);
		AssertFloatEq(arr.GetFloat(2), 3.3, "", 0.01);
		AssertFloatEq(arr.GetFloat(4), 5.5);

		delete arr;
	}
	TestEnd();

	TestStart("Array_FromFloat_Negative");
	{
		float values[] = {-3.14, 0.0, 2.718, -1.414};
		JSONArray arr = JSONArray.FromFloat(values, sizeof(values));

		AssertValidHandle(arr);
		AssertEq(arr.Length, 4);
		AssertFloatEq(arr.GetFloat(0), -3.14);
		AssertFloatEq(arr.GetFloat(1), 0.0);
		AssertFloatEq(arr.GetFloat(2), 2.718);

		delete arr;
	}
	TestEnd();

	// Test Insert methods
	TestStart("Array_InsertInt");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt(1);
		arr.PushInt(3);
		arr.PushInt(4);

		AssertTrue(arr.InsertInt(1, 2));
		AssertEq(arr.Length, 4);
		AssertEq(arr.GetInt(0), 1);
		AssertEq(arr.GetInt(1), 2);
		AssertEq(arr.GetInt(2), 3);
		AssertEq(arr.GetInt(3), 4);

		delete arr;
	}
	TestEnd();

	TestStart("Array_InsertBool");
	{
		JSONArray arr = new JSONArray();
		arr.PushBool(true);
		arr.PushBool(true);

		AssertTrue(arr.InsertBool(1, false));
		AssertEq(arr.Length, 3);
		AssertTrue(arr.GetBool(0));
		AssertFalse(arr.GetBool(1));
		AssertTrue(arr.GetBool(2));

		delete arr;
	}
	TestEnd();

	TestStart("Array_InsertFloat");
	{
		JSONArray arr = new JSONArray();
		arr.PushFloat(1.1);
		arr.PushFloat(3.3);

		AssertTrue(arr.InsertFloat(1, 2.2));
		AssertEq(arr.Length, 3);
		AssertFloatEq(arr.GetFloat(0), 1.1);
		AssertFloatEq(arr.GetFloat(1), 2.2);
		AssertFloatEq(arr.GetFloat(2), 3.3, "", 0.01);

		delete arr;
	}
	TestEnd();

	TestStart("Array_InsertString");
	{
		JSONArray arr = new JSONArray();
		arr.PushString("first");
		arr.PushString("third");

		AssertTrue(arr.InsertString(1, "second"));
		AssertEq(arr.Length, 3);

		char buffer[64];
		arr.GetString(0, buffer, sizeof(buffer));
		AssertStrEq(buffer, "first");

		arr.GetString(1, buffer, sizeof(buffer));
		AssertStrEq(buffer, "second");

		arr.GetString(2, buffer, sizeof(buffer));
		AssertStrEq(buffer, "third");

		delete arr;
	}
	TestEnd();

	TestStart("Array_InsertInt64");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt64("1111111111111111");
		arr.PushInt64("3333333333333333");

		AssertTrue(arr.InsertInt64(1, "2222222222222222"));
		AssertEq(arr.Length, 3);

		char buffer[32];
		arr.GetInt64(0, buffer, sizeof(buffer));
		AssertStrEq(buffer, "1111111111111111");

		arr.GetInt64(1, buffer, sizeof(buffer));
		AssertStrEq(buffer, "2222222222222222");

		arr.GetInt64(2, buffer, sizeof(buffer));
		AssertStrEq(buffer, "3333333333333333");

		delete arr;
	}
	TestEnd();

	TestStart("Array_InsertNull");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt(1);
		arr.PushInt(2);

		AssertTrue(arr.InsertNull(1));
		AssertEq(arr.Length, 3);
		AssertEq(arr.GetInt(0), 1);
		AssertTrue(arr.IsNull(1));
		AssertEq(arr.GetInt(2), 2);

		delete arr;
	}
	TestEnd();

	TestStart("Array_InsertAtBeginning");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt(2);
		arr.PushInt(3);

		AssertTrue(arr.InsertInt(0, 1));
		AssertEq(arr.Length, 3);
		AssertEq(arr.GetInt(0), 1);
		AssertEq(arr.GetInt(1), 2);
		AssertEq(arr.GetInt(2), 3);

		delete arr;
	}
	TestEnd();

	TestStart("Array_InsertAtEnd");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt(1);
		arr.PushInt(2);

		AssertTrue(arr.InsertInt(2, 3));
		AssertEq(arr.Length, 3);
		AssertEq(arr.GetInt(0), 1);
		AssertEq(arr.GetInt(1), 2);
		AssertEq(arr.GetInt(2), 3);

		delete arr;
	}
	TestEnd();

	TestStart("Array_InsertHandle");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt(1);
		arr.PushInt(3);

		JSON val = JSON.CreateInt(2);
		AssertTrue(arr.Insert(1, val));

		AssertEq(arr.Length, 3);
		AssertEq(arr.GetInt(0), 1);
		AssertEq(arr.GetInt(1), 2);
		AssertEq(arr.GetInt(2), 3);

		delete val;
		delete arr;
	}
	TestEnd();

	// Test Prepend methods
	TestStart("Array_PrependInt");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt(2);
		arr.PushInt(3);

		AssertTrue(arr.PrependInt(1));
		AssertEq(arr.Length, 3);
		AssertEq(arr.GetInt(0), 1);
		AssertEq(arr.GetInt(1), 2);
		AssertEq(arr.GetInt(2), 3);

		delete arr;
	}
	TestEnd();

	TestStart("Array_PrependBool");
	{
		JSONArray arr = new JSONArray();
		arr.PushBool(false);
		arr.PushBool(false);

		AssertTrue(arr.PrependBool(true));
		AssertEq(arr.Length, 3);
		AssertTrue(arr.GetBool(0));
		AssertFalse(arr.GetBool(1));
		AssertFalse(arr.GetBool(2));

		delete arr;
	}
	TestEnd();

	TestStart("Array_PrependFloat");
	{
		JSONArray arr = new JSONArray();
		arr.PushFloat(2.2);
		arr.PushFloat(3.3);

		AssertTrue(arr.PrependFloat(1.1));
		AssertEq(arr.Length, 3);
		AssertFloatEq(arr.GetFloat(0), 1.1);
		AssertFloatEq(arr.GetFloat(1), 2.2);

		delete arr;
	}
	TestEnd();

	TestStart("Array_PrependString");
	{
		JSONArray arr = new JSONArray();
		arr.PushString("second");
		arr.PushString("third");

		AssertTrue(arr.PrependString("first"));
		AssertEq(arr.Length, 3);

		char buffer[64];
		arr.GetString(0, buffer, sizeof(buffer));
		AssertStrEq(buffer, "first");

		arr.GetString(1, buffer, sizeof(buffer));
		AssertStrEq(buffer, "second");

		delete arr;
	}
	TestEnd();

	TestStart("Array_PrependInt64");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt64("2222222222222222");
		arr.PushInt64("3333333333333333");

		AssertTrue(arr.PrependInt64("1111111111111111"));
		AssertEq(arr.Length, 3);

		char buffer[32];
		arr.GetInt64(0, buffer, sizeof(buffer));
		AssertStrEq(buffer, "1111111111111111");

		arr.GetInt64(1, buffer, sizeof(buffer));
		AssertStrEq(buffer, "2222222222222222");

		delete arr;
	}
	TestEnd();

	TestStart("Array_PrependNull");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt(1);
		arr.PushInt(2);

		AssertTrue(arr.PrependNull());
		AssertEq(arr.Length, 3);
		AssertTrue(arr.IsNull(0));
		AssertEq(arr.GetInt(1), 1);
		AssertEq(arr.GetInt(2), 2);

		delete arr;
	}
	TestEnd();

	TestStart("Array_PrependHandle");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt(2);
		arr.PushInt(3);

		JSON val = JSON.CreateInt(1);
		AssertTrue(arr.Prepend(val));

		AssertEq(arr.Length, 3);
		AssertEq(arr.GetInt(0), 1);
		AssertEq(arr.GetInt(1), 2);

		delete val;
		delete arr;
	}
	TestEnd();

	TestStart("Array_PrependToEmpty");
	{
		JSONArray arr = new JSONArray();

		AssertTrue(arr.PrependInt(1));
		AssertEq(arr.Length, 1);
		AssertEq(arr.GetInt(0), 1);

		delete arr;
	}
	TestEnd();

	// Test combined Insert and Prepend
	TestStart("Array_CombinedInsertPrepend");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt(5);
		arr.PrependInt(1);
		arr.InsertInt(1, 3);
		arr.InsertInt(1, 2);
		arr.InsertInt(3, 4);

		AssertEq(arr.Length, 5);
		AssertEq(arr.GetInt(0), 1);
		AssertEq(arr.GetInt(1), 2);
		AssertEq(arr.GetInt(2), 3);
		AssertEq(arr.GetInt(3), 4);
		AssertEq(arr.GetInt(4), 5);

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
		JSON json = JSON.Parse("{\"key\":\"value\",\"num\":42}");
		AssertValidHandle(json);
		AssertTrue(json.IsObject);
		delete json;
	}
	TestEnd();

	TestStart("Parse_SimpleArray");
	{
		JSON json = JSON.Parse("[1,2,3,4,5]");
		AssertValidHandle(json);
		AssertTrue(json.IsArray);
		delete json;
	}
	TestEnd();

	TestStart("Parse_NestedStructure");
	{
		JSON json = JSON.Parse("{\"user\":{\"name\":\"test\",\"age\":25},\"items\":[1,2,3]}");
		AssertValidHandle(json);
		delete json;
	}
	TestEnd();

	// Test mutable/immutable
	TestStart("Parse_ImmutableDocument");
	{
		JSON json = JSON.Parse("{\"key\":\"value\"}");
		AssertValidHandle(json);
		AssertTrue(json.IsImmutable);
		AssertFalse(json.IsMutable);
		delete json;
	}
	TestEnd();

	TestStart("Parse_MutableDocument");
	{
		JSON json = JSON.Parse("{\"key\":\"value\"}", .is_mutable_doc = true);
		AssertValidHandle(json);
		AssertTrue(json.IsMutable);
		AssertFalse(json.IsImmutable);
		delete json;
	}
	TestEnd();

	// Test ToString
	TestStart("Serialize_ToString");
	{
		JSONObject obj = new JSONObject();
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
		JSONObject obj = new JSONObject();
		obj.SetInt("a", 1);
		obj.SetInt("b", 2);

		char buffer[256];
		int len = obj.ToString(buffer, sizeof(buffer), JSON_WRITE_PRETTY);
		AssertTrue(len > 0);
		Assert(StrContains(buffer, "\n") != -1, "Pretty output should contain newlines");

		delete obj;
	}
	TestEnd();

	// Test GetSerializedSize
	TestStart("Serialize_GetSerializedSize");
	{
		JSONObject obj = new JSONObject();
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
		JSON json = JSON.Parse("[1,2,3,]", .flag = JSON_READ_ALLOW_TRAILING_COMMAS);
		AssertValidHandle(json);
		delete json;
	}
	TestEnd();

	TestStart("Parse_WithComments");
	{
		JSON json = JSON.Parse("/* comment */ {\"key\":\"value\"}", .flag = JSON_READ_ALLOW_COMMENTS);
		AssertValidHandle(json);
		delete json;
	}
	TestEnd();

	// Test file operations (create temporary test file)
	TestStart("Parse_ToFile_FromFile");
	{
		JSONObject obj = new JSONObject();
		obj.SetInt("filetest", 999);
		obj.SetString("name", "testfile");

		// Write to file
		AssertTrue(obj.ToFile("json_test_temp.json"));

		// Read from file
		JSONObject loaded = JSON.Parse("json_test_temp.json", true);
		AssertValidHandle(loaded);

		// Verify content
		JSONObject loadedObj = loaded;
		AssertEq(loadedObj.GetInt("filetest"), 999);

		char buffer[64];
		loadedObj.GetString("name", buffer, sizeof(buffer));
		AssertStrEq(buffer, "testfile");

		delete obj;
		delete loaded;

		// Cleanup
		DeleteFile("json_test_temp.json");
	}
	TestEnd();

	// Test round-trip serialization
	TestStart("Parse_RoundTrip");
	{
		char original[] = "{\"int\":42,\"float\":3.14,\"bool\":true,\"str\":\"test\",\"null\":null}";
		JSON json1 = JSON.Parse(original);

		char buffer[256];
		json1.ToString(buffer, sizeof(buffer));

		JSON json2 = JSON.Parse(buffer);
		AssertTrue(JSON.Equals(json1, json2));

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
		JSONObject obj = new JSONObject();
		obj.SetInt("a", 1);
		obj.SetInt("b", 2);
		obj.SetInt("c", 3);

		int count = 0;
		char key[32];
		JSONObjIter iter = new JSONObjIter(obj);

		while (iter.Next(key, sizeof(key)))
		{
			count++;
			JSON value = iter.Value;
			AssertValidHandle(value);
			delete value;
		}
		AssertFalse(iter.HasNext);
		AssertEq(count, 3);

		AssertTrue(iter.Reset());

		count = 0;
		while (iter.Next(key, sizeof(key)))
		{
			count++;
			JSON value = iter.Value;
			AssertValidHandle(value);
			delete value;
		}
		AssertEq(count, 3);
		delete iter;
		delete obj;
	}
	TestEnd();

	// Test ForeachArray
	TestStart("Iterator_ForeachArray");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt(10);
		arr.PushInt(20);
		arr.PushInt(30);

		int count = 0;
		JSONArrIter iter = new JSONArrIter(arr);

		while (iter.HasNext)
		{
			JSON value = iter.Next;
			AssertValidHandle(value);
			AssertEq(iter.Index, count);
			delete value;
			count++;
		}
		AssertFalse(iter.HasNext);
		AssertEq(count, 3);

		AssertTrue(iter.Reset());

		count = 0;
		while (iter.HasNext)
		{
			JSON value = iter.Next;
			AssertValidHandle(value);
			AssertEq(iter.Index, count);
			delete value;
			count++;
		}
		AssertEq(count, 3);
		delete iter;
		delete arr;
	}
	TestEnd();

	// Test ForeachKey
	TestStart("Iterator_ForeachKey");
	{
		JSONObject obj = new JSONObject();
		obj.SetInt("key1", 1);
		obj.SetInt("key2", 2);
		obj.SetInt("key3", 3);

		int count = 0;
		char key[32];
		JSONObjIter iter = new JSONObjIter(obj);

		while (iter.Next(key, sizeof(key)))
		{
			AssertTrue(strlen(key) > 0);
			count++;
		}
		AssertFalse(iter.HasNext);
		AssertEq(count, 3);

		AssertTrue(iter.Reset());

		count = 0;
		while (iter.Next(key, sizeof(key)))
		{
			AssertTrue(strlen(key) > 0);
			count++;
		}
		AssertEq(count, 3);
		delete iter;
		delete obj;
	}
	TestEnd();

	// Test ForeachIndex
	TestStart("Iterator_ForeachIndex");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt(1);
		arr.PushInt(2);
		arr.PushInt(3);

		int count = 0;
		JSONArrIter iter = new JSONArrIter(arr);

		while (iter.HasNext)
		{
			JSON value = iter.Next;
			AssertEq(iter.Index, count);
			delete value;
			count++;
		}
		AssertFalse(iter.HasNext);
		AssertEq(count, 3);

		AssertTrue(iter.Reset());

		count = 0;
		while (iter.HasNext)
		{
			JSON value = iter.Next;
			AssertEq(iter.Index, count);
			delete value;
			count++;
		}
		AssertEq(count, 3);
		delete iter;
		delete arr;
	}
	TestEnd();

	// Test empty iterations
	TestStart("Iterator_EmptyObject");
	{
		JSONObject obj = new JSONObject();
		char key[32];
		JSONObjIter iter = new JSONObjIter(obj);
		AssertFalse(iter.Next(key, sizeof(key)));
		AssertTrue(iter.Reset());
		AssertFalse(iter.Next(key, sizeof(key)));
		delete iter;

		delete obj;
	}
	TestEnd();

	TestStart("Iterator_EmptyArray");
	{
		JSONArray arr = new JSONArray();
		JSONArrIter iter = new JSONArrIter(arr);
		AssertFalse(iter.HasNext);
		AssertTrue(iter.Reset());
		AssertFalse(iter.HasNext);
		delete iter;

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
		JSONObject obj = new JSONObject();
		AssertTrue(obj.PtrSetInt("/number", 42));
		AssertEq(obj.PtrGetInt("/number"), 42);
		delete obj;
	}
	TestEnd();

	TestStart("Pointer_PtrSetFloat");
	{
		JSONObject obj = new JSONObject();
		AssertTrue(obj.PtrSetFloat("/pi", 3.14159));
		AssertFloatEq(obj.PtrGetFloat("/pi"), 3.14159);
		delete obj;
	}
	TestEnd();

	TestStart("Pointer_PtrSetBool");
	{
		JSONObject obj = new JSONObject();
		AssertTrue(obj.PtrSetBool("/flag", true));
		AssertTrue(obj.PtrGetBool("/flag"));
		delete obj;
	}
	TestEnd();

	TestStart("Pointer_PtrSetString");
	{
		JSONObject obj = new JSONObject();
		AssertTrue(obj.PtrSetString("/name", "test"));

		char buffer[64];
		AssertTrue(obj.PtrGetString("/name", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "test");

		delete obj;
	}
	TestEnd();

	TestStart("Pointer_PtrSetInt64");
	{
		JSONObject obj = new JSONObject();
		AssertTrue(obj.PtrSetInt64("/bignum", "9223372036854775807"));

		char buffer[32];
		AssertTrue(obj.PtrGetInt64("/bignum", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "9223372036854775807");

		delete obj;
	}
	TestEnd();

	TestStart("Pointer_PtrSetNull");
	{
		JSONObject obj = new JSONObject();
		AssertTrue(obj.PtrSetNull("/nullable"));
		AssertTrue(obj.PtrGetIsNull("/nullable"));
		delete obj;
	}
	TestEnd();

	// Test nested path creation
	TestStart("Pointer_NestedPathCreation");
	{
		JSONObject obj = new JSONObject();
		AssertTrue(obj.PtrSetInt("/a/b/c/d", 123));
		AssertEq(obj.PtrGetInt("/a/b/c/d"), 123);
		delete obj;
	}
	TestEnd();

	// Test PtrGet with handle
	TestStart("Pointer_PtrGet");
	{
		JSONObject obj = new JSONObject();
		obj.PtrSetInt("/test", 999);

		JSON val = obj.PtrGet("/test");
		AssertValidHandle(val);
		AssertEq(val.GetInt(), 999);

		delete val;
		delete obj;
	}
	TestEnd();

	// Test PtrAdd methods
	TestStart("Pointer_PtrAddInt");
	{
		JSONObject obj = new JSONObject();
		obj.PtrSetInt("/arr/0", 1);
		AssertTrue(obj.PtrAddInt("/arr/1", 2));
		AssertEq(obj.PtrGetInt("/arr/1"), 2);
		delete obj;
	}
	TestEnd();

	TestStart("Pointer_PtrAddString");
	{
		JSONObject obj = new JSONObject();
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
		JSONObject obj = new JSONObject();
		obj.PtrSetInt("/remove_me", 1);
		obj.PtrSetInt("/keep_me", 2);

		AssertTrue(obj.PtrRemove("/remove_me"));

		JSON val;
		obj.PtrTryGetVal("/remove_me", val);
		AssertNullHandle(val);

		delete obj;
	}
	TestEnd();

	// Test PtrGetLength
	TestStart("Pointer_PtrGetLength");
	{
		JSONObject obj = new JSONObject();
		obj.PtrSetString("/text", "hello");

		int len = obj.PtrGetLength("/text");
		AssertEq(len, 6); // Including null terminator

		delete obj;
	}
	TestEnd();

	// Test PtrTryGet methods
	TestStart("Pointer_PtrTryGetInt");
	{
		JSONObject obj = new JSONObject();
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
		JSONObject obj = new JSONObject();
		obj.PtrSetBool("/flag", true);

		bool value;
		AssertTrue(obj.PtrTryGetBool("/flag", value));
		AssertTrue(value);

		delete obj;
	}
	TestEnd();

	TestStart("Pointer_PtrTryGetFloat");
	{
		JSONObject obj = new JSONObject();
		obj.PtrSetFloat("/pi", 3.14);

		float value;
		AssertTrue(obj.PtrTryGetFloat("/pi", value));
		AssertFloatEq(value, 3.14);

		delete obj;
	}
	TestEnd();

	TestStart("Pointer_PtrTryGetString");
	{
		JSONObject obj = new JSONObject();
		obj.PtrSetString("/name", "test");

		char buffer[64];
		AssertTrue(obj.PtrTryGetString("/name", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "test");

		delete obj;
	}
	TestEnd();

	TestStart("Pointer_PtrTryGetInt64");
	{
		JSONObject obj = new JSONObject();
		obj.PtrSetInt64("/bignum", "123456789012345");

		char buffer[32];
		AssertTrue(obj.PtrTryGetInt64("/bignum", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "123456789012345");

		delete obj;
	}
	TestEnd();

	TestStart("Pointer_PtrTryGetVal");
	{
		JSONObject obj = new JSONObject();
		obj.PtrSetInt("/test", 42);

		JSON value;
		AssertTrue(obj.PtrTryGetVal("/test", value));
		AssertValidHandle(value);
		AssertEq(value.GetInt(), 42);

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
		JSONObject original = new JSONObject();
		original.SetInt("num", 42);
		original.SetString("str", "test");

		JSONObject target = new JSONObject();
		JSONObject copy = JSON.DeepCopy(target, original);

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
		JSONArray original = new JSONArray();
		original.PushInt(1);
		original.PushInt(2);
		original.PushInt(3);

		JSONArray target = new JSONArray();
		JSONArray copy = JSON.DeepCopy(target, original);

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
		JSONObject obj1 = new JSONObject();
		obj1.SetInt("a", 1);
		obj1.SetString("b", "test");

		JSONObject obj2 = new JSONObject();
		obj2.SetInt("a", 1);
		obj2.SetString("b", "test");

		AssertTrue(JSON.Equals(obj1, obj2));

		delete obj1;
		delete obj2;
	}
	TestEnd();

	TestStart("Advanced_Equals_False");
	{
		JSONObject obj1 = new JSONObject();
		obj1.SetInt("a", 1);

		JSONObject obj2 = new JSONObject();
		obj2.SetInt("a", 2);

		AssertFalse(JSON.Equals(obj1, obj2));

		delete obj1;
		delete obj2;
	}
	TestEnd();

	// Test ToMutable/ToImmutable
	TestStart("Advanced_ToMutable");
	{
		JSON immutable = JSON.Parse("{\"key\":\"value\"}");
		AssertTrue(immutable.IsImmutable);

		JSON mutable = immutable.ToMutable();
		AssertValidHandle(mutable);
		AssertTrue(mutable.IsMutable);

		delete immutable;
		delete mutable;
	}
	TestEnd();

	TestStart("Advanced_ToImmutable");
	{
		JSONObject mutable = new JSONObject();
		mutable.SetInt("key", 42);
		AssertTrue(mutable.IsMutable);

		JSON immutable = mutable.ToImmutable();
		AssertValidHandle(immutable);
		AssertTrue(immutable.IsImmutable);

		delete mutable;
		delete immutable;
	}
	TestEnd();

	// Test ApplyJsonPatch (new value, immutable result)
	TestStart("Advanced_ApplyJsonPatch");
	{
		JSONObject original = new JSONObject();
		original.SetInt("score", 10);
		original.SetString("name", "bot");

		JSON patch = JSON.Parse("[{\"op\":\"replace\",\"path\":\"/score\",\"value\":42}]");
		AssertValidHandle(patch);

		JSON result = original.ApplyJsonPatch(patch);
		AssertValidHandle(result);
		AssertTrue(result.IsImmutable);

		AssertEq(result.PtrGetInt("/score"), 42);
		char buffer[32];
		result.PtrGetString("/name", buffer, sizeof(buffer));
		AssertStrEq(buffer, "bot");

		// ensure original unchanged
		AssertEq(original.GetInt("score"), 10);

		delete result;
		delete patch;
		delete original;
	}
	TestEnd();

	// Test ApplyJsonPatch resultMutable = true
	TestStart("Advanced_ApplyJsonPatch_MutableResult");
	{
		JSONObject original = new JSONObject();
		original.SetInt("count", 1);

		JSON patch = JSON.Parse("[{\"op\":\"add\",\"path\":\"/newField\",\"value\":\"hello\"}]");
		AssertValidHandle(patch);

		JSON result = original.ApplyJsonPatch(patch, true);
		AssertValidHandle(result);
		AssertTrue(result.IsMutable);

		char buffer[16];
		result.PtrGetString("/newField", buffer, sizeof(buffer));
		AssertStrEq(buffer, "hello");

		delete result;
		delete patch;
		delete original;
	}
	TestEnd();

	// Test JsonPatchInPlace
	TestStart("Advanced_JsonPatchInPlace");
	{
		JSONObject target = new JSONObject();
		target.SetInt("score", 5);
		target.SetInt("lives", 3);

		JSON patch = JSON.Parse("[{\"op\":\"remove\",\"path\":\"/lives\"},{\"op\":\"replace\",\"path\":\"/score\",\"value\":9}]");
		AssertValidHandle(patch);

		AssertTrue(target.JsonPatchInPlace(patch));
		AssertEq(target.GetInt("score"), 9);
		AssertFalse(target.HasKey("lives"));

		delete patch;
		delete target;
	}
	TestEnd();

	// Test ApplyMergePatch (immutable result)
	TestStart("Advanced_ApplyMergePatch");
	{
		JSONObject original = new JSONObject();
		original.PtrSetString("/settings/mode", "coop");
		original.PtrSetInt("/settings/difficulty", 1);

		JSON mergePatch = JSON.Parse("{\"settings\":{\"difficulty\":3,\"friendlyFire\":true}}");
		AssertValidHandle(mergePatch);

		JSON result = original.ApplyMergePatch(mergePatch);
		AssertValidHandle(result);
		AssertTrue(result.IsImmutable);

		AssertEq(result.PtrGetInt("/settings/difficulty"), 3);
		AssertTrue(result.PtrGetBool("/settings/friendlyFire"));

		delete result;
		delete mergePatch;
		delete original;
	}
	TestEnd();

	// Test ApplyMergePatch resultMutable = true
	TestStart("Advanced_ApplyMergePatch_MutableResult");
	{
		JSONObject original = new JSONObject();
		original.PtrSetString("/profile/name", "player");

		JSON mergePatch = JSON.Parse("{\"profile\":{\"rank\":10}}");
		AssertValidHandle(mergePatch);

		JSON result = original.ApplyMergePatch(mergePatch, true);
		AssertValidHandle(result);
		AssertTrue(result.IsMutable);

		AssertEq(result.PtrGetInt("/profile/rank"), 10);
		char buffer[16];
		result.PtrGetString("/profile/name", buffer, sizeof(buffer));
		AssertStrEq(buffer, "player");

		delete result;
		delete mergePatch;
		delete original;
	}
	TestEnd();

	// Test MergePatchInPlace
	TestStart("Advanced_MergePatchInPlace");
	{
		JSONObject target = new JSONObject();
		target.PtrSetString("/config/mode", "coop");
		target.PtrSetInt("/config/players", 4);

		JSON mergePatch = JSON.Parse("{\"config\":{\"players\":6,\"region\":\"EU\"}}");
		AssertValidHandle(mergePatch);

		AssertTrue(target.MergePatchInPlace(mergePatch));
		AssertEq(target.PtrGetInt("/config/players"), 6);
		char buffer[16];
		target.PtrGetString("/config/region", buffer, sizeof(buffer));
		AssertStrEq(buffer, "EU");

		delete mergePatch;
		delete target;
	}
	TestEnd();

	// Test Pack
	TestStart("Advanced_Pack_SimpleObject");
	{
		JSONObject json = JSON.Pack("{s:i,s:s,s:b}",
			"num", 42,
			"str", "test",
			"bool", true
		);

		AssertValidHandle(json);
		AssertTrue(json.IsObject);

		JSONObject obj = json;
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
		JSONArray json = JSON.Pack("[i,i,i]", 1, 2, 3);

		AssertValidHandle(json);
		AssertTrue(json.IsArray);

		JSONArray arr = json;
		AssertEq(arr.Length, 3);
		AssertEq(arr.GetInt(0), 1);
		AssertEq(arr.GetInt(2), 3);

		delete json;
	}
	TestEnd();

	TestStart("Advanced_Pack_Nested");
	{
		JSONObject json = JSON.Pack("{s:{s:s,s:i}}",
			"user",
				"name", "test",
				"age", 25
		);

		AssertValidHandle(json);

		char buffer[64];
		JSONObject obj = json;
		obj.PtrGetString("/user/name", buffer, sizeof(buffer));
		AssertStrEq(buffer, "test");
		AssertEq(obj.PtrGetInt("/user/age"), 25);

		delete json;
	}
	TestEnd();

	// Test mixed type array sorting
	TestStart("Advanced_MixedTypeSort");
	{
		JSONArray json = JSON.Parse("[true, 42, \"hello\", 1.5, false]", .is_mutable_doc = true);
		JSONArray arr = json;

		AssertTrue(arr.Sort(JSON_SORT_ASC));
		AssertEq(arr.Length, 5);

		delete json;
	}
	TestEnd();
}

// ============================================================================
// 2.8 Int64 Operations Tests
// ============================================================================

void Test_Int64Operations()
{
	PrintToServer("\n[Category] Int64 Operations Tests");

	// Test JSON.SetInt64 - Modify value in-place
	TestStart("Int64_SetInt64_Positive");
	{
		JSON val = JSON.CreateInt(100);
		AssertTrue(val.SetInt64("123456789012345"));

		char buffer[32];
		AssertTrue(val.GetInt64(buffer, sizeof(buffer)));
		AssertStrEq(buffer, "123456789012345");

		delete val;
	}
	TestEnd();

	TestStart("Int64_SetInt64_Negative");
	{
		JSON val = JSON.CreateInt(100);
		AssertTrue(val.SetInt64("-987654321098765"));

		char buffer[32];
		AssertTrue(val.GetInt64(buffer, sizeof(buffer)));
		AssertStrEq(buffer, "-987654321098765");

		delete val;
	}
	TestEnd();

	TestStart("Int64_SetInt64_Zero");
	{
		JSON val = JSON.CreateInt(999);
		AssertTrue(val.SetInt64("0"));

		char buffer[32];
		AssertTrue(val.GetInt64(buffer, sizeof(buffer)));
		AssertStrEq(buffer, "0");

		delete val;
	}
	TestEnd();

	// Test JSONArray.SetInt64 - Replace value in array
	TestStart("Int64_Array_SetInt64");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt(100);
		arr.PushInt(200);
		arr.PushInt(300);

		AssertTrue(arr.SetInt64(1, "5555555555555555"));

		char buffer[32];
		arr.GetInt64(1, buffer, sizeof(buffer));
		AssertStrEq(buffer, "5555555555555555");

		delete arr;
	}
	TestEnd();

	TestStart("Int64_Array_SetInt64_Negative");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt64("1111111111111111");

		AssertTrue(arr.SetInt64(0, "-2222222222222222"));

		char buffer[32];
		arr.GetInt64(0, buffer, sizeof(buffer));
		AssertStrEq(buffer, "-2222222222222222");

		delete arr;
	}
	TestEnd();

	// Test large unsigned int64 values
	TestStart("Int64_Array_PushLargeUnsigned");
	{
		JSONArray arr = new JSONArray();
		AssertTrue(arr.PushInt64("18446744073709551615"));

		char buffer[32];
		arr.GetInt64(0, buffer, sizeof(buffer));
		AssertStrEq(buffer, "18446744073709551615");

		delete arr;
	}
	TestEnd();

	TestStart("Int64_Array_IndexOfLargeValues");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt64("11111111111111111");
		arr.PushInt64("22222222222222222");
		arr.PushInt64("33333333333333333");

		AssertEq(arr.IndexOfInt64("22222222222222222"), 1);
		AssertEq(arr.IndexOfInt64("99999999999999999"), -1);

		delete arr;
	}
	TestEnd();

	// Test PtrAddInt64
	TestStart("Int64_Pointer_PtrAddInt64");
	{
		JSONObject obj = new JSONObject();
		obj.PtrSetInt("/numbers/0", 1);
		AssertTrue(obj.PtrAddInt64("/numbers/1", "7777777777777777"));

		char buffer[32];
		AssertTrue(obj.PtrGetInt64("/numbers/1", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "7777777777777777");

		delete obj;
	}
	TestEnd();

	TestStart("Int64_Pointer_PtrAddInt64_Negative");
	{
		JSONObject obj = new JSONObject();
		obj.PtrSetInt("/data/0", 1);
		AssertTrue(obj.PtrAddInt64("/data/1", "-8888888888888888"));

		char buffer[32];
		AssertTrue(obj.PtrGetInt64("/data/1", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "-8888888888888888");

		delete obj;
	}
	TestEnd();

	// Test PtrAddInt64 with large unsigned value
	TestStart("Int64_Pointer_PtrAddInt64_LargeUnsigned");
	{
		JSONObject obj = new JSONObject();
		obj.PtrSetInt("/bigvals/0", 1);
		AssertTrue(obj.PtrAddInt64("/bigvals/1", "18446744073709551615"));

		char buffer[32];
		AssertTrue(obj.PtrGetInt64("/bigvals/1", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "18446744073709551615");

		delete obj;
	}
	TestEnd();

	// Test mixed signed/unsigned values
	TestStart("Int64_Mixed_SignedUnsigned_Array");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt64("-9223372036854775808");
		arr.PushInt64("18446744073709551615");
		arr.PushInt64("0");
		arr.PushInt64("9223372036854775807");

		AssertEq(arr.Length, 4);

		char buffer[32];
		arr.GetInt64(0, buffer, sizeof(buffer));
		AssertStrEq(buffer, "-9223372036854775808");

		arr.GetInt64(1, buffer, sizeof(buffer));
		AssertStrEq(buffer, "18446744073709551615");

		arr.GetInt64(2, buffer, sizeof(buffer));
		AssertStrEq(buffer, "0");

		arr.GetInt64(3, buffer, sizeof(buffer));
		AssertStrEq(buffer, "9223372036854775807");

		delete arr;
	}
	TestEnd();

	TestStart("Int64_Mixed_SignedUnsigned_Object");
	{
		JSONObject obj = new JSONObject();
		obj.SetInt64("min_int64", "-9223372036854775808");
		obj.SetInt64("max_int64", "9223372036854775807");
		obj.SetInt64("max_uint64", "18446744073709551615");

		char buffer[32];

		AssertTrue(obj.GetInt64("min_int64", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "-9223372036854775808");

		AssertTrue(obj.GetInt64("max_int64", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "9223372036854775807");

		AssertTrue(obj.GetInt64("max_uint64", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "18446744073709551615");

		delete obj;
	}
	TestEnd();

	// Test serialization with int64
	TestStart("Int64_Serialization_ToString");
	{
		JSONObject obj = new JSONObject();
		obj.SetInt64("large_num", "9007199254740992");
		obj.SetInt64("negative_num", "-9007199254740992");

		char json[256];
		int len = obj.ToString(json, sizeof(json));
		AssertTrue(len > 0);

		// Parse it back
		JSONObject parsed = JSONObject.FromString(json);
		AssertValidHandle(parsed);

		char buffer[32];
		AssertTrue(parsed.GetInt64("large_num", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "9007199254740992");

		AssertTrue(parsed.GetInt64("negative_num", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "-9007199254740992");

		delete obj;
		delete parsed;
	}
	TestEnd();

	TestStart("Int64_Serialization_Array");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt64("1234567890123456");
		arr.PushInt64("-9876543210987654");
		arr.PushInt64("18000000000000000000");

		char json[256];
		int len = arr.ToString(json, sizeof(json));
		AssertTrue(len > 0);

		// Parse it back
		JSONArray parsed = JSONArray.FromString(json);
		AssertValidHandle(parsed);
		AssertEq(parsed.Length, 3);

		char buffer[32];
		parsed.GetInt64(0, buffer, sizeof(buffer));
		AssertStrEq(buffer, "1234567890123456");

		parsed.GetInt64(1, buffer, sizeof(buffer));
		AssertStrEq(buffer, "-9876543210987654");

		parsed.GetInt64(2, buffer, sizeof(buffer));
		AssertStrEq(buffer, "18000000000000000000");

		delete arr;
		delete parsed;
	}
	TestEnd();

	// Test boundary values
	TestStart("Int64_Boundary_JustAboveInt32Max");
	{
		JSON val = JSON.CreateInt64("2147483648");
		AssertValidHandle(val);

		char buffer[32];
		AssertTrue(val.GetInt64(buffer, sizeof(buffer)));
		AssertStrEq(buffer, "2147483648");

		delete val;
	}
	TestEnd();

	TestStart("Int64_Boundary_JustBelowInt32Min");
	{
		JSON val = JSON.CreateInt64("-2147483649");
		AssertValidHandle(val);

		char buffer[32];
		AssertTrue(val.GetInt64(buffer, sizeof(buffer)));
		AssertStrEq(buffer, "-2147483649");

		delete val;
	}
	TestEnd();

	TestStart("Int64_Boundary_UInt32Max");
	{
		JSON val = JSON.CreateInt64("4294967295");
		AssertValidHandle(val);

		char buffer[32];
		AssertTrue(val.GetInt64(buffer, sizeof(buffer)));
		AssertStrEq(buffer, "4294967295");

		delete val;
	}
	TestEnd();

	TestStart("Int64_Boundary_JustAboveUInt32Max");
	{
		JSON val = JSON.CreateInt64("4294967296");
		AssertValidHandle(val);

		char buffer[32];
		AssertTrue(val.GetInt64(buffer, sizeof(buffer)));
		AssertStrEq(buffer, "4294967296");

		delete val;
	}
	TestEnd();

	// Test Int64 in nested structures
	TestStart("Int64_Nested_ObjectInObject");
	{
		JSONObject obj = new JSONObject();
		obj.PtrSetInt64("/outer/inner/deep", "5432109876543210");

		char buffer[32];
		AssertTrue(obj.PtrGetInt64("/outer/inner/deep", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "5432109876543210");

		delete obj;
	}
	TestEnd();

	TestStart("Int64_Nested_ArrayInObject");
	{
		JSONObject obj = new JSONObject();
		obj.PtrSetInt64("/data/values/0", "1111111111111111");
		obj.PtrAddInt64("/data/values/1", "2222222222222222");
		obj.PtrAddInt64("/data/values/2", "3333333333333333");

		char buffer[32];
		AssertTrue(obj.PtrGetInt64("/data/values/0", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "1111111111111111");

		AssertTrue(obj.PtrGetInt64("/data/values/1", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "2222222222222222");

		AssertTrue(obj.PtrGetInt64("/data/values/2", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "3333333333333333");

		delete obj;
	}
	TestEnd();

	// Test DeepCopy with int64
	TestStart("Int64_DeepCopy_Object");
	{
		JSONObject original = new JSONObject();
		original.SetInt64("bignum", "9999999999999999");
		original.SetInt64("negative", "-8888888888888888");

		JSONObject target = new JSONObject();
		JSONObject copy = JSON.DeepCopy(target, original);

		AssertValidHandle(copy);

		char buffer[32];
		AssertTrue(copy.GetInt64("bignum", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "9999999999999999");

		AssertTrue(copy.GetInt64("negative", buffer, sizeof(buffer)));
		AssertStrEq(buffer, "-8888888888888888");

		delete original;
		delete target;
		delete copy;
	}
	TestEnd();

	TestStart("Int64_DeepCopy_Array");
	{
		JSONArray original = new JSONArray();
		original.PushInt64("1111111111111111");
		original.PushInt64("-2222222222222222");
		original.PushInt64("18446744073709551615");

		JSONArray target = new JSONArray();
		JSONArray copy = JSON.DeepCopy(target, original);

		AssertValidHandle(copy);
		AssertEq(copy.Length, 3);

		char buffer[32];
		copy.GetInt64(0, buffer, sizeof(buffer));
		AssertStrEq(buffer, "1111111111111111");

		copy.GetInt64(1, buffer, sizeof(buffer));
		AssertStrEq(buffer, "-2222222222222222");

		copy.GetInt64(2, buffer, sizeof(buffer));
		AssertStrEq(buffer, "18446744073709551615");

		delete original;
		delete target;
		delete copy;
	}
	TestEnd();

	// Test array sorting with int64 values
	TestStart("Int64_Array_Sort_Ascending");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt64("9999999999999999");
		arr.PushInt64("1111111111111111");
		arr.PushInt64("5555555555555555");
		arr.PushInt64("3333333333333333");

		AssertTrue(arr.Sort(JSON_SORT_ASC));

		char buffer[32];
		arr.GetInt64(0, buffer, sizeof(buffer));
		AssertStrEq(buffer, "1111111111111111");

		arr.GetInt64(3, buffer, sizeof(buffer));
		AssertStrEq(buffer, "9999999999999999");

		delete arr;
	}
	TestEnd();

	TestStart("Int64_Array_Sort_Descending");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt64("1111111111111111");
		arr.PushInt64("5555555555555555");
		arr.PushInt64("3333333333333333");
		arr.PushInt64("9999999999999999");

		AssertTrue(arr.Sort(JSON_SORT_DESC));

		char buffer[32];
		arr.GetInt64(0, buffer, sizeof(buffer));
		AssertStrEq(buffer, "9999999999999999");

		arr.GetInt64(3, buffer, sizeof(buffer));
		AssertStrEq(buffer, "1111111111111111");

		delete arr;
	}
	TestEnd();

	// Test int64 with type checks
	TestStart("Int64_TypeChecks");
	{
		JSON val = JSON.CreateInt64("9223372036854775807");

		AssertTrue(val.IsNum);
		AssertTrue(val.IsInt);
		AssertFalse(val.IsFloat);
		AssertFalse(val.IsStr);
		AssertFalse(val.IsBool);

		delete val;
	}
	TestEnd();

	TestStart("Int64_TypeChecks_Signed");
	{
		JSON val = JSON.CreateInt64("-9223372036854775808");

		AssertTrue(val.IsNum);
		AssertTrue(val.IsInt);
		AssertTrue(val.IsSint);
		AssertFalse(val.IsUint);

		delete val;
	}
	TestEnd();

	TestStart("Int64_TypeChecks_LargeUnsigned");
	{
		JSON val = JSON.CreateInt64("18446744073709551615");

		AssertTrue(val.IsNum);
		AssertTrue(val.IsInt);
		// Large unsigned value should be detected as unsigned integer
		AssertTrue(val.IsUint);
		AssertFalse(val.IsSint);

		delete val;
	}
	TestEnd();
}

// ============================================================================
// 2.9 Edge Cases & Error Handling Tests
// ============================================================================

void Test_EdgeCases()
{
	PrintToServer("\n[Category] Edge Cases & Error Handling Tests");

	// Test empty containers
	TestStart("EdgeCase_EmptyObject");
	{
		JSONObject obj = new JSONObject();
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
		JSONArray arr = new JSONArray();
		AssertEq(arr.Length, 0);
		delete arr;
	}
	TestEnd();

	// Test nonexistent keys/indices
	TestStart("EdgeCase_NonexistentKey");
	{
		JSONObject obj = new JSONObject();
		obj.SetInt("exists", 1);

		AssertTrue(obj.HasKey("exists"));
		AssertFalse(obj.HasKey("notexists"));

		delete obj;
	}
	TestEnd();

	TestStart("EdgeCase_ArrayOutOfBounds");
	{
		JSONArray arr = new JSONArray();
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

		JSON val = JSON.CreateString(longStr);
		AssertValidHandle(val);

		char retrieved[1024];
		AssertTrue(val.GetString(retrieved, sizeof(retrieved)));
		AssertStrEq(retrieved, longStr);

		delete val;
	}
	TestEnd();

	// Test deep nesting
	TestStart("EdgeCase_DeepNesting");
	{
		JSONObject obj = new JSONObject();

		// Create deeply nested structure
		obj.PtrSetInt("/a/b/c/d/e/f/g/h/i/j", 42);
		AssertEq(obj.PtrGetInt("/a/b/c/d/e/f/g/h/i/j"), 42);

		delete obj;
	}
	TestEnd();

	// Test large arrays
	TestStart("EdgeCase_LargeArray");
	{
		JSONArray arr = new JSONArray();

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
		JSONObject obj = new JSONObject();

		char key[32];
		for (int i = 0; i < 100; i++)
		{
			FormatEx(key, sizeof(key), "key_%d", i);
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
		JSON val = JSON.CreateInt64("9223372036854775807");
		AssertValidHandle(val);

		char buffer[32];
		val.GetInt64(buffer, sizeof(buffer));
		AssertStrEq(buffer, "9223372036854775807");

		delete val;
	}
	TestEnd();

	TestStart("EdgeCase_Int64_MinValue");
	{
		JSON val = JSON.CreateInt64("-9223372036854775808");
		AssertValidHandle(val);

		char buffer[32];
		val.GetInt64(buffer, sizeof(buffer));
		AssertStrEq(buffer, "-9223372036854775808");

		delete val;
	}
	TestEnd();

	// Test special float values
	TestStart("EdgeCase_Float_VerySmall");
	{
		JSON val = JSON.CreateFloat(0.000001);
		AssertValidHandle(val);
		AssertFloatEq(val.GetFloat(), 0.000001);
		delete val;
	}
	TestEnd();

	TestStart("EdgeCase_Float_VeryLarge");
	{
		JSON val = JSON.CreateFloat(999999.999999);
		AssertValidHandle(val);
		AssertFloatEq(val.GetFloat(), 999999.999999, "", 0.001);
		delete val;
	}
	TestEnd();

	// Test special characters in strings
	TestStart("EdgeCase_SpecialCharacters");
	{
		JSON val = JSON.CreateString("Line1\nLine2\tTabbed\"Quoted\"");
		AssertValidHandle(val);

		char buffer[128];
		val.GetString(buffer, sizeof(buffer));
		Assert(StrContains(buffer, "Line1") != -1);

		delete val;
	}
	TestEnd();

	// Test removing from empty array
	TestStart("EdgeCase_RemoveFromEmptyArray");
	{
		JSONArray arr = new JSONArray();
		AssertFalse(arr.RemoveFirst());
		AssertFalse(arr.RemoveLast());
		delete arr;
	}
	TestEnd();

	// Test clearing already empty containers
	TestStart("EdgeCase_ClearEmpty");
	{
		JSONObject obj = new JSONObject();
		AssertTrue(obj.Clear());
		AssertEq(obj.Size, 0);

		JSONArray arr = new JSONArray();
		AssertTrue(arr.Clear());
		AssertEq(arr.Length, 0);

		delete obj;
		delete arr;
	}
	TestEnd();

	// Test IndexOf on empty array
	TestStart("EdgeCase_IndexOfEmpty");
	{
		JSONArray arr = new JSONArray();
		AssertEq(arr.IndexOfInt(42), -1);
		AssertEq(arr.IndexOfString("test"), -1);
		AssertEq(arr.IndexOfBool(true), -1);
		delete arr;
	}
	TestEnd();

	// Test pointer to nonexistent path
	TestStart("EdgeCase_PointerNonexistentPath");
	{
		JSONObject obj = new JSONObject();

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
		JSONObject obj = new JSONObject();
		AssertTrue(obj.Sort());

		JSONArray arr = new JSONArray();
		AssertTrue(arr.Sort());

		delete obj;
		delete arr;
	}
	TestEnd();

	// Test single element operations
	TestStart("EdgeCase_SingleElement");
	{
		JSONArray arr = new JSONArray();
		arr.PushInt(42);

		AssertTrue(arr.Sort());
		AssertEq(arr.GetInt(0), 42);

		AssertEq(arr.IndexOfInt(42), 0);

		JSON first = arr.First;
		JSON last = arr.Last;
		AssertEq(first.GetInt(), last.GetInt());

		delete first;
		delete last;
		delete arr;
	}
	TestEnd();
}