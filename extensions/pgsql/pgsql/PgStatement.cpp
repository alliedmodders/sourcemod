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

#include "PgStatement.h"

PgStatement::PgStatement(PgDatabase *db, const char* stmtName)
	: m_pgsql(db->m_pgsql), m_pParent(db), m_insertID(0), m_affectedRows(0), m_rs(NULL), m_Results(false)
{
	m_stmtName = new char[10];
	strncopy(m_stmtName, stmtName, 10);

	PGresult *desc = PQdescribePrepared(m_pgsql, m_stmtName);

	// TODO: Proper error handling?
	if (PQresultStatus(desc) != PGRES_COMMAND_OK)
	{
		PQclear(desc);
		return;
	}

	m_Params = (unsigned int)PQnparams(desc);

	if (m_Params)
	{
		m_pushinfo = (ParamBind *)malloc(sizeof(ParamBind) * m_Params);
		memset(m_pushinfo, 0, sizeof(ParamBind) * m_Params);
	} else {
		m_pushinfo = NULL;
	}

	m_Results = false;
}

PgStatement::~PgStatement()
{
	/* Free result set structures */
	if (m_Results)
	{
		if (m_rs->m_pRes != NULL)
			PQclear(m_rs->m_pRes);
		delete m_rs;
	}

	/* Free old blobs */
	for (unsigned int i=0; i<m_Params; i++)
	{
		free(m_pushinfo[i].blob);
	}

	/* Free our allocated arrays */
	free(m_pushinfo);
}

void PgStatement::Destroy()
{
	delete this;
}

bool PgStatement::FetchMoreResults()
{
	return false;
}

void *PgStatement::CopyBlob(unsigned int param, const void *blobptr, size_t length)
{
	void *copy_ptr = NULL;

	if (m_pushinfo[param].blob != NULL)
	{
		if (m_pushinfo[param].length < length)
		{
			free(m_pushinfo[param].blob);
		} else {
			copy_ptr = m_pushinfo[param].blob;
		}
	}

	if (copy_ptr == NULL)
	{
		copy_ptr = malloc(length);
		m_pushinfo[param].blob = copy_ptr;
		m_pushinfo[param].length = length;
	}

	memcpy(copy_ptr, blobptr, length);

	return copy_ptr;
}

bool PgStatement::BindParamInt(unsigned int param, int num, bool signd)
{
	if (param >= m_Params)
	{
		return false;
	}

	m_pushinfo[param].data.ival = num;
	m_pushinfo[param].type = DBType_Integer;

	return true;
}

bool PgStatement::BindParamFloat(unsigned int param, float f)
{
	if (param >= m_Params)
	{
		return false;
	}

	m_pushinfo[param].data.fval = f;
	m_pushinfo[param].type = DBType_Float;

	return true;
}

bool PgStatement::BindParamString(unsigned int param, const char *text, bool copy)
{
	if (param >= m_Params)
	{
		return false;
	}

	const void *final_ptr;
	size_t len;

	if (copy)
	{
		len = strlen(text);
		final_ptr = CopyBlob(param, text, len+1);
	} else {
		len = strlen(text);
		final_ptr = text;
	}

	m_pushinfo[param].blob = (void *)final_ptr;
	m_pushinfo[param].length = len;
	m_pushinfo[param].type = DBType_String;

	return true;
}

bool PgStatement::BindParamBlob(unsigned int param, const void *data, size_t length, bool copy)
{
	if (param >= m_Params)
	{
		return false;
	}

	const void *final_ptr;
	
	if (copy)
	{
		final_ptr = CopyBlob(param, data, length);
	} else {
		final_ptr = data;
	}

	m_pushinfo[param].blob = (void *)final_ptr;
	m_pushinfo[param].length = length;
	m_pushinfo[param].type = DBType_Blob;

	return true;
}

bool PgStatement::BindParamNull(unsigned int param)
{
	if (param >= m_Params)
	{
		return false;
	}

	m_pushinfo[param].type = DBType_NULL;

	return true;
}

bool PgStatement::Execute()
{
	/* Clear any past result first! */
	if (m_Results)
		delete m_rs;
	m_Results = false;

	PGresult *res;
	/* Bind the parameters */
	if (m_Params)
	{
		// Put bound params into a nice array
		const char **paramValues = new const char*[m_Params];
		int *paramLengths = new int[m_Params];
		int *paramFormats = new int[m_Params];

		for (unsigned int i=0; i<m_Params; i++)
		{
			switch (m_pushinfo[i].type)
			{
			case DBType_Integer:
				{
					char *buf = new char[64]; // 64 chars should be enough?
					snprintf(buf, 64, "%d", m_pushinfo[i].data.ival);
					paramValues[i] = buf;
					paramLengths[i] = 0;
					paramFormats[i] = 0;
					break;
				}
			case DBType_Float:
				{
					char *buf = new char[64]; // 64 chars should be enough?
					snprintf(buf, 64, "%f", m_pushinfo[i].data.fval);
					paramValues[i] = buf;
					paramLengths[i] = 0;
					paramFormats[i] = 0;
					break;
				}
			case DBType_String:
				{
					paramValues[i] = (char *)m_pushinfo[i].blob;
					paramLengths[i] = m_pushinfo[i].length;
					paramFormats[i] = 0;
					break;
				}
			case DBType_Blob:
				{
					paramValues[i] = (char *)m_pushinfo[i].blob;
					paramLengths[i] = m_pushinfo[i].length;
					paramFormats[i] = 1;
					break;
				}
			case DBType_NULL:
			default:
				{
					paramValues[i] = NULL;
					paramLengths[i] = 0;
					paramFormats[i] = 0;
					break;
				}
			}
		}

		res = PQexecPrepared(m_pgsql, m_stmtName, m_Params, paramValues, paramLengths, paramFormats, 0);
		delete [] paramFormats;
		delete [] paramLengths;

		// .. need to free our char buffers
		for (unsigned int i=0; i<m_Params; i++)
		{
			switch (m_pushinfo[i].type)
			{
			case DBType_Integer:
			case DBType_Float:
				{
					delete paramValues[i];
					break;
				}
			}
		}
		delete [] paramValues;
	}
	// There are no parameters to be bound!
	else
	{
		res = PQexecPrepared(m_pgsql, m_stmtName, 0, NULL, NULL, NULL, 0);
	}

	ExecStatusType status = PQresultStatus(res);
	if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK)
	{
		PQclear(res);
		return false;
	}

	m_affectedRows = (unsigned int)atoi(PQcmdTuples(res));
	m_insertID = (unsigned int)PQoidValue(res);

	m_pParent->SetLastIDAndRows(m_insertID, m_affectedRows);

	/* Skip away if we don't have data */
	if (status == PGRES_COMMAND_OK)
	{
		PQclear(res);
		return true;
	}

	m_rs = new PgBasicResults(res);
	m_Results = true;

	return true;
}

const char *PgStatement::GetError(int *errCode/* =NULL */)
{
	if (m_Results)
	{
		if (errCode)
		{
			// PostgreSQL only supports SQLSTATE error codes.
			// https://www.postgresql.org/docs/9.6/errcodes-appendix.html
			*errCode = -1;
		}

		return PQresultErrorMessage(m_rs->m_pRes);
	}
	else
	{
		return PQerrorMessage(m_pgsql);
	}
}

unsigned int PgStatement::GetAffectedRows()
{
	return m_affectedRows;
}

unsigned int PgStatement::GetInsertID()
{
	return m_insertID;
}

IResultSet *PgStatement::GetResultSet()
{
	return (m_Results ? m_rs : NULL);
}
