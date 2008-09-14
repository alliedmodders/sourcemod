#include <sourcemod>

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
	if (db == INVALID_HANDLE)
	{
		PrintToServer("Failed to connect: %s", error)
		return Plugin_Handled
	}
	
	new Handle:query = SQL_Query(db, "SELECT * FROM gab")
	if (query == INVALID_HANDLE)
	{
		SQL_GetError(db, error, sizeof(error))
		PrintToServer("Failed to query: %s", error)
	} else {
		PrintQueryData(query)
		CloseHandle(query)
	}
	
	CloseHandle(db)
	
	return Plugin_Handled;
}

public Action:Command_TestSql2(args)
{
	new String:error[255]
	new Handle:db = SQL_DefConnect(error, sizeof(error))
	if (db == INVALID_HANDLE)
	{
		PrintToServer("Failed to connect: %s", error)
		return Plugin_Handled
	}
	
	new Handle:stmt = SQL_PrepareQuery(db, "SELECT * FROM gab WHERE gaben > ?", error, sizeof(error))
	if (stmt == INVALID_HANDLE)
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
		CloseHandle(stmt)
	}
	
	CloseHandle(db)
	
	return Plugin_Handled;
}

new Handle:g_ThreadedHandle = INVALID_HANDLE;

public CallbackTest3(Handle:owner, Handle:hndl, const String:error[], any:data)
{
	PrintToServer("CallbackTest1() (owner %x) (hndl %x) (error \"%s\") (data %d)", owner, hndl, error, data);
	if (g_ThreadedHandle != INVALID_HANDLE && hndl != INVALID_HANDLE)
	{
		CloseHandle(hndl);
	} else {
		g_ThreadedHandle = hndl;
	}
}

public Action:Command_TestSql3(args)
{
	if (g_ThreadedHandle != INVALID_HANDLE)
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
	if (query == INVALID_HANDLE)
	{
		new String:error[255];
		SQL_GetError(g_ThreadedHandle, error, sizeof(error))
		PrintToServer("Failed to query: %s", error)
	} else {
		PrintQueryData(query)
		CloseHandle(query)
	}
	SQL_UnlockDatabase(g_ThreadedHandle);
	
	return Plugin_Handled;
}

public CallbackTest5(Handle:owner, Handle:hndl, const String:error[], any:data)
{
	if (hndl == INVALID_HANDLE)
	{
		PrintToServer("Failed to query: %s", error)
	} else {
		PrintQueryData(hndl)
		SQL_TQuery(g_ThreadedHandle, CallbackTest6, "UPDATE gab SET `gaben` = `gaben` + 1 WHERE `gaben` >= 4", 52)
	}
}

public CallbackTest6(Handle:owner, Handle:hndl, const String:error[], any:data)
{
	if (hndl == INVALID_HANDLE)
	{
		PrintToServer("Failed to query: %s", error)
	} else {
		PrintToServer("Queried!");
		SQL_TQuery(g_ThreadedHandle, CallbackTest7, "UPDATE gab SET `gaben` = `gaben` + 1 WHERE `gaben` >= 4", 52)
	}
}

public CallbackTest7(Handle:owner, Handle:hndl, const String:error[], any:data)
{
	if (hndl == INVALID_HANDLE)
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

