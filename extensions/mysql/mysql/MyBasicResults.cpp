/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod MySQL Extension
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
#include "MyBasicResults.h"

MyBasicResults::MyBasicResults(MYSQL_RES *res)
: m_pRes(res)
{
	Update();
}

MyBasicResults::~MyBasicResults()
{
}

void MyBasicResults::Update()
{
	if (m_pRes)
	{
		m_ColCount = (unsigned int)mysql_num_fields(m_pRes);
		m_RowCount = (unsigned int)mysql_num_rows(m_pRes);
		m_CurRow = 0;
		m_Row = NULL;
	}
}

unsigned int MyBasicResults::GetRowCount()
{
	return m_RowCount;
}

unsigned int MyBasicResults::GetFieldCount()
{
	return m_ColCount;
}

bool MyBasicResults::FieldNameToNum(const char *name, unsigned int *columnId)
{
	unsigned int total = GetFieldCount();

	for (unsigned int i=0; i<total; i++)
	{
		if (strcmp(FieldNumToName(i), name) == 0)
		{
			*columnId = i;
			return true;
		}
	}

	return false;
}

const char *MyBasicResults::FieldNumToName(unsigned int colId)
{
	if (colId >= GetFieldCount())
	{
		return NULL;
	}

	MYSQL_FIELD *field = mysql_fetch_field_direct(m_pRes, colId);
	return field ? (field->name ? field->name : "") : "";
}

bool MyBasicResults::MoreRows()
{
	return (m_CurRow < m_RowCount);
}

IResultRow *MyBasicResults::FetchRow()
{
	if (m_CurRow >= m_RowCount)
	{
		/* Put us one after so we know to block CurrentRow() */
		m_CurRow = m_RowCount + 1;
		return NULL;
	}
	m_Row = mysql_fetch_row(m_pRes);
	m_Lengths = mysql_fetch_lengths(m_pRes);
	m_CurRow++;
	return this;
}

IResultRow *MyBasicResults::CurrentRow()
{
	if (!m_pRes
		|| !m_CurRow
		|| m_CurRow > m_RowCount)
	{
		return NULL;
	}
	
	return this;
}

bool MyBasicResults::Rewind()
{
	mysql_data_seek(m_pRes, 0);
	m_CurRow = 0;
	return true;
}

DBType MyBasicResults::GetFieldType(unsigned int field)
{
	if (field >= m_ColCount)
	{
		return DBType_Unknown;
	}

	MYSQL_FIELD *fld = mysql_fetch_field_direct(m_pRes, field);
	if (!fld)
	{
		return DBType_Unknown;
	}

	return GetOurType(fld->type);
}

DBType MyBasicResults::GetFieldDataType(unsigned int field)
{
	DBType type = GetFieldType(field);
	if (type == DBType_Blob)
	{
		return DBType_Blob;
	} else {
		return DBType_String;
	}
}

bool MyBasicResults::IsNull(unsigned int columnId)
{
	if (columnId >= m_ColCount)
	{
		return true;
	}

	return (m_Row[columnId] == NULL);
}

DBResult MyBasicResults::GetString(unsigned int columnId, const char **pString, size_t *length)
{
	if (columnId >= m_ColCount)
	{
		return DBVal_Error;
	} else if (m_Row[columnId] == NULL) {
		*pString = "";
		if (length)
		{
			*length = 0;
		}
		return DBVal_Null;
	}

	*pString = m_Row[columnId];

	if (length)
	{
		*length = (size_t)m_Lengths[columnId];
	}

	return DBVal_Data;
}

DBResult MyBasicResults::CopyString(unsigned int columnId, 
									char *buffer, 
									size_t maxlength, 
									size_t *written)
{
	DBResult res;
	const char *str;
	if ((res=GetString(columnId, &str, NULL)) == DBVal_Error)
	{
		return DBVal_Error;
	}

	size_t wr = strncopy(buffer, str, maxlength);
	if (written)
	{
		*written = wr;
	}

	return res;
}

size_t MyBasicResults::GetDataSize(unsigned int columnId)
{
	if (columnId >= m_ColCount)
	{
		return 0;
	}

	return (size_t)m_Lengths[columnId];
}

DBResult MyBasicResults::GetFloat(unsigned int col, float *fval)
{
	if (col >= m_ColCount)
	{
		return DBVal_Error;
	} else if (m_Row[col] == NULL) {
		*fval = 0.0f;
		return DBVal_Null;
	}

	*fval = (float)atof(m_Row[col]);

	return DBVal_Data;
}

DBResult MyBasicResults::GetInt(unsigned int col, int *val)
{
	if (col >= m_ColCount)
	{
		return DBVal_Error;
	} else if (m_Row[col] == NULL) {
		*val = 0;
		return DBVal_Null;
	}

	*val = atoi(m_Row[col]);

	return DBVal_Data;
}

DBResult MyBasicResults::GetBlob(unsigned int col, const void **pData, size_t *length)
{
	if (col >= m_ColCount)
	{
		return DBVal_Error;
	} else if (m_Row[col] == NULL) {
		*pData = NULL;
		if (length)
		{
			*length = 0;
		}
		return DBVal_Null;
	}

	*pData = m_Row[col];

	if (length)
	{
		*length = (size_t)m_Lengths[col];
	}

	return DBVal_Data;
}

DBResult MyBasicResults::CopyBlob(unsigned int columnId, void *buffer, size_t maxlength, size_t *written)
{
	const void *addr;
	size_t length;
	DBResult res;

	if ((res=GetBlob(columnId, &addr, &length)) == DBVal_Error)
	{
		return DBVal_Error;
	}

	if (addr == NULL)
	{
		return DBVal_Null;
	}

	if (length > maxlength)
	{
		length = maxlength;
	}

	memcpy(buffer, addr, length);
	if (written)
	{
		*written = length;
	}

	return res;
}

MyQuery::MyQuery(MyDatabase *db, MYSQL_RES *res)
: m_pParent(db), m_rs(res)
{
	m_InsertID = m_pParent->GetInsertID();
	m_AffectedRows = m_pParent->GetAffectedRows();
}

IResultSet *MyQuery::GetResultSet()
{
	if (m_rs.m_pRes == NULL)
	{
		return NULL;
	}

	return &m_rs;
}

unsigned int MyQuery::GetInsertID()
{
	return m_InsertID;
}

unsigned int MyQuery::GetAffectedRows()
{
	return m_AffectedRows;
}

bool MyQuery::FetchMoreResults()
{
	if (m_rs.m_pRes == NULL)
	{
		return false;
	} else if (!mysql_more_results(m_pParent->m_mysql)) {
		return false;
	}

	mysql_free_result(m_rs.m_pRes);
	m_rs.m_pRes = NULL;

	if (mysql_next_result(m_pParent->m_mysql) != 0)
	{
		return false;
	}

	m_rs.m_pRes = mysql_store_result(m_pParent->m_mysql);
	m_rs.Update();

	return (m_rs.m_pRes != NULL);
}

void MyQuery::Destroy()
{
	/* :TODO: All this rot should be moved into the destructor,
	 * and the Update() function needs to not be so stupid.
	 */

	while (FetchMoreResults())
	{
		/* Spin until all are gone */
	}

	/* Free the last, if any */
	if (m_rs.m_pRes != NULL)
	{
		mysql_free_result(m_rs.m_pRes);
	}

	/* Self destruct */
	delete this;
}
