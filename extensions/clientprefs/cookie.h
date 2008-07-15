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

#ifndef _INCLUDE_SOURCEMOD_CLIENTPREFS_COOKIE_H_
#define _INCLUDE_SOURCEMOD_CLIENTPREFS_COOKIE_H_

#include "extension.h"
#include "sh_list.h"
#include "sm_trie_tpl.h"


#define MAX_NAME_LENGTH 30
#define MAX_DESC_LENGTH 255
#define MAX_VALUE_LENGTH 100

enum CookieAccess
{
	CookieAccess_Public,			/**< Visible and Changeable by users */
	CookieAccess_Protected,			/**< Read only to users */
	CookieAccess_Private,			/**< Completely hidden cookie */
};

struct Cookie;

struct CookieData
{
	CookieData(const char *value)
	{
		strncpy(this->value, value, MAX_VALUE_LENGTH);
		this->value[MAX_VALUE_LENGTH-1] = '\0';
	}

	char value[MAX_VALUE_LENGTH];
	bool changed;
	Cookie *parent;
};

struct Cookie
{
	Cookie(const char *name, const char *description, CookieAccess access)
	{
		strncpy(this->name, name, MAX_NAME_LENGTH);
		this->name[MAX_NAME_LENGTH-1] = '\0';
		strncpy(this->description, description, MAX_DESC_LENGTH);
		this->description[MAX_DESC_LENGTH-1] = '\0';

		this->access = access;

		dbid = -1;

		for (int i=0; i<=MAXCLIENTS; i++)
		{
			data[i] = NULL;
		}

		shouldDelete = false;
		usedInQuery = 0;
	}

	~Cookie()
	{
		for (int i=0; i<=MAXCLIENTS; i++)
		{
			if (data[i] != NULL)
			{
				delete data[i];
			}
		}
	}

	char name[MAX_NAME_LENGTH];
	char description[MAX_DESC_LENGTH];
	int dbid;
	CookieData *data[MAXCLIENTS+1];
	CookieAccess access;

	/* Reference counting stuff */
	bool shouldDelete;
	int usedInQuery;
};

class CookieManager : public IClientListener, public IPluginsListener
{
public:
	CookieManager();
	~CookieManager();

	void OnClientAuthorized(int client, const char *authstring);
	void OnClientDisconnecting(int client);
	
	bool GetCookieValue(Cookie *pCookie, int client, char **value);
	bool SetCookieValue(Cookie *pCookie, int client, char *value);

	void Unload();

	void ClientConnectCallback(int client, IQuery *data);
	void InsertCookieCallback(Cookie *pCookie, int dbId);
	void SelectIdCallback(Cookie *pCookie, IQuery *data);

	Cookie *FindCookie(const char *name);
	Cookie *CreateCookie(const char *name, const char *description, CookieAccess access);

	bool AreClientCookiesCached(int client);

	void OnPluginDestroyed(IPlugin *plugin);

public:
	IForward *cookieDataLoadedForward;
	SourceHook::List<Cookie *> cookieList;
	IBaseMenu *clientMenu;

private:
	KTrie<Cookie *> cookieTrie;
	SourceHook::List<CookieData *> clientData[MAXCLIENTS];

	bool connected[MAXCLIENTS+1];
	bool statsLoaded[MAXCLIENTS+1];
};

extern CookieManager g_CookieManager;

#endif // _INCLUDE_SOURCEMOD_CLIENTPREFS_COOKIE_H_
