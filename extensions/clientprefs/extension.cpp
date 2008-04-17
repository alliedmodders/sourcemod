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

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

ClientPrefs g_ClientPrefs;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_ClientPrefs);

HandleType_t g_CookieType = 0;
CookieTypeHandler g_CookieTypeHandler;


int driver = 0;

bool ClientPrefs::SDK_OnLoad(char *error, size_t maxlength, bool late)
{	
	const DatabaseInfo *DBInfo = dbi->FindDatabaseConf("clientprefs");

	if (DBInfo == NULL)
	{
		DBInfo = dbi->FindDatabaseConf("default");

		if (DBInfo == NULL)
		{
			snprintf(error, maxlength, "Could not find \"clientprefs\" or \"default\" database configs");
			return false;
		}

	}

	if (DBInfo->driver[0] != '\0')
	{
		Driver = dbi->FindOrLoadDriver(DBInfo->driver);
	}
	else
	{
		Driver = dbi->GetDefaultDriver();
	}

	if (Driver == NULL)
	{
		snprintf(error, maxlength, "Could not load DB Driver \"%s\"", DBInfo->driver);
		return false;
	}

	Database = Driver->Connect(DBInfo, true, error, maxlength);

	if (Database == NULL)
	{
		return false;
	}

	const char *identifier = Driver->GetIdentifier();

	if (strcmp(identifier, "sqlite") == 0)
	{
		driver = DRIVER_SQLITE;
	}
	else if (strcmp(identifier, "mysql") == 0)
	{
		driver = DRIVER_MYSQL;
	}
	else
	{
		snprintf(error, maxlength, "Unsupported driver \"%s\"", identifier);
		return false;
	}

	if (driver == DRIVER_SQLITE)
	{
		Query_InsertCookie = Database->PrepareQuery("INSERT OR IGNORE INTO sm_cookies(name, description) VALUES(?, ?)", error, maxlength);
	}
	else
	{
		Query_InsertCookie = Database->PrepareQuery("INSERT IGNORE INTO sm_cookies(name, description) VALUES(?, ?)", error, maxlength);
	}

	if (Query_InsertCookie == NULL)
	{
		return false;
	}

	Query_SelectData = Database->PrepareQuery("SELECT sm_cookies.name, sm_cookie_cache.value, sm_cookies.description FROM sm_cookies JOIN sm_cookie_cache ON sm_cookies.id = sm_cookie_cache.cookie_id WHERE player = ?", error, maxlength);

	if (Query_SelectData == NULL)
	{
		return false;
	}

	if (driver == DRIVER_SQLITE)
	{
		Query_InsertData = Database->PrepareQuery("INSERT OR REPLACE INTO sm_cookie_cache(player,cookie_id, value, timestamp) VALUES(?, ?, ?, ?)", error, maxlength);
	}
	else
	{
		Query_InsertData = Database->PrepareQuery("INSERT INTO sm_cookie_cache(player,cookie_id, value, timestamp) VALUES(?, ?, ?, ?) ON DUPLICATE KEY UPDATE value = ?, timestamp = ?", error, maxlength);
	}

	if (Query_InsertData == NULL)
	{
		return false;
	}

	sharesys->AddNatives(myself, g_ClientPrefNatives);
	sharesys->RegisterLibrary(myself, "clientprefs");
	g_CookieManager.cookiesLoadedForward = forwards->CreateForward("OnClientCookiesLoaded", ET_Ignore, 1, NULL, Param_Cell);

	g_CookieType = handlesys->CreateType("Cookie", 
		&g_CookieTypeHandler, 
		0, 
		NULL, 
		NULL, 
		myself->GetIdentity(), 
		NULL);

	return true;
}

void ClientPrefs::SDK_OnAllLoaded()
{
	playerhelpers->AddClientListener(&g_CookieManager);
}

void ClientPrefs::SDK_OnUnload()
{
	handlesys->RemoveType(g_CookieType, myself->GetIdentity());

	g_CookieManager.Unload();

	Query_InsertCookie->Destroy();
	Query_InsertData->Destroy();
	Query_SelectData->Destroy();

	Database->Close();

	forwards->ReleaseForward(g_CookieManager.cookiesLoadedForward);
}


