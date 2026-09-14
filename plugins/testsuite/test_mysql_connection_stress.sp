#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>

public Plugin myinfo =
{
    name = "MySQL Connection Stress Test",
    author = "AlliedModders",
    description = "Exercises one MySQL connection from synchronous and threaded queries",
    version = "1.0",
    url = "https://www.sourcemod.net/"
};

static const char g_ConfigName[] = "mysql-stress";
static const int g_DefaultRounds = 5000;
static const int g_AsyncQueriesPerRound = 4;

static Database g_Database;
static DBStatement g_Statement;
static bool g_Running;
static int g_TargetRounds;
static int g_RoundsRun;
static int g_QueriesSubmitted;
static int g_QueriesCompleted;
static int g_ExpectedValue;
static int g_Failures;

public void OnPluginStart()
{
    RegServerCmd("sm_mysql_stress", Command_Run,
        "Stress a shared MySQL connection. Usage: sm_mysql_stress [rounds]");
}

public Action Command_Run(int args)
{
    if (g_Running)
    {
        PrintToServer("[mysql-stress] A test is already running.");
        return Plugin_Handled;
    }

    int rounds = g_DefaultRounds;
    if (args >= 1)
    {
        char argument[16];
        GetCmdArg(1, argument, sizeof(argument));
        rounds = StringToInt(argument);
    }

    if (rounds < 1 || rounds > 100000)
    {
        PrintToServer("[mysql-stress] Rounds must be between 1 and 100000.");
        return Plugin_Handled;
    }

    StartTest(rounds);
    return Plugin_Handled;
}

static void StartTest(int rounds)
{
    char error[256];
    g_Database = SQL_Connect(g_ConfigName, false, error, sizeof(error));
    if (g_Database == null)
    {
        PrintToServer("[mysql-stress] Connection to '%s' failed: %s", g_ConfigName, error);
        return;
    }

    if (!PrepareDatabase())
    {
        CloseTestHandles();
        return;
    }

    g_Statement = SQL_PrepareQuery(g_Database,
        "UPDATE sm_mysql_connection_stress SET value = value + 1 WHERE id = ?",
        error, sizeof(error));
    if (g_Statement == null)
    {
        PrintToServer("[mysql-stress] Failed to prepare statement: %s", error);
        CloseTestHandles();
        return;
    }

    g_Running = true;
    g_TargetRounds = rounds;
    g_RoundsRun = 0;
    g_QueriesSubmitted = 0;
    g_QueriesCompleted = 0;
    g_ExpectedValue = 0;
    g_Failures = 0;

    PrintToServer("[mysql-stress] Starting %d rounds (%d threaded queries per round).",
        g_TargetRounds, g_AsyncQueriesPerRound);
    CreateTimer(0.01, Timer_RunRound, _, TIMER_REPEAT);
}

static bool PrepareDatabase()
{
    return FastQuery("CREATE TABLE IF NOT EXISTS sm_mysql_connection_stress (id INT PRIMARY KEY, value INT NOT NULL)", "create table")
        && FastQuery("INSERT INTO sm_mysql_connection_stress (id, value) VALUES (1, 0) ON DUPLICATE KEY UPDATE value = 0", "reset table");
}

public Action Timer_RunRound(Handle timer)
{
    if (g_RoundsRun == g_TargetRounds)
    {
        FinishIfComplete();
        return Plugin_Stop;
    }

    for (int i = 0; i < g_AsyncQueriesPerRound; i++)
    {
        g_Database.Query(OnThreadedQuery,
            "SELECT SLEEP(0.002) AS delay, value FROM sm_mysql_connection_stress WHERE id = 1");
        g_QueriesSubmitted++;
    }

    RunSynchronousRound();
    g_RoundsRun++;
    return Plugin_Continue;
}

static void RunSynchronousRound()
{
    char escaped[128];
    int written;
    if (!SQL_EscapeString(g_Database, "quote ' test", escaped, sizeof(escaped), written)
        || written == 0)
    {
        ReportDatabaseFailure("escape string", g_Database);
    }

    if (FastQuery("UPDATE sm_mysql_connection_stress SET value = value + 1 WHERE id = 1", "fast query"))
    {
        g_ExpectedValue++;
    }

    DBResultSet result = SQL_Query(g_Database,
        "SELECT value FROM sm_mysql_connection_stress WHERE id = 1");
    if (result == null || !result.FetchRow())
    {
        ReportDatabaseFailure("synchronous query", g_Database);
    }
    delete result;

    SQL_BindParamInt(g_Statement, 0, 1);
    if (!SQL_Execute(g_Statement))
    {
        ReportDatabaseFailure("prepared statement", g_Statement);
    }
    else
    {
        g_ExpectedValue++;
    }

    if (g_RoundsRun % 64 == 0)
    {
        if (!SQL_SetCharset(g_Database, "utf8"))
        {
            ReportDatabaseFailure("set charset", g_Database);
        }

        // This verifies that the public recursive lock still works when the
        // driver also acquires it internally.
        SQL_LockDatabase(g_Database);
        bool succeeded = FastQuery(
            "UPDATE sm_mysql_connection_stress SET value = value + 1 WHERE id = 1",
            "explicit lock query");
        SQL_UnlockDatabase(g_Database);

        if (succeeded)
        {
            g_ExpectedValue++;
        }
    }
}

public void OnThreadedQuery(Database db, DBResultSet results, const char[] error, any data)
{
    g_QueriesCompleted++;

    if (results == null)
    {
        ReportFailure("threaded query failed: %s", error);
    }
    else if (!results.FetchRow())
    {
        ReportFailure("threaded query returned no row");
    }
    else
    {
        results.FetchInt(1);
    }

    FinishIfComplete();
}

static bool FastQuery(const char[] query, const char[] operation)
{
    if (SQL_FastQuery(g_Database, query))
    {
        return true;
    }

    ReportDatabaseFailure(operation, g_Database);
    return false;
}

static void FinishIfComplete()
{
    if (!g_Running || g_RoundsRun != g_TargetRounds || g_QueriesCompleted != g_QueriesSubmitted)
    {
        return;
    }

    DBResultSet result = SQL_Query(g_Database,
        "SELECT value FROM sm_mysql_connection_stress WHERE id = 1");
    if (result == null || !result.FetchRow())
    {
        ReportDatabaseFailure("final value query", g_Database);
    }
    else
    {
        int actual = result.FetchInt(0);
        if (actual != g_ExpectedValue)
        {
            ReportFailure("final value mismatch: expected %d, received %d", g_ExpectedValue, actual);
        }
    }
    delete result;

    PrintToServer("[mysql-stress] %s: %d rounds, %d threaded queries, %d failure%s.",
        g_Failures == 0 ? "PASS" : "FAIL", g_RoundsRun, g_QueriesCompleted, g_Failures,
        g_Failures == 1 ? "" : "s");

    g_Running = false;
    CloseTestHandles();
}

static void ReportDatabaseFailure(const char[] operation, Handle handle)
{
    char error[256];
    SQL_GetError(handle, error, sizeof(error));
    ReportFailure("%s failed: %s", operation, error);
}

static void ReportFailure(const char[] format, any ...)
{
    g_Failures++;

    if (g_Failures <= 10)
    {
        char message[256];
        VFormat(message, sizeof(message), format, 2);
        PrintToServer("[mysql-stress] FAIL: %s", message);
    }
}

static void CloseTestHandles()
{
    delete g_Statement;
    delete g_Database;
}
