/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Client Preferences Extension
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

#ifndef _INCLUDE_SOURCEMOD_CLIENTPREFS_QUERY_H_
#define _INCLUDE_SOURCEMOD_CLIENTPREFS_QUERY_H_

#include "extension.h"
#include "cookie.h"
#include "sh_string.h"

enum querytype
{
	Query_InsertCookie = 0,
	Query_SelectData,
	Query_InsertData,
	Query_SelectId,
	Query_CreateTable,
};

class TQueryOp : public IDBThreadOperation
{
public:
	TQueryOp(IDatabase *db, const char *query, enum querytype type, int client);
	TQueryOp(IDatabase *db, const char *query, enum querytype type, Cookie *cookie);
	~TQueryOp() {}

	IDBDriver *GetDriver();
	IdentityToken_t *GetOwner();

	void Destroy();

	void RunThreadPart();
	/* Thread has been cancelled due to driver unloading. Nothing else to do? */
	void CancelThinkPart()
	{
	}
	void RunThinkPart();

private:
	IDatabase *m_pDatabase;
	IQuery *m_pQuery;
	SourceHook::String m_Query;
	char error[255];
	enum querytype m_type;
	int m_client;
	int m_insertId;
	Cookie *pCookie;
};


#endif // _INCLUDE_SOURCEMOD_CLIENTPREFS_QUERY_H_
