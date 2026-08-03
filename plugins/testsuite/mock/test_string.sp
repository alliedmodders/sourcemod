#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <testing>


public void OnPluginStart()
{
    Test_StringToI64();
}


void Test_StringToI64()
{
    SetTestContext("Test StringToI64");

    AssertInt64Eq("DEC 0",                    StringToI64("0"),                     0);
    AssertInt64Eq("DEC 1234567",              StringToI64("1234567"),               1234567);
    AssertInt64Eq("DEC 1234567654321",        StringToI64("1234567654321"),         1234567654321);
    AssertInt64Eq("DEC 9223372036854775807",  StringToI64("9223372036854775807"),   9223372036854775807);
    AssertInt64Eq("DEC -1234567",             StringToI64("-1234567"),              -1234567);
    AssertInt64Eq("DEC -1234567654321",       StringToI64("-1234567654321"),        -1234567654321);
    AssertInt64Eq("DEC -9223372036854775807", StringToI64("-9223372036854775807"),  -9223372036854775807);

    int nConsumed;

    // nConsumed
    AssertInt64Eq(
        "result 0",
        StringToI64("0", .nConsumed=nConsumed),
        0);
    AssertEq("nConsumed 0", nConsumed, 1);

    AssertInt64Eq(
        "result 1234567",
        StringToI64("1234567", .nConsumed=nConsumed),
        1234567);
    AssertEq("nConsumed 1234567", nConsumed, 7);

    AssertInt64Eq(
        "result 1234567654321",
        StringToI64("1234567654321", .nConsumed=nConsumed),
        1234567654321);
    AssertEq("nConsumed 1234567654321", nConsumed, 13);

    AssertInt64Eq(
        "result 9223372036854775807",
        StringToI64("9223372036854775807", .nConsumed=nConsumed),
        9223372036854775807);
    AssertEq("nConsumed 9223372036854775807", nConsumed, 19);

    AssertInt64Eq(
        "result -1234567",
        StringToI64("-1234567", .nConsumed=nConsumed),
        -1234567);
    AssertEq("nConsumed -1234567", nConsumed, 8);

    AssertInt64Eq(
        "result -1234567654321",
        StringToI64("-1234567654321", .nConsumed=nConsumed),
        -1234567654321);
    AssertEq("nConsumed -1234567654321", nConsumed, 14);

    AssertInt64Eq(
        "result -9223372036854775807",
        StringToI64("-9223372036854775807", .nConsumed=nConsumed),
        -9223372036854775807);
    AssertEq("nConsumed -9223372036854775807", nConsumed, 20);

    // Special nBase
    AssertInt64Eq(
        "result 10001111101110001111101110110101110110001",
        StringToI64("10001111101110001111101110110101110110001", 2, nConsumed),
        1234567654321);
    AssertEq("nConsumed 10001111101110001111101110110101110110001", nConsumed, 41);

    AssertInt64Eq(
        "result 21756175665661",
        StringToI64("21756175665661", 8, nConsumed),
        1234567654321);
    AssertEq("nConsumed 21756175665661", nConsumed, 14);

    AssertInt64Eq(
        "result 11F71F76BB1",
        StringToI64("11F71F76BB1", 16, nConsumed),
        1234567654321);
    AssertEq("nConsumed 11F71F76BB1", nConsumed, 11);

    // Other
    AssertInt64Eq(
        "result a1b2c3d4e5f6g7",
        StringToI64("a1b2c3d4e5f6g7", .nConsumed=nConsumed),
        0);
    AssertEq("nConsumed a1b2c3d4e5f6g7", nConsumed, 0);

    AssertInt64Eq(
        "result 0b10101",
        StringToI64("0b10101", .nConsumed=nConsumed),
        0);
    AssertEq("nConsumed 0b10101", nConsumed, 1);

    AssertInt64Eq(
        "result 1_234_567",
        StringToI64("1_234_567", .nConsumed=nConsumed),
        1);
    AssertEq("nConsumed 1_234_567", nConsumed, 1);

    AssertInt64Eq(
        "result 9223372036854775807",
        StringToI64("9223372036854775807", .nConsumed=nConsumed),
        9223372036854775807);
    AssertEq("nConsumed 9223372036854775807", nConsumed, 19);

    AssertInt64Eq(
        "result 9223372036854775808",
        StringToI64("9223372036854775807", .nConsumed=nConsumed),
        9223372036854775807);
    AssertEq("nConsumed 9223372036854775808", nConsumed, 19);

    AssertInt64Eq(
        "result 92233720368547758070",
        StringToI64("9223372036854775807", .nConsumed=nConsumed),
        9223372036854775807);
    AssertEq("nConsumed 92233720368547758070", nConsumed, 19);

    AssertInt64Eq(
        "result -9223372036854775808",
        StringToI64("-9223372036854775808", .nConsumed=nConsumed),
        -9223372036854775807-1);
    AssertEq("nConsumed -9223372036854775808", nConsumed, 20);

    AssertInt64Eq(
        "result -9223372036854775809",
        StringToI64("-9223372036854775808", .nConsumed=nConsumed),
        -9223372036854775807-1);
    AssertEq("nConsumed -9223372036854775809", nConsumed, 20);

    AssertInt64Eq(
        "result -92233720368547758080",
        StringToI64("-9223372036854775808", .nConsumed=nConsumed),
        -9223372036854775807-1);
    AssertEq("nConsumed -92233720368547758080", nConsumed, 20);
}
