/**
 * vim: set ts=4 sw=4 tw=99 noet:
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
	for (int i=0; i<=SM_MAXPLAYERS; i++)
	{
		connected[i] = false;
		statsLoaded[i] = false;
		statsPending[i] = false;
	}

	cookieDataLoadedForward = NULL;
	clientMenu = NULL;
}
CookieManager::~CookieManager(){}

void CookieManager::Unload()
{
	/* If clients are connected we should try save their data */
	for (int i = playerhelpers->GetMaxClients()+1; --i > 0;)
	{
		if (connected[i])
			OnClientDisconnecting(i);
	}

	/* Find all cookies and delete them */
	for (size_t iter = 0; iter < cookieList.size(); ++iter)
		delete cookieList[iter];
	
	cookieList.clear();
}

Cookie *CookieManager::FindCookie(const char *name)
{
	Cookie *cookie;
	if (!cookieFinder.retrieve(name, &cookie))
		return NULL;
	return cookie;
}

Cookie *CookieManager::CreateCookie(const char *name, const char *description, CookieAccess access)
{
	Cookie *pCookie = FindCookie(name);

	/* Check if cookie already exists */
	if (pCookie != NULL)
	{
		/* Update data fields to the provided values */
		UTIL_strncpy(pCookie->description, description, MAX_DESC_LENGTH);
		pCookie->access = access;

		return pCookie;
	}

	/* First time cookie - Create from scratch */
	pCookie = new Cookie(name, description, access);
	
	/* Attempt to insert cookie into the db and get its ID num */
	TQueryOp *op = new TQueryOp(Query_InsertCookie, pCookie);
	op->m_params.cookie = pCookie;
	
	cookieFinder.insert(name, pCookie);
	cookieList.push_back(pCookie);

	g_ClientPrefs.AddQueryToQueue(op);

	return pCookie;
}

bool CookieManager::GetCookieValue(Cookie *pCookie, int client, char **value)
{
	CookieData *data = pCookie->data[client];

	/* Check if a value has been set before */
	if (data == NULL)
	{
		data = new CookieData("");
		data->parent = pCookie;
		clientData[client].push_back(data);
		pCookie->data[client] = data;
		data->changed = false;
		data->timestamp = 0;
	}

	*value = &data->value[0];

	return true;
}

bool CookieManager::SetCookieValue(Cookie *pCookie, int client, const char *value)
{
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
		UTIL_strncpy(data->value, value, MAX_VALUE_LENGTH);
	}

	data->changed = true;
	data->timestamp = time(NULL);

	return true;
}

void CookieManager::OnClientAuthorized(int client, const char *authstring)
{
	IGamePlayer *player = playerhelpers->GetGamePlayer(client);

	if (player == NULL)
	{
		return;
	}

	connected[client] = true;
	statsPending[client] = true;

	g_ClientPrefs.AttemptReconnection();
	
	TQueryOp *op = new TQueryOp(Query_SelectData, player->GetSerial());
	UTIL_strncpy(op->m_params.steamId, GetPlayerCompatAuthId(player), MAX_NAME_LENGTH);

	g_ClientPrefs.AddQueryToQueue(op);
}

void CookieManager::OnClientDisconnecting(int client)
{
	connected[client] = false;
	statsLoaded[client] = false;
	statsPending[client] = false;

	CookieData *current = NULL;

	g_ClientPrefs.AttemptReconnection();
	
	/* Save this cookie to the database */
	IGamePlayer *player = playerhelpers->GetGamePlayer(client);
	const char *pAuth = NULL;
	int dbId;
	
	if (player)
	{
		pAuth = GetPlayerCompatAuthId(player);
		g_ClientPrefs.ClearQueryCache(player->GetSerial());
	}

	std::vector<CookieData *> &clientvec = clientData[client];
	for (size_t iter = 0; iter < clientvec.size(); ++iter)
	{
		current = clientvec[iter];
		dbId = current->parent->dbid;
		
		if (player == NULL || pAuth == NULL || !current->changed || dbId == -1)
		{
			current->parent->data[client] = NULL;
			delete current;
			continue;
		}

		TQueryOp *op = new TQueryOp(Query_InsertData, client);

		UTIL_strncpy(op->m_params.steamId, pAuth, MAX_NAME_LENGTH);
		op->m_params.cookieId = dbId;
		op->m_params.data = current;

		g_ClientPrefs.AddQueryToQueue(op);

		current->parent->data[client] = NULL;
	}
	
	clientvec.clear();
}

void CookieManager::ClientConnectCallback(int serial, IQuery *data)
{
	int client;

	/* Check validity of client */
	if ((client = playerhelpers->GetClientFromSerial(serial)) == 0)
	{
		return;
	}
	statsPending[client] = false;
	
	IResultSet *results;
	/* Check validity of results */
	if (data == NULL || (results = data->GetResultSet()) == NULL)
	{
		return;
	}

	CookieData *pData;
	IResultRow *row;
	unsigned int timestamp;
	CookieAccess access;
	
	while (results->MoreRows() && ((row = results->FetchRow()) != NULL))
	{
		const char *name = "";
		row->GetString(0, &name, NULL);
		
		const char *value = "";
		row->GetString(1, &value, NULL);

		pData = new CookieData(value);
		pData->changed = false;

		pData->timestamp = (row->GetInt(4, (int *)&timestamp) == DBVal_Data) ? timestamp : 0;

		Cookie *parent = FindCookie(name);

		if (parent == NULL)
		{
			const char *desc = "";
			row->GetString(2, &desc, NULL);

			access = CookieAccess_Public;
			row->GetInt(3, (int *)&access);

			parent = CreateCookie(name, desc, access);
		}

		pData->parent = parent;
		parent->data[client] = pData;
		clientData[client].push_back(pData);
	}

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

	TQueryOp *op = new TQueryOp(Query_SelectId, pCookie);
	/* Put the cookie name into the steamId field to save space - Make sure we remember that it's there */
	UTIL_strncpy(op->m_params.steamId, pCookie->name, MAX_NAME_LENGTH);
	g_ClientPrefs.AddQueryToQueue(op);
}

void CookieManager::SelectIdCallback(Cookie *pCookie, IQuery *data)
{
	IResultSet *results;
	
	if (data == NULL || (results = data->GetResultSet()) == NULL)
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

bool CookieManager::AreClientCookiesPending(int client)
{
	return statsPending[client];
}

void CookieManager::OnPluginDestroyed(IPlugin *plugin)
{
	std::vector<char *> *pList;

	if (plugin->GetProperty("SettingsMenuItems", (void **)&pList, true))
	{
		std::vector<char *> &menuitems = (*pList);
		char *name;
		ItemDrawInfo draw;
		const char *info;
		AutoMenuData * data;
		unsigned itemcount;
		
		for (size_t p_iter = 0; p_iter < menuitems.size(); ++p_iter)
		{
			name = menuitems[p_iter];
			itemcount = clientMenu->GetItemCount();
			//remove from this plugins list
			for (unsigned int i=0; i < itemcount; i++)
			{
				info = clientMenu->GetItemInfo(i, &draw);

				if (info == NULL)
				{
					continue;
				}

				if (strcmp(draw.display, name) == 0)
				{
					data = (AutoMenuData *)strtoul(info, NULL, 16);

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

			delete [] name;
		}
		
		menuitems.clear();
	}
}

bool CookieManager::GetCookieTime(Cookie *pCookie, int client, time_t *value)
{
	CookieData *data = pCookie->data[client];

	/* Check if a value has been set before */
	if (data == NULL)
	{
		return false;
	}

	*value = data->timestamp;

	return true;
}
