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

#include "SqQuery.h"

SqQuery::SqQuery(SqDatabase *parent, sqlite3_stmt *stmt) : 
 m_pParent(parent), m_pStmt(stmt), m_pResults(NULL), m_AffectedRows(0), m_InsertID(0)
{
	m_ParamCount = sqlite3_bind_parameter_count(m_pStmt);
	m_ColCount = sqlite3_column_count(m_pStmt);
}

SqQuery::~SqQuery()
{
	delete m_pResults;
	sqlite3_finalize(m_pStmt);
}

IResultSet *SqQuery::GetResultSet()
{
	return m_pResults;
}

bool SqQuery::FetchMoreResults()
{
	/* We never have multiple result sets */
	return false;
}

void SqQuery::Destroy()
{
	delete this;
}

bool SqQuery::BindParamFloat(unsigned int param, float f)
{
	/* SQLite is 1 indexed */
	param++;

	if (param > m_ParamCount)
	{
		return false;
	}

	return (sqlite3_bind_double(m_pStmt, param, (double)f) == SQLITE_OK);
}

bool SqQuery::BindParamNull(unsigned int param)
{
	/* SQLite is 1 indexed */
	param++;

	if (param > m_ParamCount)
	{
		return false;
	}

	return (sqlite3_bind_null(m_pStmt, param) == SQLITE_OK);
}

bool SqQuery::BindParamString(unsigned int param, const char *text, bool copy)
{
	/* SQLite is 1 indexed */
	param++;

	if (param > m_ParamCount)
	{
		return false;
	}

	return (sqlite3_bind_text(m_pStmt, param, text, -1, copy ? SQLITE_TRANSIENT : SQLITE_STATIC) == SQLITE_OK);
}

bool SqQuery::BindParamInt(unsigned int param, int num, bool signd/* =true */)
{
	/* SQLite is 1 indexed */
	param++;

	if (param > m_ParamCount)
	{
		return false;
	}

	return (sqlite3_bind_int(m_pStmt, param, num) == SQLITE_OK);
}

bool SqQuery::BindParamBlob(unsigned int param, const void *data, size_t length, bool copy)
{
	/* SQLite is 1 indexed */
	param++;

	if (param > m_ParamCount)
	{
		return false;
	}

	return (sqlite3_bind_blob(m_pStmt, param, data, length, copy ? SQLITE_TRANSIENT : SQLITE_STATIC) == SQLITE_OK);
}

sqlite3_stmt *SqQuery::GetStmt()
{
	return m_pStmt;
}

bool SqQuery::Execute()
{
	int rc;

	/* If we don't have a result set and we have a column count, 
	 * create a result set pre-emptively.  This is in case there
	 * are no rows in the upcoming result set.
	 */
	if (!m_pResults && m_ColCount)
	{
		m_pResults = new SqResults(this);
	}

	/* If we've got results, throw them away */
	if (m_pResults)
	{
		m_pResults->ResetResultCount();
	}

	/* Fetch each row, if any */
	while ((rc = sqlite3_step(m_pStmt)) == SQLITE_ROW)
	{
		/* This should NEVER happen but we're being safe. */
		if (!m_pResults)
		{
			m_pResults = new SqResults(this);
		}
		m_pResults->PushResult();
	}

	sqlite3 *db = m_pParent->GetDb();
	if (rc != SQLITE_OK && rc != SQLITE_DONE && rc == sqlite3_errcode(db))
	{
		/* Something happened... */
		m_LastErrorCode = rc;
		m_LastError.assign(sqlite3_errmsg(db));
		m_AffectedRows = 0;
		m_InsertID = 0;
	} else {
		m_LastErrorCode = SQLITE_OK;
		m_AffectedRows = (unsigned int)sqlite3_changes(db);
		m_InsertID = (unsigned int)sqlite3_last_insert_rowid(db);
	}

	/* Reset everything for the next execute */
	sqlite3_reset(m_pStmt);
	sqlite3_clear_bindings(m_pStmt);

	return (m_LastErrorCode == SQLITE_OK);
}

const char *SqQuery::GetError(int *errCode/* =NULL */)
{
	if (errCode)
	{
		*errCode = m_LastErrorCode;
	}
	return m_LastError.c_str();
}

unsigned int SqQuery::GetAffectedRows()
{
	return m_AffectedRows;
}

unsigned int SqQuery::GetInsertID()
{
	return m_InsertID;
}


