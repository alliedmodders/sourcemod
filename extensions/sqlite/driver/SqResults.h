/**
 * vim: set ts=4 :
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

#ifndef _INCLUDE_SQLITE_SOURCEMOD_RESULT_SET_H_
#define _INCLUDE_SQLITE_SOURCEMOD_RESULT_SET_H_

#include "SqDriver.h"
#include "sm_memtable.h"

class SqQuery;

struct SqField
{
	int type;
	union
	{
		int idx;
		float f;
	} u;
	size_t size;
};

class SqResults : 
	public IResultSet,
	public IResultRow
{
	friend class SqQuery;
public:
	SqResults(SqQuery *query);
	~SqResults();
public: //IResultSet
	unsigned int GetRowCount();
	unsigned int GetFieldCount();
	const char *FieldNumToName(unsigned int columnId);
	bool FieldNameToNum(const char *name, unsigned int *columnId);
	bool MoreRows();
	IResultRow *FetchRow();
	IResultRow *CurrentRow();
	bool Rewind();
	DBType GetFieldType(unsigned int field);
	DBType GetFieldDataType(unsigned int field);
public: //IResultRow
	DBResult GetString(unsigned int columnId, const char **pString, size_t *length);
	DBResult CopyString(unsigned int columnId, 
		char *buffer, 
		size_t maxlength, 
		size_t *written);
	DBResult GetFloat(unsigned int columnId, float *pFloat);
	DBResult GetInt(unsigned int columnId, int *pInt);
	bool IsNull(unsigned int columnId);
	size_t GetDataSize(unsigned int columnId);
	DBResult GetBlob(unsigned int columnId, const void **pData, size_t *length);
	DBResult CopyBlob(unsigned int columnId, void *buffer, size_t maxlength, size_t *written);
public:
	void ResetResultCount();
	void PushResult();
private:
	SqField *GetField(unsigned int col);
private:
	sqlite3_stmt *m_pStmt;		/** DOES NOT CHANGE */
	String *m_ColNames;			/** DOES NOT CHANGE */
	unsigned int m_ColCount;	/** DOES NOT CHANGE */
	BaseStringTable m_Strings;	/** DOES NOT CHANGE */
	BaseMemTable *m_pMemory;	/** DOES NOT CHANGE */
	int m_RowCount;
	int m_MaxRows;
	SqField *m_Rows;
	int m_CurRow;
	int m_NextRow;
};

#endif //_INCLUDE_SQLITE_SOURCEMOD_RESULT_SET_H_
