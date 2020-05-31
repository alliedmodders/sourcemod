/**
 * vim: set ts=4 sw=4 tw=99 noet :
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
#include "am-vector.h"
#include <sm_namehashset.h>

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
		UTIL_strncpy(this->value, value, MAX_VALUE_LENGTH);
	}

	char value[MAX_VALUE_LENGTH+1];
	bool changed;
	time_t timestamp;
	Cookie *parent;
};

struct Cookie
{
	Cookie(const char *name, const char *description, CookieAccess access)
	{
		UTIL_strncpy(this->name, name, MAX_NAME_LENGTH);
		UTIL_strncpy(this->description, description, MAX_DESC_LENGTH);

		this->access = access;

		dbid = -1;

		for (int i=0; i<=SM_MAXPLAYERS; i++)
			data[i] = NULL;
	}

	~Cookie()
	{
		for (int i=0; i<=SM_MAXPLAYERS; i++)
		{
			if (data[i] != NULL)
				delete data[i];
		}
	}

	char name[MAX_NAME_LENGTH+1];
	char description[MAX_DESC_LENGTH+1];
	int dbid;
	CookieData *data[SM_MAXPLAYERS+1];
	CookieAccess access;

	static inline bool matches(const char *name, const Cookie *cookie)
	{
		return strcmp(name, cookie->name) == 0;
	}
	static inline uint32_t hash(const detail::CharsAndLength &key)
	{
		return key.hash();
	}
};

class CookieManager : public IClientListener, public IPluginsListener
{
public:
	CookieManager();
	~CookieManager();

	void OnClientAuthorized(int client, const char *authstring);
	void OnClientDisconnecting(int client);
	
	bool GetCookieValue(Cookie *pCookie, int client, char **value);
	bool SetCookieValue(Cookie *pCookie, int client, const char *value);
	bool GetCookieTime(Cookie *pCookie, int client, time_t *value);

	void Unload();

	void ClientConnectCallback(int serial, IQuery *data);
	void InsertCookieCallback(Cookie *pCookie, int dbId);
	void SelectIdCallback(Cookie *pCookie, IQuery *data);

	Cookie *FindCookie(const char *name);
	Cookie *CreateCookie(const char *name, const char *description, CookieAccess access);

	bool AreClientCookiesCached(int client);

	void OnPluginDestroyed(IPlugin *plugin);
	
	bool AreClientCookiesPending(int client);

public:
	IForward *cookieDataLoadedForward;
	std::vector<Cookie *> cookieList;
	IBaseMenu *clientMenu;

private:
	NameHashSet<Cookie *> cookieFinder;
	std::vector<CookieData *> clientData[SM_MAXPLAYERS+1];

	bool connected[SM_MAXPLAYERS+1];
	bool statsLoaded[SM_MAXPLAYERS+1];
	bool statsPending[SM_MAXPLAYERS+1];
};

extern CookieManager g_CookieManager;

#endif // _INCLUDE_SOURCEMOD_CLIENTPREFS_COOKIE_H_
