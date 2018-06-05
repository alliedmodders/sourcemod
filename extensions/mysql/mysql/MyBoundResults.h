/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod MySQL Extension
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SM_MYSQL_BOUND_RESULTS_H_
#define _INCLUDE_SM_MYSQL_BOUND_RESULTS_H_

#include "MyDatabase.h"

class MyStatement;

struct ResultBind
{
	my_bool my_null;
	unsigned long my_length;
	union
	{
		int ival;
		float fval;
	} data;
	unsigned char *blob;
	size_t length;
};

class MyBoundResults : 
	public IResultSet,
	public IResultRow
{
	friend class MyStatement;
public:
	MyBoundResults(MYSQL_STMT *stmt, MYSQL_RES *res, unsigned int num_fields);
	~MyBoundResults();
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
	DBResult GetString(unsigned int id, const char **pString, size_t *length);
	DBResult CopyString(unsigned int id, 
		char *buffer, 
		size_t maxlength, 
		size_t *written);
	DBResult GetFloat(unsigned int id, float *pFloat);
	DBResult GetInt(unsigned int id, int *pInt);
	bool IsNull(unsigned int id);
	size_t GetDataSize(unsigned int id);
	DBResult GetBlob(unsigned int id, const void **pData, size_t *length);
	DBResult CopyBlob(unsigned int id, void *buffer, size_t maxlength, size_t *written);
public:
	bool Initialize();
	void Update();
private:
	bool RefetchField(MYSQL_STMT *stmt, 
		unsigned int id,
		size_t initSize,
		enum_field_types type);
private:
	MYSQL_STMT *m_stmt;
	MYSQL_RES *m_pRes;
	MYSQL_BIND *m_bind;
	ResultBind *m_pull;
	unsigned int m_ColCount;
	bool m_Initialized;
	unsigned int m_RowCount;
	unsigned int m_CurRow;
	bool m_bUpdatedBinds;
};

#endif //_INCLUDE_SM_MYSQL_BOUND_RESULTS_H_
