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

#include <stdlib.h>
#include "extension.h"
#include "SqResults.h"
#include "SqQuery.h"

SqResults::SqResults(SqQuery *query) : 
	m_pStmt(query->GetStmt()), m_Strings(1024),
	m_RowCount(0), m_MaxRows(0), m_Rows(NULL),
	m_CurRow(-1), m_NextRow(0)
{
	m_ColCount = sqlite3_column_count(m_pStmt);
	if (m_ColCount)
	{
		m_ColNames = new String[m_ColCount];
		for (unsigned int i=0; i<m_ColCount; i++)
		{
			m_ColNames[i].assign(sqlite3_column_name(m_pStmt, i));
		}
	} else {
		m_ColNames = NULL;
	}

	m_pMemory = m_Strings.GetMemTable();
}

SqResults::~SqResults()
{
	delete [] m_ColNames;
	free(m_Rows);
}

unsigned int SqResults::GetRowCount()
{
	return m_RowCount;
}

unsigned int SqResults::GetFieldCount()
{
	return m_ColCount;
}

const char *SqResults::FieldNumToName(unsigned int columnId)
{
	if (columnId >= m_ColCount)
	{
		return NULL;
	}

	return m_ColNames[columnId].c_str();
}

bool SqResults::FieldNameToNum(const char *name, unsigned int *columnId)
{
	for (unsigned int i=0; i<m_ColCount; i++)
	{
		if (m_ColNames[i].compare(name) == 0)
		{
			if (columnId)
			{
				*columnId = i;
			}
			return true;
		}
	}
	return false;
}

void SqResults::ResetResultCount()
{
	m_RowCount = 0;
	m_CurRow = -1;
	m_NextRow = 0;
	m_pMemory->Reset();
}

void SqResults::PushResult()
{
	/* First make sure we can fit one more row */
	if (m_RowCount + 1 > m_MaxRows)
	{
		/* Create a new array */
		if (!m_Rows)
		{
			m_MaxRows = 8;
			m_Rows = (SqField *)malloc(sizeof(SqField) * m_ColCount * m_MaxRows);
		} else {
			m_MaxRows *= 2;
			m_Rows = (SqField *)realloc(m_Rows, sizeof(SqField) * m_ColCount * m_MaxRows);
		}
	}

	SqField *row = &m_Rows[m_RowCount * m_ColCount];
	for (unsigned int i=0; i<m_ColCount; i++)
	{
		row[i].type = sqlite3_column_type(m_pStmt, i);
		if (row[i].type == SQLITE_INTEGER)
		{
			row[i].u.idx = sqlite3_column_int(m_pStmt, i);
			row[i].size = sizeof(int);
		} else if (row[i].type == SQLITE_FLOAT) {
			row[i].u.f = (float)sqlite3_column_double(m_pStmt, i);
			row[i].size = sizeof(float);
		} else if (row[i].type == SQLITE_BLOB) {
			int bytes = sqlite3_column_bytes(m_pStmt, i);
			const void *pOrig;
			if ((pOrig = sqlite3_column_blob(m_pStmt, i)) != NULL)
			{
				void *pAddr;
				row[i].u.idx = m_pMemory->CreateMem(bytes, &pAddr);
				memcpy(pAddr, pOrig, bytes);
			} else {
				row[i].u.idx = -1;
			}
			row[i].size = sqlite3_column_bytes(m_pStmt, i);
		} else if (row[i].type == SQLITE_TEXT) {
			const char *str = (const char *)sqlite3_column_text(m_pStmt, i);
			if (str)
			{
				row[i].u.idx = m_Strings.AddString(str);
			} else {
				row[i].u.idx = -1;
			}
			row[i].size = sqlite3_column_bytes(m_pStmt, i);
		} else {
			row[i].size = 0;
		}
	}

	/* Finally, increase the row count */
	m_RowCount++;
}

bool SqResults::MoreRows()
{
	return (m_CurRow < 0) ? (m_RowCount > 0) : (m_CurRow < m_RowCount);
}

IResultRow *SqResults::FetchRow()
{
	m_CurRow = m_NextRow;
	if (m_CurRow >= m_RowCount)
	{
		return NULL;
	}
	m_NextRow++;
	return this;
}

IResultRow *SqResults::CurrentRow()
{
	if (!m_RowCount || m_CurRow < 0 || m_CurRow >= m_RowCount)
	{
		return NULL;
	}
	return this;
}

bool SqResults::Rewind()
{
	m_CurRow = -1;
	m_NextRow = 0;
	return true;
}

SqField *SqResults::GetField(unsigned int col)
{
	if (m_CurRow < 0 || m_CurRow >= m_RowCount || col >= m_ColCount)
	{
		return NULL;
	}

	return &m_Rows[(m_CurRow * m_ColCount) + col];
}

DBType SqResults::GetFieldType(unsigned int field)
{
	/* Leaving unimplemented... */
	return DBType_Unknown;
}

DBType SqResults::GetFieldDataType(unsigned int field)
{
	/* Leaving unimplemented... */
	return DBType_Unknown;
}

DBResult SqResults::GetString(unsigned int columnId, const char **pString, size_t *_length)
{
	SqField *field = GetField(columnId);
	if (!field)
	{
		return DBVal_Error;
	}

	DBResult res = DBVal_Data;
	const char *ptr = NULL;
	size_t length = 0;
	if (field->type == SQLITE_TEXT || field->type == SQLITE_BLOB)
	{
		ptr = m_Strings.GetString(field->u.idx);
		length = field->size;
	} else if (field->type == SQLITE_INTEGER) {
		char number[24];
		field->size = UTIL_Format(number, sizeof(number), "%d", field->u.idx);
		field->type = SQLITE_TEXT;
		field->u.idx = m_Strings.AddString(number);
		ptr = m_Strings.GetString(field->u.idx);
		length = field->size;
	} else if (field->type == SQLITE_FLOAT) {
		char number[24];
		field->size = UTIL_Format(number, sizeof(number), "%f", field->u.f);
		field->type = SQLITE_TEXT;
		field->u.idx = m_Strings.AddString(number);
		ptr = m_Strings.GetString(field->u.idx);
		length = field->size;
	} else if (field->type == SQLITE_NULL) {
		res = DBVal_Null;
	}

	if (!ptr)
	{
		ptr = "";
	}

	if (pString)
	{
		*pString = ptr;
	}
	if (_length)
	{
		*_length = length;
	}

	return res;
}

DBResult SqResults::CopyString(unsigned int columnId, char *buffer, size_t maxlength, size_t *written)
{
	SqField *field = GetField(columnId);
	if (!field)
	{
		return DBVal_Error;
	}

	DBResult res = DBVal_Data;
	if (field->type == SQLITE_TEXT || field->type == SQLITE_BLOB)
	{
		const char *ptr = m_Strings.GetString(field->u.idx);
		if (!ptr)
		{
			ptr = "";
			field->type = SQLITE_TEXT;
			res = DBVal_Null;
		}
		size_t wr = 0;
		if (field->type == SQLITE_TEXT)
		{
			wr = strncopy(buffer, ptr, maxlength);
		} else if (field->type == SQLITE_BLOB) {
			wr = (maxlength < field->size) ? maxlength : field->size;
			memcpy(buffer, ptr, wr);
		}
		if (written)
		{
			*written = wr;
		}
		return res;
	} else if (field->type == SQLITE_INTEGER) {
		size_t wr = 0;
		if (buffer)
		{
			wr = UTIL_Format(buffer, maxlength, "%d", field->u.idx);
		}
		if (written)
		{
			*written = wr;
		}
		return DBVal_Data;
	} else if (field->type == SQLITE_FLOAT) {
		size_t wr = 0;
		if (buffer)
		{
			wr = UTIL_Format(buffer, maxlength, "%f", field->u.f);
		}
		if (written)
		{
			*written = wr;
		}
		return DBVal_Data;
	}

	if (buffer)
	{
		strncopy(buffer, "", maxlength);
	}
	if (written)
	{
		*written = 0;
	}

	return DBVal_Null;
}

bool SqResults::IsNull(unsigned int columnId)
{
	SqField *field = GetField(columnId);
	if (!field)
	{
		return true;
	}

	return (field->type == SQLITE_NULL);
}

size_t SqResults::GetDataSize(unsigned int columnId)
{
	SqField *field = GetField(columnId);
	if (!field)
	{
		return 0;
	}

	return field->size;
}

DBResult SqResults::GetFloat(unsigned int columnId, float *pFloat)
{
	SqField *field = GetField(columnId);
	if (!field)
	{
		return DBVal_Error;
	} else if (field->type == SQLITE_BLOB) {
		return DBVal_Error;
	}

	float fVal = 0.0f;
	if (field->type == SQLITE_FLOAT)
	{
		fVal = field->u.f;
	} else if (field->type == SQLITE_TEXT) {
		const char *ptr = m_Strings.GetString(field->u.idx);
		if (ptr)
		{
			fVal = (float)atof(ptr);
		}
	} else if (field->type == SQLITE_INTEGER) {
		fVal = (float)field->u.idx;
	}

	if (pFloat)
	{
		*pFloat = fVal;
	}

	return (field->type == SQLITE_NULL) ? DBVal_Null : DBVal_Data;
}

DBResult SqResults::GetInt(unsigned int columnId, int *pInt)
{
	SqField *field = GetField(columnId);
	if (!field)
	{
		return DBVal_Error;
	} else if (field->type == SQLITE_BLOB) {
		return DBVal_Error;
	}

	int val = 0;
	if (field->type == SQLITE_INTEGER)
	{
		val = field->u.idx;
	} else if (field->type == SQLITE_TEXT) {
		const char *ptr = m_Strings.GetString(field->u.idx);
		if (ptr)
		{
			val = atoi(ptr);
		}
	} else if (field->type == SQLITE_FLOAT) {
		val = (int)field->u.f;
	}

	if (pInt)
	{
		*pInt = val;
	}

	return (field->type == SQLITE_NULL) ? DBVal_Null : DBVal_Data;
}

DBResult SqResults::GetBlob(unsigned int columnId, const void **pData, size_t *length)
{
	SqField *field = GetField(columnId);
	if (!field)
	{
		return DBVal_Error;
	} 

	void *addr = NULL;

	if (field->type == SQLITE_TEXT || field->type == SQLITE_BLOB)
	{
		addr = m_pMemory->GetAddress(field->u.idx);
	} else if (field->type == SQLITE_FLOAT || field->type == SQLITE_INTEGER) {
		addr = &(field->u);
	}

	if (pData)
	{
		*pData = addr;
	}

	if (length)
	{
		*length = field->size;
	}

	return (field->type == SQLITE_NULL) ? DBVal_Null : DBVal_Data;
}

DBResult SqResults::CopyBlob(unsigned int columnId, void *buffer, size_t maxlength, size_t *written)
{
	SqField *field = GetField(columnId);
	if (!field)
	{
		return DBVal_Error;
	} 

	void *addr = NULL;

	if (field->type == SQLITE_TEXT || field->type == SQLITE_BLOB)
	{
		addr = m_pMemory->GetAddress(field->u.idx);
	} else if (field->type == SQLITE_FLOAT || field->type == SQLITE_INTEGER) {
		addr = &(field->u);
	}

	size_t toCopy = field->size > maxlength ? maxlength : field->size;
	if (buffer && addr && toCopy)
	{
		memcpy(buffer, addr, toCopy);
	} else {
		toCopy = 0;
	}

	if (written)
	{
		*written = toCopy;
	}

	return (field->type == SQLITE_NULL) ? DBVal_Null : DBVal_Data;
}
