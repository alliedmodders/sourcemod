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

	cookieDataLoadedForward = nullptr;
	clientMenu = nullptr;
}
CookieManager::~CookieManager() {}

void CookieManager::Unload()
{
	/* If clients are connected we should try save their data */
	for (int i = playerhelpers->GetMaxClients()+1; --i > 0;)
	{
		if (connected[i])
			OnClientDisconnecting(i);
	}

	cookieList.clear();
}

Cookie *CookieManager::FindCookie(std::string const &name)
{
	Cookie *cookie;
	if (!cookieFinder.retrieve(name.c_str(), &cookie))
		return nullptr;
	return cookie;
}

Cookie *CookieManager::CreateCookie(std::string const &name, std::string const &description, CookieAccess access)
{
	Cookie *pCookie = FindCookie(name);

	/* Check if cookie already exists */
	if (pCookie != nullptr)
	{
		/* Update data fields to the provided values */
		pCookie->description = description;
		pCookie->access = access;

		return pCookie;
	}

	/* First time cookie - Create from scratch */
	pCookie = new Cookie(name, description, access);
	
	/* Attempt to insert cookie into the db and get its ID num */
	TQueryOp *op = new TQueryOp(Query_InsertCookie, pCookie);
	op->m_params.cookie = pCookie;
	
	cookieFinder.insert(name.c_str(), pCookie);
	cookieList.emplace_back(pCookie);

	g_ClientPrefs.AddQueryToQueue(op);

	return pCookie;
}

bool CookieManager::GetCookieValue(Cookie *pCookie, int client, std::string &value)
{
	auto &data = pCookie->data[client];

	/* Check if a value has been set before */
	if (data == nullptr)
	{
		data.reset(new CookieData());
		data->parent = pCookie;
		data->changed = false;
		data->timestamp = 0;
	}

	value = data->val;
	return true;
}

bool CookieManager::SetCookieValue(Cookie *pCookie, int client, std::string const &value)
{
	auto &data = pCookie->data[client];

	if (data == nullptr)
	{
		data.reset(new CookieData());
		data->parent = pCookie;
	}

	data->val = value;
	data->changed = true;
	data->timestamp = time(nullptr);
	return true;
}

void CookieManager::OnClientAuthorized(int client, const char *authstring)
{
	IGamePlayer *player = playerhelpers->GetGamePlayer(client);
	if (player == nullptr)
	{
		return;
	}

	connected[client] = true;
	statsPending[client] = true;

	g_ClientPrefs.AttemptReconnection();
	
	TQueryOp *op = new TQueryOp(Query_SelectData, player->GetSerial());
	op->m_params.steamId = GetPlayerCompatAuthId(player);

	g_ClientPrefs.AddQueryToQueue(op);
}

void CookieManager::OnClientDisconnecting(int client)
{
	connected[client] = false;
	statsLoaded[client] = false;
	statsPending[client] = false;

	g_ClientPrefs.AttemptReconnection();
	
	/* Save this cookie to the database */
	IGamePlayer *player = playerhelpers->GetGamePlayer(client);
	const char *pAuth = nullptr;
	if (player)
	{
		pAuth = GetPlayerCompatAuthId(player);
		g_ClientPrefs.ClearQueryCache(player->GetSerial());
	}
	
	/* Loop all known cookies */
	for (auto &cookie : cookieList)
	{
		/* Skip cookies that clients have no data in */
		auto &data = cookie->data[client];
		if (data == nullptr)
			continue;

		/* If we can't or shouldn't save, then dont. */
		int dbId = cookie->dbid;
		if (player == nullptr || pAuth == nullptr || !data->changed || dbId == -1)
		{
			data.reset();
			continue;
		}

		/* Send query out */
		TQueryOp *op = new TQueryOp(Query_InsertData, client);
		op->m_params.steamId = pAuth;
		op->m_params.cookieId = dbId;
		op->m_params.data = std::move(data);

		g_ClientPrefs.AddQueryToQueue(op);
		data.reset();
	}
}

void CookieManager::ClientConnectCallback(int serial, IQuery *data)
{
	/* Check validity of client */
	int client = playerhelpers->GetClientFromSerial(serial);
	if (!client)
		return;

	statsPending[client] = false;
	
	if (data == nullptr)
		return;

	/* Check validity of results */
	IResultSet *results = data->GetResultSet();
	if (results  == nullptr)
		return;

	IResultRow *row;
	unsigned int timestamp;
	CookieAccess access;
	
	while (results->MoreRows() && ((row = results->FetchRow()) != nullptr))
	{
		const char *name = "";
		row->GetString(0, &name, nullptr);
		
		const char *value = "";
		row->GetString(1, &value, nullptr);

		Cookie *parent = FindCookie(name);
		if (parent == nullptr)
		{
			const char *desc = "";
			row->GetString(2, &desc, nullptr);

			access = CookieAccess_Public;
			row->GetInt(3, (int *)&access);

			parent = CreateCookie(name, desc, access);
		}

		auto &data = parent->data[client];
		data.reset(new CookieData(value));
		data->changed = false;
		data->timestamp = (row->GetInt(4, (int *)&timestamp) == DBVal_Data) ? timestamp : 0;
		data->parent = parent;
	}

	statsLoaded[client] = true;

	cookieDataLoadedForward->PushCell(client);
	cookieDataLoadedForward->Execute(nullptr);
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
	op->m_params.steamId = pCookie->name;
	g_ClientPrefs.AddQueryToQueue(op);
}

void CookieManager::SelectIdCallback(Cookie *pCookie, IQuery *data)
{
	if (data == nullptr)
		return;

	IResultSet *results = data->GetResultSet();	
	if (results == nullptr)
		return;

	IResultRow *row = results->FetchRow();
	if (row == nullptr)
		return;

	row->GetInt(0, &pCookie->dbid);
}

void CookieManager::OnPluginDestroyed(IPlugin *plugin)
{
	ke::Vector<char *> *pList;

	if (plugin->GetProperty("SettingsMenuItems", (void **)&pList, true))
	{
		ke::Vector<char *> &menuitems = (*pList);
		for (char const *name : menuitems)
		{
			//remove from this plugins list
			size_t itemcount = clientMenu->GetItemCount();
			for (size_t i = 0; i < itemcount; i++)
			{
				ItemDrawInfo draw;
				const char *info = clientMenu->GetItemInfo(i, &draw);
				if (info == nullptr)
					continue;

				if (strcmp(draw.display, name) == 0)
				{
					auto *data = reinterpret_cast<AutoMenuData *>(strtoul(info, nullptr, 16));
					if (data->handler->forward != nullptr)
					{
						forwards->ReleaseForward(data->handler->forward);
					}

					delete data->handler;
					delete data;

					clientMenu->RemoveItem(i);
					break;
				}
			}

			delete []name;
		}
		
		menuitems.clear();
	}
}

bool CookieManager::GetCookieTime(Cookie *pCookie, int client, time_t &value)
{
	auto &data = pCookie->data[client];

	/* Check if a value has been set before */
	if (data == nullptr)
	{
		return false;
	}

	value = data->timestamp;
	return true;
}
