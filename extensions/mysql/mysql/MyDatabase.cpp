/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod MySQL Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#include "MyDatabase.h"
#include "smsdk_ext.h"
#include "MyBasicResults.h"
#include "MyStatement.h"

DBType GetOurType(enum_field_types type)
{
	switch (type)
	{
	case MYSQL_TYPE_DOUBLE:
	case MYSQL_TYPE_FLOAT:
		{
			return DBType_Float;
		}
	case MYSQL_TYPE_TINY:
	case MYSQL_TYPE_SHORT:
	case MYSQL_TYPE_LONG:
	case MYSQL_TYPE_INT24:
	case MYSQL_TYPE_YEAR:
	case MYSQL_TYPE_BIT:
		{
			return DBType_Integer;
		}
	case MYSQL_TYPE_LONGLONG:
	case MYSQL_TYPE_DATE:
	case MYSQL_TYPE_TIME:
	case MYSQL_TYPE_DATETIME:
	case MYSQL_TYPE_TIMESTAMP:
	case MYSQL_TYPE_NEWDATE:
	case MYSQL_TYPE_VAR_STRING:
	case MYSQL_TYPE_VARCHAR:
	case MYSQL_TYPE_STRING:
	case MYSQL_TYPE_NEWDECIMAL:
	case MYSQL_TYPE_DECIMAL:
	case MYSQL_TYPE_ENUM:
	case MYSQL_TYPE_SET:
		{
			return DBType_String;
		}

	case MYSQL_TYPE_TINY_BLOB:
	case MYSQL_TYPE_MEDIUM_BLOB:
	case MYSQL_TYPE_LONG_BLOB:
	case MYSQL_TYPE_BLOB:
	case MYSQL_TYPE_GEOMETRY:
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

// Reimplements mysql_cset_escape_slashes() for charsets where no byte of an mb
// sequence can be below 0x80 which makes escaping independent of the connection.
static size_t EscapeStringBackslashes(char *to, const char *from, size_t length)
{
	char *start = to;

	for (const char *end = from + length; from < end; from++)
	{
		char esc = '\0';

		switch (*from)
		{
			case 0:
				esc = '0';
				break;
			case '\n':
				esc = 'n';
				break;
			case '\r':
				esc = 'r';
				break;
			case '\\':
			case '\'':
			case '"':
				esc = *from;
				break;
			case '\032':
				esc = 'Z';
				break;
		}

		if (esc)
		{
			*to++ = '\\';
			*to++ = esc;
		}
		else
		{
			*to++ = *from;
		}
	}

	*to = '\0';

	return (size_t)(to - start);
}

static bool IsByteWiseEscapable(const MY_CHARSET_INFO &cs)
{
	if (cs.mbmaxlen <= 1)
	{
		return true;
	}

	// big5, gbk, sjis, etc. can end a sequence in 0x5c; UTF-8 can't.
	return cs.csname != NULL
		&& (strcmp(cs.csname, "utf8") == 0
			|| strcmp(cs.csname, "utf8mb3") == 0
			|| strcmp(cs.csname, "utf8mb4") == 0);
}

// Caches whether QuoteString() can escape without the connection. Must be called
// with m_FullLock held.
void MyDatabase::RefreshEscapeContext()
{
	if (m_bNoBackslashEscapes)
	{
		return;
	}

	// Escaping a lone backslash reveals whether the server has NO_BACKSLASH_ESCAPES
	// without reading server_status out of the connection struct. Check it, because
	// sql_mode can change while a query is in flight, and mishandling here could
	// allow for injection.
	char probe[3];
	if (mysql_real_escape_string(m_mysql, probe, "\\", 1) != 2)
	{
		m_bNoBackslashEscapes = true;
		m_bCanEscapeLocally = false;
		return;
	}

	MY_CHARSET_INFO cs;
	mysql_get_character_set_info(m_mysql, &cs);
	m_bCanEscapeLocally = IsByteWiseEscapable(cs);
}

MyDatabase::MyDatabase(MYSQL *mysql, const DatabaseInfo *info, bool persistent)
: m_mysql(mysql), m_bPersistent(persistent)
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

	// Nothing else can reach the connection yet, so no lock is needed here.
	RefreshEscapeContext();

	// DBI, for historical reasons, guarantees an initial refcount of 1.
	AddRef();
}

MyDatabase::~MyDatabase()
{
	std::lock_guard<std::recursive_mutex> lock(m_FullLock);

	/* Remove us from the search list */
	if (m_bPersistent)
		g_MyDriver.RemoveFromList(this, true);
	mysql_close(m_mysql);
}

void MyDatabase::IncReferenceCount()
{
	AddRef();
}

bool MyDatabase::Close()
{
	return !Release();
}

const DatabaseInfo &MyDatabase::GetInfo()
{
	return m_Info;
}

unsigned int MyDatabase::GetInsertID()
{
	std::lock_guard<std::recursive_mutex> lock(m_FullLock);
	return (unsigned int)mysql_insert_id(m_mysql);
}

unsigned int MyDatabase::GetAffectedRows()
{
	std::lock_guard<std::recursive_mutex> lock(m_FullLock);
	return (unsigned int)mysql_affected_rows(m_mysql);
}

const char *MyDatabase::GetError(int *errCode)
{
	std::lock_guard<std::recursive_mutex> lock(m_FullLock);

	if (errCode)
	{
		*errCode = mysql_errno(m_mysql);
	}

	return mysql_error(m_mysql);
}

bool MyDatabase::QuoteString(const char *str, char buffer[], size_t maxlength, size_t *newSize)
{
	unsigned long size = static_cast<unsigned long>(strlen(str));
	unsigned long needed = size * 2 + 1;

	if (maxlength < needed)
	{
		if (newSize)
		{
			*newSize = (size_t)needed;
		}
		return false;
	}

	// Refresh only while the connection is idle; blocking would stall the game
	// thread for as long as a threaded query takes to come back.
	if (m_FullLock.try_lock())
	{
		RefreshEscapeContext();
		m_FullLock.unlock();
	}

	if (m_bCanEscapeLocally)
	{
		needed = static_cast<unsigned long>(EscapeStringBackslashes(buffer, str, size));
	}
	else
	{
		std::lock_guard<std::recursive_mutex> lock(m_FullLock);
		needed = mysql_real_escape_string(m_mysql, buffer, str, size);
	}

	if (newSize)
	{
		*newSize = (size_t)needed;
	}

	return true;
}

bool MyDatabase::DoSimpleQuery(const char *query)
{
	IQuery *pQuery = DoQuery(query);
	if (!pQuery)
	{
		return false;
	}
	pQuery->Destroy();
	return true;
}

IQuery *MyDatabase::DoQuery(const char *query)
{
	// A MYSQL connection may be shared between threads only if the entire
	// mysql_real_query()/mysql_store_result() sequence is serialized.
	std::lock_guard<std::recursive_mutex> lock(m_FullLock);

	if (mysql_real_query(m_mysql, query, static_cast<unsigned long>(strlen(query))) != 0)
	{
		return NULL;
	}

	MYSQL_RES *res = NULL;
	if (mysql_field_count(m_mysql))
	{
		res = mysql_store_result(m_mysql);
		if (!res)
		{
			return NULL;
		}
	}

	return new MyQuery(this, res);
}

bool MyDatabase::DoSimpleQueryEx(const char *query, size_t len)
{
	IQuery *pQuery = DoQueryEx(query, len);
	if (!pQuery)
	{
		return false;
	}
	pQuery->Destroy();
	return true;
}

IQuery *MyDatabase::DoQueryEx(const char *query, size_t len)
{
	// See DoQuery().
	std::lock_guard<std::recursive_mutex> lock(m_FullLock);

	if (mysql_real_query(m_mysql, query, static_cast<unsigned long>(len)) != 0)
	{
		return NULL;
	}

	MYSQL_RES *res = NULL;
	if (mysql_field_count(m_mysql))
	{
		res = mysql_store_result(m_mysql);
		if (!res)
		{
			return NULL;
		}
	}

	return new MyQuery(this, res);
}

unsigned int MyDatabase::GetAffectedRowsForQuery(IQuery *query)
{
	return static_cast<MyQuery*>(query)->GetAffectedRows();
}

unsigned int MyDatabase::GetInsertIDForQuery(IQuery *query)
{
	return static_cast<MyQuery*>(query)->GetInsertID();
}

IPreparedQuery *MyDatabase::PrepareQuery(const char *query, char *error, size_t maxlength, int *errCode)
{
	std::lock_guard<std::recursive_mutex> lock(m_FullLock);

	MYSQL_STMT *stmt = mysql_stmt_init(m_mysql);
	if (!stmt)
	{
		if (error)
		{
			strncopy(error, GetError(errCode), maxlength);
		} else if (errCode) {
			*errCode = mysql_errno(m_mysql);
		}
		return NULL;
	}

	if (mysql_stmt_prepare(stmt, query, static_cast<unsigned long>(strlen(query))) != 0)
	{
		if (error)
		{
			strncopy(error, mysql_stmt_error(stmt), maxlength);
		}
		if (errCode)
		{
			*errCode = mysql_stmt_errno(stmt);
		}
		mysql_stmt_close(stmt);
		return NULL;
	}

	return new MyStatement(this, stmt);
}

bool MyDatabase::LockForFullAtomicOperation()
{
	m_FullLock.lock();
	return true;
}

void MyDatabase::UnlockFromFullAtomicOperation()
{
	m_FullLock.unlock();
}

IDBDriver *MyDatabase::GetDriver()
{
	return &g_MyDriver;
}

bool MyDatabase::SetCharacterSet(const char *characterset)
{
	bool res;
	LockForFullAtomicOperation();
	res = mysql_set_character_set(m_mysql, characterset) == 0 ? true : false;
	if (res)
	{
		RefreshEscapeContext();
	}
	UnlockFromFullAtomicOperation();
	return res;
}
