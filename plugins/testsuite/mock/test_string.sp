#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <testing>


public void OnPluginStart()
{
    Test_StringToInt64Ex();

    // Bnechmark_StringToInt64Ex();
}


void Test_StringToInt64Ex()
{
    SetTestContext("Test StringToInt64Ex");

    AssertInt64Eq("DEC 0",                    StringToInt64Ex("0"),                     0);
    AssertInt64Eq("DEC 1234567",              StringToInt64Ex("1234567"),               1234567);
    AssertInt64Eq("DEC 1234567654321",        StringToInt64Ex("1234567654321"),         1234567654321);
    AssertInt64Eq("DEC 9223372036854775807",  StringToInt64Ex("9223372036854775807"),   9223372036854775807);
    AssertInt64Eq("DEC -1234567",             StringToInt64Ex("-1234567"),              -1234567);
    AssertInt64Eq("DEC -1234567654321",       StringToInt64Ex("-1234567654321"),        -1234567654321);
    AssertInt64Eq("DEC -9223372036854775807", StringToInt64Ex("-9223372036854775807"),  -9223372036854775807);

    int bytes;

    // bytes
    AssertInt64Eq(
        "result 0", 
        StringToInt64Ex("0", bytes), 
        0);
    AssertEq("bytes  0", bytes, 1);

    AssertInt64Eq(
        "result 1234567", 
        StringToInt64Ex("1234567", bytes), 
        1234567);
    AssertEq("bytes  1234567", bytes, 7);

    AssertInt64Eq(
        "result 1234567654321", 
        StringToInt64Ex("1234567654321", bytes), 
        1234567654321);
    AssertEq("bytes  1234567654321", bytes, 13);

    AssertInt64Eq(
        "result 9223372036854775807", 
        StringToInt64Ex("9223372036854775807", bytes), 
        9223372036854775807);
    AssertEq("bytes  9223372036854775807", bytes, 19);

    AssertInt64Eq(
        "result -1234567", 
        StringToInt64Ex("-1234567", bytes), 
        -1234567);
    AssertEq("bytes  -1234567", bytes, 8);

    AssertInt64Eq(
        "result -1234567654321", 
        StringToInt64Ex("-1234567654321", bytes), 
        -1234567654321);
    AssertEq("bytes  -1234567654321", bytes, 14);

    AssertInt64Eq(
        "result -9223372036854775807", 
        StringToInt64Ex("-9223372036854775807", bytes), 
        -9223372036854775807);
    AssertEq("bytes  -9223372036854775807", bytes, 20);

    // Special nBase
    AssertInt64Eq(
        "result 10001111101110001111101110110101110110001", 
        StringToInt64Ex("10001111101110001111101110110101110110001", bytes, 2),
        1234567654321);
    AssertEq("bytes  10001111101110001111101110110101110110001", bytes, 41);

    AssertInt64Eq(
        "result 21756175665661", 
        StringToInt64Ex("21756175665661", bytes, 8),
        1234567654321);
    AssertEq("bytes  11F71F76BB1", bytes, 14);

    AssertInt64Eq(
        "result 11F71F76BB1", 
        StringToInt64Ex("11F71F76BB1", bytes, 16),
        1234567654321);
    AssertEq("bytes  11F71F76BB1", bytes, 11);

    // Orther
    AssertInt64Eq(
        "result a1b2c3d4e5f6g7", 
        StringToInt64Ex("a1b2c3d4e5f6g7", bytes), 
        0);
    AssertEq("bytes  a1b2c3d4e5f6g7", bytes, 0);

    AssertInt64Eq(
        "result 0b10101", 
        StringToInt64Ex("0b10101", bytes), 
        0);
    AssertEq("bytes  0b10101", bytes, 1);

    AssertInt64Eq(
        "result 1_234_567", 
        StringToInt64Ex("1_234_567", bytes), 
        1);
    AssertEq("bytes  1_234_567", bytes, 1);
}



// ================================================================================================

/*
stock int64 StringToInt64Ex_IncOnly(const char[] str, int &bytes=0, int nBase=10)
{
    // Error in: 9223372036854775807, -1234567654321
    int result[2];
    bytes = StringToInt64(str, result, nBase);
    return (view_as<int64>(result[1]) << 32) | result[0];
}

#include <profiler>

void Bnechmark_StringToInt64Ex()
{
    Profiler profile = new Profiler();
    int iters = 1_000_000;

    profile.Start();
    for (int i = 0; i < iters; ++i)
    {
        StringToInt64Ex("1234567654321");
    }
    profile.Stop();
    float delta = profile.Time;

    profile.Start();
    for (int i = 0; i < iters; ++i)
    {
        StringToInt64Ex_IncOnly("1234567654321");
    }
    profile.Stop();
    float deltaIncOnly = profile.Time;

    delete profile;

    PrintToServer(
        "[benchmark] [StringToInt64Ex] | Iters %7d | Elapsed %6.3f secs %9d/sec", 
        iters, 
        delta, 
        RoundToFloor(iters / delta));

    PrintToServer(
        "[benchmark] [StringToInt64Ex_IncOnly] | Iters %7d | Elapsed %6.3f secs %9d/sec", 
        iters, 
        deltaIncOnly, 
        RoundToFloor(iters / deltaIncOnly));
}
*/
