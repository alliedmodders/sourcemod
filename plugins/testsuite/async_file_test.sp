#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>

public Plugin myinfo =
{
    name = "Async File API Test",
    author = "AlliedModders",
    description = "Manual functional test for the asynchronous file API",
    version = "1.0",
    url = "https://www.sourcemod.net/"
};

enum TestStep
{
    TestStep_CreateDirectory,
    TestStep_DirExists,
    TestStep_Write,
    TestStep_FileExists,
    TestStep_FileSize,
    TestStep_Read,
    TestStep_Copy,
    TestStep_Rename,
    TestStep_OpenDirectory,
    TestStep_DeleteRenamed,
    TestStep_DeleteSource,
    TestStep_RemoveDirectory,
    TestStep_Done,
};

static const char g_Directory[] = "addons/sourcemod/data/async_file_api_test";
static const char g_Source[] = "addons/sourcemod/data/async_file_api_test/source.bin";
static const char g_Copy[] = "addons/sourcemod/data/async_file_api_test/copy.bin";
static const char g_Renamed[] = "addons/sourcemod/data/async_file_api_test/renamed.bin";
static const char g_PathID[] = "DEFAULT_WRITE_PATH";
static int g_Bytes[] = {0x00, 0x01, 0x41, 0x00, 0xff, 0x7f};
static const int g_ByteCount = 6;

static bool g_Running;
static bool g_ValveFS;
static int g_Failures;
static TestStep g_Step;

public void OnPluginStart()
{
    RegAdminCmd("sm_async_file_test", Command_RunOS, ADMFLAG_ROOT,
        "Run the asynchronous file API test against the OS filesystem");
    RegAdminCmd("sm_async_file_test_valve", Command_RunValve, ADMFLAG_ROOT,
        "Run the asynchronous file API test against the Valve filesystem");
}

public Action Command_RunOS(int client, int args)
{
    StartTest(false);
    return Plugin_Handled;
}

public Action Command_RunValve(int client, int args)
{
    StartTest(true);
    return Plugin_Handled;
}

static void StartTest(bool valveFS)
{
    if (g_Running)
    {
        PrintToServer("[async-file-test] A test is already running.");
        return;
    }

    g_Running = true;
    g_ValveFS = valveFS;
    g_Failures = 0;
    g_Step = TestStep_CreateDirectory;

    // Cleanup is deliberately synchronous: it only removes this test's stale
    // artifacts before the async test begins.
    DeleteFile(g_Source, valveFS, g_PathID);
    DeleteFile(g_Copy, valveFS, g_PathID);
    DeleteFile(g_Renamed, valveFS, g_PathID);
    RemoveDir(g_Directory);

    PrintToServer("[async-file-test] Starting %s filesystem test.",
        valveFS ? "Valve" : "OS");
    RunNextStep();
}

static void RunNextStep()
{
    switch (g_Step)
    {
        case TestStep_CreateDirectory:
        {
            CreateDirectoryAsync(g_Directory,
                FPERM_U_READ | FPERM_U_WRITE | FPERM_U_EXEC |
                FPERM_G_READ | FPERM_G_EXEC | FPERM_O_READ | FPERM_O_EXEC,
                OnResult, g_Step, g_ValveFS, g_PathID);
        }
        case TestStep_DirExists:
        {
            DirExistsAsync(g_Directory, OnExists, g_Step, g_ValveFS, g_PathID);
        }
        case TestStep_Write:
        {
            char bytes[6];
            for (int i = 0; i < g_ByteCount; i++)
            {
                bytes[i] = g_Bytes[i];
            }
            WriteFileAsync(g_Source, bytes, g_ByteCount, OnResult, g_Step,
                false, g_ValveFS, g_PathID);
        }
        case TestStep_FileExists:
        {
            FileExistsAsync(g_Source, OnExists, g_Step, g_ValveFS, g_PathID);
        }
        case TestStep_FileSize:
        {
            FileSizeAsync(g_Source, OnFileSize, g_Step, g_ValveFS, g_PathID);
        }
        case TestStep_Read:
        {
            ReadFileAsync(g_Source, OnRead, g_Step, g_ValveFS, g_PathID);
        }
        case TestStep_Copy:
        {
            CopyFileAsync(g_Copy, g_Source, OnResult, g_Step, g_ValveFS,
                g_PathID, g_PathID);
        }
        case TestStep_Rename:
        {
            RenameFileAsync(g_Renamed, g_Copy, OnResult, g_Step, g_ValveFS, g_PathID);
        }
        case TestStep_OpenDirectory:
        {
            if (g_ValveFS)
            {
                PrintToServer("[async-file-test] Skipping Valve directory enumeration (not supported asynchronously).");
                Advance();
                return;
            }

            OpenDirectoryAsync(g_Directory, OnDirectory, g_Step);
        }
        case TestStep_DeleteRenamed:
        {
            DeleteFileAsync(g_Renamed, OnResult, g_Step, g_ValveFS, g_PathID);
        }
        case TestStep_DeleteSource:
        {
            DeleteFileAsync(g_Source, OnResult, g_Step, g_ValveFS, g_PathID);
        }
        case TestStep_RemoveDirectory:
        {
            // RemoveDirAsync currently operates on the OS filesystem only.
            RemoveDirAsync(g_Directory, OnResult, g_Step);
        }
        case TestStep_Done:
        {
            FinishTest();
        }
    }
}

void OnResult(FileOpResult result, const char[] path, any data)
{
    CheckCallbackData(data);
    Check(result == FileOp_Success, "operation result");
    CheckExpectedPath(path);
    Advance();
}

void OnExists(bool exists, const char[] path, any data)
{
    CheckCallbackData(data);
    Check(exists, "entry exists");
    CheckExpectedPath(path);
    Advance();
}

void OnFileSize(FileOpResult result, const char[] path, int size, any data)
{
    CheckCallbackData(data);
    Check(result == FileOp_Success, "file-size result");
    Check(size == g_ByteCount, "file size");
    CheckExpectedPath(path);
    Advance();
}

void OnRead(FileOpResult result, const char[] path, const char[] contents, int size, any data)
{
    CheckCallbackData(data);
    Check(result == FileOp_Success, "read result");
    Check(size == g_ByteCount, "read size");
    CheckExpectedPath(path);
    bool mismatch = false;
    for (int i = 0; i < size && i < g_ByteCount; i++)
    {
        if ((contents[i] & 0xff) != (g_Bytes[i] & 0xff))
        {
            PrintToServer("[async-file-test] Read mismatch at byte %d: expected 0x%02X, async 0x%02X.",
                i, g_Bytes[i] & 0xff, contents[i] & 0xff);
            Check(false, "read byte");
            mismatch = true;
        }
    }
    if (mismatch && g_ValveFS)
        DiagnoseValveRead();
    Advance();
}

void DiagnoseValveRead()
{
    File file = OpenFile(g_Source, "rb", true, g_PathID);
    if (file == null)
    {
        PrintToServer("[async-file-test] DIAG: synchronous Valve read could not open '%s'.", g_Source);
        return;
    }

    int bytes[6];
    int count = file.Read(bytes, sizeof(bytes), 1);
    delete file;

    PrintToServer("[async-file-test] DIAG: synchronous Valve read returned %d bytes: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X.",
        count, bytes[0] & 0xff, bytes[1] & 0xff, bytes[2] & 0xff,
        bytes[3] & 0xff, bytes[4] & 0xff, bytes[5] & 0xff);
}

void OnDirectory(FileOpResult result, const char[] path, DirectoryListing listing, any data)
{
    char name[PLATFORM_MAX_PATH];
    FileType type;
    bool foundSource;
    bool foundRenamed;

    CheckCallbackData(data);
    Check(result == FileOp_Success, "directory result");
    CheckExpectedPath(path);
    Check(listing != null, "directory listing");
    if (listing != null)
    {
        while (listing.GetNext(name, sizeof(name), type))
        {
            if (StrEqual(name, "source.bin"))
            {
                Check(type == FileType_File, "source entry type");
                foundSource = true;
            }
            else if (StrEqual(name, "renamed.bin"))
            {
                Check(type == FileType_File, "renamed entry type");
                foundRenamed = true;
            }
        }
        delete listing;
    }
    Check(foundSource, "source directory entry");
    Check(foundRenamed, "renamed directory entry");
    Advance();
}

static void CheckCallbackData(any data)
{
    Check(data == g_Step, "callback data");
}

static void CheckExpectedPath(const char[] path)
{
    char expected[PLATFORM_MAX_PATH];
    switch (g_Step)
    {
        case TestStep_CreateDirectory, TestStep_DirExists, TestStep_OpenDirectory,
             TestStep_RemoveDirectory:
        {
            strcopy(expected, sizeof(expected), g_Directory);
        }
        case TestStep_Write, TestStep_FileExists, TestStep_FileSize, TestStep_Read,
             TestStep_DeleteSource:
        {
            strcopy(expected, sizeof(expected), g_Source);
        }
        case TestStep_Copy:
        {
            strcopy(expected, sizeof(expected), g_Copy);
        }
        case TestStep_Rename, TestStep_DeleteRenamed:
        {
            strcopy(expected, sizeof(expected), g_Renamed);
        }
    }
    Check(StrEqual(path, expected), "callback path");
}

static void Check(bool condition, const char[] name)
{
    if (condition)
        return;

    g_Failures++;
    PrintToServer("[async-file-test] FAIL: %s (step %d, %s filesystem)", name,
        g_Step, g_ValveFS ? "Valve" : "OS");
}

static void Advance()
{
    g_Step++;
    RunNextStep();
}

static void FinishTest()
{
    PrintToServer("[async-file-test] %s filesystem test %s (%d failure%s).",
        g_ValveFS ? "Valve" : "OS", g_Failures ? "FAILED" : "PASSED", g_Failures,
        g_Failures == 1 ? "" : "s");
    g_Running = false;
}
