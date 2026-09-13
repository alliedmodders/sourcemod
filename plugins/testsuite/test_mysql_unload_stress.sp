#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>

public Plugin myinfo =
{
    name = "MySQL unload stress test",
    author = "AlliedModders",
    description = "Verifies that threaded-query unload behavior is safe",
    version = "1.0",
    url = "https://www.sourcemod.net/"
};

#define DATABASE_CONFIG "mysql-stress"
#define RELOAD_COMMAND "sm plugins reload test_mysql_unload_stress"

ConVar g_TestState;
ConVar g_TestCallback;
ConVar g_TestStartedAt;

public void OnPluginStart()
{
    g_TestState = CreateConVar("sm_mysql_unload_stress_state", "0", "Internal state for the MySQL unload stress test.");
    g_TestCallback = CreateConVar("sm_mysql_unload_stress_callback", "0", "Internal callback marker for the MySQL unload stress test.");
    g_TestStartedAt = CreateConVar("sm_mysql_unload_stress_started_at", "0", "Internal start timestamp for the MySQL unload stress test.");

    RegConsoleCmd("sm_mysql_unload_stress", Command_StartStress, "Run the MySQL threaded-query unload stress test.");

    if (g_TestState.IntValue == 1)
    {
        float elapsed = GetEngineTime() - g_TestStartedAt.FloatValue;
        if (g_TestCallback.IntValue == 0 && elapsed < 2.0)
        {
            PrintToServer("[mysql-unload-stress] PASS: discard reloaded promptly and did not run the old callback.");
        }
        else if (g_TestCallback.IntValue == 1 && elapsed >= 4.0)
        {
            PrintToServer("[mysql-unload-stress] PASS: wait preserved the historical callback-before-unload behavior.");
        }
        else
        {
            LogError("[mysql-unload-stress] FAIL: unexpected unload behavior (callback %d, elapsed %.2f seconds).", g_TestCallback.IntValue, elapsed);
        }
        g_TestState.IntValue = 0;
    }
}

public Action Command_StartStress(int client, int args)
{
    if (g_TestState.IntValue != 0)
    {
        ReplyToCommand(client, "[mysql-unload-stress] A test is already in progress.");
        return Plugin_Handled;
    }

    char error[256];
    Database db = SQL_Connect(DATABASE_CONFIG, false, error, sizeof(error));
    if (db == null)
    {
        ReplyToCommand(client, "[mysql-unload-stress] Could not connect: %s", error);
        return Plugin_Handled;
    }

    g_TestState.IntValue = 1;
    g_TestCallback.IntValue = 0;
    g_TestStartedAt.FloatValue = GetEngineTime();
    SQL_TQuery(db, OnSlowQueryComplete, "SELECT SLEEP(5)");
    delete db;

    CreateTimer(0.25, Timer_ReloadPlugin);
    ReplyToCommand(client, "[mysql-unload-stress] Started a five-second query; reloading this plugin shortly.");
    return Plugin_Handled;
}

public Action Timer_ReloadPlugin(Handle timer)
{
    ServerCommand(RELOAD_COMMAND);
    return Plugin_Stop;
}

public void OnSlowQueryComplete(Database db, DBResultSet results, const char[] error, any data)
{
    g_TestCallback.IntValue = 1;
    PrintToServer("[mysql-unload-stress] The old query callback completed before unload.");
}