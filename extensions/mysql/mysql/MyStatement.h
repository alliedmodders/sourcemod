/**
 * vim: set ts=4 sw=4 tw=99 noet :
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

#ifndef _INCLUDE_SM_MYSQL_STATEMENT_H_
#define _INCLUDE_SM_MYSQL_STATEMENT_H_

#include "MyDatabase.h"
#include "MyBoundResults.h"

struct ParamBind
{
	union
	{
		float fval;
		int ival;
	} data;
	void *blob;
	size_t length;
};

class MyStatement : public IPreparedQuery
{
public:
	MyStatement(MyDatabase *db, MYSQL_STMT *stmt);
	~MyStatement();
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
private:
	void *CopyBlob(unsigned int param, const void *blobptr, size_t length);
	void ClearResults();
private:
	MYSQL *m_mysql;
	ke::RefPtr<MyDatabase> m_pParent;
	MYSQL_STMT *m_stmt;
	MYSQL_BIND *m_bind;
	MYSQL_RES *m_pRes;
	ParamBind *m_pushinfo;
	unsigned int m_Params;
	MyBoundResults *m_rs;
	bool m_Results;
};

#endif //_INCLUDE_SM_MYSQL_STATEMENT_H_
