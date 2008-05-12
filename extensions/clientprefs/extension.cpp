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

void ClientPrefs::SDK_OnUnload()
{
	handlesys->RemoveType(g_CookieType, myself->GetIdentity());

	g_CookieManager.Unload();

	Database->Close();

	forwards->ReleaseForward(g_CookieManager.cookieDataLoadedForward);

	g_CookieManager.clientMenu->Destroy();
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



