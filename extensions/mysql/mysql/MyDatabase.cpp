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

	// DBI, for historical reasons, guarantees an initial refcount of 1.
	AddRef();
}

MyDatabase::~MyDatabase()
{
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
	return (unsigned int)mysql_insert_id(m_mysql);
}

unsigned int MyDatabase::GetAffectedRows()
{
	return (unsigned int)mysql_affected_rows(m_mysql);
}

const char *MyDatabase::GetError(int *errCode)
{
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

	needed = mysql_real_escape_string(m_mysql, buffer, str, size);
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
	UnlockFromFullAtomicOperation();
	return res;
}
