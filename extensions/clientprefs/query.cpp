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

	if (m_type == Query_Connect)
	{
		return;
	}

	if (m_pQuery)
	{
		switch (m_type)
		{
			case Query_InsertCookie:
			{
				g_CookieManager.InsertCookieCallback(m_pCookie, m_insertId);
				break;
			}

			case Query_SelectData:
			{
				g_CookieManager.ClientConnectCallback(m_client, m_pQuery);
				break;
			}

			case Query_SelectId:
			{
				g_CookieManager.SelectIdCallback(m_pCookie, m_pQuery);
				break;
			}

			default:
			{
				break;
			}
		}
	}
}

void TQueryOp::RunThreadPart()
{
	if (m_type == Query_Connect)
	{
		g_ClientPrefs.DatabaseConnect();
	}
	else
	{
		if (m_database == NULL)
		{
			return;
		}

		m_database->LockForFullAtomicOperation();

		if (!BindParamsAndRun())
		{
			g_pSM->LogError(myself, "Failed SQL Query, Error: \"%s\" (Query id %i - client %i)", m_database->GetError(), m_type, m_client);
		}

		m_insertId = m_database->GetInsertID();

		m_database->UnlockFromFullAtomicOperation();
	}
}

IDBDriver *TQueryOp::GetDriver()
{
	if (m_database == NULL)
	{
		return NULL;
	}

	return m_database->GetDriver();
}
IdentityToken_t *TQueryOp::GetOwner()
{
	return myself->GetIdentity();
}

void TQueryOp::Destroy()
{
	delete this;
}

TQueryOp::TQueryOp(enum querytype type, int client)
{
	m_type = type;
	m_client = client;
	m_pQuery = NULL;
	m_database = NULL;
}

TQueryOp::TQueryOp(enum querytype type, Cookie *cookie)
{
	m_type = type;
	m_pCookie = cookie;
	m_pQuery = NULL;
	m_database = NULL;
}

void TQueryOp::SetDatabase( IDatabase *db )
{
	m_database = db;
	m_database->IncReferenceCount();
}

bool TQueryOp::BindParamsAndRun()
{
	if (m_pQuery == NULL)
	{
		g_pSM->LogError(myself, "Attempted to run with a NULL Query");
		return false;
	}

	switch (m_type)
	{
		case Query_InsertCookie:
		{
			m_pQuery->BindParamString(0, m_params.cookie->name, false);
			m_pQuery->BindParamString(1, m_params.cookie->description, false);
			m_pQuery->BindParamInt(2, m_params.cookie->access);

			break;
		}

		case Query_SelectData:
		{
			m_pQuery->BindParamString(0, m_params.steamId, false);

			break;
		}

		case Query_InsertData:
		{
			m_pQuery->BindParamString(0, m_params.steamId, false);
			m_pQuery->BindParamInt(1, m_params.cookieId);
			m_pQuery->BindParamString(2, m_params.data->value, false);
			m_pQuery->BindParamInt(3, (unsigned int)time(NULL), false);

			if (driver == DRIVER_MYSQL)
			{
				m_pQuery->BindParamString(4, m_params.data->value, false);
				m_pQuery->BindParamInt(5, (unsigned int)time(NULL), false);
			}

			break;
		}

		case Query_SelectId:
		{
			/* the steamId var was actually used to store the name of the cookie - Save duplicating vars */
			m_pQuery->BindParamString(0, m_params.steamId, false);

			break;
		}

		default:
		{
			break;
		}
			
	}

	return m_pQuery->Execute();
}

void TQueryOp::SetPreparedQuery()
{
	switch (m_type)
	{
		case Query_InsertCookie:
		{
			m_pQuery = g_ClientPrefs.InsertCookieQuery;
			break;
		}

		case Query_SelectData:
		{
			m_pQuery = g_ClientPrefs.SelectDataQuery;
			break;
		}

		case Query_InsertData:
		{
			m_pQuery = g_ClientPrefs.InsertDataQuery;
			break;
		}

		case Query_SelectId:
		{
			m_pQuery = g_ClientPrefs.SelectIdQuery;
			break;
		}

		default:
		{
			break;
		}

	}
}

ParamData::~ParamData()
{
	if (cookie)
	{
		g_ClientPrefs.cookieMutex->Lock();
		cookie->usedInQuery--;

		if (cookie->shouldDelete && cookie->usedInQuery <= 0)
		{
			g_ClientPrefs.cookieMutex->Unlock();
			delete cookie;
			cookie = NULL;
		}

		g_ClientPrefs.cookieMutex->Unlock();	
	}

	if (data)
	{
		/* Data is only ever passed in a client disconnect query and always needs to be deleted */
		delete data;
		data = NULL;
	}
}

ParamData::ParamData()
{
	cookie = NULL;
	data = NULL;
	steamId[0] = '\0';
	cookieId = 0;
}
