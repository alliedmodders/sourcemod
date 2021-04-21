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
	switch (m_type)
	{
		case Query_InsertCookie:
		{
			g_CookieManager.InsertCookieCallback(m_pCookie, m_insertId);
			break;
		}

		case Query_SelectData:
		{
			g_CookieManager.ClientConnectCallback(m_serial, m_pResult);
			break;
		}

		case Query_SelectId:
		{
			g_CookieManager.SelectIdCallback(m_pCookie, m_pResult);
			break;
		}

		case Query_Connect:
		{
			return;
		}
		
		default:
		{
			break;
		}
	}
}

void TQueryOp::RunThreadPart()
{
	if (m_type == Query_Connect)
	{
		g_ClientPrefs.DatabaseConnect();
		return;
	}
	
	assert(m_database != NULL);
	/* I don't think this is needed anymore... keeping for now. */
	m_database->LockForFullAtomicOperation();
		if (!BindParamsAndRun())
	{
		g_pSM->LogError(myself, 
						"Failed SQL Query, Error: \"%s\" (Query id %i - serial %i)", 
						m_database->GetError(),
						m_type, 
						m_serial);
	}

	m_database->UnlockFromFullAtomicOperation();
}

IDBDriver *TQueryOp::GetDriver()
{
	return m_driver;
}

IdentityToken_t *TQueryOp::GetOwner()
{
	return myself->GetIdentity();
}

void TQueryOp::Destroy()
{
	if (m_pResult != NULL)
	{
		m_pResult->Destroy();
	}
	delete this;
}

TQueryOp::TQueryOp(enum querytype type, int serial)
{
	m_type = type;
	m_serial = serial;
	m_database = NULL;
	m_driver = NULL;
	m_insertId = -1;
	m_pResult = NULL;
}

TQueryOp::TQueryOp(enum querytype type, Cookie *cookie)
{
	m_type = type;
	m_pCookie = cookie;
	m_database = NULL;
	m_driver = NULL;
	m_insertId = -1;
	m_pResult = NULL;
	m_serial = 0;
}

void TQueryOp::SetDatabase(IDatabase *db)
{
	m_database = db;
	m_driver = m_database->GetDriver();
}

bool TQueryOp::BindParamsAndRun()
{
	size_t ignore;
	char query[2048];

	switch (m_type)
	{
		case Query_InsertCookie:
		{
			char safe_name[MAX_NAME_LENGTH*2 + 1];
			char safe_desc[MAX_DESC_LENGTH*2 + 1];
			
			m_database->QuoteString(m_params.cookie->name, 
				safe_name, 
				sizeof(safe_name),
				&ignore);
			m_database->QuoteString(m_params.cookie->description,
				safe_desc,
				sizeof(safe_desc),
				&ignore);

			if (g_DriverType == Driver_MySQL)
			{
				g_pSM->Format(query,
					sizeof(query),
					"INSERT IGNORE INTO sm_cookies (name, description, access) \
					 VALUES (\"%s\", \"%s\", %d)",
					safe_name,
					safe_desc,
					m_params.cookie->access);
			}
			else if (g_DriverType == Driver_SQLite)
			{
				g_pSM->Format(query,
					sizeof(query),
					"INSERT OR IGNORE INTO sm_cookies (name, description, access) \
					 VALUES ('%s', '%s', %d)",
					 safe_name,
					 safe_desc,
					 m_params.cookie->access);
			}
			else if (g_DriverType == Driver_PgSQL)
			{
				// just insert. Returns error on already exists, so ignore the error.
				g_pSM->Format(query,
					sizeof(query),
					"INSERT INTO sm_cookies (name, description, access) \
					VALUES ('%s', '%s', %d)",
					safe_name,
					safe_desc,
					m_params.cookie->access);
			}

			if (!m_database->DoSimpleQuery(query))
			{
				return false;
			}

			m_insertId = m_database->GetInsertID();

			return true;
		}

		case Query_SelectData:
		{
			char safe_str[128];

			m_database->QuoteString(m_params.steamId, safe_str, sizeof(safe_str), &ignore);

			g_pSM->Format(query,
				sizeof(query),
				"SELECT sm_cookies.name, sm_cookie_cache.value, sm_cookies.description, \
						sm_cookies.access, sm_cookie_cache.timestamp 	\
				FROM sm_cookies				\
				JOIN sm_cookie_cache		\
				ON sm_cookies.id = sm_cookie_cache.cookie_id \
				WHERE player = '%s'",
				safe_str);

			m_pResult = m_database->DoQuery(query);

			return (m_pResult != NULL);
		}

		case Query_InsertData:
		{
			char safe_id[128];
			char safe_val[MAX_VALUE_LENGTH*2 + 1];

			m_database->QuoteString(m_params.steamId,
				safe_id,
				sizeof(safe_id),
				&ignore);
			m_database->QuoteString(m_params.data->value,
				safe_val,
				sizeof(safe_val),
				&ignore);

			if (g_DriverType == Driver_MySQL)
			{
				g_pSM->Format(query, 
					sizeof(query),
					"INSERT INTO sm_cookie_cache (player, cookie_id, value, timestamp) 	\
					 VALUES (\"%s\", %d, \"%s\", %d)									\
					 ON DUPLICATE KEY UPDATE											\
					 value = \"%s\", timestamp = %d",
					safe_id,
					m_params.cookieId,
					safe_val,
					(unsigned int)m_params.data->timestamp,
					safe_val,
					(unsigned int)m_params.data->timestamp);
			}
			else if (g_DriverType == Driver_SQLite)
			{
				g_pSM->Format(query,
					sizeof(query),
					"INSERT OR REPLACE INTO sm_cookie_cache						\
					 (player, cookie_id, value, timestamp)						\
					 VALUES ('%s', %d, '%s', %d)",
					safe_id,
					m_params.cookieId,
					safe_val,
					(unsigned int)m_params.data->timestamp);
			}
			else if (g_DriverType == Driver_PgSQL)
			{
				// Using a PL/Pgsql function, called add_or_update_cookie(),
				// since Postgres does not have an 'OR REPLACE' functionality.
				g_pSM->Format(query,
					sizeof(query),
					"SELECT add_or_update_cookie ('%s', %d, '%s', %d)",
					safe_id,
					m_params.cookieId,
					safe_val,
					(unsigned int)m_params.data->timestamp);
			}

			if (!m_database->DoSimpleQuery(query))
			{
				return false;
			}

			m_insertId = m_database->GetInsertID();

			return true;
		}

		case Query_SelectId:
		{
			char safe_name[MAX_NAME_LENGTH*2 + 1];

			/* the steamId var was actually used to store the name of the cookie - Save duplicating vars */
			m_database->QuoteString(m_params.steamId, 
				safe_name,
				sizeof(safe_name),
				&ignore);

			g_pSM->Format(query, 
				sizeof(query),
				"SELECT id FROM sm_cookies WHERE name = '%s'",
				safe_name);

			m_pResult = m_database->DoQuery(query);

			return (m_pResult != NULL);
		}

		default:
		{
			break;
		}
			
	}

	assert(false);

	return false;
}

querytype TQueryOp::PullQueryType()
{
	return m_type;
}

int TQueryOp::PullQuerySerial()
{
	return m_serial;
}

ParamData::~ParamData()
{
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

