/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
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
 
#if defined _clientprefs_included
 #endinput
#endif
#define _clientprefs_included

/**
 * Creates a new Client preference cookie.
 *
 * @param name			Name of the new preference cookie.
 * @param description	Optional description of the preference cookie.
 * @return				A handle to the newly created cookie. If the cookie already exists, a handle to it will still be returned.
 * @error				Cookie name is blank.
 */
native Handle:RegClientPrefCookie(const String:name[], const String:description[]);

/**
 * Searches for a Client preference cookie.
 *
 * @param name			Name of cookie to find.
 * @return				A handle to the cookie if it is found. INVALID_HANDLE otherwise.
 */
native Handle:FindClientPrefCookie(const String:name[]);

/**
 * Set the value of a Client preference cookie.
 *
 * @param client		Client index.
 * @param cookie		Client preference cookie handle.
 * @param value			String value to set.
 * @noreturn			
 * @error				Invalid cookie handle or invalid client index.
 */
native SetClientPrefCookie(client, Handle:cookie, const String:value[]);

/**
 * Retrieve the value of a Client preference cookie.
 *
 * @param client		Client index.
 * @param cookie		Client preference cookie handle.
 * @param buffer		Copyback buffer for value.
 * @param maxlen		Maximum length of the buffer.
 * @error				Invalid cookie handle or invalid client index.
 */
native GetClientPrefCookie(client, Handle:cookie, String:buffer[], maxlen);

/**
 * Checks if a clients cookies have been loaded from the database.
 *
 * @param client		Client index.
 * @return 				True if loaded, false otherwise.
 * @error				Invalid client index.
 */
native bool:AreClientCookiesCached(client);

/**
 * Called once a client's saved cookies have been loaded from the database.
 *
 * @param client		Client index.
 */
forward OnClientCookiesLoaded(client);


/**
 * Do not edit below this line!
 */
public Extension:__ext_clientprefs = 
{
	name = "Client Preferences",
	file = "clientprefs.ext",
	autoload = 1,
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};
