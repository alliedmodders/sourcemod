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
#include <stdlib.h>
#include "PgBasicResults.h"

PgBasicResults::PgBasicResults(PGresult *res)
: m_pRes(res)
{
	Update();
}

PgBasicResults::~PgBasicResults()
{
}

void PgBasicResults::Update()
{
	if (m_pRes)
	{
		m_ColCount = (unsigned int)PQnfields(m_pRes);
		m_RowCount = (unsigned int)PQntuples(m_pRes);
		m_CurRow = 0;
		m_RowFetched = false;
	}
}

unsigned int PgBasicResults::GetRowCount()
{
	return m_RowCount;
}

unsigned int PgBasicResults::GetFieldCount()
{
	return m_ColCount;
}

bool PgBasicResults::FieldNameToNum(const char *name, unsigned int *columnId)
{
	int id = PQfnumber(m_pRes, name);

	if (id == -1)
		return false;

	*columnId = (unsigned int)id;

	return true;
}

const char *PgBasicResults::FieldNumToName(unsigned int colId)
{
	if (colId >= m_ColCount)
		return nullptr;

	return PQfname(m_pRes, colId);
}

bool PgBasicResults::MoreRows()
{
	return ((!m_RowFetched && m_RowCount > 0) || (m_RowFetched && m_CurRow < m_RowCount-1));
}

IResultRow *PgBasicResults::FetchRow()
{
	// Rows start at 0. Start incrementing the current row when fetching the second time.
	if (m_RowFetched)
		m_CurRow++;

	// Reached the end of the rows.
	if (m_CurRow >= m_RowCount)
	{
		m_CurRow = m_RowCount;
		return nullptr;
	}

	// We fetched some rows.
	m_RowFetched = true;

	return this;
}

IResultRow *PgBasicResults::CurrentRow()
{
	if (!m_pRes
		|| !m_RowFetched
		|| m_CurRow >= m_RowCount)
	{
		return nullptr;
	}
	
	return this;
}

bool PgBasicResults::Rewind()
{
	m_RowFetched = false;
	m_CurRow = 0;
	return true;
}

DBType PgBasicResults::GetFieldType(unsigned int field)
{
	if (field >= m_ColCount)
	{
		return DBType_Unknown;
	}

	Oid fld = PQftype(m_pRes, field);
	if (fld == InvalidOid)
	{
		return DBType_Unknown;
	}

	return GetOurType(fld);
}

DBType PgBasicResults::GetFieldDataType(unsigned int field)
{
	if (field >= m_ColCount)
	{
		return DBType_Unknown;
	}

	if (PQfformat(m_pRes, field) == 1)
	{
		return DBType_Blob;
	} else {
		return DBType_String;
	}
}

bool PgBasicResults::IsNull(unsigned int columnId)
{
	if (columnId >= m_ColCount)
	{
		return true;
	}

	return (PQgetisnull(m_pRes, m_CurRow, columnId) == 1);
}

DBResult PgBasicResults::GetString(unsigned int columnId, const char **pString, size_t *length)
{
	if (columnId >= m_ColCount)
	{
		return DBVal_Error;
	} else if (IsNull(columnId)) {
		*pString = "";
		if (length)
		{
			*length = 0;
		}
		return DBVal_Null;
	}

	*pString = PQgetvalue(m_pRes, m_CurRow, columnId);

	if (length)
	{
		*length = GetDataSize(columnId);
	}

	return DBVal_Data;
}

DBResult PgBasicResults::CopyString(unsigned int columnId, 
									char *buffer, 
									size_t maxlength, 
									size_t *written)
{
	DBResult res;
	const char *str;
	if ((res=GetString(columnId, &str, nullptr)) == DBVal_Error)
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

size_t PgBasicResults::GetDataSize(unsigned int columnId)
{
	if (columnId >= m_ColCount)
	{
		return 0;
	}

	return (size_t)PQgetlength(m_pRes, m_CurRow, columnId);
}

DBResult PgBasicResults::GetFloat(unsigned int col, float *fval)
{
	if (col >= m_ColCount)
	{
		return DBVal_Error;
	} else if (IsNull(col)) {
		*fval = 0.0f;
		return DBVal_Null;
	}

	*fval = (float)atof(PQgetvalue(m_pRes, m_CurRow, col));

	return DBVal_Data;
}

DBResult PgBasicResults::GetInt(unsigned int col, int *val)
{
	if (col >= m_ColCount)
	{
		return DBVal_Error;
	} else if (IsNull(col)) {
		*val = 0;
		return DBVal_Null;
	}

	*val = atoi(PQgetvalue(m_pRes, m_CurRow, col));

	return DBVal_Data;
}

DBResult PgBasicResults::GetBlob(unsigned int col, const void **pData, size_t *length)
{
	if (col >= m_ColCount)
	{
		return DBVal_Error;
	} else if (IsNull(col)) {
		*pData = nullptr;
		if (length)
		{
			*length = 0;
		}
		return DBVal_Null;
	}

	*pData = PQgetvalue(m_pRes, m_CurRow, col);

	if (length)
	{
		*length = GetDataSize(col);
	}

	return DBVal_Data;
}

DBResult PgBasicResults::CopyBlob(unsigned int columnId, void *buffer, size_t maxlength, size_t *written)
{
	const void *addr;
	size_t length;
	DBResult res;

	if ((res=GetBlob(columnId, &addr, &length)) == DBVal_Error)
	{
		return DBVal_Error;
	}

	if (!addr)
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

PgQuery::PgQuery(PgDatabase *db, PGresult *res)
: m_pParent(db), m_rs(res)
{
	m_InsertID = (unsigned int)PQoidValue(res);
	m_AffectedRows = atoi(PQcmdTuples(res));
	m_pParent->SetLastIDAndRows(m_InsertID, m_AffectedRows);
}

IResultSet *PgQuery::GetResultSet()
{
	if (!m_rs.m_pRes)
	{
		return nullptr;
	}

	return &m_rs;
}

unsigned int PgQuery::GetInsertID()
{
	return m_InsertID;
}

unsigned int PgQuery::GetAffectedRows()
{
	return m_AffectedRows;
}

bool PgQuery::FetchMoreResults()
{
	return false;
}

void PgQuery::Destroy()
{
	if (m_rs.m_pRes != nullptr)
	{
		PQclear(m_rs.m_pRes);
	}

	/* Self destruct */
	delete this;
}
