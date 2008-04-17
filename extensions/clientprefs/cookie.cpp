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
#include "query.h"

CookieManager g_CookieManager;

CookieManager::CookieManager()
{
	for (int i=0; i<=MAXCLIENTS; i++)
	{
		connected[i] = false;
		statsLoaded[i] = false;
	}

	cookiesLoadedForward = NULL;
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


Cookie *CookieManager::CreateCookie(const char *name, const char *description)
{
	Cookie *pCookie = FindCookie(name);

	/* Check if cookie already exists */
	if (pCookie != NULL)
	{
		return pCookie;
	}

	/* First time cookie - Create from scratch */
	pCookie = new Cookie(name, description);
	cookieTrie.insert(name, pCookie);
	cookieList.push_back(pCookie);

	
	/* Attempt to load cookie from the db and get its ID num */
	g_ClientPrefs.Query_InsertCookie->BindParamString(0, name, true);
	g_ClientPrefs.Query_InsertCookie->BindParamString(1, description, true);
	TQueryOp *op = new TQueryOp(g_ClientPrefs.Database, g_ClientPrefs.Query_InsertCookie, Query_InsertCookie, pCookie);
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

	g_ClientPrefs.Query_SelectData->BindParamString(0, authstring, true);
	TQueryOp *op = new TQueryOp(g_ClientPrefs.Database, g_ClientPrefs.Query_SelectData, Query_SelectData, client);
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

		g_ClientPrefs.Query_InsertData->BindParamString(0, player->GetAuthString(), true);
		g_ClientPrefs.Query_InsertData->BindParamInt(1, dbId, false);
		g_ClientPrefs.Query_InsertData->BindParamString(2, current->value, true);
		g_ClientPrefs.Query_InsertData->BindParamInt(3, time(NULL), false);

		if (driver == DRIVER_MYSQL)
		{
			g_ClientPrefs.Query_InsertData->BindParamString(4, current->value, true);
			g_ClientPrefs.Query_InsertData->BindParamInt(5, time(NULL), false);
		}
	
		TQueryOp *op = new TQueryOp(g_ClientPrefs.Database, g_ClientPrefs.Query_InsertData, Query_InsertData, client);
		dbi->AddToThreadQueue(op, PrioQueue_Normal);

		current->parent->data[client] = NULL;
		delete current;

		_iter = clientData[client].erase(_iter);
	}
}

void CookieManager::ClientConnectCallback(int client, IPreparedQuery *data)
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

			parent = CreateCookie(name, desc);
			cookieTrie.insert(name, parent);
			cookieList.push_back(parent);
		}

		pData->parent = parent;
		parent->data[client] = pData;

		clientData[client].push_back(pData);

	}  while (results->MoreRows());

	statsLoaded[client] = true;

	cookiesLoadedForward->PushCell(client);
	cookiesLoadedForward->Execute(NULL);
}

void CookieManager::InsertCookieCallback(Cookie *pCookie, int dbId)
{
	pCookie->dbid = dbId;
}

bool CookieManager::AreClientCookiesCached(int client)
{
	return statsLoaded[client];
}

