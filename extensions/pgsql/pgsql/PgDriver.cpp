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

#include "PgDriver.h"
#include "PgDatabase.h"
#include "smsdk_ext.h"

PgDriver g_PgDriver;

PgDriver::PgDriver()
{
	m_Handle = BAD_HANDLE;
}

void CloseDBList(List<PgDatabase *> &l)
{
	List<PgDatabase *>::iterator iter;
	for (iter=l.begin(); iter!=l.end(); iter++)
	{
		PgDatabase *db = (*iter);
		while (!db->Close())
		{
			/* Spool until it closes  */
		}
	}
	l.clear();
}

void PgDriver::Shutdown()
{
	List<PgDatabase *>::iterator iter;
	CloseDBList(m_PermDbs);

	if (m_Handle != BAD_HANDLE)
	{
		dbi->ReleaseHandle(m_Handle, DBHandle_Driver, myself->GetIdentity());
		m_Handle = BAD_HANDLE;
	}
}

const char *PgDriver::GetIdentifier()
{
	return "pgsql";
}

Handle_t PgDriver::GetHandle()
{
	if (m_Handle == BAD_HANDLE)
	{
		m_Handle = dbi->CreateHandle(DBHandle_Driver, this, myself->GetIdentity());
	}

	return m_Handle;
}

IdentityToken_t *PgDriver::GetIdentity()
{
	return myself->GetIdentity();
}

const char *PgDriver::GetProductName()
{
	return "PostgreSQL";
}

PGconn *Connect(const DatabaseInfo *info, char *error, size_t maxlength)
{
	/* https://www.postgresql.org/docs/9.6/libpq-connect.html#LIBPQ-CONNSTRING */
	/* TODO: Switch to PQconnectdbParams to prevent escaping issues. */
	char *options = new char[1024];
	int offs = snprintf(options, 1024, "host='%s' dbname='%s'", info->host, info->database);

	if (info->port > 0)
	{
		offs += snprintf(&options[offs], 1024 - offs, " port=%d", info->port);
	}

	if (info->user[0] != '\0')
	{
		offs += snprintf(&options[offs], 1024 - offs, " user='%s'", info->user);
	}
	if (info->pass[0] != '\0')
	{
		offs += snprintf(&options[offs], 1024 - offs, " password='%s'", info->pass);
	}

	if (info->maxTimeout > 0)
	{
		offs += snprintf(&options[offs], 1024 - offs, " connect_timeout=%d", info->maxTimeout);
	}

	/* Make a connection to the database */
	PGconn *conn = PQconnectdb(options);

	delete [] options;

	/* Check to see that the backend connection was successfully made */
	if (PQstatus(conn) != CONNECTION_OK)
	{
		/* :TODO: expose UTIL_Format from smutil! */
		snprintf(error, maxlength, "%s", PQerrorMessage(conn));
		PQfinish(conn);
		return NULL;
	}

	// :TODO: Check for connection problems everytime we talk to the database server and call PQreset on it, when it fails.
	// There is no automatic reconnect in libpg.
	// While we're at that. Save the prepared queries and rerun them when we reconnect!

	return conn;
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

IDatabase *PgDriver::Connect(const DatabaseInfo *info, bool persistent, char *error, size_t maxlength)
{
	std::lock_guard<std::mutex> lock(m_Lock);

	if (persistent)
	{
		/* Try to find a matching persistent connection */
		List<PgDatabase *>::iterator iter;
		for (iter=m_PermDbs.begin();
			 iter!=m_PermDbs.end();
			 iter++)
		{
			PgDatabase *db = (*iter);
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

	PGconn *pgsql = ::Connect(info, error, maxlength);
	if (!pgsql)
	{
		return NULL;
	}

	PgDatabase *db = new PgDatabase(pgsql, info, persistent);

	if (persistent)
	{
		m_PermDbs.push_back(db);
	}
	
	return db;
}

void PgDriver::RemoveFromList(PgDatabase *pdb, bool persistent)
{
	std::lock_guard<std::mutex> lock(m_Lock);
	if (persistent)
	{
		m_PermDbs.remove(pdb);
	}
}

bool PgDriver::IsThreadSafe()
{
	return (PQisthreadsafe() != 0);
}

bool PgDriver::InitializeThreadSafety()
{
	return IsThreadSafe();
}

void PgDriver::ShutdownThreadSafety()
{
	return;
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
