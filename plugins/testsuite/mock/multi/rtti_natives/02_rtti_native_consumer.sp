#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <testing>
#include <rtti_native_test>

public Plugin myinfo =
{
	name = "RTTI Native Consumer Test",
	author = "SourceMod",
	description = "Tests consuming RTTI auto-registered dynamic natives across plugins",
	version = "1.0.0",
	url = "https://www.sourcemod.net/"
};

public void OnPluginStart()
{
	SetTestContext("Test RTTI int64 return, const char[], int64 value, and int& byref");
	int txCount = 0;
	int64 amount = 5000000000; // > 32-bit INT_MAX
	int64 totalGold = RTTITest_AddGold(1, "Quest Reward", amount, txCount);
	AssertInt64Eq("totalGold", totalGold, 5000000000);
	AssertEq("txCount", txCount, 1);

	SetTestContext("Test RTTI int64& byref and int64 value");
	int64 mutVal = 123456789012;
	int64 addend = 10;
	RTTITest_MutateInt64(mutVal, addend);
	AssertInt64Eq("mutVal", mutVal, 246913578034);

	SetTestContext("Test RTTI const char[] and mutable char[] copyback");
	char greeting[64];
	int len = RTTITest_FormatGreeting("Ellis", greeting, sizeof(greeting));
	AssertEq("greeting len", len, 13);
	AssertStrEq("greeting", greeting, "Hello, Ellis!");

	SetTestContext("Test RTTI mutable int[] array copyback");
	int arr[4] = { 10, 20, 30, 40 };
	int expectedArr[4] = { 30, 60, 90, 120 };
	int sum = RTTITest_SumAndScaleArray(arr, sizeof(arr), 3);
	AssertEq("array sum", sum, 100);
	AssertArrayEq("scaled array", arr, expectedArr, sizeof(arr));

	SetTestContext("Test RTTI enum struct sizing and copyback");
	TestItem item;
	item.id = 7;
	item.quantity = 100;
	item.weight = 1.5;
	int prevId = RTTITest_ModifyEnumStruct(10, 20, 5, item);
	AssertEq("prevId", prevId, 7);
	AssertEq("item.id", item.id, 30);
	AssertEq("item.quantity", item.quantity, 105);
	AssertTrue("item.weight", item.weight == 3.0);

	SetTestContext("Test RTTI methodmap __native_ClassName__MethodName");
	GoldWallet wallet = GoldWallet(1);
	int64 updatedBal = 0;
	int64 deltaBal = 1000000000;
	wallet.AddBalance(deltaBal, updatedBal);
	int64 queriedBal = wallet.GetBalance();
	AssertInt64Eq("updatedBal", updatedBal, 6000000000);
	AssertInt64Eq("queriedBal", queriedBal, 6000000000);

	SetTestContext("Test RTTI variadic native with FormatNativeString");
	char formatted[128];
	int written = RTTITest_VariadicFormat(formatted, sizeof(formatted), "Score=%d", 42);
	AssertEq("written", written, 8);
	AssertStrEq("formatted", formatted, "Score=42");

	SetTestContext("Test RTTI legacy (Handle, int) fallback");
	int legacySum = RTTITest_LegacyAdd(15, 27);
	AssertEq("legacySum", legacySum, 42);

	SetTestContext("Test RTTI Address (intptr/int64) return, value, and byref");
	Address baseAddr = view_as<Address>((view_as<int64>(0x12345678) << 32) | view_as<int64>(0x0badf000));
	Address prevAddr = Address_Null;
	Address newAddr = RTTITest_OffsetAddress(baseAddr, 0x0d, prevAddr);
	AssertInt64Eq("prevAddr", view_as<int64>(prevAddr), view_as<int64>(baseAddr));
	AssertInt64Eq("newAddr", view_as<int64>(newAddr), (view_as<int64>(0x12345678) << 32) | view_as<int64>(0x0badf00d));
}
