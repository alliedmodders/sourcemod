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

#include <sourcemod_version.h>
#include "extension.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

ClientPrefs g_ClientPrefs;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_ClientPrefs);

HandleType_t g_CookieType = 0;
CookieTypeHandler g_CookieTypeHandler;

HandleType_t g_CookieIterator = 0;
CookieIteratorHandler g_CookieIteratorHandler;
DbDriver g_DriverType;
static const DatabaseInfo *storage_local = NULL;

bool ClientPrefs::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	queryMutex = threader->MakeMutex();
	cookieMutex = threader->MakeMutex();

	DBInfo = dbi->FindDatabaseConf("clientprefs");

	if (DBInfo == NULL)
	{
		DBInfo = dbi->FindDatabaseConf("default");

		if (DBInfo == NULL ||
			(strcmp(DBInfo->host, "localhost") == 0 &&
			 strcmp(DBInfo->database, "sourcemod") == 0 && 
			 strcmp(DBInfo->user, "root") == 0 &&
			 strcmp(DBInfo->pass, "") == 0 &&
			 strcmp(DBInfo->driver, "") == 0))
		{
			storage_local = dbi->FindDatabaseConf("storage-local");
			if (DBInfo == NULL)
			{
				DBInfo = storage_local;
			}
		}

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

	Database = NULL;
	databaseLoading = true;
	TQueryOp *op = new TQueryOp(Query_Connect, 0);
	dbi->AddToThreadQueue(op, PrioQueue_High);

	dbi->AddDependency(myself, Driver);

	sharesys->AddNatives(myself, g_ClientPrefNatives);
	sharesys->RegisterLibrary(myself, "clientprefs");
	g_CookieManager.cookieDataLoadedForward = forwards->CreateForward("OnClientCookiesCached", ET_Ignore, 1, NULL, Param_Cell);

	g_CookieType = handlesys->CreateType("Cookie", 
		&g_CookieTypeHandler, 
		0, 
		NULL, 
		NULL, 
		myself->GetIdentity(), 
		NULL);

	g_CookieIterator = handlesys->CreateType("CookieIterator", 
		&g_CookieIteratorHandler, 
		0, 
		NULL, 
		NULL, 
		myself->GetIdentity(), 
		NULL);

	IMenuStyle *style = menus->GetDefaultStyle();
	g_CookieManager.clientMenu = style->CreateMenu(&g_Handler, NULL);
	g_CookieManager.clientMenu->SetDefaultTitle("Client Settings:");

	plsys->AddPluginsListener(&g_CookieManager);

	phrases = translator->CreatePhraseCollection();
	phrases->AddPhraseFile("clientprefs.phrases");
	phrases->AddPhraseFile("common.phrases");

	if (late)
	{
		int maxclients = playerhelpers->GetMaxClients();

		for (int i = 1; i <= maxclients; i++)
		{
			IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(i);

			if (!pPlayer || !pPlayer->IsAuthorized())
			{
				continue;
			}

			g_CookieManager.OnClientAuthorized(i, pPlayer->GetAuthString());
		}
	}

	return true;
}

void ClientPrefs::SDK_OnAllLoaded()
{
	playerhelpers->AddClientListener(&g_CookieManager);
}

bool ClientPrefs::QueryInterfaceDrop(SMInterface *pInterface)
{
	if ((void *)pInterface == (void *)(Database->GetDriver()))
	{
		return false;
	}

	return true;
}

void ClientPrefs::NotifyInterfaceDrop(SMInterface *pInterface)
{
	if (Database != NULL && (void *)pInterface == (void *)(Database->GetDriver()))
	{
		Database->Close();
		Database = NULL;
	}
}

void ClientPrefs::SDK_OnUnload()
{
	handlesys->RemoveType(g_CookieType, myself->GetIdentity());
	handlesys->RemoveType(g_CookieIterator, myself->GetIdentity());

	g_CookieManager.Unload();

	if (Database != NULL)
	{
		Database->Close();
	}

	forwards->ReleaseForward(g_CookieManager.cookieDataLoadedForward);

	g_CookieManager.clientMenu->Destroy();

	phrases->Destroy();

	plsys->RemovePluginsListener(&g_CookieManager);
	playerhelpers->RemoveClientListener(&g_CookieManager);

	queryMutex->DestroyThis();
	cookieMutex->DestroyThis();
}

void ClientPrefs::DatabaseConnect()
{
	char error[256];
	int errCode = 0;

	Database = Driver->Connect(DBInfo, true, error, sizeof(error));

	if (Database == NULL &&
		DBInfo != storage_local &&
		storage_local != NULL)
	{
		DBInfo = storage_local;
		Database = Driver->Connect(DBInfo, true, error, sizeof(error));
	}

	if (Database == NULL)
	{
		g_pSM->LogError(myself, error);
		databaseLoading = false;
		ProcessQueryCache();
		return;
	}

	const char *identifier = Driver->GetIdentifier();

	if (strcmp(identifier, "sqlite") == 0)
	{
		g_DriverType = Driver_SQLite;

		if (!Database->DoSimpleQuery(
				"CREATE TABLE IF NOT EXISTS sm_cookies  \
				( \
					id INTEGER PRIMARY KEY AUTOINCREMENT, \
					name varchar(30) NOT NULL UNIQUE, \
					description varchar(255), \
					access INTEGER \
				)"))
		{
			g_pSM->LogMessage(myself, "Failed to CreateTable sm_cookies: %s", Database->GetError());
			goto fatal_fail;
		}

		if (!Database->DoSimpleQuery(
				"CREATE TABLE IF NOT EXISTS sm_cookie_cache \
				( \
					player varchar(65) NOT NULL, \
					cookie_id int(10) NOT NULL, \
					value varchar(100), \
					timestamp int, \
					PRIMARY KEY (player, cookie_id) \
				)"))
		{
			g_pSM->LogMessage(myself, "Failed to CreateTable sm_cookie_cache: %s", Database->GetError());
			goto fatal_fail;
		}
	}
	else if (strcmp(identifier, "mysql") == 0)
	{
		g_DriverType = Driver_MySQL;

		if (!Database->DoSimpleQuery(
				"CREATE TABLE IF NOT EXISTS sm_cookies \
				( \
					id INTEGER unsigned NOT NULL auto_increment, \
					name varchar(30) NOT NULL UNIQUE, \
					description varchar(255), \
					access INTEGER, \
					PRIMARY KEY (id) \
				)"))
		{
			g_pSM->LogMessage(myself, "Failed to CreateTable sm_cookies: %s", Database->GetError());
			goto fatal_fail;
		}

		if (!Database->DoSimpleQuery(
				"CREATE TABLE IF NOT EXISTS sm_cookie_cache \
				( \
					player varchar(65) NOT NULL, \
					cookie_id int(10) NOT NULL, \
					value varchar(100), \
					timestamp int NOT NULL, \
					PRIMARY KEY (player, cookie_id) \
				)"))
		{
			g_pSM->LogMessage(myself, "Failed to CreateTable sm_cookie_cache: %s", Database->GetError());
			goto fatal_fail;
		}
	}
	else
	{
		g_pSM->LogError(myself, "Unsupported driver \"%s\"", identifier);
		goto fatal_fail;
	}

	databaseLoading = false;

	ProcessQueryCache();

	return;

fatal_fail:
	Database->Close();
	Database = NULL;
	databaseLoading = false;
	ProcessQueryCache();
}

bool ClientPrefs::AddQueryToQueue( TQueryOp *query )
{
	queryMutex->Lock();

	if (Database == NULL && databaseLoading)
	{
		cachedQueries.push_back(query);
		queryMutex->Unlock();
		return true;
	}

	queryMutex->Unlock();

	if (Database)
	{
		query->SetDatabase(Database);
		dbi->AddToThreadQueue(query, PrioQueue_Normal);
		return true;
	}

	query->Destroy();

	/* If Database is NULL and we're not in the loading phase it must have failed - Can't do much */
	return false;
}

void ClientPrefs::ProcessQueryCache()
{
	SourceHook::List<TQueryOp *>::iterator iter;

	queryMutex->Lock();

	iter = cachedQueries.begin();
	
	while (iter != cachedQueries.end())
	{
		TQueryOp *op = *iter;

		if (Database != NULL)
		{
			op->SetDatabase(Database);
			dbi->AddToThreadQueue(op, PrioQueue_Normal);
		}
		else
		{
			op->Destroy();
		}

		iter++;
	}

	cachedQueries.clear();

	queryMutex->Unlock();
}

size_t IsAuthIdConnected(char *authID)
{
	IGamePlayer *player;
	int maxPlayers = playerhelpers->GetMaxClients();

	for (int playerIndex = 1; playerIndex <= maxPlayers; playerIndex++)
	{
		player = playerhelpers->GetGamePlayer(playerIndex);
		if (!player || !player->IsConnected())
		{
			continue;
		}
		const char *authString = player->GetAuthString();
		if (!authString || authString[0] == '\0')
		{
			continue;
		}

		if (strcmp(authString, authID) == 0)
		{
			return playerIndex;
		}
	}
	return 0;
}

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	size_t len = vsnprintf(buffer, maxlength, fmt, ap);
	va_end(ap);

	if (len >= maxlength)
	{
		buffer[maxlength - 1] = '\0';
		return (maxlength - 1);
	}
	else
	{
		return len;
	}
}

bool Translate(char *buffer, 
				   size_t maxlength,
				   const char *format,
				   unsigned int numparams,
				   size_t *pOutLength,
				   ...)
{
	va_list ap;
	unsigned int i;
	const char *fail_phrase;
	void *params[MAX_TRANSLATE_PARAMS];

	if (numparams > MAX_TRANSLATE_PARAMS)
	{
		assert(false);
		return false;
	}

	va_start(ap, pOutLength);
	for (i = 0; i < numparams; i++)
	{
		params[i] = va_arg(ap, void *);
	}
	va_end(ap);

	if (!g_ClientPrefs.phrases->FormatString(buffer,
		maxlength, 
		format, 
		params,
		numparams,
		pOutLength,
		&fail_phrase))
	{
		if (fail_phrase != NULL)
		{
			g_pSM->LogError(myself, "[SM] Could not find core phrase: %s", fail_phrase);
		}
		else
		{
			g_pSM->LogError(myself, "[SM] Unknown fatal error while translating a core phrase.");
		}

		return false;
	}

	return true;
}

const char *ClientPrefs::GetExtensionVerString()
{
	return SM_FULL_VERSION;
}

const char *ClientPrefs::GetExtensionDateString()
{
	return SM_BUILD_TIMESTAMP;
}

