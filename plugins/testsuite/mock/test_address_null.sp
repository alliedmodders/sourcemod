#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <testing>

// Each use of Address_Null below needs a constant expression, so this plugin only compiles if it is one.
Address g_Address = Address_Null;
Address g_Addresses[2] = { Address_Null, Address_Null };


public void OnPluginStart()
{
    Test_GlobalInitializer();
    Test_ArrayInitializer();
    Test_StaticLocal();
    Test_DefaultArgument();
}


void Test_GlobalInitializer()
{
    SetTestContext("Test Address_Null as a global initializer");

    AssertInt64Eq("g_Address", view_as<int64>(g_Address), 0);
}


void Test_ArrayInitializer()
{
    SetTestContext("Test Address_Null in an array initializer");

    AssertInt64Eq("g_Addresses[0]", view_as<int64>(g_Addresses[0]), 0);
    AssertInt64Eq("g_Addresses[1]", view_as<int64>(g_Addresses[1]), 0);
}


void Test_StaticLocal()
{
    SetTestContext("Test Address_Null as a static local initializer");

    static Address address = Address_Null;
    AssertInt64Eq("address", view_as<int64>(address), 0);
}


void Test_DefaultArgument()
{
    SetTestContext("Test Address_Null as a default argument");

    AssertTrue("IsNull()", IsNull());
    AssertFalse("IsNull(1)", IsNull(view_as<Address>(1)));
}


bool IsNull(Address address = Address_Null)
{
    return address == Address_Null;
}
