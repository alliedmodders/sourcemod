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

#include <sourcemod_version.h>
#include "extension.h"

using namespace ke;

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

bool ClientPrefs::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	DBInfo = dbi->FindDatabaseConf("clientprefs");

	if (DBInfo == NULL)
	{
		DBInfo = dbi->FindDatabaseConf("storage-local");
		if (DBInfo == NULL)
		{
			ke::SafeStrcpy(error, maxlength, "Could not find any suitable database configs");
			return false;
		}
	}
	
	if (DBInfo->driver && DBInfo->driver[0] != '\0')
	{
		Driver = dbi->FindOrLoadDriver(DBInfo->driver);
	}
	else
	{
		Driver = dbi->GetDefaultDriver();
	}

	if (Driver == NULL)
	{
		ke::SafeSprintf(error, maxlength, "Could not load DB Driver \"%s\"", DBInfo->driver);
		return false;
	}

	databaseLoading = true;
	TQueryOp *op = new TQueryOp(Query_Connect, 0);
	dbi->AddToThreadQueue(op, PrioQueue_High);

	dbi->AddDependency(myself, Driver);

	sharesys->AddNatives(myself, g_ClientPrefNatives);
	sharesys->RegisterLibrary(myself, "clientprefs");
	identity = sharesys->CreateIdentity(sharesys->CreateIdentType("ClientPrefs"), this);
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
	g_CookieManager.clientMenu = style->CreateMenu(&g_Handler, identity);
	g_CookieManager.clientMenu->SetDefaultTitle("Client Settings:");

	plsys->AddPluginsListener(&g_CookieManager);

	phrases = translator->CreatePhraseCollection();
	phrases->AddPhraseFile("clientprefs.phrases");
	phrases->AddPhraseFile("common.phrases");

	if (late)
	{
		this->CatchLateLoadClients();
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
	if (Database && (void *)pInterface == (void *)(Database->GetDriver()))
		Database = NULL;
}

void ClientPrefs::SDK_OnDependenciesDropped()
{
	// At this point, we're guaranteed that DBI has flushed the worker thread
	// for us, so no cookies should have outstanding queries.
	g_CookieManager.Unload();

	handlesys->RemoveType(g_CookieType, myself->GetIdentity());
	handlesys->RemoveType(g_CookieIterator, myself->GetIdentity());

	Database = NULL;

	if (g_CookieManager.cookieDataLoadedForward != NULL)
	{
		forwards->ReleaseForward(g_CookieManager.cookieDataLoadedForward);
		g_CookieManager.cookieDataLoadedForward = NULL;
	}

	if (g_CookieManager.clientMenu != NULL)
	{
		Handle_t menuHandle = g_CookieManager.clientMenu->GetHandle();
		
		if (menuHandle != BAD_HANDLE)
		{
			HandleSecurity sec = HandleSecurity(identity, identity);
			HandleError err = handlesys->FreeHandle(menuHandle, &sec);
			if (HandleError_None != err)
			{
				g_pSM->LogError(myself, "Error %d when attempting to free client menu handle", err);
			}
		}
		
		g_CookieManager.clientMenu = NULL;
	}

	if (phrases != NULL)
	{
		phrases->Destroy();
		phrases = NULL;
	}

	plsys->RemovePluginsListener(&g_CookieManager);
	playerhelpers->RemoveClientListener(&g_CookieManager);
}

void ClientPrefs::OnCoreMapStart(edict_t *pEdictList, int edictCount, int clientMax)
{
	this->AttemptReconnection();
}

void ClientPrefs::AttemptReconnection()
{
	if (Database || databaseLoading)
		return; /* We're already loading, or have loaded. */
	
	g_pSM->LogMessage(myself, "Attempting to reconnect to database...");
	databaseLoading = true;
	
	TQueryOp *op = new TQueryOp(Query_Connect, 0);
	dbi->AddToThreadQueue(op, PrioQueue_High);
	
	this->CatchLateLoadClients(); /* DB reconnection, we should check if we missed anyone... */
}

void ClientPrefs::DatabaseConnect()
{
	char error[256];
	int errCode = 0;

	Database = AdoptRef(Driver->Connect(DBInfo, true, error, sizeof(error)));

	if (!Database)
	{
		g_pSM->LogError(myself, error);
		databaseLoading = false;
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
	else if (strcmp(identifier, "pgsql") == 0)
	{
		g_DriverType = Driver_PgSQL;
		// PostgreSQL supports 'IF NOT EXISTS' as of 9.1
		if (!Database->DoSimpleQuery(
				"CREATE TABLE IF NOT EXISTS sm_cookies \
				( \
					id serial, \
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
					cookie_id int NOT NULL, \
					value varchar(100), \
					timestamp int NOT NULL, \
					PRIMARY KEY (player, cookie_id) \
				)"))
		{
			g_pSM->LogMessage(myself, "Failed to CreateTable sm_cookie_cache: %s", Database->GetError());
			goto fatal_fail;
		}

		if (!Database->DoSimpleQuery(
				"CREATE TABLE IF NOT EXISTS sm_cookie_cache \
				( \
					player varchar(65) NOT NULL, \
					cookie_id int NOT NULL, \
					value varchar(100), \
					timestamp int NOT NULL, \
					PRIMARY KEY (player, cookie_id) \
				)"))
		{
			g_pSM->LogMessage(myself, "Failed to CreateTable sm_cookie_cache: %s", Database->GetError());
			goto fatal_fail;
		}

		if (!Database->DoSimpleQuery(
				"CREATE OR REPLACE FUNCTION add_or_update_cookie(in_player VARCHAR(65), in_cookie INT, in_value VARCHAR(100), in_time INT) RETURNS VOID AS \
					$$ \
					BEGIN \
					  LOOP \
						UPDATE sm_cookie_cache SET value = in_value, timestamp = in_time WHERE player = in_player AND cookie_id = in_cookie; \
						IF found THEN \
						  RETURN; \
						END IF; \
						BEGIN \
						  INSERT INTO sm_cookie_cache (player, cookie_id, value, timestamp) VALUES (in_player, in_cookie, in_value, in_time); \
						  RETURN; \
						EXCEPTION WHEN unique_violation THEN \
						END; \
					  END LOOP; \
					END; \
					$$ LANGUAGE plpgsql;"))
		{
			g_pSM->LogMessage(myself, "Failed to create function add_or_update_cookie: %s", Database->GetError());
			goto fatal_fail;
		}
	}
	else
	{
		g_pSM->LogError(myself, "Unsupported driver \"%s\"", identifier);
		goto fatal_fail;
	}

	databaseLoading = false;

	// Need a new scope because of the goto above.
	{
		std::lock_guard<std::mutex> lock(queryLock);
		this->ProcessQueryCache();	
	}
	return;

fatal_fail:
	Database = NULL;
	databaseLoading = false;
}

bool ClientPrefs::AddQueryToQueue(TQueryOp *query)
{
	{
		std::lock_guard<std::mutex> lock(queryLock);
		if (!Database)
		{
			cachedQueries.push_back(query);
			return false;
		}
		
		if (!cachedQueries.empty())
			this->ProcessQueryCache();
	}

	query->SetDatabase(Database);
	dbi->AddToThreadQueue(query, PrioQueue_Normal);
	return true;
}

void ClientPrefs::ProcessQueryCache()
{
	if (!Database)
		return;

	for (size_t iter = 0; iter < cachedQueries.size(); ++iter)
	{
		TQueryOp *op = cachedQueries[iter];
		op->SetDatabase(Database);
		dbi->AddToThreadQueue(op, PrioQueue_Normal);
	}

	cachedQueries.clear();
}

const char *GetPlayerCompatAuthId(IGamePlayer *pPlayer)
{
	/* For legacy reasons, OnClientAuthorized gives the Steam2 id here if using Steam auth */
	const char *steamId = pPlayer->GetSteam2Id();
	return steamId ? steamId : pPlayer->GetAuthString();
}

void ClientPrefs::CatchLateLoadClients()
{
	IGamePlayer *pPlayer;
	for (int i = playerhelpers->GetMaxClients()+1; --i > 0;)
	{
		if (g_CookieManager.AreClientCookiesPending(i) || g_CookieManager.AreClientCookiesCached(i))
		{
			continue;
		}
	
		pPlayer = playerhelpers->GetGamePlayer(i);

		if (!pPlayer || !pPlayer->IsAuthorized())
		{
			continue;
		}

		g_CookieManager.OnClientAuthorized(i, GetPlayerCompatAuthId(pPlayer));
	}
}

void ClientPrefs::ClearQueryCache(int serial)
{
	std::lock_guard<std::mutex> lock(queryLock);

	for (size_t iter = 0; iter < cachedQueries.size(); ++iter)
	{
		TQueryOp *op = cachedQueries[iter];
		if (op && op->PullQueryType() == Query_SelectData && op->PullQuerySerial() == serial)
 		{
			op->Destroy();
			cachedQueries.erase(cachedQueries.begin() + iter);
			iter--;
		}
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

char * UTIL_strncpy(char * destination, const char * source, size_t num)
{
	if (source == NULL)
	{
		destination[0] = '\0';
		return destination;
	}
	
	size_t req = strlen(source);
	if (!req)
	{
		destination[0] = '\0';
		return destination;
	}
	else if (req >= num)
	{
		req = num-1;
	}
	
	strncpy(destination, source, req);
	destination[req] = '\0';
	return destination;
}

IdentityToken_t *ClientPrefs::GetIdentity() const
{
	return identity;
}

const char *ClientPrefs::GetExtensionVerString()
{
	return SOURCEMOD_VERSION;
}

const char *ClientPrefs::GetExtensionDateString()
{
	return SOURCEMOD_BUILD_TIME;
}

ClientPrefs::ClientPrefs()
{
	Driver = NULL;
	databaseLoading = false;
	phrases = NULL;
	DBInfo = NULL;

	identity = NULL;
}
