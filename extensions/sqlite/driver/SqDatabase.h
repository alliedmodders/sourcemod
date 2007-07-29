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

#ifndef _INCLUDE_SQLITE_SOURCEMOD_DATABASE_H_
#define _INCLUDE_SQLITE_SOURCEMOD_DATABASE_H_

#include <IThreader.h>
#include "SqDriver.h"

class SqDatabase : public IDatabase
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
	bool QuoteString(const char *str, char buffer[], size_t maxlen, size_t *newSize);
	unsigned int GetAffectedRows();
	unsigned int GetInsertID();
	bool LockForFullAtomicOperation();
	void UnlockFromFullAtomicOperation();
	void IncReferenceCount();
	IDBDriver *GetDriver();
public:
	sqlite3 *GetDb();
private:
	sqlite3 *m_sq3;
	unsigned int m_refcount;
	IMutex *m_pFullLock;
	IMutex *m_pRefLock;
	bool m_Persistent;
	String m_LastError;
	int m_LastErrorCode;
};

#endif //_INCLUDE_SQLITE_SOURCEMOD_DATABASE_H_
