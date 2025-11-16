#pragma semicolon 1
#pragma newdecls required
#include <testing>

enum struct TestStruct
{
	int intval;
	char strval[32];
}

public void OnPluginStart()
{
	ArrayList list = new ArrayList(sizeof(TestStruct));
	
	// --------------------------------------------------------------------------------

	SetTestContext("EmptyArrayTest");

	AssertEq("test_forward", list.FindString("index3", TestStruct::strval, -1, false), -1);
	AssertEq("test_reverse", list.FindString("index3", TestStruct::strval, -1, true), -1);
	AssertEq("test_forward", list.FindValue(3, TestStruct::intval, -1, false), -1);
	AssertEq("test_reverse", list.FindValue(3, TestStruct::intval, -1, true), -1);

	// --------------------------------------------------------------------------------

	// Fill
	TestStruct ts;
	for (int i = 0; i < 10; i++)
	{
		ts.intval = i;
		Format(ts.strval, sizeof(ts.strval), "index%d", i);
		list.PushArray(ts);
	}

	// --------------------------------------------------------------------------------

	SetTestContext("FindString");

	AssertEq("test_defaults", list.FindString("index3", TestStruct::strval), 3);

	AssertEq("test_forward", list.FindString("index3", TestStruct::strval, -1, false), 3);
	AssertEq("test_forward", list.FindString("index3", TestStruct::strval, 0, false), 3);
	AssertEq("test_forward", list.FindString("index3", TestStruct::strval, 2, false), 3);
	AssertEq("test_forward", list.FindString("index3", TestStruct::strval, 3, false), -1);
	AssertEq("test_forward", list.FindString("index3", TestStruct::strval, 10, false), -1);

	AssertEq("test_reverse", list.FindString("index3", TestStruct::strval, -1, true), 3);
	AssertEq("test_reverse", list.FindString("index3", TestStruct::strval, 0, true), -1);
	AssertEq("test_reverse", list.FindString("index3", TestStruct::strval, 3, true), -1);
	AssertEq("test_reverse", list.FindString("index3", TestStruct::strval, 4, true), 3);
	AssertEq("test_reverse", list.FindString("index3", TestStruct::strval, 10, true), 3);

	AssertEq("test_bottom", list.FindString("index0", TestStruct::strval, -1, false), 0);
	AssertEq("test_bottom", list.FindString("index0", TestStruct::strval, -1, true), 0);
	AssertEq("test_bottom", list.FindString("index0", TestStruct::strval, 1, true), 0);
	AssertEq("test_bottom", list.FindString("index0", TestStruct::strval, 10, false), -1);
	AssertEq("test_bottom", list.FindString("index0", TestStruct::strval, 10, true), 0);

	AssertEq("test_top", list.FindString("index9", TestStruct::strval, -1, false), 9);
	AssertEq("test_top", list.FindString("index9", TestStruct::strval, -1, true), 9);
	AssertEq("test_top", list.FindString("index9", TestStruct::strval, 8, false), 9);
	AssertEq("test_top", list.FindString("index9", TestStruct::strval, 10, false), -1);
	AssertEq("test_top", list.FindString("index9", TestStruct::strval, 10, true), 9);
	
	AssertEq("test_case_sensitive", list.FindString("INDEX0", TestStruct::strval, .caseSensitive = true), -1);
	AssertEq("test_case_sensitive", list.FindString("INDEX0", TestStruct::strval, .caseSensitive = false), 0);

	// --------------------------------------------------------------------------------

	SetTestContext("FindValue");

	AssertEq("test_defaults", list.FindValue(3, TestStruct::intval), 3);

	AssertEq("test_forward", list.FindValue(3, TestStruct::intval, -1, false), 3);
	AssertEq("test_forward", list.FindValue(3, TestStruct::intval, 0, false), 3);
	AssertEq("test_forward", list.FindValue(3, TestStruct::intval, 2, false), 3);
	AssertEq("test_forward", list.FindValue(3, TestStruct::intval, 3, false), -1);
	AssertEq("test_forward", list.FindValue(3, TestStruct::intval, 10, false), -1);

	AssertEq("test_reverse", list.FindValue(3, TestStruct::intval, -1, true), 3);
	AssertEq("test_reverse", list.FindValue(3, TestStruct::intval, 0, true), -1);
	AssertEq("test_reverse", list.FindValue(3, TestStruct::intval, 3, true), -1);
	AssertEq("test_reverse", list.FindValue(3, TestStruct::intval, 4, true), 3);
	AssertEq("test_reverse", list.FindValue(3, TestStruct::intval, 10, true), 3);

	AssertEq("test_bottom", list.FindValue(0, TestStruct::intval, -1, false), 0);
	AssertEq("test_bottom", list.FindValue(0, TestStruct::intval, -1, true), 0);
	AssertEq("test_bottom", list.FindValue(0, TestStruct::intval, 1, true), 0);
	AssertEq("test_bottom", list.FindValue(0, TestStruct::intval, 10, false), -1);
	AssertEq("test_bottom", list.FindValue(0, TestStruct::intval, 10, true), 0);

	AssertEq("test_top", list.FindValue(9, TestStruct::intval, -1, false), 9);
	AssertEq("test_top", list.FindValue(9, TestStruct::intval, -1, true), 9);
	AssertEq("test_top", list.FindValue(9, TestStruct::intval, 8, false), 9);
	AssertEq("test_top", list.FindValue(9, TestStruct::intval, 10, false), -1);
	AssertEq("test_top", list.FindValue(9, TestStruct::intval, 10, true), 9);
	
	// --------------------------------------------------------------------------------

	SetTestContext("IterateOverFindString");
	int found, index;

	// Duplicate last entry
	list.PushArray(ts);

	found = 0; index = -1;
	while ((index = list.FindString("index9", TestStruct::strval, index, false)) != -1)
	{
		found++;
	}
	AssertEq("test_find_all_strings_forward", found, 2);

	found = 0; index = -1;
	while ((index = list.FindString("index9", TestStruct::strval, index, true)) != -1)
	{
		found++;
	}
	AssertEq("test_find_all_strings_reverse", found, 2);
	
	// --------------------------------------------------------------------------------
	
	SetTestContext("IterateOverFindValue");

	found = 0, index = -1;
	while ((index = list.FindValue(9, TestStruct::intval, index, false)) != -1)
	{
		found++;
	}
	AssertEq("test_find_all_values_forward", found, 2);

	found = 0; index = -1;
	while ((index = list.FindValue(9, TestStruct::intval, index, true)) != -1)
	{
		found++;
	}
	AssertEq("test_find_all_values_reverse", found, 2);

	// --------------------------------------------------------------------------------

	PrintToServer("OK");
}