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

#ifndef _INCLUDE_SQLITE_SOURCEMOD_DRIVER_H_
#define _INCLUDE_SQLITE_SOURCEMOD_DRIVER_H_

#define SOURCEMOD_SQL_DRIVER_CODE
#include <IDBDriver.h>
#include <sh_list.h>
#include <sh_string.h>
#include <mutex>
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
	~SqDriver();
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
	std::mutex m_OpenLock;
	List<SqDbInfo> m_Cache;
	bool m_bThreadSafe;
	bool m_bShutdown;
};

extern SqDriver g_SqDriver;

unsigned int strncopy(char *dest, const char *src, size_t count);

#endif //_INCLUDE_SQLITE_SOURCEMOD_DRIVER_H_
