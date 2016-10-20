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

#ifndef _INCLUDE_SM_PGSQL_BASIC_RESULTS_H_
#define _INCLUDE_SM_PGSQL_BASIC_RESULTS_H_

#include <amtl/am-refcounting.h>
#include "PgDatabase.h"

class PgQuery;
class PgStatement;

class PgBasicResults : 
	public IResultSet,
	public IResultRow
{
	friend class PgQuery;
	friend class PgStatement;
public:
	PgBasicResults(PGresult *res);
	~PgBasicResults();
public: //IResultSet
	unsigned int GetRowCount();
	unsigned int GetFieldCount();
	const char *FieldNumToName(unsigned int columnId);
	bool FieldNameToNum(const char *name, unsigned int *columnId);
	bool MoreRows();
	IResultRow *FetchRow();
	bool Rewind();
	DBType GetFieldType(unsigned int field);
	DBType GetFieldDataType(unsigned int field);
	IResultRow *CurrentRow();
public: //IResultRow
	DBResult GetString(unsigned int columnId, const char **pString, size_t *length);
	DBResult GetFloat(unsigned int columnId, float *pFloat);
	DBResult GetInt(unsigned int columnId, int *pInt);
	bool IsNull(unsigned int columnId);
	DBResult GetBlob(unsigned int columnId, const void **pData, size_t *length);
	DBResult CopyBlob(unsigned int columnId, void *buffer, size_t maxlength, size_t *written);
	DBResult CopyString(unsigned int columnId, 
						char *buffer, 
						size_t maxlength, 
						size_t *written);
	size_t GetDataSize(unsigned int columnId);
protected:
	void Update();
private:
	PGresult *m_pRes;
	bool m_RowFetched;
	unsigned int m_CurRow;
	unsigned int m_ColCount;
	unsigned int m_RowCount;
};

class PgQuery : public IQuery
{
	friend class PgBasicResults;
public:
	PgQuery(PgDatabase *db, PGresult *res);
public:
	IResultSet *GetResultSet();
	bool FetchMoreResults();
	void Destroy();
public: // Used by the driver to implement GetInsertIDForQuery()/GetAffectedRowsForQuery()
	unsigned int GetInsertID();
	unsigned int GetAffectedRows();
private:
	ke::RefPtr<PgDatabase> m_pParent;
	PgBasicResults m_rs;
	unsigned int m_InsertID;
	unsigned int m_AffectedRows;
};

#endif //_INCLUDE_SM_PGSQL_BASIC_RESULTS_H_
