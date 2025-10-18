#include <sourcemod>
#include <yyjson>

public Plugin myinfo =
{
	name = "YYJSON Test Suite",
	author = "ProjectSky",
	description = "Test suite for YYJSON extension",
	version = "1.0.2",
	url = "https://github.com/ProjectSky/sm-ext-yyjson"
};

public void OnPluginStart()
{
	RegServerCmd("sm_yyjson_test", Command_RunTests, "Run YYJSON test suite");
}

Action Command_RunTests(int args)
{
	// Run all test cases
	TestBasicOperations();
	TestArrayOperations();
	TestObjectOperations();
	TestSortOperations();
	TestSearchOperations();
	TestPointerOperations();
	TestIterationOperations();
	TestTypeOperations();
	TestFileOperations();
	TestImmutabilityOperations();
	TestPackOperations();
	TestFromStringsOperations();

	PrintToServer("[YYJSON] All tests completed!");
	return Plugin_Handled;
}

void TestBasicOperations()
{
	PrintToServer("[YYJSON] Testing basic operations...");

	// Test creation and parsing
	YYJSONObject obj = new YYJSONObject();
	obj.SetInt("int", 42);
	obj.SetFloat("float", 3.14);
	obj.SetBool("bool", true);
	obj.SetString("string", "hello");
	obj.SetNull("null");

	// Test serialization
	char buffer[1024];
	obj.ToString(buffer, sizeof(buffer));
	PrintToServer("Serialized: %s", buffer);

	// Test type checking
	PrintToServer("Type of 'int': %d", obj.Type);
	PrintToServer("SubType of 'int': %d", obj.SubType);
	PrintToServer("Is object: %d", obj.IsObject);

	// Test value existence
	PrintToServer("Has 'int': %d", obj.HasKey("int"));
	PrintToServer("Has 'nonexistent': %d", obj.HasKey("nonexistent"));

	// Test value retrieval
	PrintToServer("Int value: %d", obj.GetInt("int"));
	PrintToServer("Float value: %f", obj.GetFloat("float"));
	PrintToServer("Bool value: %d", obj.GetBool("bool"));

	char strBuffer[64];
	obj.GetString("string", strBuffer, sizeof(strBuffer));
	PrintToServer("String value: %s", strBuffer);

	PrintToServer("Is 'null' null: %d", obj.IsNull("null"));

	// Test parsing
	YYJSONObject parsed = YYJSON.Parse(buffer);

	// Test equality
	PrintToServer("Objects are equal: %d", YYJSON.Equals(obj, parsed));

	// Test deep copy
	YYJSONObject copy = new YYJSONObject();
	YYJSONObject copyResult = YYJSON.DeepCopy(copy, obj);
	PrintToServer("Copy equals original: %d", YYJSON.Equals(copyResult, obj));

	// Test size and read size
	PrintToServer("Object size: %d", obj.Size);
	PrintToServer("Read size: %d", obj.ReadSize);

	// Test type description
	char typeDesc[64];
	YYJSON.GetTypeDesc(obj, typeDesc, sizeof(typeDesc));
	PrintToServer("Type description: %s", typeDesc);

	delete obj;
	delete parsed;
	delete copy;
	delete copyResult;
}

void TestArrayOperations()
{
	PrintToServer("[YYJSON] Testing array operations...");

	YYJSONArray arr = new YYJSONArray();

	// Test push operations
	arr.PushInt(1);
	arr.PushFloat(2.5);
	arr.PushBool(true);
	arr.PushString("test");
	arr.PushNull();

	PrintToServer("Array after push operations:");
	PrintJson(arr);

	// Test get operations
	PrintToServer("First element: %d", arr.GetInt(0));
	PrintToServer("Second element: %f", arr.GetFloat(1));
	PrintToServer("Third element: %d", arr.GetBool(2));

	char strBuffer[64];
	arr.GetString(3, strBuffer, sizeof(strBuffer));
	PrintToServer("Fourth element: %s", strBuffer);

	PrintToServer("Fifth element is null: %d", arr.IsNull(4));

	YYJSON first = arr.First;
	YYJSON last	 = arr.Last;

	// Test array properties
	PrintToServer("Array length: %d", arr.Length);
	PrintToServer("First value: %x", first);
	PrintToServer("Last value: %x", last);

	// Test set operations
	arr.SetInt(0, 100);
	arr.SetFloat(1, 3.14);
	arr.SetBool(2, false);
	arr.SetString(3, "modified");
	arr.SetNull(4);

	PrintToServer("Array after set operations:");
	PrintJson(arr);

	// Test remove operations
	arr.RemoveFirst();
	PrintToServer("After RemoveFirst:");
	PrintJson(arr);

	arr.RemoveLast();
	PrintToServer("After RemoveLast:");
	PrintJson(arr);

	arr.Remove(1);
	PrintToServer("After Remove(1):");
	PrintJson(arr);

	arr.RemoveRange(0, 1);
	PrintToServer("After RemoveRange(0, 1):");
	PrintJson(arr);

	arr.Clear();
	PrintToServer("Array length after Clear: %d", arr.Length);

	delete arr;
	delete first;
	delete last;
}

void TestObjectOperations()
{
	PrintToServer("[YYJSON] Testing object operations...");

	YYJSONObject obj = new YYJSONObject();

	// Test set operations
	obj.SetInt("int", 123);
	obj.SetFloat("float", 3.14);
	obj.SetBool("bool", true);
	obj.SetString("string", "test");
	obj.SetNull("null");

	PrintToServer("Object after set operations:");
	PrintJson(obj);

	// Test get operations
	PrintToServer("Int value: %d", obj.GetInt("int"));
	PrintToServer("Float value: %f", obj.GetFloat("float"));
	PrintToServer("Bool value: %d", obj.GetBool("bool"));

	char strBuffer[64];
	obj.GetString("string", strBuffer, sizeof(strBuffer));
	PrintToServer("String value: %s", strBuffer);

	PrintToServer("Is null value null: %d", obj.IsNull("null"));

	// Test key operations
	char key[64];
	for (int i = 0; i < obj.Size; i++)
	{
		obj.GetKey(i, key, sizeof(key));
		PrintToServer("Key at %d: %s", i, key);

		YYJSON value = obj.GetValueAt(i);
		PrintToServer("Value type at %d: %d", i, value.Type);
		delete value;
	}

	// Test rename key
	obj.RenameKey("int", "number");
	PrintToServer("After renaming 'int' to 'number':");
	PrintJson(obj);

	// Test remove operations
	obj.Remove("number");
	PrintToServer("After removing 'number':");
	PrintJson(obj);

	obj.Clear();
	PrintToServer("Object size after Clear: %d", obj.Size);

	delete obj;
}

void TestSortOperations()
{
	PrintToServer("[YYJSON] Testing sort operations...");

	// Test array sorting
	YYJSONArray arr = YYJSON.Parse("[3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5]", .is_mutable_doc = true);

	PrintToServer("Original array:");
	PrintJson(arr);

	arr.Sort();
	PrintToServer("After ascending sort:");
	PrintJson(arr);

	arr.Sort(YYJSON_SORT_DESC);
	PrintToServer("After descending sort:");
	PrintJson(arr);

	arr.Sort(YYJSON_SORT_RANDOM);
	PrintToServer("After random sort:");
	PrintJson(arr);

	// Test mixed type array sorting
	YYJSONArray mixed = YYJSON.Parse("[true, 42, \"hello\", 1.23, false, \"world\"]", .is_mutable_doc = true);
	PrintToServer("Original mixed array:");
	PrintJson(mixed);

	mixed.Sort();
	PrintToServer("After sorting mixed array:");
	PrintJson(mixed);

	// Test object sorting
	YYJSONObject obj = YYJSON.Parse("{\"zebra\": 1, \"alpha\": 2, \"beta\": 3}", .is_mutable_doc = true);
	PrintToServer("Original object:");
	PrintJson(obj);

	obj.Sort();
	PrintToServer("After ascending sort:");
	PrintJson(obj);

	obj.Sort(YYJSON_SORT_DESC);
	PrintToServer("After descending sort:");
	PrintJson(obj);

	obj.Sort(YYJSON_SORT_RANDOM);
	PrintToServer("After random sort:");
	PrintJson(obj);

	delete arr;
	delete mixed;
	delete obj;
}

void TestSearchOperations()
{
	PrintToServer("[YYJSON] Testing search operations...");

	YYJSONArray arr = YYJSON.Parse("[42, true, \"hello\", 3.14, \"world\", false, 42]");
	PrintToServer("Test array:");
	PrintJson(arr);

	// Test all search methods
	PrintToServer("Search results:");
	PrintToServer("IndexOfInt(42): %d", arr.IndexOfInt(42));
	PrintToServer("IndexOfInt(999): %d", arr.IndexOfInt(999));

	PrintToServer("IndexOfBool(true): %d", arr.IndexOfBool(true));
	PrintToServer("IndexOfBool(false): %d", arr.IndexOfBool(false));

	PrintToServer("IndexOfString(\"hello\"): %d", arr.IndexOfString("hello"));
	PrintToServer("IndexOfString(\"missing\"): %d", arr.IndexOfString("missing"));

	PrintToServer("IndexOfFloat(3.14): %d", arr.IndexOfFloat(3.14));
	PrintToServer("IndexOfFloat(2.718): %d", arr.IndexOfFloat(2.718));

	delete arr;
}

void TestPointerOperations()
{
	PrintToServer("[YYJSON] Testing JSON pointer operations...");

	YYJSONObject obj = new YYJSONObject();

	// Test setting nested values
	obj.PtrSetInt("/a/b/c", 1);
	obj.PtrSetString("/a/b/name", "test");
	obj.PtrSetBool("/a/flag", true);
	obj.PtrSetFloat("/a/b/pi", 3.14);
	obj.PtrSetNull("/a/b/empty");

	PrintToServer("After setting values:");
	PrintJson(obj);

	// Test getting values
	PrintToServer("Pointer get operations:");
	PrintToServer("/a/b/c: %d", obj.PtrGetInt("/a/b/c"));

	char strBuffer[64];
	obj.PtrGetString("/a/b/name", strBuffer, sizeof(strBuffer));
	PrintToServer("/a/b/name: %s", strBuffer);

	PrintToServer("/a/flag: %d", obj.PtrGetBool("/a/flag"));
	PrintToServer("/a/b/pi: %f", obj.PtrGetFloat("/a/b/pi"));
	PrintToServer("/a/b/empty is null: %d", obj.PtrGetIsNull("/a/b/empty"));

	// Test adding values
	obj.PtrAddInt("/a/b/numbers/0", 1);
	obj.PtrAddInt("/a/b/numbers/1", 2);
	obj.PtrAddString("/a/b/strings", "append");

	PrintToServer("After adding values:");
	PrintJson(obj);

	// Test length
	PrintToServer("Length of /a/b/numbers: %d", obj.PtrGetLength("/a/b/numbers"));

	// Test removing values
	obj.PtrRemove("/a/b/c");
	PrintToServer("After removing /a/b/c:");
	PrintJson(obj);

	delete obj;
}

void TestIterationOperations()
{
	PrintToServer("[YYJSON] Testing iteration operations...");

	// Test object iteration
	YYJSONObject obj = YYJSON.Parse("{\"a\": 1, \"b\": 2, \"c\": 3}");
	char key[64];
	YYJSON value;

	while (obj.ForeachObject(key, sizeof(key), value))
	{
		PrintToServer("Key: %s", key);
		delete value;
	}

	// Test array iteration
	YYJSONArray arr = YYJSON.Parse("[1, 2, 3, 4, 5]");
	int index;

	while (arr.ForeachArray(index, value))
	{
		PrintToServer("Index: %d", index);
		delete value;
	}

	delete obj;
	delete arr;
}

void TestTypeOperations()
{
	PrintToServer("[YYJSON] Testing type operations...");

	// Test value creation
	YYJSON boolVal  = YYJSON.CreateBool(true);
	YYJSON intVal   = YYJSON.CreateInt(42);
	YYJSON floatVal = YYJSON.CreateFloat(3.14);
	YYJSON strVal   = YYJSON.CreateString("test");
	YYJSON nullVal  = YYJSON.CreateNull();

	// Test value types
	PrintToServer("Value types:");
	PrintToServer("Bool type: %d", boolVal.Type);
	PrintToServer("Int type: %d", intVal.Type);
	PrintToServer("Float type: %d", floatVal.Type);
	PrintToServer("String type: %d", strVal.Type);
	PrintToServer("Null type: %d", nullVal.Type);

	// Test value retrieval
	PrintToServer("Value contents:");
	PrintToServer("Bool value: %d", YYJSON.GetBool(boolVal));
	PrintToServer("Int value: %d", YYJSON.GetInt(intVal));
	PrintToServer("Float value: %f", YYJSON.GetFloat(floatVal));

	char strBuffer[64];
	YYJSON.GetString(strVal, strBuffer, sizeof(strBuffer));
	PrintToServer("String value: %s", strBuffer);

	delete boolVal;
	delete intVal;
	delete floatVal;
	delete strVal;
	delete nullVal;
}

void TestFileOperations()
{
	PrintToServer("[YYJSON] Testing file operations...");

	// Create test data
	YYJSONObject obj = new YYJSONObject();
	obj.SetInt("id", 1);
	obj.SetString("name", "test");
	obj.SetBool("active", true);

	// Test file writing
	PrintToServer("Writing to file...");
	obj.ToFile("test.json", YYJSON_WRITE_PRETTY_TWO_SPACES);

	// Test file reading
	PrintToServer("Reading from file...");
	YYJSONObject loaded = YYJSON.Parse("test.json", true);

	PrintToServer("File content:");
	PrintJson(loaded);
	delete loaded;

	delete obj;
}

void TestImmutabilityOperations()
{
	PrintToServer("[YYJSON] Testing immutability operations...");

	// Test immutable document creation
	YYJSONObject immutable = YYJSON.Parse("{\"key\": 123, \"str\": \"test\"}");
	PrintToServer("Created immutable document:");
	PrintJson(immutable);

	// Test property checks
	PrintToServer("Is mutable: %d", immutable.IsMutable);
	PrintToServer("Is immutable: %d", immutable.IsImmutable);

	// Test read operations (should succeed)
	PrintToServer("Read operations on immutable document:");
	PrintToServer("Int value: %d", immutable.GetInt("key"));
	char buffer[64];
	immutable.GetString("str", buffer, sizeof(buffer));
	PrintToServer("String value: %s", buffer);

	// Test conversion to mutable
	YYJSONObject mutable = immutable.ToMutable();
	PrintToServer("\nConverted to mutable document:");
	PrintToServer("Is mutable: %d", mutable.IsMutable);
	PrintToServer("Is immutable: %d", mutable.IsImmutable);

	// Now modifications should work
	mutable.SetInt("key", 456)
	PrintToServer("Successfully modified mutable document:");
	PrintJson(mutable);

	// Test conversion back to immutable
	YYJSONObject backToImmutable = mutable.ToImmutable();
	PrintToServer("\nConverted back to immutable:");
	PrintToServer("Is mutable: %d", backToImmutable.IsMutable);
	PrintToServer("Is immutable: %d", backToImmutable.IsImmutable);
	delete backToImmutable;
	delete mutable;
	delete immutable;

	// Test file operations with immutability
	PrintToServer("\nTesting file operations with immutability...");

	// Create and write a mutable document
	YYJSONObject writeObj = new YYJSONObject();
	writeObj.SetInt("test", 123);
	writeObj.ToFile("test_immutable.json");
	delete writeObj;

	// Read as immutable
	YYJSONObject readImmutable = YYJSON.Parse("test_immutable.json", true);
	PrintToServer("Read as immutable document:");
	PrintJson(readImmutable);
	PrintToServer("Is mutable: %d", readImmutable.IsMutable);
	PrintToServer("Is immutable: %d", readImmutable.IsImmutable);
	delete readImmutable;
}

void TestPackOperations()
{
	PrintToServer("[YYJSON] Testing pack operations...");

	// Test basic pack operation with different types
	YYJSON packed = YYJSON.Pack("{s:s,s:i,s:f,s:b,s:n}", 
		"name", "test",
		"age", 25,
		"height", 1.75,
		"active", true,
		"extra"
	);

	PrintToServer("Packed JSON:");
	PrintJson(packed);

	// Test nested object packing
	YYJSON nested = YYJSON.Pack("{s:{s:s,s:[i,i,i]}}", 
		"user",
			"name", "test",
			"scores", 85, 90, 95
	);

	PrintToServer("Nested packed JSON:");
	PrintJson(nested);

	// Test array packing with mixed types
	YYJSON array = YYJSON.Pack("[s,i,f,b,n]",
		"test", 42, 3.14, true
	);

	PrintToServer("Array packed JSON:");
	PrintJson(array);

	delete packed;
	delete nested;
	delete array;
}

void TestFromStringsOperations()
{
	PrintToServer("[YYJSON] Testing FromStrings operations...");

	// Test object creation from key-value string arrays
	char pairs[][] = {"name", "test", "type", "demo", "version", "1.0.0"};

	YYJSONObject obj = YYJSONObject.FromStrings(pairs, sizeof(pairs));
	PrintToServer("Object from strings:");
	PrintJson(obj);

	// Test array creation from string array
	char items[][] = {"apple", "banana", "orange", "grape"};
	YYJSONArray arr = YYJSONArray.FromStrings(items, sizeof(items));
	PrintToServer("Array from strings:");
	PrintJson(arr);

	delete obj;
	delete arr;
}

// Helper function to print json contents
void PrintJson(YYJSON data)
{
	int len = data.GetSerializedSize(YYJSON_WRITE_PRETTY_TWO_SPACES);
	char[] buffer = new char[len];
	data.ToString(buffer, len, YYJSON_WRITE_PRETTY_TWO_SPACES);
	PrintToServer("%s", buffer);
}