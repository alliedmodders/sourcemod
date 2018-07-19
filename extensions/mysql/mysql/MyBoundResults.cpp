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

#include "MyBoundResults.h"

#define DEFAULT_BUFFER_SIZE		5

/* :IDEA: When we have to refetch a buffer to do type changes, should we rebind
 * the buffer so the next fetch will predict the proper cast?  Probably yes since
 * these things are done in standard iterations, but maybe users should be punished
 * for not using the API as it was intended?  Maybe it should be an option set to 
 * on by default to catch the bad users?
 */

enum_field_types GetTheirType(DBType type)
{
	switch (type)
	{
	case DBType_Float:
		{
			return MYSQL_TYPE_FLOAT;
		}
	case DBType_Integer:
		{
			return MYSQL_TYPE_LONG;
		}
	case DBType_String:
		{
			return MYSQL_TYPE_STRING;
		}
	case DBType_Blob:
		{
			return MYSQL_TYPE_BLOB;
		}
	case DBType_Unknown:
		{
			return MYSQL_TYPE_STRING;
		}
	}

	return MYSQL_TYPE_STRING;
}

MyBoundResults::MyBoundResults(MYSQL_STMT *stmt, MYSQL_RES *res, unsigned int num_fields)
: m_stmt(stmt), m_pRes(res), m_ColCount(num_fields), m_Initialized(false), m_RowCount(0), m_CurRow(0)
{
	/**
	 * Important thing to note here: 
	 * We're guaranteed at least one field.
	 */

	/* Allocate buffers */
	m_bind = (MYSQL_BIND *)malloc(sizeof(MYSQL_BIND) * m_ColCount);
	m_pull = (ResultBind *)malloc(sizeof(ResultBind) * m_ColCount);

	/* Zero data */
	memset(m_bind, 0, sizeof(MYSQL_BIND) * m_ColCount);
	memset(m_pull, 0, sizeof(ResultBind) * m_ColCount);

	m_bUpdatedBinds = false;
}

MyBoundResults::~MyBoundResults()
{
	if (m_Initialized)
	{
		/* Make sure we free our internal buffers */
		for (unsigned int i=0; i<m_ColCount; i++)
		{
			delete [] m_pull[i].blob;
		}
	}

	free(m_pull);
	free(m_bind);
}

void MyBoundResults::Update()
{
	m_RowCount = (unsigned int)mysql_stmt_num_rows(m_stmt);
	m_CurRow = 0;
}

bool MyBoundResults::Initialize()
{
	/* Check if we need to build our result binding information */
	if (!m_Initialized)
	{
		for (unsigned int i=0; i<m_ColCount; i++)
		{
			MYSQL_FIELD *field = mysql_fetch_field_direct(m_pRes, i);
			DBType type = GetOurType(field->type);

			m_bind[i].length = &(m_pull[i].my_length);
			m_bind[i].is_null = &(m_pull[i].my_null);

			if (type == DBType_Integer)
			{
				m_bind[i].buffer_type = MYSQL_TYPE_LONG;
				m_bind[i].buffer = &(m_pull[i].data.ival);
			} else if (type == DBType_Float) {
				m_bind[i].buffer_type = MYSQL_TYPE_FLOAT;
				m_bind[i].buffer = &(m_pull[i].data.ival);
			} else if (type == DBType_String || type == DBType_Blob) {
				m_bind[i].buffer_type = GetTheirType(type);

				/* We bound this to 2048 bytes.  Otherwise a MEDIUMBLOB
				 * or something could allocate horrible amounts of memory
				 * because MySQL is incompetent.  
				 */
				size_t creat_length = (size_t)field->length;
				if (!creat_length || creat_length > DEFAULT_BUFFER_SIZE)
				{
					creat_length = DEFAULT_BUFFER_SIZE;
				}
				m_pull[i].blob = new unsigned char[creat_length];
				m_pull[i].length = creat_length;

				m_bind[i].buffer = m_pull[i].blob;
				m_bind[i].buffer_length = (unsigned long)creat_length;
			} else {
				return false;
			}
		}
		m_Initialized = true;
	}

	/* Do the actual bind */
	return (mysql_stmt_bind_result(m_stmt, m_bind) == 0);
}

unsigned int MyBoundResults::GetRowCount()
{
	return m_RowCount;
}

unsigned int MyBoundResults::GetFieldCount()
{
	return m_ColCount;
}

const char *MyBoundResults::FieldNumToName(unsigned int columnId)
{
	if (columnId >= m_ColCount)
	{
		return NULL;
	}

	MYSQL_FIELD *field = mysql_fetch_field_direct(m_pRes, columnId);

	return field ? (field->name ? field->name : "") : "";
}

bool MyBoundResults::FieldNameToNum(const char *name, unsigned int *columnId)
{
	for (unsigned int i=0; i<m_ColCount; i++)
	{
		if (strcmp(name, FieldNumToName(i)) == 0)
		{
			*columnId = i;
			return true;
		}
	}

	return false;
}

bool MyBoundResults::MoreRows()
{
	return (m_CurRow < m_RowCount);
}

IResultRow *MyBoundResults::FetchRow()
{
	if (!MoreRows())
	{
		m_CurRow = m_RowCount + 1;
		return NULL;
	}

	m_CurRow++;

	if (m_bUpdatedBinds)
	{
		if (mysql_stmt_bind_result(m_stmt, m_bind) != 0)
		{
			return NULL;
		}
		m_bUpdatedBinds = false;
	}

	/* We should be able to get another row */
	int err = mysql_stmt_fetch(m_stmt);
	if (err == 0 || err == MYSQL_DATA_TRUNCATED)
	{
		return this;
	}

	if (err == MYSQL_NO_DATA && m_CurRow == m_RowCount)
	{
		return this;
	}

	/* Some sort of error occurred */
	return NULL;
}

IResultRow *MyBoundResults::CurrentRow()
{
	if (!m_CurRow || m_CurRow > m_RowCount)
	{
		return NULL;
	}

	return this;
}

bool MyBoundResults::Rewind()
{
	mysql_stmt_data_seek(m_stmt, 0);
	m_CurRow = 0;
	return true;
}

DBType MyBoundResults::GetFieldType(unsigned int field)
{
	if (field >= m_ColCount)
	{
		return DBType_Unknown;
	}

	MYSQL_FIELD *fld = mysql_fetch_field_direct(m_pRes, field);
	return GetOurType(fld->type);
}

DBType MyBoundResults::GetFieldDataType(unsigned int field)
{
	return GetFieldType(field);
}

void ResizeBuffer(ResultBind *bind, size_t len)
{
	if (!bind->blob)
	{
		bind->blob = new unsigned char[len];
		bind->length = len;
	}
	else if (bind->length < len)
	{
		delete [] bind->blob;
		bind->blob = new unsigned char[len];
		bind->length = len;
	}
}

bool MyBoundResults::RefetchField(MYSQL_STMT *stmt, 
				  unsigned int id,
				  size_t initSize,
				  enum_field_types type)
{
	ResultBind *rbind = &m_pull[id];

	/* Make sure there is a buffer to pull into */
	ResizeBuffer(rbind, initSize);

	/* Update the bind with the buffer size */
	m_bind[id].buffer = rbind->blob;
	m_bind[id].buffer_length = (unsigned long)rbind->length;
	m_bUpdatedBinds = true;

	MYSQL_BIND bind;

	/* Initialize bind info */
	memset(&bind, 0, sizeof(MYSQL_BIND));
	bind.buffer = rbind->blob;
	bind.buffer_type = type;
	bind.buffer_length = (unsigned long)rbind->length;
	bind.length = &(rbind->my_length);
	bind.is_null = &(rbind->my_null);

	/* Attempt to fetch */
	return (mysql_stmt_fetch_column(stmt, &bind, id, 0) == 0);
}

DBResult RefetchSize4Field(MYSQL_STMT *stmt,
						unsigned int id,
						void *buffer,
						enum_field_types type)
{
	MYSQL_BIND bind;
	my_bool is_null;

	/* Initialize bind info  */
	memset(&bind, 0, sizeof(MYSQL_BIND));
	bind.buffer = buffer;
	bind.buffer_type = type;
	bind.is_null = &is_null;

	/* Attempt to fetch */
	if (mysql_stmt_fetch_column(stmt, &bind, id, 0) != 0)
	{
		return DBVal_TypeMismatch;
	}

	return is_null ? DBVal_Null : DBVal_Data;
}


bool RefetchUserField(MYSQL_STMT *stmt,
					  unsigned int id,
					  void *userbuf,
					  size_t userlen,
					  enum_field_types type,
					  my_bool &is_null,
					  size_t *written)
{
	MYSQL_BIND bind;
	unsigned long length;

	/* Initialize bind info */
	memset(&bind, 0, sizeof(MYSQL_BIND));
	bind.buffer = userbuf;
	bind.buffer_type = type;
	bind.length = &length;
	bind.is_null = &is_null;
	bind.buffer_length = (unsigned long)userlen;

	if (mysql_stmt_fetch_column(stmt, &bind, id, 0) != 0)
	{
		return false;
	}

	if (is_null)
	{
		return true;
	}

	if (type == MYSQL_TYPE_STRING && (size_t)length == userlen)
	{
		/* Enforce null termination in case MySQL forgot.
		 * Note we subtract one from the length (which must be >= 1)
		 * so we can pass the number of bytes written below.
		 */
		char *data = (char *)userbuf;
		data[--userlen] = '\0';
	}

	if (written)
	{
		/* In the case of strings, they will never be equal */
		*written = (userlen < length) ? userlen : length;
	}

	return true;
}

#define BAD_COL_CHECK() \
	if (id >= m_ColCount) \
		return DBVal_Error;

#define STR_NULL_CHECK_0(var) \
	if (var) { \
		*pString = NULL; \
		if (length) \
			*length = 0; \
		return DBVal_Null; \
	}

DBResult MyBoundResults::GetString(unsigned int id, const char **pString, size_t *length)
{
	BAD_COL_CHECK();

	STR_NULL_CHECK_0(m_pull[id].my_null);

	if (m_bind[id].buffer_type != MYSQL_TYPE_STRING)
	{
		/* Ugh, we have to re-get this as a string.  Sigh, stupid user.
		 * We're going to disallow conversions from blobs.
		 */
		if (m_bind[id].buffer_type == MYSQL_TYPE_BLOB)
		{
			return DBVal_TypeMismatch;
		}

		/* Attempt to refetch the string */
		if (!RefetchField(m_stmt, id, 128, MYSQL_TYPE_STRING))
		{
			return DBVal_TypeMismatch;
		}

		/* Check if we have a new null */
		STR_NULL_CHECK_0(m_pull[id].my_null);
	}

	/* Okay, we should now have a blob type whether we originally wanted one or not. */

	/* Check if the size is too small.  Note that MySQL will not null terminate small buffers,
	 * and it returns the size without the null terminator.  This means we need to add an extra
	 * byte onto the end to accept the terminator until there is a workaround.
	 *
	 * Note that we do an >= check because MySQL appears to want the null terminator included,
	 * so just to be safe and avoid its inconsistencies, we make sure we'll always have room.
	 */
	if ((size_t)(m_pull[id].my_length) >= m_pull[id].length)
	{
		/* Yes, we need to refetch. */
		if (!RefetchField(m_stmt, id, m_pull[id].my_length + 1, MYSQL_TYPE_STRING))
		{
			return DBVal_Error;
		}
	}

	/* Finally, we can return.  We're guaranteed to have a properly NULL-terminated string
	 * here because we have refetched the string to a bigger length.
	 */
	*pString = (const char *)m_pull[id].blob;
	if (length)
	{
		*length = (size_t)m_pull[id].my_length;
	}

	return DBVal_Data;
}

#define STR_NULL_CHECK_1(var) \
	if (var) { \
		buffer[0] = '\0'; \
		if (written) \
			*written = 0; \
		return DBVal_Null; \
	} 

DBResult MyBoundResults::CopyString(unsigned int id, char *buffer, size_t maxlength, size_t *written)
{
	BAD_COL_CHECK();

	STR_NULL_CHECK_1(m_pull[id].my_null);

	if (!buffer || !maxlength)
	{
		return DBVal_Error;
	}

	if (m_bind[id].buffer_type != MYSQL_TYPE_STRING)
	{
		/* We're going to disallow conversions from blobs. */
		if (m_bind[id].buffer_type == MYSQL_TYPE_BLOB)
		{
			return DBVal_TypeMismatch;
		}

		/* Re-fetch this for the user.  This call will guarantee NULL termination. */
		my_bool is_null;
		if (!RefetchUserField(m_stmt, id, buffer, maxlength, MYSQL_TYPE_STRING, is_null, written))
		{
			return DBVal_TypeMismatch;
		}

		STR_NULL_CHECK_1(is_null);

		return DBVal_Data;
	}

	size_t pull_length = (size_t)m_pull[id].my_length;
	size_t orig_length = m_pull[id].length;

	/* If there's more data in the buffer, we have to look at two cases. */
	if (pull_length >= orig_length)
	{
		/* If the user supplied a bigger buffer, just refetch for them. */
		if (maxlength > orig_length)
		{
			my_bool is_null;
			RefetchUserField(m_stmt, id, buffer, maxlength, MYSQL_TYPE_STRING, is_null, written);
			STR_NULL_CHECK_1(is_null);
			return DBVal_Data;
		}
		/* Otherwise, we should enforce null termination from MySQL. */
		else if (pull_length == orig_length)
		{
			char *data = (char *)m_pull[id].blob;
			data[pull_length] = '\0';
		}
	}

	/* If we got here, we need to copy the resultant string to the user and be done with it.
	 * Null termination is guaranteed from the pulled string.
	 */
	size_t wr = strncopy(buffer, (const char *)m_pull[id].blob, maxlength);
	if (written)
	{
		*written = wr;
	}

	return DBVal_Data;
}

DBResult MyBoundResults::GetFloat(unsigned int id, float *pFloat)
{
	BAD_COL_CHECK();

	if (m_pull[id].my_null)
	{
		*pFloat = 0.0f;
		return DBVal_Null;
	}

	if (m_bind[id].buffer_type != MYSQL_TYPE_FLOAT)
	{
		if (m_bind[id].buffer_type == MYSQL_TYPE_BLOB)
		{
			return DBVal_TypeMismatch;
		}
		/* We have to convert... */
		return RefetchSize4Field(m_stmt, id, pFloat, MYSQL_TYPE_FLOAT);
	}

	*pFloat = m_pull[id].data.fval;

	return DBVal_Data;
}

DBResult MyBoundResults::GetInt(unsigned int id, int *pInt)
{
	BAD_COL_CHECK();

	if (m_pull[id].my_null)
	{
		*pInt = 0;
		return DBVal_Null;
	}

	if (m_bind[id].buffer_type != MYSQL_TYPE_LONG)
	{
		if (m_bind[id].buffer_type == MYSQL_TYPE_BLOB)
		{
			return DBVal_TypeMismatch;
		}
		/* We have to convert... */
		return RefetchSize4Field(m_stmt, id, pInt, MYSQL_TYPE_LONG);
	}

	*pInt = m_pull[id].data.ival;

	return DBVal_Data;
}

bool MyBoundResults::IsNull(unsigned int id)
{
	if (id >= m_ColCount)
	{
		return true;
	}

	return m_pull[id].my_null ? true : false;
}

#define BLOB_CHECK_NULL_0() \
	if (m_pull[id].my_null) { \
		*pData = NULL; \
		if (length) \
			*length = 0; \
		return DBVal_Null; \
	}

DBResult MyBoundResults::GetBlob(unsigned int id, const void **pData, size_t *length)
{
	BAD_COL_CHECK();

	BLOB_CHECK_NULL_0();

	/* We only want blobs to be read as blobs */
	if (m_bind[id].buffer_type != MYSQL_TYPE_BLOB)
	{
		return DBVal_TypeMismatch;
	}

	if ((size_t)m_pull[id].my_length > m_pull[id].length)
	{
		if (!RefetchField(m_stmt, id, m_pull[id].my_length, MYSQL_TYPE_BLOB))
		{
			return DBVal_TypeMismatch;
		}
		BLOB_CHECK_NULL_0();
	}

	*pData = m_pull[id].blob;
	if (length)
	{
		*length = (size_t)m_pull[id].my_length;
	}

	return DBVal_Data;
}

#define BLOB_CHECK_NULL_1(var) \
	if (var) { \
		if (written) \
			*written = 0; \
		return DBVal_Null; \
	}

DBResult MyBoundResults::CopyBlob(unsigned int id, void *buffer, size_t maxlength, size_t *written)
{
	BAD_COL_CHECK();

	/* We only want blobs to be read as blobs */
	if (m_bind[id].buffer_type != MYSQL_TYPE_BLOB)
	{
		return DBVal_TypeMismatch;
	}

	BLOB_CHECK_NULL_1(m_pull[id].my_null);

	size_t pull_size = (size_t)m_pull[id].my_length;
	size_t push_size = m_pull[id].length;

	/* Check if we can do a resize and copy in one step */
	if (pull_size > push_size
		&& maxlength > push_size)
	{
		my_bool is_null;
		if (!RefetchUserField(m_stmt, id, buffer, maxlength, MYSQL_TYPE_BLOB, is_null, written))
		{
			return DBVal_TypeMismatch;
		}
		BLOB_CHECK_NULL_1(is_null);
		return DBVal_Data;
	}

	/* If we got here, either there is no more data to refetch,
	 * or our buffer is too small to receive the refetched data.
	 */
	size_t buf_bytes = pull_size > push_size ? push_size : pull_size;
	size_t to_copy = buf_bytes > maxlength ? maxlength : buf_bytes;

	memcpy(buffer, m_pull[id].blob, to_copy);
	if (written)
	{
		*written = to_copy;
	}

	return DBVal_Data;
}

size_t MyBoundResults::GetDataSize(unsigned int id)
{
	if (id >= m_ColCount)
	{
		return 0;
	}

	return (size_t)m_pull[id].my_length;
}
