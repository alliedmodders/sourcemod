/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */
 
#if defined _dbi_included
 #endinput
#endif
#define _dbi_included

/**
 * @handle Driver
 *
 * Contains information about an SQL driver.
 */
 
/**
 * @handle Database
 *
 * Contains information about a database connection.
 */
 
/**
 * @handle Query
 *
 * Contains information about an active query and its 
 * result sets.
 */
 
/**
 * @handle Statement : Query
 *
 * Extends a Query Handle and can be used as a Query Handle.
 * Statement Handles are for prepared queries and contain
 * their own function for binding parameters.  Statement
 * Handles can be used instead of database Handles in a few
 * select functions.
 */

/**
 * Describes a database field fetch status.
 */
enum DBResult
{
	DBVal_Error = 0,		/**< Column number/field is invalid. */
	DBVal_TypeMismatch = 1,	/**< You cannot retrieve this data with this type. */
	DBVal_Null = 2,			/**< Field has no data (NULL) */
	DBVal_Data = 3,			/**< Field has data */
};

/**
 * Describes binding types.
 */
enum DBBindType
{
	DBBind_Int = 0,			/**< Bind an integer. */
	DBBind_Float = 1,		/**< Bind a float. */
	DBBind_String = 2,		/**< Bind a string. */
};

/**
 * Threading priority level.
 */
enum DBPriority
{
	DBPrio_High = 0,		/**< High priority. */
	DBPrio_Normal = 1,		/**< Normal priority. */
	DBPrio_Low = 2,			/**< Low priority. */
};

/**
 * Creates an SQL connection from a named configuration.
 *
 * @param confname		Named configuration.
 * @param persistent	True to re-use a previous persistent connection if
 *						possible, false otherwise.
 * @param error			Error buffer.
 * @param maxlength		Maximum length of the error buffer.
 * @return				A database connection Handle, or INVALID_HANDLE on failure.
 */
native Handle:SQL_Connect(const String:confname[], bool:persistent, String:error[], maxlength);

/**
 * Creates a default SQL connection.
 *
 * @param error			Error buffer.
 * @param maxlength		Maximum length of the error buffer.
 * @param persistent	True to re-use a previous persistent connection
 *						if possible, false otherwise.
 * @return				A database connection Handle, or INVALID_HANDLE on failure.
 * 						On failure the error buffer will be filled with a message.
 */
stock Handle:SQL_DefConnect(String:error[], maxlength, bool:persistent=true)
{
	return SQL_Connect("default", persistent, error, maxlength);
}

/**
 * Connects to a database using key value pairs containing the database info.
 * The key/value pairs should match what would be in databases.cfg.
 *
 * I.e. "driver" should be "default" or a driver name (or ommitted for 
 * the default).  For SQLite, only the "database" parameter is needed in addition.
 * For drivers which require external connections, more of the parameters may be 
 * needed.
 *
 * In general it is discouraged to use this function.  Connections should go through 
 * databases.cfg for greatest flexibility on behalf of users.
 *
 * @param keyvalues		Key/value pairs from a KeyValues handle, describing the connection.
 * @param error			Error buffer.
 * @param maxlength		Maximum length of the error buffer.
 * @return				A database connection Handle, or INVALID_HANDLE on failure.
 * 						On failure the error buffer will be filled with a message.
 * @error				Invalid KeyValues handle.
 */
native Handle:SQL_ConnectCustom(Handle:keyvalues,
								String:error[],
								maxlength,
								bool:persistent);

/**
 * Grabs a handle to an SQLite database, creating one if it does not exist.  
 *
 * Unless there are extenuating circumstances, you should consider using "sourcemod-local" as the 
 * database name.  This provides some unification between plugins on behalf of users.
 *
 * As a precaution, you should always create some sort of unique prefix to your table names so 
 * there are no conflicts, and you should never drop or modify tables that you do not own.
 *
 * @param database		Database name.  
 * @param error			Error buffer.
 * @param maxlength		Maximum length of the error buffer.
 * @return				A database connection Handle, or INVALID_HANDLE on failure.
 * 						On failure the error buffer will be filled with a message.
 */
stock Handle:SQLite_UseDatabase(const String:database[],
								String:error[], 
								maxlength)
{
	new Handle:kv, Handle:db;

	kv = CreateKeyValues("");
	KvSetString(kv, "driver", "sqlite");
	KvSetString(kv, "database", database);

	db = SQL_ConnectCustom(kv, error, maxlength, false);

	CloseHandle(kv);

	return db;
}

/**
 * This function is deprecated.  Use SQL_ConnectCustom or SQLite_UseDatabase instead.
 */
#pragma deprecated Use SQL_ConnectCustom instead.
native Handle:SQL_ConnectEx(Handle:driver, 
							const String:host[],
							const String:user[], 
							const String:pass[],
							const String:database[],
							String:error[],
							maxlength,
							bool:persistent=true,
							port=0,
							maxTimeout=0);
							
/**
 * Returns if a named configuration is present in databases.cfg.
 *
 * @param name			Configuration name.
 * @return				True if it exists, false otherwise.
 */
native bool:SQL_CheckConfig(const String:name[]);

/**
 * Returns a driver Handle from a name string.
 *
 * If the driver is not found, SourceMod will attempt
 * to load an extension named dbi.<name>.ext.[dll|so].
 *
 * @param name			Driver identification string, or an empty
 *						string to return the default driver.
 * @return				Driver Handle, or INVALID_HANDLE on failure.
 */
native Handle:SQL_GetDriver(const String:name[]="");

/**
 * Reads the driver of an opened database.
 *
 * @param database		Database Handle.
 * @param ident			Option buffer to store the identification string.
 * @param ident_length	Maximum length of the buffer.
 * @return				Driver Handle.
 */
native Handle:SQL_ReadDriver(Handle:database, String:ident[]="", ident_length=0);

/**
 * Retrieves a driver's identification string.
 *
 * Example: "mysql", "sqlite"
 *
 * @param driver		Driver Handle, or INVALID_HANDLE for the default driver.
 * @param ident			Identification string buffer.
 * @param maxlength		Maximum length of the buffer.
 * @noreturn
 * @error				Invalid Handle other than INVALID_HANDLE.
 */
native SQL_GetDriverIdent(Handle:driver, String:ident[], maxlength);

/**
 * Retrieves a driver's product string.
 *
 * Example: "MySQL", "SQLite"
 *
 * @param driver		Driver Handle, or INVALID_HANDLE for the default driver.
 * @param product		Product string buffer.
 * @param maxlength		Maximum length of the buffer.
 * @noreturn
 * @error				Invalid Handle other than INVALID_HANDLE.
 */
native SQL_GetDriverProduct(Handle:driver, String:product[], maxlength);

/**
 * Returns the number of affected rows from the last query.
 *
 * @param hndl			A database OR statement Handle.
 * @return				Number of rows affected by the last query.
 * @error				Invalid database or statement Handle.
 */
native SQL_GetAffectedRows(Handle:hndl);

/**
 * Returns the last query's insertion id.
 *
 * @param hndl			A database OR statement Handle.
 * @return				Last query's insertion id.
 * @error				Invalid database or statement Handle.
 */
native SQL_GetInsertId(Handle:hndl);

/**
 * Returns the error reported by the last query.
 *
 * @param hndl			A database OR statement Handle.
 * @param error			Error buffer.
 * @param maxlength		Maximum length of the buffer.
 * @return				True if there was an error, false otherwise.
 * @error				Invalid database or statement Handle.
 */
native bool:SQL_GetError(Handle:hndl, String:error[], maxlength);

/**
 * Escapes a database string for literal insertion.  This is not needed
 * for binding strings in prepared statements.  
 *
 * Generally, database strings are inserted into queries enclosed in 
 * single quotes (').  If user input has a single quote in it, the 
 * quote needs to be escaped.  This function ensures that any unsafe 
 * characters are safely escaped according to the database engine and 
 * the database's character set.
 *
 * NOTE: SourceMod only guarantees properly escaped strings when the query
 * encloses the string in ''. While drivers tend to allow " instead, the string
 * may be not be escaped (for example, on SQLite)!
 *
 * @param hndl			A database Handle.
 * @param string		String to quote.
 * @param buffer		Buffer to store quoted string in.
 * @param maxlength		Maximum length of the buffer.
 * @param written		Optionally returns the number of bytes written.
 * @return				True on success, false if buffer is not big enough.
 *						The buffer must be at least 2*strlen(string)+1.
 * @error				Invalid database or statement Handle.
 */
native bool:SQL_EscapeString(Handle:database, 
							 const String:string[], 
							 String:buffer[], 
							 maxlength, 
							 &written=0);

/**
 * This is a backwards compatibility stock.  You should use SQL_EscapeString() 
 * instead, as this function will probably be deprecated in SourceMod 1.1.
 */
stock bool:SQL_QuoteString(Handle:database,
						   const String:string[],
						   String:buffer[],
						   maxlength,
						   &written=0)
{
	return SQL_EscapeString(database, string, buffer, maxlength, written);
}

/**
 * Executes a query and ignores the result set.
 *
 * @param database		A database Handle.
 * @param query			Query string.
 * @param len			Optional parameter to specify the query length, in 
 *						bytes.  This can be used to send binary queries that 
 * 						have a premature terminator.
 * @return				True if query succeeded, false otherwise.  Use
 *						SQL_GetError to find the last error.
 * @error				Invalid database Handle.
 */
native bool:SQL_FastQuery(Handle:database, const String:query[], len=-1);

/**
 * Executes a simple query and returns a new query Handle for
 * receiving the results.
 *
 * @param database		A database Handle.
 * @param query			Query string.
 * @param len			Optional parameter to specify the query length, in 
 *						bytes.  This can be used to send binary queries that 
 * 						have a premature terminator.
 * @return				A new Query Handle on success, INVALID_HANDLE
 *						otherwise.  The Handle must be freed with CloseHandle().
 * @error				Invalid database Handle.
 */
native Handle:SQL_Query(Handle:database, const String:query[], len=-1);

/**
 * Creates a new prepared statement query.  Prepared statements can
 * be executed any number of times.  They can also have placeholder
 * parameters, similar to variables, which can be bound safely and
 * securely (for example, you do not need to quote bound strings).
 *
 * Statement handles will work in any function that accepts a Query handle.
 *
 * @param database		A database Handle.
 * @param query			Query string.
 * @param error			Error buffer.
 * @param maxlength		Maximum size of the error buffer.
 * @return				A new statement Handle on success, INVALID_HANDLE
 *						otherwise.  The Handle must be freed with CloseHandle().
 * @error				Invalid database Handle.
 */
native Handle:SQL_PrepareQuery(Handle:database, const String:query[], String:error[], maxlength);

/**
 * Advances to the next set of results.
 *
 * In some SQL implementations, multiple result sets can exist on one query.  
 * This is possible in MySQL with simple queries when executing a CALL 
 * query.  If this is the case, all result sets must be processed before
 * another query is made.
 *
 * @param query			A query Handle.
 * @return				True if there was another result set, false otherwise.
 * @error				Invalid query Handle.
 */
native bool:SQL_FetchMoreResults(Handle:query);

/**
 * Returns whether or not a result set exists.  This will
 * return true even if 0 results were returned, but false
 * on queries like UPDATE, INSERT, or DELETE.
 *
 * @param query			A query (or statement) Handle.
 * @return				True if there is a result set, false otherwise.
 * @error				Invalid query Handle.
 */
native bool:SQL_HasResultSet(Handle:query);

/**
 * Retrieves the number of rows in the last result set.
 * 
 * @param query			A query (or statement) Handle.
 * @return				Number of rows in the current result set.
 * @error				Invalid query Handle.
 */
native SQL_GetRowCount(Handle:query);

/**
 * Retrieves the number of fields in the last result set.
 * 
 * @param query			A query (or statement) Handle.
 * @return				Number of fields in the current result set.
 * @error				Invalid query Handle.
 */
native SQL_GetFieldCount(Handle:query);

/**
 * Retrieves the name of a field by index.
 * 
 * @param query			A query (or statement) Handle.
 * @param field			Field number (starting from 0).
 * @param name			Name buffer.
 * @param maxlength		Maximum length of the name buffer.
 * @noreturn
 * @error				Invalid query Handle, invalid field index, or
 *						no current result set.
 */
native SQL_FieldNumToName(Handle:query, field, String:name[], maxlength);

/**
 * Retrieves a field index by name.
 * 
 * @param query			A query (or statement) Handle.
 * @param name			Name of the field (case sensitive).
 * @param field			Variable to store field index in.
 * @return				True if found, false if not found.
 * @error				Invalid query Handle or no current result set.
 */
native bool:SQL_FieldNameToNum(Handle:query, const String:name[], &field);

/**
 * Fetches a row from the current result set.  This must be 
 * successfully called before any results are fetched.
 *
 * If this function fails, SQL_MoreResults() can be used to
 * tell if there was an error or the result set is finished.
 * 
 * @param query			A query (or statement) Handle.
 * @return				True if a row was fetched, false otherwise.
 * @error				Invalid query Handle.
 */
native bool:SQL_FetchRow(Handle:query);

/**
 * Returns if there are more rows.
 * 
 * @param query			A query (or statement) Handle.
 * @return				True if there are more rows, false otherwise.
 * @error				Invalid query Handle.
 */
native bool:SQL_MoreRows(Handle:query);

/**
 * Rewinds a result set back to the first result.
 * 
 * @param query			A query (or statement) Handle.
 * @return				True on success, false otherwise.
 * @error				Invalid query Handle or no current result set.
 */
native bool:SQL_Rewind(Handle:query);

/**
 * Fetches a string from a field in the current row of a result set.  
 * If the result is NULL, an empty string will be returned.  A NULL 
 * check can be done with the result parameter, or SQL_IsFieldNull().
 * 
 * @param query			A query (or statement) Handle.
 * @param field			The field index (starting from 0).
 * @param buffer		String buffer.
 * @param maxlength		Maximum size of the string buffer.
 * @param result		Optional variable to store the status of the return value.
 * @return				Number of bytes written.
 * @error				Invalid query Handle or field index, invalid
 *						type conversion requested from the database,
 *						or no current result set.
 */
native SQL_FetchString(Handle:query, field, String:buffer[], maxlength, &DBResult:result=DBVal_Error);

/**
 * Fetches a float from a field in the current row of a result set.  
 * If the result is NULL, a value of 0.0 will be returned.  A NULL 
 * check can be done with the result parameter, or SQL_IsFieldNull().
 * 
 * @param query			A query (or statement) Handle.
 * @param field			The field index (starting from 0).
 * @param result		Optional variable to store the status of the return value.
 * @return				A float value.
 * @error				Invalid query Handle or field index, invalid
 *						type conversion requested from the database,
 *						or no current result set.
 */
native Float:SQL_FetchFloat(Handle:query, field, &DBResult:result=DBVal_Error);

/**
 * Fetches an integer from a field in the current row of a result set.  
 * If the result is NULL, a value of 0 will be returned.  A NULL 
 * check can be done with the result parameter, or SQL_IsFieldNull().
 * 
 * @param query			A query (or statement) Handle.
 * @param field			The field index (starting from 0).
 * @param result		Optional variable to store the status of the return value.
 * @return				An integer value.
 * @error				Invalid query Handle or field index, invalid
 *						type conversion requested from the database,
 *						or no current result set.
 */
native SQL_FetchInt(Handle:query, field, &DBResult:result=DBVal_Error);

/**
 * Returns whether a field's data in the current row of a result set is 
 * NULL or not.  NULL is an SQL type which means "no data."
 * 
 * @param query			A query (or statement) Handle.
 * @param field			The field index (starting from 0).
 * @return				True if data is NULL, false otherwise.
 * @error				Invalid query Handle or field index, or no
 *						current result set.
 */
native bool:SQL_IsFieldNull(Handle:query, field);

/**
 * Returns the length of a field's data in the current row of a result
 * set.  This only needs to be called for strings to determine how many
 * bytes to use.  Note that the return value does not include the null
 * terminator.
 * 
 * @param query			A query (or statement) Handle.
 * @param field			The field index (starting from 0).
 * @return				Number of bytes for the field's data size.
 * @error				Invalid query Handle or field index or no
 *						current result set.
 */
native SQL_FetchSize(Handle:query, field);

/**
 * Binds a parameter in a prepared statement to a given integer value.
 * 
 * @param statement		A statement (prepared query) Handle.
 * @param param			The parameter index (starting from 0).
 * @param number		The number to bind.
 * @param signed		True to bind the number as signed, false to 
 *						bind it as unsigned.
 * @noreturn
 * @error				Invalid statement Handle or parameter index, or
 *						SQL error.
 */
native SQL_BindParamInt(Handle:statement, param, number, bool:signed=true);

/**
 * Binds a parameter in a prepared statement to a given float value.
 * 
 * @param statement		A statement (prepared query) Handle.
 * @param param			The parameter index (starting from 0).
 * @param float			The float number to bind.
 * @noreturn
 * @error				Invalid statement Handle or parameter index, or
 *						SQL error.
 */
native SQL_BindParamFloat(Handle:statement, param, Float:value);

/**
 * Binds a parameter in a prepared statement to a given string value.
 * 
 * @param statement		A statement (prepared query) Handle.
 * @param param			The parameter index (starting from 0).
 * @param value			The string to bind.
 * @param copy			Whether or not SourceMod should copy the value
 *						locally if necessary.  If the string contents
 *						won't change before calling SQL_Execute(), this
 *						can be set to false for optimization.
 * @noreturn
 * @error				Invalid statement Handle or parameter index, or
 *						SQL error.
 */
native SQL_BindParamString(Handle:statement, param, const String:value[], bool:copy);

/**
 * Executes a prepared statement.  All parameters must be bound beforehand.
 *
 * @param statement		A statement (prepared query) Handle.
 * @return				True on success, false on failure.
 * @error				Invalid statement Handle.
 */
native bool:SQL_Execute(Handle:statement);

/**
 * Locks a database so threading operations will not interrupt.
 * 
 * If you are using a database Handle for both threading and non-threading,
 * this MUST be called before doing any set of non-threading DB operations.
 * Otherwise you risk corrupting the database driver's memory or network
 * connection.
 * 
 * Leaving a lock on a database and then executing a threaded query results
 * in a dead lock! Make sure to call SQL_UnlockDatabase()!
 *
 * If the lock cannot be acquired, the main thread will pause until the 
 * threaded operation has concluded.
 *
 * @param database		A database Handle.
 * @noreturn
 * @error				Invalid database Handle.
 */
native SQL_LockDatabase(Handle:database);

/**
 * Unlocks a database so threading operations may continue.
 *
 * @param database		A database Handle.
 * @noreturn
 * @error				Invalid database Handle.
 */
native SQL_UnlockDatabase(Handle:database);

/**
 * General callback for threaded SQL stuff.
 *
 * @param db			Parent object of the Handle (or INVALID_HANDLE if none).
 * @param hndl			Handle to the child object (or INVALID_HANDLE if none).
 * @param error			Error string if there was an error.  The error could be 
 *						empty even if an error condition exists, so it is important 
 *						to check the actual Handle value instead.
 * @param data			Data passed in via the original threaded invocation.
 * @param 
 */
functag public SQLTCallback(Handle:owner, Handle:hndl, const String:error[], any:data);

/**
 * Tells whether two database handles both point to the same database 
 * connection.
 *
 * @param hndl1			First database Handle.
 * @param hndl2			Second database Handle.
 * @return				True if the Handles point to the same 
 *						connection, false otherwise.
 * @error				Invalid Handle.
 */
native bool:SQL_IsSameConnection(Handle:hndl1, Handle:hndl2);

/**
 * Connects to a database via a thread.  This can be used instead of
 * SQL_Connect() if you wish for non-blocking functionality.
 *
 * It is not necessary to use this to use threaded queries.  However, if you 
 * don't (or you mix threaded/non-threaded queries), you should see 
 * SQL_LockDatabase().
 *
 * @param callback		Callback; new Handle will be in hndl, owner is the driver.
 *						If no driver was found, the owner is INVALID_HANDLE.
 * @param name			Database name.
 * @noreturn
 */
native SQL_TConnect(SQLTCallback:callback, const String:name[]="default", any:data=0);

/**
 * Executes a simple query via a thread.  The query Handle is passed through
 * the callback.
 *
 * The database Handle returned through the callback is always a new Handle,
 * and if necessary, SQL_IsSameConnection() should be used to test against
 * other conenctions.
 *
 * The query Handle returned through the callback is temporary and destroyed 
 * at the end of the callback.  If you need to hold onto it, use CloneHandle().
 *
 * @param database		A database Handle.
 * @param callback		Callback; database is in "owner" and the query Handle
 *						is passed in "hndl".
 * @param query			Query string.
 * @param data			Extra data value to pass to the callback.
 * @param prio			Priority queue to use.
 * @noreturn
 * @error				Invalid database Handle.
 */
native SQL_TQuery(Handle:database, SQLTCallback:callback, const String:query[], any:data=0, DBPriority:prio=DBPrio_Normal);
