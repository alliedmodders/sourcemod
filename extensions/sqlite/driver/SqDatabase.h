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

#ifndef _INCLUDE_SQLITE_SOURCEMOD_DATABASE_H_
#define _INCLUDE_SQLITE_SOURCEMOD_DATABASE_H_

#include <am-refcounting-threadsafe.h>
#include <mutex>
#include "SqDriver.h"

class SqDatabase
	: public IDatabase,
	  public ke::RefcountedThreadsafe<SqDatabase>
{
public:
	SqDatabase(sqlite3 *sq3, bool persistent);
	~SqDatabase();
public:
	bool Close();
	const char *GetError(int *errorCode=NULL);
	bool DoSimpleQuery(const char *query);
	IQuery *DoQuery(const char *query);
	IPreparedQuery *PrepareQuery(const char *query, char *error, size_t maxlength, int *errCode=NULL);
	IPreparedQuery *PrepareQueryEx(const char *query, size_t len, char *error, size_t maxlength, int *errCode=NULL);
	bool QuoteString(const char *str, char buffer[], size_t maxlen, size_t *newSize);
	unsigned int GetAffectedRows();
	unsigned int GetInsertID();
	bool LockForFullAtomicOperation();
	void UnlockFromFullAtomicOperation();
	void IncReferenceCount();
	IDBDriver *GetDriver();
	bool DoSimpleQueryEx(const char *query, size_t len);
	IQuery *DoQueryEx(const char *query, size_t len);
	unsigned int GetAffectedRowsForQuery(IQuery *query);
	unsigned int GetInsertIDForQuery(IQuery *query);
	bool SetCharacterSet(const char *characterset);
public:
	sqlite3 *GetDb();
	void PrepareForForcedShutdown()
	{
		m_Persistent = false;
	}
private:
	sqlite3 *m_sq3;
	std::mutex m_FullLock;
	bool m_Persistent;
	String m_LastError;
	int m_LastErrorCode;
};

#endif //_INCLUDE_SQLITE_SOURCEMOD_DATABASE_H_
