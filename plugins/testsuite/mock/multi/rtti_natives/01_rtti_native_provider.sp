#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <testing>
#include <rtti_native_test>

int64 g_PlayerGold[MAXPLAYERS + 1];
int g_PlayerTransactions[MAXPLAYERS + 1];

public Plugin myinfo =
{
	name = "RTTI Native Provider Test",
	author = "SourceMod",
	description = "Tests RTTI auto-registered dynamic natives without CreateNative",
	version = "1.0.0",
	url = "https://www.sourcemod.net/"
};

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	// Notice: NO CreateNative calls! All natives below are auto-registered via __native_ RTTI!
	RegPluginLibrary("rtti_native_test");
	return APLRes_Success;
}

// 1. Returns int64, takes cell, const char[], int64 by value, and int& byref
public int64 __native_RTTITest_AddGold(int client, const char[] reason, int64 amount, int &newTotal)
{
	SetTestContext("Provider __native_RTTITest_AddGold");
	AssertStrEq("reason", reason, "Quest Reward");

	if (client < 0 || client > MAXPLAYERS)
	{
		ThrowError("Invalid client index %d (reason: %s)", client, reason);
	}

	g_PlayerGold[client] += amount;
	g_PlayerTransactions[client] += 1;
	newTotal = g_PlayerTransactions[client];
	return g_PlayerGold[client];
}

// 2. Takes int64& byref and int64 by value
public void __native_RTTITest_MutateInt64(int64 &val, int64 addend)
{
	val = (val * 2) + addend;
}

// 3. Takes const char[], mutable char[], and int maxLen
public int __native_RTTITest_FormatGreeting(const char[] name, char[] outBuf, int maxLen)
{
	return Format(outBuf, maxLen, "Hello, %s!", name);
}

// 4. Takes mutable int[] array, count, and scale
public int __native_RTTITest_SumAndScaleArray(int[] values, int count, int scale)
{
	int sum = 0;
	for (int i = 0; i < count; i++)
	{
		sum += values[i];
		values[i] *= scale;
	}
	return sum;
}

// 5. Takes enum struct (or any[]) and mutates fields
public int __native_RTTITest_ModifyEnumStruct(int a, int b, int c, TestItem itemOut)
{
	int prevId = itemOut.id;
	itemOut.id = a + b;
	itemOut.quantity += c;
	itemOut.weight *= 2.0;
	return prevId;
}

// 6. Methodmap natives encoded via __native_ClassName__MethodName
public int64 __native_GoldWallet__GetBalance(GoldWallet self)
{
	int client = view_as<int>(self);
	return g_PlayerGold[client];
}

public void __native_GoldWallet__AddBalance(GoldWallet self, int64 delta, int64 &outNewBalance)
{
	int client = view_as<int>(self);
	g_PlayerGold[client] += delta;
	outNewBalance = g_PlayerGold[client];
}

// 7. Variadic native -> marshals formal args + varargs package and sets up ScopedFakeNativeCall
public int __native_RTTITest_VariadicFormat(char[] buffer, int maxlength, const char[] format, any ...)
{
	int written = 0;
	FormatNativeString(0, 3, 4, maxlength, written, buffer);
	return written;
}

// 8. Legacy (Handle, int) -> int signature -> automatically uses FakeNativeRouter fallback
public int __native_RTTITest_LegacyAdd(Handle plugin, int numParams)
{
	return GetNativeCell(1) + GetNativeCell(2);
}

// 9. Address (intptr / int64) return, value, and byref
public Address __native_RTTITest_OffsetAddress(Address base, int64 offset, Address &outPrev)
{
	outPrev = base;
	return view_as<Address>(view_as<int64>(base) + offset);
}
