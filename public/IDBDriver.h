/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod, Copyright (C) 2004-2007 AlliedModders LLC. 
 * All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be 
 * used or modified under the Terms and Conditions of its License Agreement, 
 * which is found in public/licenses/LICENSE.txt.  As of this notice, derivative 
 * works must be licensed under the GNU General Public License (version 2 or 
 * greater).  A copy of the GPL is included under public/licenses/GPL.txt.
 * 
 * To view the latest information, see: http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_INTERFACE_DBDRIVER_H_
#define _INCLUDE_SOURCEMOD_INTERFACE_DBDRIVER_H_

#include <IShareSys.h>
#include <string.h>

#define SMINTERFACE_DBI_NAME		"IDBI"
#define SMINTERFACE_DBI_VERSION		1

namespace SourceMod
{
	/**
	 * @brief Describes a database field value.
	 */
	enum DBResult
	{
		DBVal_Error = 0,		/**< Column number/field is invalid */
		DBVal_TypeMismatch = 1,	/**< You cannot retrieve this data with this type */
		DBVal_Null = 2,			/**< Field has no data (NULL) */
		DBVal_Data = 3,			/**< Field has data */
	};

	/**
	 * @brief Represents a one database result row.
	 */
	class IResultRow
	{
	public:
		/**
		 * @brief Retrieves a database field result as a string.
		 *
		 * For NULL values, the resulting string pointer will be non-NULL,
		 * but empty.
		 *
		 * @param columnId		Column to use, starting from 0.
		 * @param pString		Pointer to store a pointer to the string.
		 * @return				A DBResult return code.
		 */
		virtual DBResult GetString(unsigned int columnId, const char **pString) =0;
		
		/**
		 * @brief Retrieves a database field result as a float.
		 *
		 * For NULL entries, the returned float value will be 0.0.
		 *
		 * @param columnId		Column to use, starting from 0.
		 * @param pFloat		Pointer to a floating point number to set.
		 * @return				A DBResult return code.
		 */
		virtual DBResult GetFloat(unsigned int columnId, float *pFloat) =0;

		/**
		 * @brief Retrieves a database field result as an integer.
		 *
		 * For NULL entries, the returned integer value will be 0.
		 *
		 * @param columnId		Column to use, starting from 0.
		 * @param pInt			Pointer to an integer number to set.
		 * @return				A DBResult return code.
		 */
		virtual DBResult GetInt(unsigned int columnId, int *pInt) =0;

		/**
		 * @brief Returns whether or not a field is NULL.
		 *
		 * @param columnId		Column to use, starting from 0.
		 * @return				A DBResult return code.
		 */
		virtual DBResult CheckField(unsigned int columnId) =0;

		/**
		 * @brief Retrieves field data as a raw bitstream.
		 *
		 * @param columnId		Column to use, starting from 0.
		 * @param pData			Pointer to store the raw bit stream.
		 * @param length		Pointer to store the data length.
		 * @return				A DBResult return code.
		 */
		virtual DBResult GetRaw(unsigned int columnId, void **pData, size_t *length) =0;
	};

	/**
	 * @brief Describes a set of database results.
	 */
	class IResultSet
	{
	public:
		/**
		 * @brief Returns the number of rows in the set.
		 *
		 * @return				Number of rows in the set.
		 */
		virtual unsigned int GetRowCount() =0;

		/**
		 * @brief Returns the number of fields in the set.
		 *
		 * @return				Number of fields in the set.
		 */
		virtual unsigned int GetFieldCount() =0;

		/**
		 * @brief Converts a column number to a column name.
		 *
		 * @param columnId		Column to use, starting from 0.
		 * @return				Pointer to column name, or NULL if not found.
		 */
		virtual const char *ColNumToName(unsigned int columnId) =0;

		/**
		 * @brief Converts a column name to a column id.
		 *
		 * @param name			Column name (case sensitive).
		 * @param columnId		Pointer to store the column id.  If the
		 *						name is not found, the value will be
		 *						undefined.
		 * @return				True on success, false if not found.
		 */
		virtual bool ColNumToName(const char *name, unsigned int *columnId) =0;

		/**
		 * @brief Returns if there is still data in the result set.
		 *
		 * @return				False if there is more data to be read, 
		 *						true, otherwise.
		 */
		virtual bool IsFinished() =0;

		/**
		 * @brief Returns a pointer to the current row and advances
		 * the internal row pointer/counter to the next row available.
		 *
		 * @return				IResultRow pointer to the current row,
		 *						or NULL if there is no more data.
		 */
		virtual IResultRow *FetchRow() =0;

		/**
		 * @brief Rewinds back to the beginning of the row iteration.
		 * 
		 * @return				True on success, false otherwise.
		 */
		virtual bool Rewind() =0;
	};

	class IDatabase;

	/**
	 * @brief Encapsulates one database query.
	 */
	class IQuery
	{
	public:
		/**
		 * @brief Number of rows affected by the last execute.
		 *
		 * @return				Number of rows affected by the last execute.
		 */
		virtual unsigned int GetAffectedRows() =0;

		/**
		 * @brief Retrieves the last insert ID of this query, if any.
		 *
		 * @return				Row insertion ID of the last execute, if any.
		 */
		virtual unsigned int GetInsertID() =0;

		/**
		 * @brief Returns the full query string.
		 *
		 * @return				String containing the original query.
		 */
		virtual const char *GetQueryString() =0;

		/**
		 * @brief Returns the result set from the last Execute call.
		 * 
		 * @return				IResultSet from the last Execute call. 
		 *						Pointer will be NULL if there was no previous
		 *						call, the last call failed, or the last call
		 *						had no result set and Execute() was called with
		 *						retEmptySet=false.
		 */
		virtual IResultSet *GetResultSet() =0;

		/**
		 * @brief Returns the IDatabase object which created this pointer.
		 *
		 * @return				IDatabase object which created this pointer.
		 */
		virtual IDatabase *GetDatabase() =0;

		/** 
		 * @brief Releases the resources associated with this query.
		 */
		virtual void Release() =0;
	};

	class IDBDriver;

	/**
	 * @brief Encapsulates a database connection.
	 */
	class IDatabase
	{
	public:
		/**
		 * @brief Disconnects the database and frees its associated memory.
		 * If this pointer was received via IDBDriver::PConnect(), Close()
		 * must be called once for each successful PConnect() call which 
		 * resulted in this pointer.
		 */
		virtual void Close() =0;

		/**
		 * @brief Error code and string returned by the last operation on this
		 * connection.
		 *
		 * @param errorCode		Optional pointer to retrieve an error code.
		 * @return				Error string pointer (empty if none).
		 */
		virtual const char *GetLastError(int *errorCode=NULL) =0;

		/**
		 * @brief Prepares and executes a query in one step, and returns
		 * the resulting IQuery pointer.
		 *
		 * @param fmt			String format to parse.
		 * @param ...			Format parameters.
		 * @return				An IQuery pointer which must be freed, or
		 *						NULL if the query failed.
		 */
		virtual IQuery *DoQuery(const char *fmt, ...) =0;

		/**
		 * Quotes a string for insertion into a query.
		 *
		 * @param str			Source string.
		 * @param buffer		Buffer to store new string (should not overlap source string).
		 * @param maxlen		Maximum length of the output buffer.
		 * @param newSize		Pointer to store the output size.
		 * @return				True on success, false if the output buffer is not big enough.
		 *						If not big enough, the required buffer size is passed through
		 *						newSize.
		 */
		virtual bool QuoteString(const char *str, char buffer[], size_t maxlen, size_t *newSize) =0;

		/**
		 * @brief Returns the parent driver.
		 *
		 * @return				IDBDriver pointer that created this object.
		 */
		virtual IDBDriver *GetDriver() =0;
	};

	/** 
	 * @brief Describes database connection info.
	 */
	struct DatabaseInfo
	{
		DatabaseInfo()
		{
			dbiVersion = SMINTERFACE_DBI_VERSION;
			port = 0;
			maxTimeout = 0;
		}
		unsigned int dbiVersion;		/**< DBI Version for backwards compatibility */
		const char *host;				/**< Host string */
		const char *database;			/**< Database name string */
		const char *user;				/**< User to authenticate as */
		const char *pass;				/**< Password to authenticate with */
		const char *driver;				/**< Driver to use */
		unsigned int port;				/**< Port to use, 0=default */
		unsigned int maxTimeout;		/**< Maximum timeout, 0=default */
	};

	/**
	 * @brief Describes an SQL driver.
	 */
	class IDBDriver
	{
	public:
		virtual unsigned int GetDBIVersion()
		{
			return SMINTERFACE_DBI_VERSION;
		}
	public:
		/**
		 * @brief Initiates a database connection.
		 * 
		 * @param info			Database connection info pointer.
		 * @return				A new IDatabase pointer, or NULL on failure.
		 */
		virtual IDatabase *Connect(const DatabaseInfo *info) =0;

		/**
		 * @brief Initiates a database connection.  If a connection to the given database
		 * already exists, this will re-use the old connection handle.
		 * 
		 * @param info			Database connection info pointer.
		 * @return				A new IDatabase pointer, or NULL on failure.
		 */
		virtual IDatabase *PConnect(const DatabaseInfo *info) =0;

		/**
		 * @brief Returns a case insensitive database identifier string.
		 */
		virtual const char *GetIdentifier() =0;

		/**
		 * @brief Returns a case sensitive implementation name.
		 */
		virtual const char *GetProductName() =0;

		/**
		 * @brief Returns the last connection error.
		 *
		 * @param errCode		Optional pointer to store a platform-specific error code.
		 */
		virtual const char *GetLastError(int *errCode=NULL) =0;
	};

	/**
	 * @brief Describes the DBI manager.
	 */
	class IDBManager : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName() =0;
		virtual unsigned int GetInterfaceVersion() =0;
	public:
		/**
		 * @brief Adds a driver to the DBI system.
		 *
		 * @param pDriver		Database driver.
		 */
		virtual void AddDriver(IDBDriver *pDriver, IdentityToken_t *pToken) =0;

		/**
		 * @brief Removes a driver from the DBI system.
		 *
		 * @param pDriver		Database driver.
		 */
		virtual void RemoveDriver(IDBDriver *pDriver) =0;

		/**
		 * @brief Searches for database info by name.
		 *
		 * @param name			Named database info.
		 * @return				DatabaseInfo pointer.
		 */
		virtual const DatabaseInfo *FindDatabaseConf(const char *name) =0;

		/**
		 * @brief Tries to connect to a named database.  
		 *
		 * @param name			Named database info.
		 * @param pdr			Pointer to store the IDBDriver pointer in.
		 *						If driver is not found, NULL will be stored.
		 * @param pdb			Pointer to store the IDatabase pointer in.
		 *						If connection fails, NULL will be stored.
		 * @param persistent	If true, the dbmanager will attempt to PConnect
		 *						instead of connect.
		 * @return				True on success, false otherwise.
		 */
		virtual bool Connect(const char *name, IDBDriver **pdr, IDatabase **pdb, bool persistent) =0;

		/**
		 * @brief Returns the number of drivers loaded.
		 *
		 * @return				Number of drivers loaded.
		 */
		virtual unsigned int GetDriverCount() =0;

		/**
		 * @brief Returns a driver by index.
		 *
		 * @param index			Driver index, starting from 0.
		 * @param pToken		Optional pointer to store the driver's identity token.
		 * @return				IDBDriver pointer for the given index.
		 */
		virtual IDBDriver *GetDriver(unsigned int index, IdentityToken_t **pToken=NULL) =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_INTERFACE_DBDRIVER_H_
