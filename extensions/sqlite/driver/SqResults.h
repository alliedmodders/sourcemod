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
	unsigned int m_RowCount;
	unsigned int m_MaxRows;
	BaseMemTable *m_pMemory;	/** DOES NOT CHANGE */
	BaseStringTable m_Strings;	/** DOES NOT CHANGE */
	SqField *m_Rows;
	unsigned int m_CurRow;
	unsigned int m_NextRow;
};

#endif //_INCLUDE_SQLITE_SOURCEMOD_RESULT_SET_H_
