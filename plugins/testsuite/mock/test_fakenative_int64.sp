#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <testing>

// Has both halves set, so a value truncated to one cell fails the check.
int64 g_Value;

native int64 T_ReturnInt64(int marker);
native int64 T_ReturnCell(int marker);
native int T_TakeInt64(int64 value, int marker);
native void T_SetInt64Ref(int64 &value, int marker);
native int T_GetInt64Array(const int64[] values, int size);
native void T_SetInt64Array(int64[] values, int size);
native Address T_ReturnAddress(int marker);
native int T_TakeAddress(Address value, int marker);
native void T_SetAddressRef(Address &value, int marker);
native int T_GetAddressArray(const Address[] values, int size);
native void T_SetAddressArray(Address[] values, int size);


public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
    CreateNative("T_ReturnInt64", Native_ReturnInt64, NativeFlag_ReturnsInt64);
    CreateNative("T_ReturnCell", Native_ReturnCell, NativeFlag_ReturnsInt64);
    CreateNative("T_TakeInt64", Native_TakeInt64);
    CreateNative("T_SetInt64Ref", Native_SetInt64Ref);
    CreateNative("T_GetInt64Array", Native_GetInt64Array);
    CreateNative("T_SetInt64Array", Native_SetInt64Array);
    CreateNative("T_ReturnAddress", Native_ReturnAddress, NativeFlag_ReturnsAddress);
    CreateNative("T_TakeAddress", Native_TakeAddress);
    CreateNative("T_SetAddressRef", Native_SetAddressRef);
    CreateNative("T_GetAddressArray", Native_GetAddressArray);
    CreateNative("T_SetAddressArray", Native_SetAddressArray);
    return APLRes_Success;
}


public void OnPluginStart()
{
    g_Value = (view_as<int64>(0x12345678) << 32) | view_as<int64>(0x0badf00d);

    SetTestContext("Test SetNativeReturnInt64");
    AssertInt64Eq("T_ReturnInt64", T_ReturnInt64(7), g_Value);

    SetTestContext("Test a cell returned from an int64 native");
    AssertInt64Eq("T_ReturnCell", T_ReturnCell(7), view_as<int64>(0x0badf00d));

    SetTestContext("Test GetNativeInt64");
    AssertEq("T_TakeInt64", T_TakeInt64(g_Value, 7), 1);

    SetTestContext("Test SetNativeInt64Ref");
    int64 value = 0;
    T_SetInt64Ref(value, 7);
    AssertInt64Eq("value", value, g_Value);

    SetTestContext("Test GetNativeInt64Array");
    int64 values[3];
    for (int i = 0; i < sizeof(values); i++)
        values[i] = g_Value + i;
    AssertEq("T_GetInt64Array", T_GetInt64Array(values, sizeof(values)), 1);

    SetTestContext("Test SetNativeInt64Array");
    int64 filled[3];
    T_SetInt64Array(filled, sizeof(filled));
    for (int i = 0; i < sizeof(filled); i++)
        AssertInt64Eq("filled", filled[i], g_Value + i);

    SetTestContext("Test SetNativeReturnAddress");
    AssertInt64Eq("T_ReturnAddress", view_as<int64>(T_ReturnAddress(7)), g_Value);

    SetTestContext("Test GetNativeAddress");
    AssertEq("T_TakeAddress", T_TakeAddress(view_as<Address>(g_Value), 7), 1);

    SetTestContext("Test SetNativeAddressRef");
    Address address = Address_Null;
    T_SetAddressRef(address, 7);
    AssertInt64Eq("address", view_as<int64>(address), g_Value);

    SetTestContext("Test GetNativeAddressArray");
    Address addresses[3];
    for (int i = 0; i < sizeof(addresses); i++)
        addresses[i] = view_as<Address>(g_Value + i);
    AssertEq("T_GetAddressArray", T_GetAddressArray(addresses, sizeof(addresses)), 1);

    SetTestContext("Test SetNativeAddressArray");
    Address filled_addresses[3];
    T_SetAddressArray(filled_addresses, sizeof(filled_addresses));
    for (int i = 0; i < sizeof(filled_addresses); i++)
        AssertInt64Eq("filled_addresses", view_as<int64>(filled_addresses[i]), g_Value + i);
}


public any Native_ReturnInt64(Handle plugin, int numParams)
{
    AssertEq("Native_ReturnInt64 numParams", numParams, 1);
    AssertEq("Native_ReturnInt64 marker", GetNativeCell(1), 7);
    return SetNativeReturnInt64(g_Value);
}


public any Native_ReturnCell(Handle plugin, int numParams)
{
    AssertEq("Native_ReturnCell numParams", numParams, 1);
    AssertEq("Native_ReturnCell marker", GetNativeCell(1), 7);
    return 0x0badf00d;
}


public any Native_TakeInt64(Handle plugin, int numParams)
{
    AssertEq("Native_TakeInt64 numParams", numParams, 2);
    AssertInt64Eq("Native_TakeInt64 value", GetNativeInt64(1), g_Value);
    AssertEq("Native_TakeInt64 marker", GetNativeCell(2), 7);
    return 1;
}


public any Native_SetInt64Ref(Handle plugin, int numParams)
{
    AssertEq("Native_SetInt64Ref numParams", numParams, 2);
    AssertEq("Native_SetInt64Ref marker", GetNativeCell(2), 7);
    SetNativeInt64Ref(1, g_Value);
    return 0;
}


public any Native_GetInt64Array(Handle plugin, int numParams)
{
    int64 values[3];
    AssertEq("Native_GetInt64Array size", GetNativeCell(2), sizeof(values));
    AssertEq("GetNativeInt64Array", GetNativeInt64Array(1, values, sizeof(values)), SP_ERROR_NONE);
    for (int i = 0; i < sizeof(values); i++)
        AssertInt64Eq("Native_GetInt64Array value", values[i], g_Value + i);
    return 1;
}


public any Native_SetInt64Array(Handle plugin, int numParams)
{
    int64 values[3];
    for (int i = 0; i < sizeof(values); i++)
        values[i] = g_Value + i;
    AssertEq("SetNativeInt64Array", SetNativeInt64Array(1, values, sizeof(values)), SP_ERROR_NONE);
    return 0;
}


public any Native_ReturnAddress(Handle plugin, int numParams)
{
    AssertEq("Native_ReturnAddress marker", GetNativeCell(1), 7);
    return SetNativeReturnAddress(view_as<Address>(g_Value));
}


public any Native_TakeAddress(Handle plugin, int numParams)
{
    AssertInt64Eq("Native_TakeAddress value", view_as<int64>(GetNativeAddress(1)), g_Value);
    AssertEq("Native_TakeAddress marker", GetNativeCell(2), 7);
    return 1;
}


public any Native_SetAddressRef(Handle plugin, int numParams)
{
    AssertEq("Native_SetAddressRef marker", GetNativeCell(2), 7);
    SetNativeAddressRef(1, view_as<Address>(g_Value));
    return 0;
}


public any Native_GetAddressArray(Handle plugin, int numParams)
{
    Address values[3];
    AssertEq("GetNativeAddressArray", GetNativeAddressArray(1, values, sizeof(values)), SP_ERROR_NONE);
    for (int i = 0; i < sizeof(values); i++)
        AssertInt64Eq("Native_GetAddressArray value", view_as<int64>(values[i]), g_Value + i);
    return 1;
}


public any Native_SetAddressArray(Handle plugin, int numParams)
{
    Address values[3];
    for (int i = 0; i < sizeof(values); i++)
        values[i] = view_as<Address>(g_Value + i);
    AssertEq("SetNativeAddressArray", SetNativeAddressArray(1, values, sizeof(values)), SP_ERROR_NONE);
    return 0;
}
