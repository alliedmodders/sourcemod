#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <testing>

// Has both halves set, so a value truncated to one cell fails the check.
int64 g_Value;
int g_Calls;


public void OnPluginStart()
{
    g_Value = (view_as<int64>(0x12345678) << 32) | view_as<int64>(0x0badf00d);

    Test_PushInt64ToFunction();
    Test_PushInt64ToGlobalForward();
    Test_PushInt64ToAnyParam();
    Test_PushInt64ToCellParam();
    Test_PushAddressToFunction();
    Test_PushAddressToGlobalForward();
}


void Test_PushInt64ToFunction()
{
    SetTestContext("Test Call_PushInt64 to a function");

    g_Calls = 0;
    Call_StartFunction(INVALID_HANDLE, __OnInt64);
    Call_PushInt64(g_Value);
    Call_PushCell(2);
    AssertEq("Call_Finish", Call_Finish(), SP_ERROR_NONE);
    AssertEq("Calls", g_Calls, 1);
}


void Test_PushInt64ToGlobalForward()
{
    SetTestContext("Test Call_PushInt64 to a Param_Int64 forward");

    GlobalForward fwd = new GlobalForward("__OnInt64", ET_Ignore, Param_Int64, Param_Cell);

    g_Calls = 0;
    Call_StartForward(fwd);
    Call_PushInt64(g_Value);
    Call_PushCell(2);
    AssertEq("Call_Finish", Call_Finish(), SP_ERROR_NONE);
    AssertEq("Calls", g_Calls, 1);

    delete fwd;
}


void Test_PushInt64ToAnyParam()
{
    SetTestContext("Test Call_PushInt64 to a Param_Any forward");

    PrivateForward fwd = new PrivateForward(ET_Ignore, Param_Any, Param_Cell);
    fwd.AddFunction(INVALID_HANDLE, __OnInt64);

    g_Calls = 0;
    Call_StartForward(fwd);
    Call_PushInt64(g_Value);
    Call_PushCell(2);
    AssertEq("Call_Finish", Call_Finish(), SP_ERROR_NONE);
    AssertEq("Calls", g_Calls, 1);

    delete fwd;
}


void Test_PushInt64ToCellParam()
{
    SetTestContext("Test Call_PushInt64 to a Param_Cell forward");

    PrivateForward fwd = new PrivateForward(ET_Ignore, Param_Cell, Param_Cell);
    fwd.AddFunction(INVALID_HANDLE, __OnInt64);

    g_Calls = 0;
    Call_StartForward(fwd);
    Call_PushInt64(g_Value);
    Call_PushCell(2);
    AssertEq("Call_Finish", Call_Finish(), SP_ERROR_PARAM);
    AssertEq("Calls", g_Calls, 0);

    delete fwd;
}


void Test_PushAddressToFunction()
{
    SetTestContext("Test Call_PushAddress to a function");

    g_Calls = 0;
    Call_StartFunction(INVALID_HANDLE, __OnAddress);
    Call_PushAddress(view_as<Address>(g_Value));
    Call_PushCell(2);
    AssertEq("Call_Finish", Call_Finish(), SP_ERROR_NONE);
    AssertEq("Calls", g_Calls, 1);
}


void Test_PushAddressToGlobalForward()
{
    SetTestContext("Test Call_PushAddress to a Param_SMAddress forward");

    GlobalForward fwd = new GlobalForward("__OnAddress", ET_Ignore, Param_SMAddress, Param_Cell);

    g_Calls = 0;
    Call_StartForward(fwd);
    Call_PushAddress(view_as<Address>(g_Value));
    Call_PushCell(2);
    AssertEq("Call_Finish", Call_Finish(), SP_ERROR_NONE);
    AssertEq("Calls", g_Calls, 1);

    delete fwd;
}

public void __OnInt64(int64 value, int marker)
{
    g_Calls++;
    AssertInt64Eq("__OnInt64 param value", value, g_Value);
    AssertEq("__OnInt64 param marker", marker, 2);
}


public void __OnAddress(Address value, int marker)
{
    g_Calls++;
    AssertInt64Eq("__OnAddress param value", view_as<int64>(value), g_Value);
    AssertEq("__OnAddress param marker", marker, 2);
}
