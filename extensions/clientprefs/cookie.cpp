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

#include "cookie.h"
#include "menus.h"
#include "query.h"

CookieManager g_CookieManager;

CookieManager::CookieManager()
{
	for (int i=0; i<=MAXCLIENTS; i++)
	{
		connected[i] = false;
		statsLoaded[i] = false;
	}

	cookieDataLoadedForward = NULL;
	clientMenu = NULL;
}
CookieManager::~CookieManager(){}

void CookieManager::Unload()
{
	/* If clients are connected we should try save their data */

	int maxclients = playerhelpers->GetMaxClients();

	for (int i=1; i<=maxclients; i++)
	{
		if (connected[i])
		{
			OnClientDisconnecting(i);
		}
	}

	/* Find all cookies and delete them */
	
	SourceHook::List<Cookie *>::iterator _iter;

	Cookie *current;

	_iter = cookieList.begin();

	while (_iter != cookieList.end())
	{
		current = (Cookie *)*_iter;

		if (current == NULL)
		{
			_iter++;
			continue;
		}

		delete current;

		_iter = cookieList.erase(_iter);
	}
}

Cookie *CookieManager::FindCookie(const char *name)
{
	Cookie **pCookie = cookieTrie.retrieve(name);

	if (pCookie == NULL)
	{
		return NULL;
	}

	return *pCookie;
}


Cookie *CookieManager::CreateCookie(const char *name, const char *description, CookieAccess access)
{
	Cookie *pCookie = FindCookie(name);

	/* Check if cookie already exists */
	if (pCookie != NULL)
	{
		/* Update data fields to the provided values */
		strncpy(pCookie->description, description, MAX_DESC_LENGTH);
		pCookie->description[MAX_DESC_LENGTH-1] = '\0';

		pCookie->access = access;

		return pCookie;
	}

	/* First time cookie - Create from scratch */
	pCookie = new Cookie(name, description, access);
	cookieTrie.insert(name, pCookie);
	cookieList.push_back(pCookie);

	char quotedname[2 * MAX_NAME_LENGTH + 1];
	char quoteddesc[2 * MAX_DESC_LENGTH + 1];

	g_ClientPrefs.Database->QuoteString(pCookie->name, quotedname, sizeof(quotedname), NULL);
	g_ClientPrefs.Database->QuoteString(pCookie->description, quoteddesc, sizeof(quoteddesc), NULL);
	
	/* Attempt to insert cookie into the db and get its ID num */
	char query[300];
	if (driver == DRIVER_SQLITE)
	{
		UTIL_Format(query, sizeof(query), "INSERT OR IGNORE INTO sm_cookies(name, description, access) VALUES('%s', '%s', %i)", quotedname, quoteddesc, access);
	}
	else
	{
		UTIL_Format(query, sizeof(query), "INSERT IGNORE INTO sm_cookies(name, description, access) VALUES('%s', '%s', %i)", quotedname, quoteddesc, access);
	}

	TQueryOp *op = new TQueryOp(g_ClientPrefs.Database, query, Query_InsertCookie, pCookie);
	dbi->AddToThreadQueue(op, PrioQueue_Normal);

	return pCookie;
}

bool CookieManager::GetCookieValue(Cookie *pCookie, int client, char **value)
{
	if (pCookie == NULL)
	{
		return false;
	}

	CookieData *data = pCookie->data[client];

	/* Check if a value has been set before */
	if (data == NULL)
	{
		data = new CookieData("");
		data->parent = pCookie;
		clientData[client].push_back(data);
		pCookie->data[client] = data;
		data->changed = true;
	}

	*value = &data->value[0];

	return true;
}

bool CookieManager::SetCookieValue(Cookie *pCookie, int client, char *value)
{
	if (pCookie == NULL)
	{
		return false;
	}

	CookieData *data = pCookie->data[client];

	if (data == NULL)
	{
		data = new CookieData(value);
		data->parent = pCookie;
		clientData[client].push_back(data);
		pCookie->data[client] = data;
	}
	else
	{
		strncpy(data->value, value, MAX_VALUE_LENGTH);
		data->value[MAX_VALUE_LENGTH-1] = '\0';
	}

	data->changed = true;

	return true;
}

void CookieManager::OnClientAuthorized(int client, const char *authstring)
{
	connected[client] = true;

	char query[300];
	/* Assume that the authstring doesn't need to be quoted */
	UTIL_Format(query, sizeof(query), "SELECT sm_cookies.name, sm_cookie_cache.value, sm_cookies.description, sm_cookies.access FROM sm_cookies JOIN sm_cookie_cache ON sm_cookies.id = sm_cookie_cache.cookie_id WHERE player = '%s'", authstring);
	TQueryOp *op = new TQueryOp(g_ClientPrefs.Database, query, Query_SelectData, client);
	dbi->AddToThreadQueue(op, PrioQueue_Normal);
}

void CookieManager::OnClientDisconnecting(int client)
{
	connected[client] = false;
	statsLoaded[client] = false;

	SourceHook::List<CookieData *>::iterator _iter;

	CookieData *current;

	_iter = clientData[client].begin();

	while (_iter != clientData[client].end())
	{
		current = (CookieData *)*_iter;

		if (!current->changed)
		{
			_iter = clientData[client].erase(_iter);
			continue;
		}

		/* Save this cookie to the database */
		IGamePlayer *player = playerhelpers->GetGamePlayer(client);

		if (player == NULL)
		{
			/* panic! */
			return;
		}

		int dbId = current->parent->dbid;

		if (dbId == -1)
		{
			/* Insert/Find Query must be still running or failed. */
			return;
		}

		char quotedvalue[2 * MAX_VALUE_LENGTH + 1];
		g_ClientPrefs.Database->QuoteString(current->value, quotedvalue, sizeof(quotedvalue), NULL);

		char query[300];
		if (driver == DRIVER_SQLITE)
		{
			UTIL_Format(query, sizeof(query), "INSERT OR REPLACE INTO sm_cookie_cache(player,cookie_id, value, timestamp) VALUES('%s', %i, '%s', %i)", player->GetAuthString(), dbId, quotedvalue, time(NULL));
		}
		else
		{
			UTIL_Format(query, sizeof(query), "INSERT INTO sm_cookie_cache(player,cookie_id, value, timestamp) VALUES('%s', %i, '%s', %i) ON DUPLICATE KEY UPDATE value = '%s', timestamp = %i", player->GetAuthString(), dbId, quotedvalue, time(NULL), quotedvalue, time(NULL));
		}
	
		TQueryOp *op = new TQueryOp(g_ClientPrefs.Database, query, Query_InsertData, client);
		dbi->AddToThreadQueue(op, PrioQueue_Normal);

		current->parent->data[client] = NULL;
		delete current;

		_iter = clientData[client].erase(_iter);
	}
}

void CookieManager::ClientConnectCallback(int client, IQuery *data)
{
	IResultSet *results = data->GetResultSet();
	if (results == NULL)
	{
		return;
	}

	do
	{
		IResultRow *row = results->FetchRow();

		if (row == NULL)
		{
			continue;
		}
		
		const char *name;
		row->GetString(0, &name, NULL);

		const char *value;
		row->GetString(1, &value, NULL);

		CookieData *pData = new CookieData(value);
		pData->changed = false;

		Cookie *parent = FindCookie(name);

		if (parent == NULL)
		{
			const char *desc;
			row->GetString(2, &desc, NULL);

			CookieAccess access = CookieAccess_Public;
			row->GetInt(3, (int *)&access);

			parent = CreateCookie(name, desc, access);
			cookieTrie.insert(name, parent);
			cookieList.push_back(parent);
		}

		pData->parent = parent;
		parent->data[client] = pData;

		clientData[client].push_back(pData);

	}  while (results->MoreRows());


	statsLoaded[client] = true;

	cookieDataLoadedForward->PushCell(client);
	cookieDataLoadedForward->Execute(NULL);
}

void CookieManager::InsertCookieCallback(Cookie *pCookie, int dbId)
{
	if (dbId > 0)
	{
		pCookie->dbid = dbId;
		return;
	}

	char quotedname[2 * MAX_NAME_LENGTH + 1];
	g_ClientPrefs.Database->QuoteString(pCookie->name, quotedname, sizeof(quotedname), NULL);

	char query[300];
	UTIL_Format(query, sizeof(query), "SELECT id FROM sm_cookies WHERE name='%s'", quotedname);

	TQueryOp *op = new TQueryOp(g_ClientPrefs.Database, query, Query_SelectId, pCookie);
	dbi->AddToThreadQueue(op, PrioQueue_Normal);
}

void CookieManager::SelectIdCallback(Cookie *pCookie, IQuery *data)
{
	IResultSet *results = data->GetResultSet();
	if (results == NULL)
	{
		return;
	}

	IResultRow *row = results->FetchRow();

	if (row == NULL)
	{
		return;
	}

	row->GetInt(0, &pCookie->dbid);
}

bool CookieManager::AreClientCookiesCached(int client)
{
	return statsLoaded[client];
}

void CookieManager::OnPluginDestroyed(IPlugin *plugin)
{
	SourceHook::List<char *> *pList;

	if (plugin->GetProperty("SettingsMenuItems", (void **)&pList, true))
	{
		SourceHook::List<char *>::iterator p_iter = pList->begin();
		char *name;

		while (p_iter != pList->end())
		{
			name = (char *)*p_iter;
		
			p_iter = pList->erase(p_iter); //remove from this plugins list

			ItemDrawInfo draw;
			
			for (unsigned int i=0; i<clientMenu->GetItemCount(); i++)
			{
				const char *info = clientMenu->GetItemInfo(i, &draw);

				if (info == NULL)
				{
					continue;
				}

				if (strcmp(draw.display, name) == 0)
				{
					ItemDrawInfo draw;
					const char *info = clientMenu->GetItemInfo(i, &draw);
					AutoMenuData *data = (AutoMenuData *)strtol(info, NULL, 16);

					if (data->handler->forward != NULL)
					{
						forwards->ReleaseForward(data->handler->forward);
					}
					delete data->handler;
					delete data;

					clientMenu->RemoveItem(i);
					break;
				}
			}

			delete name;
		}		
	}
}
