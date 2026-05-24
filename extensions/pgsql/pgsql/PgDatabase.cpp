/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod PostgreSQL Extension
 * Copyright (C) 2013 AlliedModders LLC.  All rights reserved.
 * =============================================================================
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

#include "PgDatabase.h"
#include "smsdk_ext.h"
#include "PgBasicResults.h"
#include "PgStatement.h"
#include <string>

// Some selected defines from postgresql 9.2.4's src/include/catalog/pg_type.h
// Fast scan to extract the types that shouldn't be read as string.

// Floats
#define FLOAT4OID 700
#define FLOAT8OID 701
#define NUMERICOID		1700

// Integer
#define BOOLOID			16
#define INT8OID			20
#define INT2OID			21
#define INT4OID			23
#define OIDOID			26
#define TIDOID		27
#define XIDOID 28
#define CIDOID 29

#define ABSTIMEOID		702
#define RELTIMEOID		703
#define TINTERVALOID	704

#define REFCURSOROID	1790

// String
#define BYTEAOID		17 // Hrmpf. Could be both, string or blob binary data..
#define CHAROID			18
#define NAMEOID			19
#define TEXTOID			25
#define BPCHAROID		1042
#define VARCHAROID		1043
#define DATEOID			1082
#define TIMEOID			1083

// Blob
#define POINTOID		600
#define LSEGOID			601
#define PATHOID			602
#define BOXOID			603
#define POLYGONOID		604

DBType GetOurType(Oid type)
{
	switch (type)
	{
	case FLOAT4OID:
	case FLOAT8OID:
		{
			return DBType_Float;
		}
	case BOOLOID:
	case INT8OID:
	case INT2OID:
	case INT4OID:
	case OIDOID:
	case TIDOID:
	case XIDOID:
	case CIDOID:
	case ABSTIMEOID:
	case RELTIMEOID:
	case TINTERVALOID:
	case REFCURSOROID:
		{
			return DBType_Integer;
		}
	case BYTEAOID:
	case CHAROID:
	case NAMEOID:
	case TEXTOID:
	case BPCHAROID:
	case VARCHAROID:
	case DATEOID:
	case TIMEOID:
		{
			return DBType_String;
		}

	case POINTOID:
	case LSEGOID:
	case PATHOID:
	case BOXOID:
	case POLYGONOID:
		{
			return DBType_Blob;
		}
	default:
		{
			return DBType_String;
		}
	}

	return DBType_Unknown;
}

PgDatabase::PgDatabase(PGconn *pgsql, const DatabaseInfo *info, bool persistent)
	: m_pgsql(pgsql), m_lastInsertID(0), m_lastAffectedRows(0), m_preparedStatementID(0),
  m_bPersistent(persistent)
{
	m_Host.assign(info->host);
	m_Database.assign(info->database);
	m_User.assign(info->user);
	m_Pass.assign(info->pass);

	m_Info.database = m_Database.c_str();
	m_Info.host = m_Host.c_str();
	m_Info.user = m_User.c_str();
	m_Info.pass = m_Pass.c_str();
	m_Info.driver = NULL;
	m_Info.maxTimeout = info->maxTimeout;
	m_Info.port = info->port;

	// DBI, for historical reasons, guarantees an initial refcount of 1.
	AddRef();
}

PgDatabase::~PgDatabase()
{
	if (m_bPersistent)
		g_PgDriver.RemoveFromList(this, true);
	PQfinish(m_pgsql);

	// libpg doesn't keep track of open resultsets of a connection.
	// maybe we should track all opened results and clear them here?
	// Otherwise there might be a memory leak, if the plugin doesn't close all result handles properly.
	// There is no restriction on the order of closing the database connection and the result handles though.
}

void PgDatabase::IncReferenceCount()
{
	AddRef();
}

void PgDatabase::SetLastIDAndRows(unsigned int insertID, unsigned int affectedRows)
{
	std::lock_guard<std::mutex> lock(m_LastQueryInfoLock);
	// Also remember the last query's insert id and affected rows. postgresql only stores them per query.
	m_lastInsertID = insertID;
	m_lastAffectedRows = affectedRows;
}

bool PgDatabase::Close()
{
	return !Release();
}

const DatabaseInfo &PgDatabase::GetInfo()
{
	return m_Info;
}

unsigned int PgDatabase::GetInsertID()
{
	std::lock_guard<std::mutex> lock(m_LastQueryInfoLock);
	return m_lastInsertID;
}

unsigned int PgDatabase::GetAffectedRows()
{
	std::lock_guard<std::mutex> lock(m_LastQueryInfoLock);
	return m_lastAffectedRows;
}

const char *PgDatabase::GetError(int *errCode)
{
	if (errCode)
	{
		// PostgreSQL only supports SQLSTATE error codes.
		// https://www.postgresql.org/docs/9.6/errcodes-appendix.html
		*errCode = -1;
	}

	return PQerrorMessage(m_pgsql);
}

bool PgDatabase::QuoteString(const char *str, char buffer[], size_t maxlength, size_t *newSize)
{
	size_t size = strlen(str);
	size_t needed = size * 2 + 1;

	if (maxlength < needed)
	{
		if (newSize)
		{
			*newSize = (size_t)needed;
		}
		return false;
	}

	int error = 0;
	needed = PQescapeStringConn(m_pgsql, buffer, str, size, &error);
	if (newSize)
	{
		*newSize = (size_t)needed;
	}

	return error == 0;
}

bool PgDatabase::DoSimpleQuery(const char *query)
{
	IQuery *pQuery = DoQuery(query);
	if (!pQuery)
	{
		return false;
	}
	pQuery->Destroy();
	return true;
}

IQuery *PgDatabase::DoQuery(const char *query)
{
	PGresult *res = PQexec(m_pgsql, query);
	if (res == NULL)
	{
		return NULL;
	}
	
	ExecStatusType status = PQresultStatus(res);
	
	if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK)
	{
		PQclear(res);
		return NULL;
	}

	return new PgQuery(this, res);
}

bool PgDatabase::DoSimpleQueryEx(const char *query, size_t len)
{
	IQuery *pQuery = DoQueryEx(query, len);
	if (!pQuery)
	{
		return false;
	}
	pQuery->Destroy();
	return true;
}

IQuery *PgDatabase::DoQueryEx(const char *query, size_t len)
{
	// PQexec only accepts a null-terminated SQL string. It can honor an
	// explicit shorter length, but it cannot send embedded null bytes.
	size_t queryLen = strlen(query);
	if (queryLen < len)
		return NULL;

	if (len == queryLen)
		return DoQuery(query);

	std::string limitedQuery(query, len);
	return DoQuery(limitedQuery.c_str());
}

unsigned int PgDatabase::GetAffectedRowsForQuery(IQuery *query)
{
	return static_cast<PgQuery*>(query)->GetAffectedRows();
}

unsigned int PgDatabase::GetInsertIDForQuery(IQuery *query)
{
	return static_cast<PgQuery*>(query)->GetInsertID();
}

IPreparedQuery *PgDatabase::PrepareQuery(const char *query, char *error, size_t maxlength, int *errCode)
{
	std::lock_guard<std::recursive_mutex> lock(m_FullLock);

	unsigned int stmtID = m_preparedStatementID;
	std::string stmtName = std::to_string(stmtID);
	m_preparedStatementID++;
	
	// Let postgresql guess the types of the arguments if there are any..
	PGresult *res = PQprepare(m_pgsql, stmtName.c_str(), query, 0, NULL);
	if (res == NULL)
	{
		if (error)
		{
			strncopy(error, PQerrorMessage(m_pgsql), maxlength);
		}

		if (errCode)
		{
			// PostgreSQL only supports SQLSTATE error codes.
			// https://www.postgresql.org/docs/9.6/errcodes-appendix.html
			*errCode = -1;
		}

		return NULL;
	}

	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		if (error)
		{
			strncopy(error, PQresultErrorMessage(res), maxlength);
		}

		if (errCode)
		{
			// PostgreSQL only supports SQLSTATE error codes.
			// https://www.postgresql.org/docs/9.6/errcodes-appendix.html
			*errCode = -1;
		}

		PQclear(res);
		return NULL;
	}

	// Only the statement name is of importance. free the PGresult here.
	PQclear(res);

	PGresult *desc = PQdescribePrepared(m_pgsql, stmtName.c_str());
	if (desc == NULL || PQresultStatus(desc) != PGRES_COMMAND_OK)
	{
		if (error)
		{
			const char *message = desc != NULL ? PQresultErrorMessage(desc) : PQerrorMessage(m_pgsql);
			strncopy(error, message, maxlength);
		}

		if (errCode)
		{
			// PostgreSQL only supports SQLSTATE error codes.
			// https://www.postgresql.org/docs/9.6/errcodes-appendix.html
			*errCode = -1;
		}

		if (desc != NULL)
			PQclear(desc);
		PGresult *close = PQclosePrepared(m_pgsql, stmtName.c_str());
		if (close != NULL)
			PQclear(close);
		return NULL;
	}

	unsigned int params = (unsigned int)PQnparams(desc);
	PQclear(desc);

	return new PgStatement(this, stmtName.c_str(), params);
}

bool PgDatabase::LockForFullAtomicOperation()
{
	m_FullLock.lock();
	return true;
}

void PgDatabase::UnlockFromFullAtomicOperation()
{
	m_FullLock.unlock();
}

IDBDriver *PgDatabase::GetDriver()
{
	return &g_PgDriver;
}

bool PgDatabase::SetCharacterSet(const char *characterset)
{
	bool res;
	LockForFullAtomicOperation();
	res = PQsetClientEncoding(m_pgsql, characterset) == 0;
	UnlockFromFullAtomicOperation();
	return res;
}
