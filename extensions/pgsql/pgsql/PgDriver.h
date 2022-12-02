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

#ifndef _INCLUDE_SM_PGSQL_DRIVER_H_
#define _INCLUDE_SM_PGSQL_DRIVER_H_

#define SOURCEMOD_SQL_DRIVER_CODE
#include <IDBDriver.h>
#include <sm_platform.h>
#if defined PLATFORM_WINDOWS
#include <winsock.h>
#endif

#include <libpq-fe.h>

#include <sh_string.h>
#include <sh_list.h>

#include <mutex>

using namespace SourceMod;
using namespace SourceHook;

#define M_CLIENT_MULTI_RESULTS    ((1) << 17)  /* Enable/disable multi-results */

class PgDatabase;

class PgDriver : public IDBDriver
{
public:
	PgDriver();
public: //IDBDriver
	IDatabase *Connect(const DatabaseInfo *info, bool persistent, char *error, size_t maxlength);
	const char *GetIdentifier();
	const char *GetProductName();
	Handle_t GetHandle();
	IdentityToken_t *GetIdentity();
	bool IsThreadSafe();
	bool InitializeThreadSafety();
	void ShutdownThreadSafety();
public:
	void Shutdown();
	void RemoveFromList(PgDatabase *pdb, bool persistent);
private:
	std::mutex m_Lock;
	Handle_t m_Handle;
	List<PgDatabase *> m_PermDbs;
};

extern PgDriver g_PgDriver;

unsigned int strncopy(char *dest, const char *src, size_t count);

#endif //_INCLUDE_SM_PGSQL_DRIVER_H_
