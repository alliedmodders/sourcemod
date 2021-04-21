/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod SQLite Extension
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

#include <sm_platform.h>
#include "extension.h"
#include "SqDriver.h"
#include "SqDatabase.h"
#include <am-string.h>

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

int busy_handler(void *unused1, int unused2)
{
#if defined PLATFORM_WINDOWS
	Sleep(100);
#elif defined PLATFORM_POSIX
	usleep(100000);
#endif
	return 1;
}

SqDriver::SqDriver()
{
	m_Handle = BAD_HANDLE;
	m_bThreadSafe = false;
	m_bShutdown = false;
}

// The extension got unloaded. Remove any open database now to avoid a use-after-free
// of g_SqDriver in SqDatabase's destructor.
SqDriver::~SqDriver()
{
	std::lock_guard<std::mutex> lock(m_OpenLock);

	List<SqDbInfo>::iterator iter;
	SqDatabase *sqdb;
	for (iter = m_Cache.begin(); iter != m_Cache.end(); iter++)
	{
		// Don't let SqDatabase try to remove itself from m_Cache
		// now that we're gone.
		sqdb = (SqDatabase *)(*iter).db;
		sqdb->PrepareForForcedShutdown();

		iter = m_Cache.erase(iter);
	}
	
	if (!m_bShutdown)
	{
		dbi->RemoveDriver(&g_SqDriver);
		Shutdown();
	}
}

void SqDriver::Initialize()
{
	InitializeThreadSafety();
}

void SqDriver::Shutdown()
{
	m_bShutdown = true;
	if (m_bThreadSafe)
	{
		sqlite3_enable_shared_cache(0);
	}
}

bool SqDriver::IsThreadSafe()
{
	return true;
}

bool SqDriver::InitializeThreadSafety()
{
	if (m_bThreadSafe)
	{
		return true;
	}

	if (sqlite3_threadsafe() == 0)
	{
		return false;
	}

	if (sqlite3_enable_shared_cache(1) != SQLITE_OK)
	{
		return false;
	}

	m_bThreadSafe = true;

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
#elif defined PLATFORM_LINUX || defined PLATFORM_APPLE
	return (c == '/');
#endif
}

IDatabase *SqDriver::Connect(const DatabaseInfo *info, bool persistent, char *error, size_t maxlength)
{
	std::lock_guard<std::mutex> lock(m_OpenLock);
	
	/* Full path to the database file */
	char fullpath[PLATFORM_MAX_PATH];

	if (strcmp(info->database, ":memory:") == 0 || strncmp(info->database, "file:", 5) == 0)
	{
		ke::SafeStrcpy(fullpath, sizeof(fullpath), info->database);
	}
	else
	{
		/* Format our path */
		char path[PLATFORM_MAX_PATH];
		size_t len = libsys->PathFormat(path, sizeof(path), "sqlite/%s", info->database);

		/* Chop any filename off */
		for (size_t i = len-1;
			 i <= len-1;
			 i--)
		{
			if (IsPathSepChar(path[i]))
			{
				path[i] = '\0';
				break;
			}
		}

		/* Test the full path */
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
	}

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
		return NULL;
	}

	sqlite3_busy_handler(sql, busy_handler, NULL);

	SqDatabase *pdb = new SqDatabase(sql, persistent);

	if (persistent)
	{
		SqDbInfo pinfo;
		pinfo.path = fullpath;
		pinfo.db = pdb;
		m_Cache.push_back(pinfo);
	}

	return pdb;
}

void SqDriver::RemovePersistent(IDatabase *pdb)
{
	std::lock_guard<std::mutex> lock(m_OpenLock);

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
