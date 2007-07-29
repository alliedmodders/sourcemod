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

#ifndef _INCLUDE_SQLITE_SOURCEMOD_DRIVER_H_
#define _INCLUDE_SQLITE_SOURCEMOD_DRIVER_H_

#include <IDBDriver.h>
#include <IThreader.h>
#include <sh_list.h>
#include <sh_string.h>
#include "sqlite-source/sqlite3.h"

using namespace SourceMod;
using namespace SourceHook;

struct SqDbInfo
{
	String path;
	IDatabase *db;
};

/**
 * Tee-hee.. sounds like "screw driver," except maybe if
 * Elmer Fudd was saying it.
 */
class SqDriver : public IDBDriver
{
public:
	SqDriver();
	void Initialize();
	void Shutdown();
public:
	IDatabase *Connect(const DatabaseInfo *info, bool persistent, char *error, size_t maxlength);
	const char *GetIdentifier();
	const char *GetProductName();
	Handle_t GetHandle();
	IdentityToken_t *GetIdentity();
	bool IsThreadSafe();
	bool InitializeThreadSafety();
	void ShutdownThreadSafety();
public:
	void RemovePersistent(IDatabase *pdb);
private:
	Handle_t m_Handle;
	IMutex *m_pOpenLock;
	List<SqDbInfo> m_Cache;
};

extern SqDriver g_SqDriver;

unsigned int strncopy(char *dest, const char *src, size_t count);

#endif //_INCLUDE_SQLITE_SOURCEMOD_DRIVER_H_
