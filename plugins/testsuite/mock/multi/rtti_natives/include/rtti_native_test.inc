#if defined _rtti_native_test_included
 #endinput
#endif
#define _rtti_native_test_included

enum struct TestItem
{
	int id;
	int quantity;
	float weight;
}

methodmap GoldWallet
{
	public GoldWallet(int client)
	{
		return view_as<GoldWallet>(client);
	}

	public native int64 GetBalance();
	public native void AddBalance(int64 delta, int64 &outNewBalance);
}

native int64 RTTITest_AddGold(int client, const char[] reason, int64 amount, int &newTotal);
native void RTTITest_MutateInt64(int64 &val, int64 addend);
native int RTTITest_FormatGreeting(const char[] name, char[] outBuf, int maxLen);
native int RTTITest_SumAndScaleArray(int[] values, int count, int scale);
native int RTTITest_ModifyEnumStruct(int a, int b, int c, any[] itemOut);
native int RTTITest_VariadicFormat(char[] buffer, int maxlength, const char[] format, any ...);
native int RTTITest_LegacyAdd(int a, int b);
native Address RTTITest_OffsetAddress(Address base, int64 offset, Address &outPrev);
