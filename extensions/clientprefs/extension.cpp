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

HandleType_t g_CookieIterator = 0;
CookieIteratorHandler g_CookieIteratorHandler;


int driver = 0;

bool ClientPrefs::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	queryMutex = threader->MakeMutex();
	cookieMutex = threader->MakeMutex();

	DBInfo = dbi->FindDatabaseConf("clientprefs");

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
		InsertCookieQuery->Destroy();
		SelectDataQuery->Destroy();
		SelectIdQuery->Destroy();
		InsertDataQuery->Destroy();

		InsertCookieQuery = NULL;
		SelectDataQuery = NULL;
		SelectIdQuery = NULL;
		InsertDataQuery = NULL;

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

	/* Kill all our prepared queries - Queries are guaranteed to be flushed before this is called */

	if (InsertCookieQuery != NULL)
	{
		InsertCookieQuery->Destroy();
	}

	if (SelectDataQuery != NULL)
	{
		SelectDataQuery->Destroy();
	}

	if (SelectIdQuery != NULL)
	{
		SelectIdQuery->Destroy();
	}

	if (InsertDataQuery != NULL)
	{
		InsertDataQuery->Destroy();
	}

	queryMutex->DestroyThis();
	cookieMutex->DestroyThis();
}

void ClientPrefs::DatabaseConnect()
{
	char error[256];
	int errCode = 0;

	Database = Driver->Connect(DBInfo, true, error, sizeof(error));

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
		driver = DRIVER_SQLITE;

		TQueryOp *op = new TQueryOp(Query_CreateTable, 0);

		op->SetDatabase(Database);
		IPreparedQuery *pQuery = Database->PrepareQuery(
				"CREATE TABLE IF NOT EXISTS sm_cookies  \
				( \
					id INTEGER PRIMARY KEY AUTOINCREMENT, \
					name varchar(30) NOT NULL UNIQUE, \
					description varchar(255), \
					access INTEGER \
				)",
				error, sizeof(error), &errCode);

		if (pQuery == NULL)
		{
			g_pSM->LogMessage(myself, "Failed to prepare query CreateTable sm_cookies: %s (%i)", error, errCode);
			return;
		}

		op->SetCustomPreparedQuery(pQuery);

		dbi->AddToThreadQueue(op, PrioQueue_High);

		op = new TQueryOp(Query_CreateTable, 0);
		op->SetDatabase(Database);

		pQuery = Database->PrepareQuery(
				"CREATE TABLE IF NOT EXISTS sm_cookie_cache \
				( \
					player varchar(65) NOT NULL, \
					cookie_id int(10) NOT NULL, \
					value varchar(100), \
					timestamp int, \
					PRIMARY KEY (player, cookie_id) \
				)",
				error, sizeof(error), &errCode);

		if (pQuery == NULL)
		{
			g_pSM->LogMessage(myself, "Failed to prepare query CreateTable sm_cookie_cache: %s (%i)", error, errCode);
			return;
		}

		op->SetCustomPreparedQuery(pQuery);

		dbi->AddToThreadQueue(op, PrioQueue_High);
	}
	else if (strcmp(identifier, "mysql") == 0)
	{
		driver = DRIVER_MYSQL;

		TQueryOp *op = new TQueryOp(Query_CreateTable, 0);
		op->SetDatabase(Database);
		
		IPreparedQuery *pQuery = Database->PrepareQuery(
				"CREATE TABLE IF NOT EXISTS sm_cookies \
				( \
					id INTEGER unsigned NOT NULL auto_increment, \
					name varchar(30) NOT NULL UNIQUE, \
					description varchar(255), \
					access INTEGER, \
					PRIMARY KEY (id) \
				)",
				error, sizeof(error), &errCode);

		if (pQuery == NULL)
		{
			g_pSM->LogMessage(myself, "Failed to prepare query CreateTable sm_cookies: %s (%i)", error, errCode);
			return;
		}

		op->SetCustomPreparedQuery(pQuery);

		dbi->AddToThreadQueue(op, PrioQueue_High);

		op = new TQueryOp(Query_CreateTable, 0);
		op->SetDatabase(Database);
			
		pQuery = Database->PrepareQuery(
				"CREATE TABLE IF NOT EXISTS sm_cookie_cache \
				( \
					player varchar(65) NOT NULL, \
					cookie_id int(10) NOT NULL, \
					value varchar(100), \
					timestamp int NOT NULL, \
					PRIMARY KEY (player, cookie_id) \
				)",
				error, sizeof(error), &errCode);

		if (pQuery == NULL)
		{
			g_pSM->LogMessage(myself, "Failed to prepare query CreateTable sm_cookie_cache: %s (%i)", error, errCode);
			return;
		}

		op->SetCustomPreparedQuery(pQuery);

		dbi->AddToThreadQueue(op, PrioQueue_High);
	}
	else
	{
		g_pSM->LogError(myself, "Unsupported driver \"%s\"", identifier);
		Database->Close();
		Database = NULL;
		databaseLoading = false;
		ProcessQueryCache();
		return;
	}

	if (driver == DRIVER_MYSQL)
	{
		InsertCookieQuery = Database->PrepareQuery(
				"INSERT IGNORE INTO sm_cookies(name, description, access) \
				VALUES(?, ?, ?)",
				error, sizeof(error), &errCode);

		if (InsertCookieQuery == NULL)
		{
			g_pSM->LogMessage(myself, "Failed to prepare query InsertCookie: %s (%i)", error, errCode);
			return;
		}

		InsertDataQuery = Database->PrepareQuery(
				"INSERT INTO sm_cookie_cache(player, cookie_id, value, timestamp) \
				VALUES(?, ?, ?, ?) \
				ON DUPLICATE KEY UPDATE value = ?, timestamp = ?",
				error, sizeof(error), &errCode);

		if (InsertDataQuery == NULL)
		{
			g_pSM->LogMessage(myself, "Failed to prepare query InsertData: %s (%i)", error, errCode);
			return;
		}
	}
	else
	{
		InsertCookieQuery = Database->PrepareQuery(
				"INSERT OR IGNORE INTO sm_cookies(name, description, access) \
				VALUES(?, ?, ?)", 
				error, sizeof(error), &errCode);

		if (InsertCookieQuery == NULL)
		{
			g_pSM->LogMessage(myself, "Failed to prepare query InsertCookie: %s (%i)", error, errCode);
			return;
		}

		InsertDataQuery = Database->PrepareQuery(
				"INSERT OR REPLACE INTO sm_cookie_cache(player, cookie_id, value, timestamp) \
				VALUES(?, ?, ?, ?)", 
				error, sizeof(error), &errCode);

		if (InsertDataQuery == NULL)
		{
			g_pSM->LogMessage(myself, "Failed to prepare query InsertData: %s (%i)", error, errCode);
			return;
		}
	}

	SelectDataQuery = Database->PrepareQuery(
			"SELECT sm_cookies.name, sm_cookie_cache.value, sm_cookies.description, sm_cookies.access \
			FROM sm_cookies \
			JOIN sm_cookie_cache \
			ON sm_cookies.id = sm_cookie_cache.cookie_id \
			WHERE player = ?",
			error, sizeof(error), &errCode);

	if (SelectDataQuery == NULL)
	{
		g_pSM->LogMessage(myself, "Failed to prepare query SelectData: %s (%i)", error, errCode);
		return;
	}

	SelectIdQuery = Database->PrepareQuery(
			"SELECT id \
			FROM sm_cookies \
			WHERE name=?",
			error, sizeof(error), &errCode);

	if (SelectIdQuery == NULL)
	{
		g_pSM->LogMessage(myself, "Failed to prepare query SelectId: %s (%i)", error, errCode);
		return;
	}

	databaseLoading = false;
	cell_t result = 0;

	ProcessQueryCache();

	return;
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
		query->SetPreparedQuery();
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
		TQueryOp *op = (TQueryOp *)*iter;

		if (Database != NULL)
		{
			op->SetDatabase(Database);
			op->SetPreparedQuery();
			dbi->AddToThreadQueue(op, PrioQueue_Normal);
		}
		else
		{
			delete op;
		}

		iter++;
	}

	cachedQueries.clear();

	queryMutex->Unlock();
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



