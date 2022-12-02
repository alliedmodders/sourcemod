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

#include "extension.h"
#include "cookie.h"
#include "menus.h"

cell_t RegClientPrefCookie(IPluginContext *pContext, const cell_t *params)
{
	g_ClientPrefs.AttemptReconnection();

	char *name;
	pContext->LocalToString(params[1], &name);

	if (name[0] == '\0')
	{
		return pContext->ThrowNativeError("Cannot create preference cookie with no name");
	}

	char *desc;
	pContext->LocalToString(params[2], &desc);

	Cookie *pCookie = g_CookieManager.CreateCookie(name, desc, (CookieAccess)params[3]);

	if (!pCookie)
	{
		return BAD_HANDLE;
	}

	return handlesys->CreateHandle(g_CookieType, 
		pCookie, 
		pContext->GetIdentity(), 
		myself->GetIdentity(), 
		NULL);
}

cell_t FindClientPrefCookie(IPluginContext *pContext, const cell_t *params)
{
	g_ClientPrefs.AttemptReconnection();

	char *name;
	pContext->LocalToString(params[1], &name);

	Cookie *pCookie = g_CookieManager.FindCookie(name);

	if (!pCookie)
	{
		return BAD_HANDLE;
	}

	return handlesys->CreateHandle(g_CookieType, 
		pCookie, 
		pContext->GetIdentity(), 
		myself->GetIdentity(), 
		NULL);
}

size_t IsAuthIdConnected(char *authID)
{
	IGamePlayer *player;
	const char *authString;
	
	for (int playerIndex = playerhelpers->GetMaxClients()+1; --playerIndex > 0;)
	{
		player = playerhelpers->GetGamePlayer(playerIndex);
		if (player == NULL || !player->IsAuthorized())
		{
			continue;
		}
		
		if (!strcmp(player->GetAuthString(), authID)
			|| !strcmp(player->GetSteam2Id(), authID)
			|| !strcmp(player->GetSteam3Id(), authID)
			)
		{
			return playerIndex;
		}
	}

	return 0;
}

cell_t SetAuthIdCookie(IPluginContext *pContext, const cell_t *params)
{
	g_ClientPrefs.AttemptReconnection();

	char *steamID;
	pContext->LocalToString(params[1], &steamID);

	// convert cookie handle to Cookie*
	Handle_t hndl = static_cast<Handle_t>(params[2]);
	HandleError err;
	HandleSecurity sec;
 
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	Cookie *pCookie;

	if ((err = handlesys->ReadHandle(hndl, g_CookieType, &sec, (void **)&pCookie))
	     != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Cookie handle %x (error %d)", hndl, err);
	}

	int i_dbId = pCookie->dbid;
	char *value;
	pContext->LocalToString(params[3], &value);

	// make sure the steamID isn't currently connected
	if (int client = IsAuthIdConnected(steamID))
	{
		// use regular code for connected players
		return g_CookieManager.SetCookieValue(pCookie, client, value);
	}

	// constructor calls strncpy for us
	CookieData *payload = new CookieData(value);

	// set changed so players connecting later in during the same map will have the correct value
	payload->changed = true;
	payload->timestamp = time(NULL);

	// edit database table
	TQueryOp *op = new TQueryOp(Query_InsertData, pCookie);
	// limit player auth length which doubles for cookie name length
	UTIL_strncpy(op->m_params.steamId, steamID, MAX_NAME_LENGTH);
	op->m_params.cookieId = i_dbId;
	op->m_params.data = payload;

	g_ClientPrefs.AddQueryToQueue(op);

	return 1;
}

cell_t SetClientPrefCookie(IPluginContext *pContext, const cell_t *params)
{
	g_ClientPrefs.AttemptReconnection();

	int client = params[1];

	if ((client < 1) || (client > playerhelpers->GetMaxClients()))
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}

	Handle_t hndl = static_cast<Handle_t>(params[2]);
	HandleError err;
	HandleSecurity sec;
 
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	Cookie *pCookie;

	if ((err = handlesys->ReadHandle(hndl, g_CookieType, &sec, (void **)&pCookie))
	     != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Cookie handle %x (error %d)", hndl, err);
	}

	char *value;
	pContext->LocalToString(params[3], &value);

	return g_CookieManager.SetCookieValue(pCookie, client, value);
}

cell_t GetClientPrefCookie(IPluginContext *pContext, const cell_t *params)
{
	g_ClientPrefs.AttemptReconnection();

	int client = params[1];

	if ((client < 1) || (client > playerhelpers->GetMaxClients()))
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}

	Handle_t hndl = static_cast<Handle_t>(params[2]);
	HandleError err;
	HandleSecurity sec;
 
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	Cookie *pCookie;

	if ((err = handlesys->ReadHandle(hndl, g_CookieType, &sec, (void **)&pCookie))
	     != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Cookie handle %x (error %d)", hndl, err);
	}
	
	char *value = NULL;

	g_CookieManager.GetCookieValue(pCookie, client, &value);

	pContext->StringToLocal(params[3], params[4], value);

	return 1;
}

cell_t AreClientCookiesCached(IPluginContext *pContext, const cell_t *params)
{
	g_ClientPrefs.AttemptReconnection();

	int client = params[1];

	if ((client < 1) || (client > playerhelpers->GetMaxClients()))
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}

	return g_CookieManager.AreClientCookiesCached(client);
}

cell_t GetCookieAccess(IPluginContext *pContext, const cell_t *params)
{
	g_ClientPrefs.AttemptReconnection();

	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;
 
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	Cookie *pCookie;

	if ((err = handlesys->ReadHandle(hndl, g_CookieType, &sec, (void **)&pCookie))
	     != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Cookie handle %x (error %d)", hndl, err);
	}

	return pCookie->access;
}

static cell_t GetCookieIterator(IPluginContext *pContext, const cell_t *params)
{
	g_ClientPrefs.AttemptReconnection();

	size_t *iter = new size_t;
	*iter = 0;

	Handle_t hndl = handlesys->CreateHandle(g_CookieIterator, iter, pContext->GetIdentity(), myself->GetIdentity(), NULL);
	if (hndl == BAD_HANDLE)
	{
		delete iter;
	}

	return hndl;
}

static cell_t ReadCookieIterator(IPluginContext *pContext, const cell_t *params)
{
	g_ClientPrefs.AttemptReconnection();

	size_t *iter;

	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	if ((err = handlesys->ReadHandle(hndl, g_CookieIterator, &sec, (void **)&iter))
	     != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Cookie iterator handle %x (error %d)", hndl, err);
	}

	if (*iter >= g_CookieManager.cookieList.size())
	{
		return 0;
	}

	Cookie *pCookie = g_CookieManager.cookieList[(*iter)++];

	pContext->StringToLocalUTF8(params[2], params[3], pCookie->name, NULL);
	pContext->StringToLocalUTF8(params[5], params[6], pCookie->description, NULL);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[4], &addr);
	*addr = pCookie->access;

	return 1;
}

cell_t ShowSettingsMenu(IPluginContext *pContext, const cell_t *params)
{
	g_ClientPrefs.AttemptReconnection();

	char message[256];
	Translate(message, sizeof(message), "%T:", 2, NULL, "Client Settings", &params[1]);

	g_CookieManager.clientMenu->SetDefaultTitle(message);
	g_CookieManager.clientMenu->Display(params[1], 0, NULL);

	return 0;
}

cell_t AddSettingsMenuItem(IPluginContext *pContext, const cell_t *params)
{
	g_ClientPrefs.AttemptReconnection();

	char *display;
	pContext->LocalToString(params[3], &display);

	/* Register a callback */
	ItemHandler *pItem = new ItemHandler;
	pItem->isAutoMenu = false;
	pItem->forward = forwards->CreateForwardEx(NULL, ET_Ignore, 5, NULL, Param_Cell, Param_Cell, Param_Cell, Param_String, Param_Cell);

	pItem->forward->AddFunction(pContext, static_cast<funcid_t>(params[1]));

	char info[20];
	AutoMenuData *data = new AutoMenuData;
	data->datavalue = params[2];
	data->handler = pItem;
	g_pSM->Format(info, sizeof(info), "%x", data);

	ItemDrawInfo draw(display, 0);

	g_CookieManager.clientMenu->AppendItem(info, draw);

	/* Track this in case the plugin unloads */

	IPlugin *pPlugin = plsys->FindPluginByContext(pContext->GetContext());
	std::vector<char *> *pList = NULL;

	if (!pPlugin->GetProperty("SettingsMenuItems", (void **)&pList, false) || !pList)
	{
		pList = new std::vector<char *>;
		pPlugin->SetProperty("SettingsMenuItems", pList);
	}

	char *copyarray = new char[strlen(display)+1];
	g_pSM->Format(copyarray, strlen(display)+1, "%s", display);

	pList->push_back(copyarray);

	return 0;
}

cell_t AddSettingsPrefabMenuItem(IPluginContext *pContext, const cell_t *params)
{
	g_ClientPrefs.AttemptReconnection();

	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;
 
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	Cookie *pCookie;

	if ((err = handlesys->ReadHandle(hndl, g_CookieType, &sec, (void **)&pCookie))
	     != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Cookie handle %x (error %d)", hndl, err);
	}

	/* Register a callback */
	ItemHandler *pItem = new ItemHandler;
	pItem->isAutoMenu = true;
	pItem->autoMenuType = (CookieMenu)params[2];


	/* User passed a function id for a callback */
	if (params[4] != -1)
	{
		pItem->forward = forwards->CreateForwardEx(NULL, ET_Ignore, 5, NULL, Param_Cell, Param_Cell, Param_Cell, Param_String, Param_Cell); 
		pItem->forward->AddFunction(pContext, static_cast<funcid_t>(params[4]));
	}
	else
	{
		pItem->forward = NULL;
	}

	char *display;
	pContext->LocalToString(params[3], &display);

	ItemDrawInfo draw(display, 0);

	char info[20];
	AutoMenuData *data = new AutoMenuData;
	data->datavalue = params[5];
	data->pCookie = pCookie;
	data->type = (CookieMenu)params[2];
	data->handler = pItem;
	g_pSM->Format(info, sizeof(info), "%x", data);

	g_CookieManager.clientMenu->AppendItem(info, draw);

	/* Track this in case the plugin unloads */

	IPlugin *pPlugin = plsys->FindPluginByContext(pContext->GetContext());
	std::vector<char *> *pList = NULL;

	if (!pPlugin->GetProperty("SettingsMenuItems", (void **)&pList, false) || !pList)
	{
		pList = new std::vector<char *>;
		pPlugin->SetProperty("SettingsMenuItems", pList);
	}

	char *copyarray = new char[strlen(display)+1];
	g_pSM->Format(copyarray, strlen(display)+1, "%s", display);

	pList->push_back(copyarray);

	return 0;
}

cell_t GetClientCookieTime(IPluginContext *pContext, const cell_t *params)
{
	g_ClientPrefs.AttemptReconnection();

	Handle_t hndl = static_cast<Handle_t>(params[2]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	Cookie *pCookie;

	if ((err = handlesys->ReadHandle(hndl, g_CookieType, &sec, (void **)&pCookie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Cookie handle %x (error %d)", hndl, err);
	}

	time_t value;

	if (!g_CookieManager.GetCookieTime(pCookie, params[1], &value))
	{
		return 0;
	}

	return value;
}

static cell_t Cookie_Set(IPluginContext *pContext, const cell_t *params)
{
	// This version takes (handle, client, value). The original is (client, handle, value).
	const cell_t new_params[4] = {
		3,
		params[2],
		params[1],
		params[3]
	};

	return SetClientPrefCookie(pContext, new_params);
}

static cell_t Cookie_Get(IPluginContext *pContext, const cell_t *params)
{
	// This verson takes (handle, client, buffer, maxlen). The original is (client, handle, buffer, maxlen).
	const cell_t new_params[5] = {
		4,
		params[2],
		params[1],
		params[3],
		params[4]
	};

	return GetClientPrefCookie(pContext, new_params);
}

static cell_t Cookie_SetByAuthId(IPluginContext *pContext, const cell_t *params)
{
	// This version takes (handle, authid, value). The original is (authid, handle, value).
	const cell_t new_params[4] = {
		3,
		params[2],
		params[1],
		params[3]
	};

	return SetAuthIdCookie(pContext, new_params);
}

static cell_t Cookie_GetClientTime(IPluginContext *pContext, const cell_t *params)
{
	// This version takes (handle, client). The original is (client, handle)
	const cell_t new_params[3] = {
		2,
		params[2],
		params[1],
	};

	return GetClientCookieTime(pContext, new_params);
}

sp_nativeinfo_t g_ClientPrefNatives[] = 
{
	{"RegClientCookie",             RegClientPrefCookie},
	{"FindClientCookie",            FindClientPrefCookie},
	{"SetClientCookie",             SetClientPrefCookie},
	{"SetAuthIdCookie",             SetAuthIdCookie},
	{"GetClientCookie",             GetClientPrefCookie},
	{"AreClientCookiesCached",      AreClientCookiesCached},
	{"GetCookieAccess",             GetCookieAccess},
	{"ReadCookieIterator",          ReadCookieIterator},
	{"GetCookieIterator",           GetCookieIterator},
	{"ShowCookieMenu",              ShowSettingsMenu},
	{"SetCookieMenuItem",           AddSettingsMenuItem},
	{"SetCookiePrefabMenu",         AddSettingsPrefabMenuItem},
	{"GetClientCookieTime",         GetClientCookieTime},

	{"Cookie.Cookie",               RegClientPrefCookie},
	{"Cookie.Find",                 FindClientPrefCookie},
	{"Cookie.Set",                  Cookie_Set},
	{"Cookie.Get",                  Cookie_Get},
	{"Cookie.SetByAuthId",          Cookie_SetByAuthId},
	{"Cookie.SetPrefabMenu",        AddSettingsPrefabMenuItem},
	{"Cookie.GetClientTime",        Cookie_GetClientTime},
	{"Cookie.AccessLevel.get",      GetCookieAccess},

	{NULL,                          NULL}
};
