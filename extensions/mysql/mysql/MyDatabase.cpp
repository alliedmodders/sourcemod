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
: m_mysql(mysql), m_refcount(1), m_handle(BAD_HANDLE), m_bPersistent(persistent)
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
}

MyDatabase::~MyDatabase()
{
	mysql_close(m_mysql);
	m_mysql = NULL;
}

void MyDatabase::IncRefCount()
{
	m_refcount++;
}

bool MyDatabase::Close(bool fromHndlSys)
{
	if (m_refcount > 1)
	{
		m_refcount--;
		return false;
	}

	/* If we don't have a Handle and the Handle is
	 * is from the Handle System, it means we need
	 * to block a re-entrant call from our own 
	 * FreeHandle().
	 */
	if (fromHndlSys && (m_handle == BAD_HANDLE))
	{
		return false;
	}

	/* Remove us from the search list */
	if (m_bPersistent)
	{
		g_MyDriver.RemoveFromList(this, true);
	}

	/* If we're not from the Handle system, and
	 * we have a Handle, we need to free it first.
	 */
	if (!fromHndlSys && m_handle != BAD_HANDLE)
	{
		Handle_t hndl = m_handle;
		m_handle = BAD_HANDLE;
		dbi->ReleaseHandle(hndl, DBHandle_Database, myself->GetIdentity());
	}

	/* Finally, free our resource(s) */
	delete this;

	return true;
}

Handle_t MyDatabase::GetHandle()
{
	if (m_handle == BAD_HANDLE)
	{
		m_handle = dbi->CreateHandle(DBHandle_Database, this, myself->GetIdentity());
	}

	return m_handle;
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
	if (mysql_real_query(m_mysql, query, strlen(query)) != 0)
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

	if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0)
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
