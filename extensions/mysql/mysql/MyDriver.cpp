#include "MyDriver.h"
#include "MyDatabase.h"
#include "sdk/smsdk_ext.h"

MyDriver g_MyDriver;

MyDriver::MyDriver()
{
	m_MyHandle = BAD_HANDLE;
}

void CloseDBList(List<MyDatabase *> &l)
{
	List<MyDatabase *>::iterator iter;
	for (iter=l.begin(); iter!=l.end(); iter++)
	{
		MyDatabase *db = (*iter);
		while (!db->Close())
		{
			/* Spool until it closes  */
		}
	}
	l.clear();
}

void MyDriver::Shutdown()
{
	List<MyDatabase *>::iterator iter;
	CloseDBList(m_PermDbs);

	if (m_MyHandle != BAD_HANDLE)
	{
		dbi->ReleaseHandle(m_MyHandle, DBHandle_Driver, myself->GetIdentity());
		m_MyHandle = BAD_HANDLE;
	}
}

const char *MyDriver::GetIdentifier()
{
	return "mysql";
}

Handle_t MyDriver::GetHandle()
{
	if (m_MyHandle == BAD_HANDLE)
	{
		m_MyHandle = dbi->CreateHandle(DBHandle_Driver, this, myself->GetIdentity());
	}

	return m_MyHandle;
}

IdentityToken_t *MyDriver::GetIdentity()
{
	return myself->GetIdentity();
}

const char *MyDriver::GetProductName()
{
	return "MySQL";
}

MYSQL *Connect(const DatabaseInfo *info, char *error, size_t maxlength)
{
	MYSQL *mysql = mysql_init(NULL);

	if (info->maxTimeout > 0)
	{
		mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&(info->maxTimeout));
	}

	if (!mysql_real_connect(mysql,
		info->host,
		info->user, 
		info->pass,
		info->database,
		info->port,
		NULL,
		M_CLIENT_MULTI_RESULTS))
	{
		/* :TODO: expose UTIL_Format from smutil! */
		snprintf(error, maxlength, "[%d]: %s", mysql_errno(mysql), mysql_error(mysql));
		mysql_close(mysql);
		return NULL;
	}

	return mysql;
}

bool CompareField(const char *str1, const char *str2)
{
	if ((str1 == NULL && str2 != NULL)
		|| (str1 != NULL && str2 == NULL))
	{
		return false;
	}

	if (str1 == NULL && str2 == NULL)
	{
		return true;
	}

	return (strcmp(str1, str2) == NULL);
}

IDatabase *MyDriver::Connect(const DatabaseInfo *info, bool persistent, char *error, size_t maxlength)
{
	if (persistent)
	{
		/* Try to find a matching persistent connection */
		List<MyDatabase *>::iterator iter;
		for (iter=m_PermDbs.begin();
			 iter!=m_PermDbs.end();
			 iter++)
		{
			MyDatabase *db = (*iter);
			const DatabaseInfo &other = db->GetInfo();
			if (CompareField(info->host, other.host)
				&& CompareField(info->user, other.user)
				&& CompareField(info->pass, other.pass)
				&& CompareField(info->database, other.database)
				&& (info->port == other.port))
			{
				db->IncRefCount();
				return db;
			}
		}
	}

	MYSQL *mysql = ::Connect(info, error, maxlength);
	if (!mysql)
	{
		return NULL;
	}

	MyDatabase *db = new MyDatabase(mysql, info, persistent);

	if (persistent)
	{
		m_PermDbs.push_back(db);
	}
	
	return db;
}

void MyDriver::RemoveFromList(MyDatabase *pdb, bool persistent)
{
	if (persistent)
	{
		m_PermDbs.remove(pdb);
	}
}

unsigned int strncopy(char *dest, const char *src, size_t count)
{
	if (!count)
	{
		return 0;
	}

	char *start = dest;
	while ((*src) && (--count))
	{
		*dest++ = *src++;
	}
	*dest = '\0';

	return (dest - start);
}
