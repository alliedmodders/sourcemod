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
	
	new Handle:query = SQL_Query(db, "SELECT * FROM gaben")
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
	
	new Handle:stmt = SQL_PrepareQuery(db, "SELECT * FROM gaben WHERE gaben > ?", error, sizeof(error))
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


