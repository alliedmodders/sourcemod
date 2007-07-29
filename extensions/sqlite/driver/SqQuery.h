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

#ifndef _INCLUDE_SQLITE_SOURCEMOD_QUERY_H_
#define _INCLUDE_SQLITE_SOURCEMOD_QUERY_H_

#include "SqDatabase.h"
#include "SqResults.h"

class SqQuery : 
	public IPreparedQuery
{
public:
	SqQuery(SqDatabase *parent, sqlite3_stmt *stmt);
	~SqQuery();
public: //IQuery
	IResultSet *GetResultSet();
	bool FetchMoreResults();
	void Destroy();
public: //IPreparedQuery
	bool BindParamInt(unsigned int param, int num, bool signd=true);
	bool BindParamFloat(unsigned int param, float f);
	bool BindParamNull(unsigned int param);
	bool BindParamString(unsigned int param, const char *text, bool copy);
	bool BindParamBlob(unsigned int param, const void *data, size_t length, bool copy);
	bool Execute();
	const char *GetError(int *errCode=NULL);
	unsigned int GetAffectedRows();
	unsigned int GetInsertID();
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
	sqlite3_stmt *GetStmt();
private:
	SqDatabase *m_pParent;
	sqlite3_stmt *m_pStmt;
	SqResults *m_pResults;
	unsigned int m_ParamCount;
	String m_LastError;
	int m_LastErrorCode;
	unsigned int m_AffectedRows;
	unsigned int m_InsertID;
	unsigned int m_ColCount;
};

#endif //_INCLUDE_SQLITE_SOURCEMOD_QUERY_H_
