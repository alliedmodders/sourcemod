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

#include "MyStatement.h"
#include "MyBoundResults.h"

MyStatement::MyStatement(MyDatabase *db, MYSQL_STMT *stmt)
: m_mysql(db->m_mysql), m_pParent(db), m_stmt(stmt), m_pRes(NULL), m_rs(NULL), m_Results(false)
{
	m_Params = (unsigned int)mysql_stmt_param_count(m_stmt);

	if (m_Params)
	{
		m_pushinfo = (ParamBind *)malloc(sizeof(ParamBind) * m_Params);
		memset(m_pushinfo, 0, sizeof(ParamBind) * m_Params);
		m_bind = (MYSQL_BIND *)malloc(sizeof(MYSQL_BIND) * m_Params);
		memset(m_bind, 0, sizeof(MYSQL_BIND) * m_Params);
	} else {
		m_pushinfo = NULL;
		m_bind = NULL;
	}

	m_Results = false;
}

MyStatement::~MyStatement()
{
	while (FetchMoreResults())
	{
		/* Spin until all are gone */
	}

	/* Free result set structures */
	ClearResults();

	/* Free old blobs */
	for (unsigned int i=0; i<m_Params; i++)
	{
		free(m_pushinfo[i].blob);
	}

	/* Free our allocated arrays */
	free(m_pushinfo);
	free(m_bind);
	
	/* Close our mysql handles */
	mysql_stmt_close(m_stmt);
}

void MyStatement::Destroy()
{
	delete this;
}

void MyStatement::ClearResults()
{
	if (m_rs)
	{
		delete m_rs;
		m_rs = NULL;
	}
	if (m_pRes)
	{
		mysql_free_result(m_pRes);
		m_pRes = NULL;
	}
	m_Results = false;
}

bool MyStatement::FetchMoreResults()
{
	if (m_pRes == NULL)
	{
		return false;
	}
	else if (!mysql_more_results(m_pParent->m_mysql)) {
		return false;
	}

	ClearResults();

	if (mysql_stmt_next_result(m_stmt) != 0)
	{
		return false;
	}

	/* the column count is > 0 if there is a result set
	 * 0 if the result is only the final status packet in CALL queries.
	 */
	unsigned int num_fields = mysql_stmt_field_count(m_stmt);
	if (num_fields == 0)
	{
		return false;
	}

	/* Skip away if we don't have data */
	m_pRes = mysql_stmt_result_metadata(m_stmt);
	if (!m_pRes)
	{
		return false;
	}

	/* If we don't have a result manager, create one. */
	if (!m_rs)
	{
		m_rs = new MyBoundResults(m_stmt, m_pRes, num_fields);
	}

	/* Tell the result set to update its bind info,
	* and initialize itself if necessary.
	*/
	if (!(m_Results = m_rs->Initialize()))
	{
		return false;
	}

	/* Try precaching the results. */
	m_Results = (mysql_stmt_store_result(m_stmt) == 0);

	/* Update now that the data is known. */
	m_rs->Update();

	/* Return indicator */
	return m_Results;
}

void *MyStatement::CopyBlob(unsigned int param, const void *blobptr, size_t length)
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

bool MyStatement::BindParamInt(unsigned int param, int num, bool signd)
{
	if (param >= m_Params)
	{
		return false;
	}

	m_pushinfo[param].data.ival = num;
	m_bind[param].buffer_type = MYSQL_TYPE_LONG;
	m_bind[param].buffer = &(m_pushinfo[param].data.ival);
	m_bind[param].is_unsigned = signd ? 0 : 1;
	m_bind[param].length = NULL;

	return true;
}

bool MyStatement::BindParamFloat(unsigned int param, float f)
{
	if (param >= m_Params)
	{
		return false;
	}

	m_pushinfo[param].data.fval = f;
	m_bind[param].buffer_type = MYSQL_TYPE_FLOAT;
	m_bind[param].buffer = &(m_pushinfo[param].data.fval);
	m_bind[param].length = NULL;

	return true;
}

bool MyStatement::BindParamString(unsigned int param, const char *text, bool copy)
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

	m_bind[param].buffer_type = MYSQL_TYPE_STRING;
	m_bind[param].buffer = (void *)final_ptr;
	m_bind[param].buffer_length = (unsigned long)len;
	m_bind[param].length = &(m_bind[param].buffer_length);

	return true;
}

bool MyStatement::BindParamBlob(unsigned int param, const void *data, size_t length, bool copy)
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

	m_bind[param].buffer_type = MYSQL_TYPE_BLOB;
	m_bind[param].buffer = (void *)final_ptr;
	m_bind[param].buffer_length = (unsigned long)length;
	m_bind[param].length = &(m_bind[param].buffer_length);

	return true;
}

bool MyStatement::BindParamNull(unsigned int param)
{
	if (param >= m_Params)
	{
		return false;
	}

	m_bind[param].buffer_type = MYSQL_TYPE_NULL;

	return true;
}

bool MyStatement::Execute()
{
	/* Clear any past result first! */
	while (FetchMoreResults())
	{
		/* Spin until all are gone */
	}

	/* Free result set structures */
	ClearResults();

	/* Bind the parameters */
	if (m_Params)
	{
		if (mysql_stmt_bind_param(m_stmt, m_bind) != 0)
		{
			return false;
		}
	}

	if (mysql_stmt_execute(m_stmt) != 0)
	{
		return false;
	}

	/* the column count is > 0 if there is a result set
	 * 0 if the result is only the final status packet in CALL queries.
	 */
	unsigned int num_fields = mysql_stmt_field_count(m_stmt);
	if (num_fields == 0)
	{
		return true;
	}

	/* Skip away if we don't have data */
	m_pRes = mysql_stmt_result_metadata(m_stmt);
	if (!m_pRes)
	{
		return true;
	}

	/* Create our result manager. */
	m_rs = new MyBoundResults(m_stmt, m_pRes, num_fields);

	/* Tell the result set to update its bind info,
	 * and initialize itself if necessary.
	 */
	if (!(m_Results = m_rs->Initialize()))
	{
		return false;
	}

	/* Try precaching the results. */
	m_Results = (mysql_stmt_store_result(m_stmt) == 0);

	/* Update now that the data is known. */
	m_rs->Update();

	/* Return indicator */
	return m_Results;
}

const char *MyStatement::GetError(int *errCode/* =NULL */)
{
	if (errCode)
	{
		*errCode = mysql_stmt_errno(m_stmt);
	}

	return mysql_stmt_error(m_stmt);
}

unsigned int MyStatement::GetAffectedRows()
{
	return (unsigned int)mysql_stmt_affected_rows(m_stmt);
}

unsigned int MyStatement::GetInsertID()
{
	return (unsigned int)mysql_stmt_insert_id(m_stmt);
}

IResultSet *MyStatement::GetResultSet()
{
	return (m_Results ? m_rs : NULL);
}
