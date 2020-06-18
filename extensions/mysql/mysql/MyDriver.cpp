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

#include "MyDriver.h"
#include "MyDatabase.h"
#include "smsdk_ext.h"
#include "am-string.h"

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
	const char *host = NULL, *socket = NULL;

	decltype(info->maxTimeout) timeout = 60;
	if (info->maxTimeout > 0)
	{
		timeout = info->maxTimeout;
	}

	mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&timeout);
	mysql_options(mysql, MYSQL_OPT_READ_TIMEOUT, (const char *)&timeout);
	mysql_options(mysql, MYSQL_OPT_WRITE_TIMEOUT, (const char *)&timeout);

	/* Have MySQL automatically reconnect if it times out or loses connection.
	 * This will prevent "MySQL server has gone away" errors after a while.
	 */
	my_bool my_true = true;
	mysql_options(mysql, MYSQL_OPT_RECONNECT, (const char *)&my_true);

	if (info->host[0] == '/')
	{
		host = "localhost";
		socket = info->host;
	}
	else
	{
		host = info->host;
		socket = NULL;
	}

	if (!mysql_real_connect(mysql,
		host,
		info->user, 
		info->pass,
		info->database,
		info->port,
		socket,
		M_CLIENT_MULTI_RESULTS))
	{
		/* :TODO: expose UTIL_Format from smutil! */
		ke::SafeSprintf(error, maxlength, "[%d]: %s", mysql_errno(mysql), mysql_error(mysql));
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

	return (strcmp(str1, str2) == 0);
}

IDatabase *MyDriver::Connect(const DatabaseInfo *info, bool persistent, char *error, size_t maxlength)
{
	std::lock_guard<std::mutex> lock(m_Lock);

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
				db->IncReferenceCount();
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
	std::lock_guard<std::mutex> lock(m_Lock);
	if (persistent)
	{
		m_PermDbs.remove(pdb);
	}
}

bool MyDriver::IsThreadSafe()
{
	return (mysql_thread_safe() != 0);
}

bool MyDriver::InitializeThreadSafety()
{
	return (mysql_thread_init() == 0);
}

void MyDriver::ShutdownThreadSafety()
{
	mysql_thread_end();
}

size_t strncopy(char *dest, const char *src, size_t count)
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
