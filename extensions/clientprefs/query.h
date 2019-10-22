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

#include <memory>
#include <string>

enum QueryType
{
	Query_InsertCookie = 0,
	Query_SelectData,
	Query_InsertData,
	Query_SelectId,
	Query_Connect,
};

struct CookieData;

/* This stores all the info required for our param binding until the thread is executed */
struct ParamData
{
	ParamData() : cookie(nullptr), cookieId(0) {
		
	}

	~ParamData() {
	}

	/* Contains a name, description and access for InsertCookie queries */
	Cookie *cookie;
	int cookieId;
	/* A clients steamid - Used for most queries - Doubles as storage for the cookie name*/
	std::string steamId;

	std::unique_ptr<CookieData> data;
};

class TQueryOp : public IDBThreadOperation
{
public:
	TQueryOp(QueryType type, int serial);
	TQueryOp(QueryType type, Cookie *cookie);
	virtual ~TQueryOp() {}

	void SetDatabase(IDatabase *db);
	bool BindParamsAndRun();

	IDatabase *GetDB()
	{
		return m_database;
	}

	QueryType PullQueryType() {
		return m_type;
	}

	int PullQuerySerial() {
		return m_serial;
	}

public:	
	// IDBThreadOperation
	IDBDriver *GetDriver();
	IdentityToken_t *GetOwner();
	void Destroy();
	void RunThreadPart();
	void CancelThinkPart()	{} // thread cancelled, nothing else to do?
	void RunThinkPart();

public:
	/* Params to be bound */
	ParamData m_params;

private:
	IDatabase *m_database;
	IDBDriver *m_driver;
	IQuery *m_pResult;

	/* Query type */
	QueryType m_type;

	/* Data to be passed to the callback */
	int m_serial;
	int m_insertId;
	Cookie *m_pCookie;
};


#endif // _INCLUDE_SOURCEMOD_CLIENTPREFS_QUERY_H_
