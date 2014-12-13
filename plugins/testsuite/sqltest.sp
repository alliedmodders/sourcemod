#include <sourcemod>
#include <testing>

public Plugin:myinfo = 
{
	name = "SQL Testing Lab",
	author = "AlliedModders LLC",
	description = "Tests basic function calls",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

public OnPluginStart()
{
	RegServerCmd("sql_test_normal", Command_TestSql1)
	RegServerCmd("sql_test_stmt", Command_TestSql2)
	RegServerCmd("sql_test_thread1", Command_TestSql3)
	RegServerCmd("sql_test_thread2", Command_TestSql4)
	RegServerCmd("sql_test_thread3", Command_TestSql5)
	RegServerCmd("sql_test_txn", Command_TestTxn)

	new Handle:hibernate = FindConVar("sv_hibernate_when_empty");
	if (hibernate != null) {
		ServerCommand("sv_hibernate_when_empty 0");
	}
}

PrintQueryData(Handle:query)
{
	if (!SQL_HasResultSet(query))
	{
		PrintToServer("Query Handle %x has no results", query)
		return
	}
	
	new rows = SQL_GetRowCount(query)
	new fields = SQL_GetFieldCount(query)
	
	decl String:fieldNames[fields][32]
	PrintToServer("Fields: %d", fields)
	for (new i=0; i<fields; i++)
	{
		SQL_FieldNumToName(query, i, fieldNames[i], 32)
		PrintToServer("-> Field %d: \"%s\"", i, fieldNames[i])
	}
	
	PrintToServer("Rows: %d", rows)
	decl String:result[255]
	new row
	while (SQL_FetchRow(query))
	{
		row++
		PrintToServer("Row %d:", row)
		for (new i=0; i<fields; i++)
		{
			SQL_FetchString(query, i, result, sizeof(result))
			PrintToServer(" [%s] %s", fieldNames[i], result)
		}
	}
}

public Action:Command_TestSql1(args)
{
	new String:error[255]
	new Handle:db = SQL_DefConnect(error, sizeof(error))
	if (db == null)
	{
		PrintToServer("Failed to connect: %s", error)
		return Plugin_Handled
	}
	
	new Handle:query = SQL_Query(db, "SELECT * FROM gab")
	if (query == null)
	{
		SQL_GetError(db, error, sizeof(error))
		PrintToServer("Failed to query: %s", error)
	} else {
		PrintQueryData(query)
		delete query
	}
	
	delete db
	
	return Plugin_Handled;
}

public Action:Command_TestSql2(args)
{
	new String:error[255]
	new Handle:db = SQL_DefConnect(error, sizeof(error))
	if (db == null)
	{
		PrintToServer("Failed to connect: %s", error)
		return Plugin_Handled
	}
	
	new Handle:stmt = SQL_PrepareQuery(db, "SELECT * FROM gab WHERE gaben > ?", error, sizeof(error))
	if (stmt == null)
	{
		PrintToServer("Failed to prepare query: %s", error)
	} else {
		SQL_BindParamInt(stmt, 0, 1)
		if (!SQL_Execute(stmt))
		{
			SQL_GetError(stmt, error, sizeof(error))
			PrintToServer("Failed to execute query: %s", error)
		} else {
			PrintQueryData(stmt)
		}
		delete stmt
	}
	
	delete db
	
	return Plugin_Handled;
}

new Handle:g_ThreadedHandle = null;

public CallbackTest3(Handle:owner, Handle:hndl, const String:error[], any:data)
{
	PrintToServer("CallbackTest1() (owner %x) (hndl %x) (error \"%s\") (data %d)", owner, hndl, error, data);
	if (g_ThreadedHandle != null && hndl != null)
	{
		delete hndl;
	} else {
		g_ThreadedHandle = hndl;
	}
}

public Action:Command_TestSql3(args)
{
	if (g_ThreadedHandle != null)
	{
		PrintToServer("A threaded connection already exists, run the next test");
		return Plugin_Handled;
	}
	
	new String:name[32];
	GetCmdArg(1, name, sizeof(name));
	
	SQL_TConnect(CallbackTest3, name);
	
	return Plugin_Handled;
}


public Action:Command_TestSql4(args)
{
	SQL_LockDatabase(g_ThreadedHandle);
	new Handle:query = SQL_Query(g_ThreadedHandle, "SELECT * FROM gab")
	if (query == null)
	{
		new String:error[255];
		SQL_GetError(g_ThreadedHandle, error, sizeof(error))
		PrintToServer("Failed to query: %s", error)
	} else {
		PrintQueryData(query)
		delete query
	}
	SQL_UnlockDatabase(g_ThreadedHandle);
	
	return Plugin_Handled;
}

public CallbackTest5(Handle:owner, Handle:hndl, const String:error[], any:data)
{
	if (hndl == null)
	{
		PrintToServer("Failed to query: %s", error)
	} else {
		PrintQueryData(hndl)
		SQL_TQuery(g_ThreadedHandle, CallbackTest6, "UPDATE gab SET `gaben` = `gaben` + 1 WHERE `gaben` >= 4", 52)
	}
}

public CallbackTest6(Handle:owner, Handle:hndl, const String:error[], any:data)
{
	if (hndl == null)
	{
		PrintToServer("Failed to query: %s", error)
	} else {
		PrintToServer("Queried!");
		SQL_TQuery(g_ThreadedHandle, CallbackTest7, "UPDATE gab SET `gaben` = `gaben` + 1 WHERE `gaben` >= 4", 52)
	}
}

public CallbackTest7(Handle:owner, Handle:hndl, const String:error[], any:data)
{
	if (hndl == null)
	{
		PrintToServer("Failed to query: %s", error)
	} else {
		PrintToServer("Queried!");
	}
}

public Action:Command_TestSql5(args)
{
	SQL_TQuery(g_ThreadedHandle, CallbackTest5, "SELECT * FROM gab", 52)
	SQL_TQuery(g_ThreadedHandle, CallbackTest5, "SELECT * FROM gab", 52)
	SQL_TQuery(g_ThreadedHandle, CallbackTest5, "SELECT * FROM gab", 52)
	SQL_TQuery(g_ThreadedHandle, CallbackTest5, "SELECT * FROM gab", 52)
	
	return Plugin_Handled;
}

FastQuery(Handle:db, const String:query[])
{
	new String:error[256];
	if (!SQL_FastQuery(db, query)) {
		SQL_GetError(db, error, sizeof(error));
		ThrowError("ERROR: %s", error);
	}
}

public Txn_Test1_OnSuccess(Handle:db, any:data, numQueries, Handle:results[], any:queryData[])
{
	SetTestContext("Transaction Test 1");
	AssertEq("data", data, 1000);
	AssertEq("numQueries", numQueries, 3);
	AssertEq("queryData[0]", queryData[0], 50);
	AssertEq("queryData[1]", queryData[1], 60);
	AssertEq("queryData[2]", queryData[2], 70);
	AssertFalse("HasResultSet(0)", SQL_HasResultSet(results[0]));
	AssertFalse("HasResultSet(1)", SQL_HasResultSet(results[1]));
	AssertTrue("HasResultSet(2)", SQL_HasResultSet(results[2]));
	AssertTrue("FetchRow(2)", SQL_FetchRow(results[2]));
	AssertEq("FetchInt(2, 0)", SQL_FetchInt(results[2], 0), 5);
	AssertFalse("FetchRow(2)", SQL_FetchRow(results[2]));
}

public Txn_Test1_OnFailure(Handle:db, any:data, numQueries, const String:error[], failIndex, any:queryData[])
{
	ThrowError("Transaction test 1 failed: %s (failIndex=%d)", error, failIndex);
}

public Txn_Test2_OnSuccess(Handle:db, any:data, numQueries, Handle:results[], any:queryData[])
{
	ThrowError("Transaction test 2 failed: should have failed");
}

public Txn_Test2_OnFailure(Handle:db, any:data, numQueries, const String:error[], failIndex, any:queryData[])
{
	SetTestContext("Transaction Test 2");
	AssertEq("data", data, 1000);
	AssertEq("numQueries", numQueries, 3);
	AssertEq("queryData[0]", queryData[0], 50);
	AssertEq("queryData[1]", queryData[1], 60);
	AssertEq("queryData[2]", queryData[2], 70);
	AssertEq("failIndex", failIndex, 1);
}

public Txn_Test3_OnSuccess(Handle:db, any:data, numQueries, Handle:results[], any:queryData[])
{
	SetTestContext("Transaction Test 3");
	AssertEq("data", data, 0);
	AssertEq("numQueries", numQueries, 1);
	AssertEq("queryData[0]", queryData[0], 0);
	AssertTrue("HasResultSet(0)", SQL_HasResultSet(results[0]));
	AssertTrue("FetchRow(0)", SQL_FetchRow(results[0]));
	AssertEq("FetchInt(0, 0)", SQL_FetchInt(results[0], 0), 5);
}

public Action:Command_TestTxn(args)
{
	new String:error[256];
	new Handle:db = SQL_Connect("storage-local", false, error, sizeof(error));
	if (db == null) {
		ThrowError("ERROR: %s", error);
		return Plugin_Handled;
	}

	FastQuery(db, "DROP TABLE IF EXISTS egg");
	FastQuery(db, "CREATE TABLE egg(id int primary key)");
	FastQuery(db, "INSERT INTO egg (id) VALUES (1)");
	FastQuery(db, "INSERT INTO egg (id) VALUES (2)");
	FastQuery(db, "INSERT INTO egg (id) VALUES (3)");

	SetTestContext("CreateTransaction");

	new Transaction:txn = SQL_CreateTransaction();
	AssertEq("AddQuery", txn.AddQuery("INSERT INTO egg (id) VALUES (4)", 50), 0);
	AssertEq("AddQuery", txn.AddQuery("INSERT INTO egg (id) VALUES (5)", 60), 1);
	AssertEq("AddQuery", txn.AddQuery("SELECT COUNT(id) FROM egg", 70), 2);
	SQL_ExecuteTransaction(
		db,
		txn,
		Txn_Test1_OnSuccess,
		Txn_Test1_OnFailure,
		1000
	);

	txn = SQL_CreateTransaction();
	AssertEq("AddQuery", txn.AddQuery("INSERT INTO egg (id) VALUES (6)", 50), 0);
	AssertEq("AddQuery", txn.AddQuery("INSERT INTO egg (id) VALUES (6)", 60), 1);
	AssertEq("AddQuery", txn.AddQuery("SELECT COUNT(id) FROM egg", 70), 2);
	SQL_ExecuteTransaction(
		db,
		txn,
		Txn_Test2_OnSuccess,
		Txn_Test2_OnFailure,
		1000
	);

	// Make sure the transaction was rolled back - COUNT should be 5.
	txn = SQL_CreateTransaction();
	AssertEq("CloneHandle", _:CloneHandle(txn), _:null);
	txn.AddQuery("SELECT COUNT(id) FROM egg");
	SQL_ExecuteTransaction(
		db,
		txn,
		Txn_Test3_OnSuccess
	);

	db.Close();
	return Plugin_Handled;
}
