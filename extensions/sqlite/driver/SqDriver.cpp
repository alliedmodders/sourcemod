/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod SQLite Driver Extension
 * Copyright (C) 2004-2007 AlliedModders LLC. All rights reserved.
 * ===============================================================
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#include <sm_platform.h>
#include "extension.h"
#include "SqDriver.h"
#include "SqDatabase.h"

SqDriver g_SqDriver;

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

SqDriver::SqDriver()
{
	m_Handle = BAD_HANDLE;
	m_pOpenLock = NULL;
}

void SqDriver::Initialize()
{
	m_pOpenLock = threader->MakeMutex();
}

void SqDriver::Shutdown()
{
	if (m_pOpenLock)
	{
		m_pOpenLock->DestroyThis();
	}
}

bool SqDriver::IsThreadSafe()
{
	return true;
}

bool SqDriver::InitializeThreadSafety()
{
	/* sqlite should be thread safe if the locks are done right.
	 * we don't enable the "shared cache" because it can corrupt
	 * open databases!
	 */
	return true;
}

void SqDriver::ShutdownThreadSafety()
{
	return;
}

IdentityToken_t *SqDriver::GetIdentity()
{
	return myself->GetIdentity();
}

const char *SqDriver::GetProductName()
{
	return "SQLite";
}

const char *SqDriver::GetIdentifier()
{
	return "sqlite";
}

Handle_t SqDriver::GetHandle()
{
	if (m_Handle == BAD_HANDLE)
	{
		m_Handle = dbi->CreateHandle(DBHandle_Driver, this, myself->GetIdentity());
	}

	return m_Handle;
}

inline bool IsPathSepChar(char c)
{
#if defined PLATFORM_WINDOWS
	return (c == '\\' || c == '/');
#elif defined PLATFORM_LINUX
	return (c == '/');
#endif
}

IDatabase *SqDriver::Connect(const DatabaseInfo *info, bool persistent, char *error, size_t maxlength)
{
	/* We wrap most of the open process in a mutex just to be safe */
	m_pOpenLock->Lock();

	/* Format our path */
	char path[PLATFORM_MAX_PATH];
	size_t len = libsys->PathFormat(path, sizeof(path), "sqlite/%s", info->database);

	/* Chop any filename off */
	for (size_t i = len-1;
		 i >= 0 && i <= len-1;
		 i--)
	{
		if (IsPathSepChar(path[i]))
		{
			path[i] = '\0';
			break;
		}
	}

	/* Test the full path */
	char fullpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_SM, fullpath, sizeof(fullpath), "data/%s", path);
	if (!libsys->IsPathDirectory(fullpath))
	{
		/* Make sure the data folder exists */
		len = g_pSM->BuildPath(Path_SM, fullpath, sizeof(fullpath), "data");
		if (!libsys->IsPathDirectory(fullpath))
		{
			if (!libsys->CreateFolder(fullpath))
			{
				strncopy(error, "Could not create or open \"data\" folder\"", maxlength);
				m_pOpenLock->Unlock();
				return NULL;
			}
		}

		/* The data folder exists - create each subdir as needed! */
		char *cur_ptr = path;

		do
		{
			/* Find the next suitable path */
			char *next_ptr = cur_ptr;
			while (*next_ptr != '\0')
			{
				if (IsPathSepChar(*next_ptr))
				{
					*next_ptr = '\0';
					next_ptr++;
					break;
				}
				next_ptr++;
			}
			if (*next_ptr == '\0')
			{
				next_ptr = NULL;
			}
			len += libsys->PathFormat(&fullpath[len], sizeof(fullpath)-len, "/%s", cur_ptr);
			if (!libsys->IsPathDirectory(fullpath) && !libsys->CreateFolder(fullpath))
			{
				break;
			}
			cur_ptr = next_ptr;
		} while (cur_ptr);
	}

	/* Build the FINAL path. */
	g_pSM->BuildPath(Path_SM, fullpath, sizeof(fullpath), "data/sqlite/%s.sq3", info->database);

	/* If we're requesting a persistent connection, see if something is already open */
	if (persistent)
	{
		/* See if anything in the cache matches */
		List<SqDbInfo>::iterator iter;
		for (iter = m_Cache.begin(); iter != m_Cache.end(); iter++)
		{
			if ((*iter).path.compare(fullpath) == 0)
			{
				(*iter).db->IncReferenceCount();
				m_pOpenLock->Unlock();
				return (*iter).db;
			}
		}
	}

	/* Try to open a new connection */
	sqlite3 *sql;
	int err = sqlite3_open(fullpath, &sql);
	if (err != SQLITE_OK)
	{
		strncopy(error, sqlite3_errmsg(sql), maxlength);
		sqlite3_close(sql);
		m_pOpenLock->Unlock();
		return NULL;
	}

	SqDatabase *pdb = new SqDatabase(sql, persistent);

	if (persistent)
	{
		SqDbInfo pinfo;
		pinfo.path = fullpath;
		pinfo.db = pdb;
		m_Cache.push_back(pinfo);
	}

	m_pOpenLock->Unlock();

	return pdb;
}

void SqDriver::RemovePersistent(IDatabase *pdb)
{
	List<SqDbInfo>::iterator iter;
	for (iter = m_Cache.begin(); iter != m_Cache.end(); iter++)
	{
		if ((*iter).db == pdb)
		{
			iter = m_Cache.erase(iter);
			return;
		}
	}
}
