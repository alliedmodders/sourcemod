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

#include "query.h"


void TQueryOp::RunThinkPart()
{
	//handler for threaded sql queries

	if (m_pQuery)
	{
		switch (m_type)
		{
		case Query_InsertCookie:
			g_CookieManager.InsertCookieCallback(pCookie, m_insertId);
			break;
		case Query_SelectData:
			g_CookieManager.ClientConnectCallback(m_client, m_pQuery);
			break;
		case Query_InsertData:
			//No specific handling
			break;
		case Query_SelectId:
			g_CookieManager.SelectIdCallback(pCookie, m_pQuery);
			break;
		default:
			break;
		}

		m_pQuery->Destroy();
	}
	else
	{
		g_pSM->LogError(myself,"Failed SQL Query, Error: \"%s\" (Query id %i - client %i)", error, m_type, m_client);
	}
}

void TQueryOp::RunThreadPart()
{
	m_pDatabase->LockForFullAtomicOperation();
	m_pQuery = m_pDatabase->DoQuery(m_Query.c_str());

	if (!m_pQuery)
	{
		g_pSM->LogError(myself, "Failed SQL Query, Error: \"%s\" (Query id %i - client %i)", m_pDatabase->GetError(), m_type, m_client);
	}

	m_insertId = g_ClientPrefs.Database->GetInsertID();

	m_pDatabase->UnlockFromFullAtomicOperation();
}

IDBDriver *TQueryOp::GetDriver()
{
	return m_pDatabase->GetDriver();
}
IdentityToken_t *TQueryOp::GetOwner()
{
	return myself->GetIdentity();
}
void TQueryOp::Destroy()
{
	delete this;
}

TQueryOp::TQueryOp(IDatabase *db, const char *query, enum querytype type, int client)
{
	m_pDatabase = db;
	m_Query = query;
	m_type = type;
	m_client = client;

	m_pDatabase->IncReferenceCount();
}

TQueryOp::TQueryOp(IDatabase *db, const char *query, enum querytype type, Cookie *cookie)
{
	m_pDatabase = db;
	m_Query = query;
	m_type = type;
	pCookie = cookie;

	m_pDatabase->IncReferenceCount();
}
