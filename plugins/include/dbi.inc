/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod (C)2004-2014 AlliedModders LLC.  All rights reserved.
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
 * Describes a database field fetch status.
 */
enum DBResult
{
	DBVal_Error = 0,        /**< Column number/field is invalid. */
	DBVal_TypeMismatch = 1, /**< You cannot retrieve this data with this type. */
	DBVal_Null = 2,         /**< Field has no data (NULL) */
	DBVal_Data = 3          /**< Field has data */
};

/**
 * Describes binding types.
 */
enum DBBindType
{
	DBBind_Int = 0,         /**< Bind an integer. */
	DBBind_Float = 1,       /**< Bind a float. */
	DBBind_String = 2       /**< Bind a string. */
};

/**
 * Threading priority level.
 */
enum DBPriority
{
	DBPrio_High = 0,        /**< High priority. */
	DBPrio_Normal = 1,      /**< Normal priority. */
	DBPrio_Low = 2          /**< Low priority. */
};

// A Driver represents a database backend, currently MySQL or SQLite.
//
// Driver handles cannot be closed.
methodmap DBDriver < Handle
{
	// Finds the driver associated with a name.
	//
	// Supported driver strings:
	//    mysql
	//    sqlite
	//
	// @param name          Driver identification string, or an empty string
	//                      to return the default driver.
	// @return              Driver handle, or null on failure.
	public static native DBDriver Find(const char[] name = "");

	// Retrieves a driver's identification string.
	//
	// Example: "mysql", "sqlite"
	//
	// @param ident         Identification string buffer.
	// @param maxlength     Maximum length of the buffer.
	public native void GetIdentifier(char[] ident, int maxlength);

	// Retrieves a driver's product string.
	//
	// Example: "MySQL", "SQLite"
	//
	// @param product       Product string buffer.
	// @param maxlength     Maximum length of the buffer.
	public native void GetProduct(char[] product, int maxlength);
};

// Represents a set of results returned from executing a query.
methodmap DBResultSet < Handle
{
	// Advances to the next set of results.
	//
	// In some SQL implementations, multiple result sets can exist on one query.  
	// This is possible in MySQL with simple queries when executing a CALL 
	// query.  If this is the case, all result sets must be processed before
	// another query is made.
	//
	// @return             True if there was another result set, false otherwise.
	public native bool FetchMoreResults();

	// Returns whether or not a result set exists.  This will
	// return true even if 0 results were returned, but false
	// on queries like UPDATE, INSERT, or DELETE.
	property bool HasResults {
		public native get();
	}

	// Retrieves the number of rows in the last result set.
	// 
	// @param query        A query (or statement) Handle.
	// @return             Number of rows in the current result set.
	property int RowCount {
		public native get();
	}

	// Retrieves the number of fields in the last result set.
	property int FieldCount {
		public native get();
	}

	// Returns the number of affected rows from the query that generated this
	// result set.
	property int AffectedRows {
		public native get();
	}

	// Returns the insert id from the query that generated this result set.
	property int InsertId {
		public native get();
	}

	// Retrieves the name of a field by index.
	// 
	// @param field        Field number (starting from 0).
	// @param name         Name buffer.
	// @param maxlength    Maximum length of the name buffer.
	// @error              Invalid field index, or no current result set.
	public native void FieldNumToName(int field, char[] name, int maxlength);

	// Retrieves a field index by name.
	// 
	// @param name         Name of the field (case sensitive).
	// @param field        Variable to store field index in.
	// @return             True if found, false if not found.
	// @error              No current result set.
	public native bool FieldNameToNum(const char[] name, int &field);

	// Fetches a row from the current result set.  This must be 
	// successfully called before any results are fetched.
	//
	// If this function fails, _MoreResults can be used to
	// tell if there was an error or the result set is finished.
	// 
	// @return             True if a row was fetched, false otherwise.
	public native bool FetchRow();

	// Returns if there are more rows.
	// 
	// @return             True if there are more rows, false otherwise.
	property bool MoreRows {
		public native get();
	}

	// Rewinds a result set back to the first result.
	// 
	// @return             True on success, false otherwise.
	// @error              No current result set.
	public native bool Rewind();

	// Fetches a string from a field in the current row of a result set.  
	// If the result is NULL, an empty string will be returned.  A NULL 
	// check can be done with the result parameter, or SQL_IsFieldNull().
	// 
	// @param field        The field index (starting from 0).
	// @param buffer       String buffer.
	// @param maxlength    Maximum size of the string buffer.
	// @param result       Optional variable to store the status of the return value.
	// @return             Number of bytes written.
	// @error              Invalid field index, invalid type conversion requested
	//                     from the database, or no current result set.
	public native int FetchString(int field, char[] buffer, int maxlength, DBResult &result=DBVal_Error);

	// Fetches a float from a field in the current row of a result set.  
	// If the result is NULL, a value of 0.0 will be returned.  A NULL 
	// check can be done with the result parameter, or SQL_IsFieldNull().
	// 
	// @param field        The field index (starting from 0).
	// @param result       Optional variable to store the status of the return value.
	// @return             A float value.
	// @error              Invalid field index, invalid type conversion requested
	//                     from the database, or no current result set.
	public native float FetchFloat(int field, DBResult &result=DBVal_Error);

	// Fetches an integer from a field in the current row of a result set.  
	// If the result is NULL, a value of 0 will be returned.  A NULL 
	// check can be done with the result parameter, or SQL_IsFieldNull().
	// 
	// @param field        The field index (starting from 0).
	// @param result       Optional variable to store the status of the return value.
	// @return             An integer value.
	// @error              Invalid field index, invalid type conversion requested
	//                     from the database, or no current result set.
	public native int FetchInt(int field, DBResult &result=DBVal_Error);

	// Returns whether a field's data in the current row of a result set is 
	// NULL or not.  NULL is an SQL type which means "no data."
	// 
	// @param field        The field index (starting from 0).
	// @return             True if data is NULL, false otherwise.
	// @error              Invalid field index, or no current result set.
	public native bool IsFieldNull(int field);

	// Returns the length of a field's data in the current row of a result
	// set.  This only needs to be called for strings to determine how many
	// bytes to use.  Note that the return value does not include the null
	// terminator.
	// 
	// @param field        The field index (starting from 0).
	// @return             Number of bytes for the field's data size.
	// @error              Invalid field index or no current result set.
	public native int FetchSize(int field);
};

typeset SQLTxnSuccess
{
	// Callback for a successful transaction.
	// 
	// @param db            Database handle.
	// @param data          Data value passed to SQL_ExecuteTransaction().
	// @param numQueries    Number of queries executed in the transaction.
	// @param results       An array of Query handle results, one for each of numQueries. They are closed automatically.
	// @param queryData     An array of each data value passed to SQL_AddQuery().
	function void (Database db, any data, int numQueries, Handle[] results, any[] queryData);
	
	// Callback for a successful transaction.
	// 
	// @param db            Database handle.
	// @param data          Data value passed to SQL_ExecuteTransaction().
	// @param numQueries    Number of queries executed in the transaction.
	// @param results       An array of DBResultSet results, one for each of numQueries. They are closed automatically.
	// @param queryData     An array of each data value passed to SQL_AddQuery().
	function void (Database db, any data, int numQueries, DBResultSet[] results, any[] queryData);	
}

/**
 * Callback for a failed transaction.
 *
 * @param db            Database handle.
 * @param data          Data value passed to SQL_ExecuteTransaction().
 * @param numQueries    Number of queries executed in the transaction.
 * @param error         Error message.
 * @param failIndex     Index of the query that failed, or -1 if something else.
 * @param queryData     An array of each data value passed to SQL_AddQuery().
 */
typedef SQLTxnFailure = function void (Database db, any data, int numQueries, const char[] error, int failIndex, any[] queryData);

// A Transaction is a collection of SQL statements that must all execute
// successfully or not at all.
methodmap Transaction < Handle
{
	// Create a new transaction.
	public native Transaction();

	// Adds a query to the transaction.
	//
	// @param query        Query string.
	// @param data         Extra data value to pass to the final callback.
	// @return             The index of the query in the transaction's query list.
	public native int AddQuery(const char[] query, any data=0);
};

// A DBStatement is a pre-compiled SQL query that may be executed multiple
// times with different parameters. A DBStatement holds a reference to the
// Database that prepared it.
methodmap DBStatement < Handle
{
	// Binds a parameter in a prepared statement to a given integer value.
	// 
	// @param param         The parameter index (starting from 0).
	// @param number        The number to bind.
	// @param signed        True to bind the number as signed, false to 
	//                      bind it as unsigned.
	// @error               Invalid parameter index, or SQL error.
	public native void BindInt(int param, int number, bool signed=true);

	// Binds a parameter in a prepared statement to a given float value.
	// 
	// @param param         The parameter index (starting from 0).
	// @param value         The float number to bind.
	// @error               Invalid parameter index, or SQL error.
	public native void BindFloat(int param, float value);

	// Binds a parameter in a prepared statement to a given string value.
	// 
	// @param param         The parameter index (starting from 0).
	// @param value         The string to bind.
	// @param copy          Whether or not SourceMod should copy the value
	//                      locally if necessary.  If the string contents
	//                      won't change before calling SQL_Execute(), this
	//                      can be set to false for optimization.
	// @error               Invalid parameter index, or SQL error.
	public native void BindString(int param, const char[] value, bool copy);
};

/**
 * Callback for receiving asynchronous database connections.
 *
 * @param db            Handle to the database connection.
 * @param error         Error string if there was an error.  The error could be 
 *                      empty even if an error condition exists, so it is important 
 *                      to check the actual Handle value instead.
 * @param data          Data passed in via the original threaded invocation.
 */
typedef SQLConnectCallback = function void (Database db, const char[] error, any data);

/**
 * Callback for receiving asynchronous database query results.
 *
 * @param db            Cloned handle to the database connection.
 * @param results       Result object, or null on failure.
 * @param error         Error string if there was an error.  The error could be 
 *                      empty even if an error condition exists, so it is important 
 *                      to check the actual results value instead.
 * @param data          Data passed in via the original threaded invocation.
 */
typedef SQLQueryCallback = function void (Database db, DBResultSet results, const char[] error, any data);

// A Database represents a live connection to a database, either over the
// wire, through a unix domain socket, or over an open file.
methodmap Database < Handle
{
	// Connects to a database asynchronously, so the game thread is not blocked.
	//
	// @param callback      Callback. If no driver was found, the owner is null.
	// @param name          Database configuration name.
	// @param data          Extra data value to pass to the callback.
	public static native void Connect(SQLConnectCallback callback, const char[] name="default", any data=0);

	// Returns the driver for this database connection.
	property DBDriver Driver {
		public native get();
	}

	// Sets the character set of the connection. 
	// Like SET NAMES .. in mysql, but stays after connection problems.
	// 
	// Example: "utf8", "latin1"
	//
	// @param characterset  The character set string to change to.
	// @return              True, if character set was changed, false otherwise.
	public native bool SetCharset(const char[] charset);

	// Escapes a database string for literal insertion.  This is not needed
	// for binding strings in prepared statements.  
	//
	// Generally, database strings are inserted into queries enclosed in 
	// single quotes (').  If user input has a single quote in it, the 
	// quote needs to be escaped.  This function ensures that any unsafe 
	// characters are safely escaped according to the database engine and 
	// the database's character set.
	//
	// NOTE: SourceMod only guarantees properly escaped strings when the query
	// encloses the string in ''. While drivers tend to allow " instead, the string
	// may be not be escaped (for example, on SQLite)!
	//
	// @param string        String to quote.
	// @param buffer        Buffer to store quoted string in.
	// @param maxlength     Maximum length of the buffer.
	// @param written       Optionally returns the number of bytes written.
	// @return              True on success, false if buffer is not big enough.
	//                      The buffer must be at least 2*strlen(string)+1.
	public native bool Escape(const char[] string, char[] buffer, int maxlength, int &written=0);

	// Formats a string according to the SourceMod format rules (see documentation).
	// All format specifiers are escaped (see SQL_EscapeString) unless the '!' flag is used.
	//
	// @param buffer        Destination string buffer.
	// @param maxlength     Maximum length of output string buffer.
	// @param format        Formatting rules.
	// @param ...           Variable number of format parameters.
	// @return              Number of cells written.
	public native int Format(const char[] buffer, int maxlength, const char[] format, any ...);

	// Returns whether a database is the same connection as another database.
	public native bool IsSameConnection(Database other);

	// Executes a query via a thread. The result handle is passed through the
	// callback.
	//
	// The database handle returned through the callback is always a new Handle,
	// and if necessary, IsSameConnection() should be used to test against other
	// connections.
	//
	// The result handle returned through the callback is temporary and destroyed 
	// at the end of the callback.
	//
	// @param callback       Callback.
	// @param query          Query string.
	// @param data           Extra data value to pass to the callback.
	// @param prio           Priority queue to use.
	public native void Query(SQLQueryCallback callback, const char[] query,
	                         any data = 0,
	                         DBPriority prio = DBPrio_Normal);

	// Sends a transaction to the database thread. The transaction handle is
	// automatically closed. When the transaction completes, the optional
	// callback is invoked.
	//
	// @param txn            A transaction handle.
	// @param onSuccess      An optional callback to receive a successful transaction.
	// @param onError        An optional callback to receive an error message.
	// @param data           An optional value to pass to callbacks.
	// @param prio           Priority queue to use.
	public native void Execute(Transaction txn,
	                           SQLTxnSuccess onSuccess = INVALID_FUNCTION,
	                           SQLTxnFailure onError = INVALID_FUNCTION,
	                           any data = 0,
	                           DBPriority priority = DBPrio_Normal);
};

/**
 * Creates an SQL connection from a named configuration.
 *
 * @param confname      Named configuration.
 * @param persistent    True to re-use a previous persistent connection if
 *                      possible, false otherwise.
 * @param error         Error buffer.
 * @param maxlength     Maximum length of the error buffer.
 * @return              A database connection Handle, or INVALID_HANDLE on failure.
 */
native Database SQL_Connect(const char[] confname, bool persistent, char[] error, int maxlength);

/**
 * Creates a default SQL connection.
 *
 * @param error         Error buffer.
 * @param maxlength     Maximum length of the error buffer.
 * @param persistent    True to re-use a previous persistent connection
 *                      if possible, false otherwise.
 * @return              A database connection Handle, or INVALID_HANDLE on failure.
 *                      On failure the error buffer will be filled with a message.
 */
stock Database SQL_DefConnect(char[] error, int maxlength, bool persistent=true)
{
	return SQL_Connect("default", persistent, error, maxlength);
}

/**
 * Connects to a database using key value pairs containing the database info.
 * The key/value pairs should match what would be in databases.cfg.
 *
 * I.e. "driver" should be "default" or a driver name (or omitted for 
 * the default).  For SQLite, only the "database" parameter is needed in addition.
 * For drivers which require external connections, more of the parameters may be 
 * needed.
 *
 * In general it is discouraged to use this function.  Connections should go through 
 * databases.cfg for greatest flexibility on behalf of users.
 *
 * @param keyvalues     Key/value pairs from a KeyValues handle, describing the connection.
 * @param error         Error buffer.
 * @param maxlength     Maximum length of the error buffer.
 * @param persistent    True to re-use a previous persistent connection if
 *                      possible, false otherwise.
 * @return              A database connection Handle, or INVALID_HANDLE on failure.
 *                      On failure the error buffer will be filled with a message.
 * @error               Invalid KeyValues handle.
 */
native Database SQL_ConnectCustom(Handle keyvalues,
								  char[] error,
								  int maxlength,
								  bool persistent);

/**
 * Grabs a handle to an SQLite database, creating one if it does not exist.  
 *
 * Unless there are extenuating circumstances, you should consider using "sourcemod-local" as the 
 * database name.  This provides some unification between plugins on behalf of users.
 *
 * As a precaution, you should always create some sort of unique prefix to your table names so 
 * there are no conflicts, and you should never drop or modify tables that you do not own.
 *
 * @param database      Database name.  
 * @param error         Error buffer.
 * @param maxlength     Maximum length of the error buffer.
 * @return              A database connection Handle, or INVALID_HANDLE on failure.
 *                      On failure the error buffer will be filled with a message.
 */
stock Database SQLite_UseDatabase(const char[] database, char[] error, int maxlength)
{
	KeyValues kv = new KeyValues("");
	kv.SetString("driver", "sqlite");
	kv.SetString("database", database);

	Database db = SQL_ConnectCustom(kv, error, maxlength, false);

	delete kv;

	return db;
}

/**
 * This function is deprecated.  Use SQL_ConnectCustom or SQLite_UseDatabase instead.
 * @deprecated
 */
#pragma deprecated Use SQL_ConnectCustom instead.
native Handle SQL_ConnectEx(Handle driver, 
							const char[] host,
							const char[] user, 
							const char[] pass,
							const char[] database,
							char[] error,
							int maxlength,
							bool persistent=true,
							int port=0,
							int maxTimeout=0);
							
/**
 * Returns if a named configuration is present in databases.cfg.
 *
 * @param name          Configuration name.
 * @return              True if it exists, false otherwise.
 */
native bool SQL_CheckConfig(const char[] name);

/**
 * Returns a driver Handle from a name string.
 *
 * If the driver is not found, SourceMod will attempt
 * to load an extension named dbi.<name>.ext.[dll|so].
 *
 * @param name          Driver identification string, or an empty
 *                      string to return the default driver.
 * @return              Driver Handle, or INVALID_HANDLE on failure.
 */
native Handle SQL_GetDriver(const char[] name="");

/**
 * Reads the driver of an opened database.
 *
 * @param database      Database Handle.
 * @param ident         Option buffer to store the identification string.
 * @param ident_length  Maximum length of the buffer.
 * @return              Driver Handle.
 */
native Handle SQL_ReadDriver(Handle database, char[] ident="", int ident_length=0);

/**
 * Retrieves a driver's identification string.
 *
 * Example: "mysql", "sqlite"
 *
 * @param driver        Driver Handle, or INVALID_HANDLE for the default driver.
 * @param ident         Identification string buffer.
 * @param maxlength     Maximum length of the buffer.
 * @error               Invalid Handle other than INVALID_HANDLE.
 */
native void SQL_GetDriverIdent(Handle driver, char[] ident, int maxlength);

/**
 * Retrieves a driver's product string.
 *
 * Example: "MySQL", "SQLite"
 *
 * @param driver        Driver Handle, or INVALID_HANDLE for the default driver.
 * @param product       Product string buffer.
 * @param maxlength     Maximum length of the buffer.
 * @error               Invalid Handle other than INVALID_HANDLE.
 */
native void SQL_GetDriverProduct(Handle driver, char[] product, int maxlength);

/**
 * Sets the character set of the current connection. 
 * Like SET NAMES .. in mysql, but stays after connection problems.
 * 
 * Example: "utf8", "latin1"
 *
 * @param database      Database Handle.
 * @param characterset  The character set string to change to.
 * @return              True, if character set was changed, false otherwise.
 */
native bool SQL_SetCharset(Handle database, const char[] charset);

/**
 * Returns the number of affected rows from the last query.
 *
 * @param hndl          A database OR statement Handle.
 * @return              Number of rows affected by the last query.
 * @error               Invalid database or statement Handle.
 */
native int SQL_GetAffectedRows(Handle hndl);

/**
 * Returns the last query's insertion id.
 *
 * @param hndl          A database, query, OR statement Handle.
 * @return              Last query's insertion id.
 * @error               Invalid database, query, or statement Handle.
 */
native int SQL_GetInsertId(Handle hndl);

/**
 * Returns the error reported by the last query.
 *
 * @param hndl          A database, query, OR statement Handle.
 * @param error         Error buffer.
 * @param maxlength     Maximum length of the buffer.
 * @return              True if there was an error, false otherwise.
 * @error               Invalid database, query, or statement Handle.
 */
native bool SQL_GetError(Handle hndl, char[] error, int maxlength);

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
 * @param database      A database Handle.
 * @param string        String to quote.
 * @param buffer        Buffer to store quoted string in.
 * @param maxlength     Maximum length of the buffer.
 * @param written       Optionally returns the number of bytes written.
 * @return              True on success, false if buffer is not big enough.
 *                      The buffer must be at least 2*strlen(string)+1.
 * @error               Invalid database or statement Handle.
 */
native bool SQL_EscapeString(Handle database, 
							 const char[] string, 
							 char[] buffer, 
							 int maxlength, 
							 int &written=0);

/**
 * Formats a string according to the SourceMod format rules (see documentation).
 * All format specifiers are escaped (see SQL_EscapeString) unless the '!' flag is used.
 *
 * @param database      A database Handle.
 * @param buffer        Destination string buffer.
 * @param maxlength     Maximum length of output string buffer.
 * @param format        Formatting rules.
 * @param ...           Variable number of format parameters.
 * @return              Number of cells written.
 */
native int SQL_FormatQuery(Handle database, const char[] buffer, int maxlength, const char[] format, any ...);

/**
 * This function is deprecated.  Use SQL_EscapeString instead.
 * @deprecated
 */
#pragma deprecated Use SQL_EscapeString instead.
stock bool SQL_QuoteString(Handle database,
						   const char[] string,
						   char[] buffer,
						   int maxlength,
						   int &written=0)
{
	return SQL_EscapeString(database, string, buffer, maxlength, written);
}

/**
 * Executes a query and ignores the result set.
 *
 * @param database      A database Handle.
 * @param query         Query string.
 * @param len           Optional parameter to specify the query length, in 
 *                      bytes.  This can be used to send binary queries that 
 *                      have a premature terminator.
 * @return              True if query succeeded, false otherwise.  Use
 *                      SQL_GetError to find the last error.
 * @error               Invalid database Handle.
 */
native bool SQL_FastQuery(Handle database, const char[] query, int len=-1);

/**
 * Executes a simple query and returns a new query Handle for
 * receiving the results.
 *
 * @param database      A database Handle.
 * @param query         Query string.
 * @param len           Optional parameter to specify the query length, in 
 *                      bytes.  This can be used to send binary queries that 
 *                      have a premature terminator.
 * @return              A new Query Handle on success, INVALID_HANDLE
 *                      otherwise.  The Handle must be freed with CloseHandle().
 * @error               Invalid database Handle.
 */
native DBResultSet SQL_Query(Handle database, const char[] query, int len=-1);

/**
 * Creates a new prepared statement query.  Prepared statements can
 * be executed any number of times.  They can also have placeholder
 * parameters, similar to variables, which can be bound safely and
 * securely (for example, you do not need to quote bound strings).
 *
 * Statement handles will work in any function that accepts a Query handle.
 *
 * @param database      A database Handle.
 * @param query         Query string.
 * @param error         Error buffer.
 * @param maxlength     Maximum size of the error buffer.
 * @return              A new statement Handle on success, INVALID_HANDLE
 *                      otherwise.  The Handle must be freed with CloseHandle().
 * @error               Invalid database Handle.
 */
native DBStatement SQL_PrepareQuery(Handle database, const char[] query, char[] error, int maxlength);

/**
 * Advances to the next set of results.
 *
 * In some SQL implementations, multiple result sets can exist on one query.  
 * This is possible in MySQL with simple queries when executing a CALL 
 * query.  If this is the case, all result sets must be processed before
 * another query is made.
 *
 * @param query         A query Handle.
 * @return              True if there was another result set, false otherwise.
 * @error               Invalid query Handle.
 */
native bool SQL_FetchMoreResults(Handle query);

/**
 * Returns whether or not a result set exists.  This will
 * return true even if 0 results were returned, but false
 * on queries like UPDATE, INSERT, or DELETE.
 *
 * @param query         A query (or statement) Handle.
 * @return              True if there is a result set, false otherwise.
 * @error               Invalid query Handle.
 */
native bool SQL_HasResultSet(Handle query);

/**
 * Retrieves the number of rows in the last result set.
 * 
 * @param query         A query (or statement) Handle.
 * @return              Number of rows in the current result set.
 * @error               Invalid query Handle.
 */
native int SQL_GetRowCount(Handle query);

/**
 * Retrieves the number of fields in the last result set.
 * 
 * @param query         A query (or statement) Handle.
 * @return              Number of fields in the current result set.
 * @error               Invalid query Handle.
 */
native int SQL_GetFieldCount(Handle query);

/**
 * Retrieves the name of a field by index.
 * 
 * @param query         A query (or statement) Handle.
 * @param field         Field number (starting from 0).
 * @param name          Name buffer.
 * @param maxlength     Maximum length of the name buffer.
 * @error               Invalid query Handle, invalid field index, or
 *                      no current result set.
 */
native void SQL_FieldNumToName(Handle query, int field, char[] name, int maxlength);

/**
 * Retrieves a field index by name.
 * 
 * @param query         A query (or statement) Handle.
 * @param name          Name of the field (case sensitive).
 * @param field         Variable to store field index in.
 * @return              True if found, false if not found.
 * @error               Invalid query Handle or no current result set.
 */
native bool SQL_FieldNameToNum(Handle query, const char[] name, int &field);

/**
 * Fetches a row from the current result set.  This must be 
 * successfully called before any results are fetched.
 *
 * If this function fails, SQL_MoreResults() can be used to
 * tell if there was an error or the result set is finished.
 * 
 * @param query         A query (or statement) Handle.
 * @return              True if a row was fetched, false otherwise.
 * @error               Invalid query Handle.
 */
native bool SQL_FetchRow(Handle query);

/**
 * Returns if there are more rows.
 * 
 * @param query         A query (or statement) Handle.
 * @return              True if there are more rows, false otherwise.
 * @error               Invalid query Handle.
 */
native bool SQL_MoreRows(Handle query);

/**
 * Rewinds a result set back to the first result.
 * 
 * @param query         A query (or statement) Handle.
 * @return              True on success, false otherwise.
 * @error               Invalid query Handle or no current result set.
 */
native bool SQL_Rewind(Handle query);

/**
 * Fetches a string from a field in the current row of a result set.  
 * If the result is NULL, an empty string will be returned.  A NULL 
 * check can be done with the result parameter, or SQL_IsFieldNull().
 * 
 * @param query         A query (or statement) Handle.
 * @param field         The field index (starting from 0).
 * @param buffer        String buffer.
 * @param maxlength     Maximum size of the string buffer.
 * @param result        Optional variable to store the status of the return value.
 * @return              Number of bytes written.
 * @error               Invalid query Handle or field index, invalid
 *                      type conversion requested from the database,
 *                      or no current result set.
 */
native int SQL_FetchString(Handle query, int field, char[] buffer, int maxlength, DBResult &result=DBVal_Error);

/**
 * Fetches a float from a field in the current row of a result set.  
 * If the result is NULL, a value of 0.0 will be returned.  A NULL 
 * check can be done with the result parameter, or SQL_IsFieldNull().
 * 
 * @param query         A query (or statement) Handle.
 * @param field         The field index (starting from 0).
 * @param result        Optional variable to store the status of the return value.
 * @return              A float value.
 * @error               Invalid query Handle or field index, invalid
 *                      type conversion requested from the database,
 *                      or no current result set.
 */
native float SQL_FetchFloat(Handle query, int field, DBResult &result=DBVal_Error);

/**
 * Fetches an integer from a field in the current row of a result set.  
 * If the result is NULL, a value of 0 will be returned.  A NULL 
 * check can be done with the result parameter, or SQL_IsFieldNull().
 * 
 * @param query         A query (or statement) Handle.
 * @param field         The field index (starting from 0).
 * @param result        Optional variable to store the status of the return value.
 * @return              An integer value.
 * @error               Invalid query Handle or field index, invalid
 *                      type conversion requested from the database,
 *                      or no current result set.
 */
native int SQL_FetchInt(Handle query, int field, DBResult &result=DBVal_Error);

/**
 * Returns whether a field's data in the current row of a result set is 
 * NULL or not.  NULL is an SQL type which means "no data."
 * 
 * @param query         A query (or statement) Handle.
 * @param field         The field index (starting from 0).
 * @return              True if data is NULL, false otherwise.
 * @error               Invalid query Handle or field index, or no
 *                      current result set.
 */
native bool SQL_IsFieldNull(Handle query, int field);

/**
 * Returns the length of a field's data in the current row of a result
 * set.  This only needs to be called for strings to determine how many
 * bytes to use.  Note that the return value does not include the null
 * terminator.
 * 
 * @param query         A query (or statement) Handle.
 * @param field         The field index (starting from 0).
 * @return              Number of bytes for the field's data size.
 * @error               Invalid query Handle or field index or no
 *                      current result set.
 */
native int SQL_FetchSize(Handle query, int field);

/**
 * Binds a parameter in a prepared statement to a given integer value.
 * 
 * @param statement     A statement (prepared query) Handle.
 * @param param         The parameter index (starting from 0).
 * @param number        The number to bind.
 * @param signed        True to bind the number as signed, false to 
 *                      bind it as unsigned.
 * @error               Invalid statement Handle or parameter index, or
 *                      SQL error.
 */
native void SQL_BindParamInt(Handle statement, int param, int number, bool signed=true);

/**
 * Binds a parameter in a prepared statement to a given float value.
 * 
 * @param statement     A statement (prepared query) Handle.
 * @param param         The parameter index (starting from 0).
 * @param value         The float number to bind.
 * @error               Invalid statement Handle or parameter index, or
 *                      SQL error.
 */
native void SQL_BindParamFloat(Handle statement, int param, float value);

/**
 * Binds a parameter in a prepared statement to a given string value.
 * 
 * @param statement     A statement (prepared query) Handle.
 * @param param         The parameter index (starting from 0).
 * @param value         The string to bind.
 * @param copy          Whether or not SourceMod should copy the value
 *                      locally if necessary.  If the string contents
 *                      won't change before calling SQL_Execute(), this
 *                      can be set to false for optimization.
 * @error               Invalid statement Handle or parameter index, or
 *                      SQL error.
 */
native void SQL_BindParamString(Handle statement, int param, const char[] value, bool copy);

/**
 * Executes a prepared statement.  All parameters must be bound beforehand.
 *
 * @param statement     A statement (prepared query) Handle.
 * @return              True on success, false on failure.
 * @error               Invalid statement Handle.
 */
native bool SQL_Execute(Handle statement);

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
 * @param database      A database Handle.
 * @error               Invalid database Handle.
 */
native void SQL_LockDatabase(Handle database);

/**
 * Unlocks a database so threading operations may continue.
 *
 * @param database      A database Handle.
 * @error               Invalid database Handle.
 */
native void SQL_UnlockDatabase(Handle database);

/**
 * General callback for threaded SQL stuff.
 * 
 * @param owner         Parent object of the Handle (or INVALID_HANDLE if none).
 * @param hndl          Handle to the child object (or INVALID_HANDLE if none).
 * @param error         Error string if there was an error.  The error could be 
 *                      empty even if an error condition exists, so it is important 
 *                      to check the actual Handle value instead.
 * @param data          Data passed in via the original threaded invocation.
 */
typedef SQLTCallback = function void (Handle owner, Handle hndl, const char[] error, any data);

/**
 * Tells whether two database handles both point to the same database 
 * connection.
 *
 * @param hndl1         First database Handle.
 * @param hndl2         Second database Handle.
 * @return              True if the Handles point to the same 
 *                      connection, false otherwise.
 * @error               Invalid Handle.
 */
native bool SQL_IsSameConnection(Handle hndl1, Handle hndl2);

/**
 * Connects to a database via a thread.  This can be used instead of
 * SQL_Connect() if you wish for non-blocking functionality.
 *
 * It is not necessary to use this to use threaded queries.  However, if you 
 * don't (or you mix threaded/non-threaded queries), you should see 
 * SQL_LockDatabase().
 *
 * @param callback      Callback; new Handle will be in hndl, owner is the driver.
 *                      If no driver was found, the owner is INVALID_HANDLE.
 * @param name          Database name.
 * @param data          Extra data value to pass to the callback.
 */
native void SQL_TConnect(SQLTCallback callback, const char[] name="default", any data=0);

/**
 * Executes a simple query via a thread.  The query Handle is passed through
 * the callback.
 *
 * The database Handle returned through the callback is always a new Handle,
 * and if necessary, SQL_IsSameConnection() should be used to test against
 * other connections.
 *
 * The query Handle returned through the callback is temporary and destroyed 
 * at the end of the callback.  If you need to hold onto it, use CloneHandle().
 *
 * @param database      A database Handle.
 * @param callback      Callback; database is in "owner" and the query Handle
 *                      is passed in "hndl".
 * @param query         Query string.
 * @param data          Extra data value to pass to the callback.
 * @param prio          Priority queue to use.
 * @error               Invalid database Handle.
 */
native void SQL_TQuery(Handle database, SQLTCallback callback, const char[] query, any data=0, DBPriority prio=DBPrio_Normal);

/**
 * Creates a new transaction object. A transaction object is a list of queries
 * that can be sent to the database thread and executed as a single transaction.
 *
 * @return              A transaction handle.
 */
native Transaction SQL_CreateTransaction();

/**
 * Adds a query to a transaction object.
 *
 * @param txn           A transaction handle.
 * @param query         Query string.
 * @param data          Extra data value to pass to the final callback.
 * @return              The index of the query in the transaction's query list.
 * @error               Invalid transaction handle.
 */
native int SQL_AddQuery(Transaction txn, const char[] query, any data=0);

/**
 * Sends a transaction to the database thread. The transaction handle is
 * automatically closed. When the transaction completes, the optional
 * callback is invoked.
 *
 * @param db            A database handle.
 * @param txn           A transaction handle.
 * @param onSuccess     An optional callback to receive a successful transaction.
 * @param onError       An optional callback to receive an error message.
 * @param data          An optional value to pass to callbacks.
 * @param prio          Priority queue to use.
 * @error               An invalid handle.
 */
native void SQL_ExecuteTransaction(
		Handle db,
		Transaction txn,
		SQLTxnSuccess onSuccess = INVALID_FUNCTION,
		SQLTxnFailure onError = INVALID_FUNCTION,
		any data=0,
		DBPriority priority=DBPrio_Normal);
