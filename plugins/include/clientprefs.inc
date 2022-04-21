/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2011 AlliedModders LLC.  All rights reserved.
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
 * Cookie access types for client viewing
 */
enum CookieAccess
{
	CookieAccess_Public,            /**< Visible and Changeable by users */
	CookieAccess_Protected,         /**< Read only to users */
	CookieAccess_Private            /**< Completely hidden cookie */
};

/**
 * Cookie Prefab menu types
 */
enum CookieMenu
{
	CookieMenu_YesNo,           /**< Yes/No menu with "yes"/"no" results saved into the cookie */
	CookieMenu_YesNo_Int,       /**< Yes/No menu with 1/0 saved into the cookie */
	CookieMenu_OnOff,           /**< On/Off menu with "on"/"off" results saved into the cookie */
	CookieMenu_OnOff_Int        /**< On/Off menu with 1/0 saved into the cookie */
};

enum CookieMenuAction
{
	/**
	 * An option is being drawn for a menu.
	 *
	 * INPUT : Client index and data if available.
	 * OUTPUT: Buffer for rendering, maxlength of buffer.
	 */
	CookieMenuAction_DisplayOption = 0,
	
	/**
	 * A menu option has been selected.
	 *
	 * INPUT : Client index and any data if available.
	 */
	CookieMenuAction_SelectOption = 1
};

#define COOKIE_MAX_NAME_LENGTH 30 		/**< Maximum Cookie name length. */
#define COOKIE_MAX_DESCRIPTION_LENGTH 255 	/**< Maximum Cookie description length. */

/**
 * Cookie Menu Callback prototype
 *
 * @param client        Client index.
 * @param action        CookieMenuAction being performed.
 * @param info          Info data passed.
 * @param buffer        Outbut buffer.
 * @param maxlen        Max length of the output buffer.
 */
typedef CookieMenuHandler = function void (
  int client,
  CookieMenuAction action,
  any info,
  char[] buffer,
  int maxlen
);

/**
 * Note:
 * 
 * A successful return value/result on any client prefs native only guarantees that the local cache has been updated.
 * Database connection problems can still prevent the data from being permanently saved. Connection problems will be logged as
 * errors by the clientprefs extension.
 */

methodmap Cookie < Handle {
	// Creates a new Client preference cookie.
	//
	// Handles returned can be closed via CloseHandle() when
	// no longer needed.
	//
	// @param name          Name of the new preference cookie.
	// @param description   Optional description of the preference cookie.
	// @param access        What CookieAccess level to assign to this cookie.
	// @return              A handle to the newly created cookie. If the cookie already
	//                      exists, a handle to it will still be returned.
	// @error               Cookie name is blank.
	public native Cookie(const char[] name, const char[] description, CookieAccess access);

	// Searches for a Client preference cookie.
	//
	// Handles returned by Cookie.Find can be closed via CloseHandle() when
	// no longer needed.
	//
	// @param name          Name of cookie to find.
	// @return              A handle to the cookie if it is found, null otherwise.

	public static native Cookie Find(const char[] name);

	// Set the value of a Client preference cookie.
	//
	// @param client        Client index.
	// @param value         String value to set.
	// @error               Invalid cookie handle or invalid client index.
	public native void Set(int client, const char[] value);

	// Retrieve the value of a Client preference cookie.
	//
	// @param client        Client index.
	// @param buffer        Copyback buffer for value.
	// @param maxlen        Maximum length of the buffer.
	// @error               Invalid cookie handle or invalid client index.
	public native void Get(int client, char[] buffer, int maxlen);

	// Sets the value of a Client preference cookie based on an authID string.
	//
	// @param authID        String Auth/STEAM ID of player to set.
	// @param value         String value to set.
	// @error               Invalid cookie handle.
	public native void SetByAuthId(const char[] authID, const char[] value);

	// Add a new prefab item to the client cookie settings menu.
	//
	// Note: This handles everything automatically and does not require a callback
	//
	// @param type          A CookieMenu prefab menu type.
	// @param display       Text to show on the menu.
	// @param handler       Optional handler callback for translations and output on selection
	// @param info          Info data to pass to the callback.
	// @error               Invalid cookie handle.
	public native void SetPrefabMenu(CookieMenu type, const char[] display, CookieMenuHandler handler=INVALID_FUNCTION, any info=0);

	// Returns the last updated timestamp for a client cookie
	//
	// @param client        Client index.
	// @return              Last updated timestamp.
	public native int GetClientTime(int client);

	// Returns the access level of a cookie
	//
	// @return              CookieAccess access level.
	// @error               Invalid cookie handle.
	property CookieAccess AccessLevel {
		public native get();
	}
};

/**
 * Creates a new Client preference cookie.
 *
 * Handles returned by RegClientCookie can be closed via CloseHandle() when
 * no longer needed.
 *
 * @param name          Name of the new preference cookie.
 * @param description   Optional description of the preference cookie.
 * @param access        What CookieAccess level to assign to this cookie.
 * @return              A handle to the newly created cookie. If the cookie already
 *                      exists, a handle to it will still be returned.
 * @error               Cookie name is blank.
 */
native Cookie RegClientCookie(const char[] name, const char[] description, CookieAccess access);

/**
 * Searches for a Client preference cookie.
 *
 * Handles returned by FindClientCookie can be closed via CloseHandle() when
 * no longer needed.
 *
 * @param name          Name of cookie to find.
 * @return              A handle to the cookie if it is found, null otherwise.

 */
native Cookie FindClientCookie(const char[] name);

/**
 * Set the value of a Client preference cookie.
 *
 * @param client        Client index.
 * @param cookie        Client preference cookie handle.
 * @param value         String value to set.
 * @error               Invalid cookie handle or invalid client index.
 */
native void SetClientCookie(int client, Handle cookie, const char[] value);

/**
 * Retrieve the value of a Client preference cookie.
 *
 * @param client        Client index.
 * @param cookie        Client preference cookie handle.
 * @param buffer        Copyback buffer for value.
 * @param maxlen        Maximum length of the buffer.
 * @error               Invalid cookie handle or invalid client index.
 */
native void GetClientCookie(int client, Handle cookie, char[] buffer, int maxlen);

/**
 * Sets the value of a Client preference cookie based on an authID string.
 *
 * @param authID        String Auth/STEAM ID of player to set.
 * @param cookie        Client preference cookie handle.
 * @param value         String value to set.
 * @error               Invalid cookie handle.
 */
native void SetAuthIdCookie(const char[] authID, Handle cookie, const char[] value);

/**
 * Checks if a clients cookies have been loaded from the database.
 *
 * @param client        Client index.
 * @return              True if loaded, false otherwise.
 * @error               Invalid client index.
 */
native bool AreClientCookiesCached(int client);

/**
 * Called once a client's saved cookies have been loaded from the database.
 *
 * @param client        Client index.
 */
forward void OnClientCookiesCached(int client);

/**
 * Add a new prefab item to the client cookie settings menu.
 *
 * Note: This handles everything automatically and does not require a callback
 *
 * @param cookie        Client preference cookie handle.
 * @param type          A CookieMenu prefab menu type.
 * @param display       Text to show on the menu.
 * @param handler       Optional handler callback for translations and output on selection
 * @param info          Info data to pass to the callback.
 * @error               Invalid cookie handle.
 */
native void SetCookiePrefabMenu(Handle cookie, CookieMenu type, const char[] display, CookieMenuHandler handler=INVALID_FUNCTION, any info=0);

/**
 * Adds a new item to the client cookie settings menu.
 *
 * Note: This only adds the top level menu item. You need to handle any submenus from the callback.
 *
 * @param handler       A MenuHandler callback function.
 * @param info          Data to pass to the callback.
 * @param display       Text to show on the menu.
 * @error               Invalid cookie handle.
 */
native void SetCookieMenuItem(CookieMenuHandler handler, any info, const char[] display);

/**
 * Displays the settings menu to a client.
 *
 * @param client        Client index.
 */
native void ShowCookieMenu(int client);

/**
 * Gets a cookie iterator.  Must be freed with CloseHandle().
 *
 * @return              A new cookie iterator.
 */
native Handle GetCookieIterator();

/**
 * Reads a cookie iterator, then advances to the next cookie if any.
 *
 * @param iter          Cookie iterator Handle.
 * @param name          Name buffer.
 * @param nameLen       Name buffer size.
 * @param access        Access level of the cookie.
 * @param desc          Cookie description buffer.
 * @param descLen       Cookie description buffer size.
 * @return              True on success, false if there are no more commands.
 */
native bool ReadCookieIterator(Handle iter, 
								char[] name, 
								int nameLen,
								CookieAccess &access, 
								char[] desc="", 
								int descLen=0);

/**
 * Returns the access level of a cookie
 *
 * @param cookie        Client preference cookie handle.
 * @return              CookieAccess access level.
 * @error               Invalid cookie handle.
 */
native CookieAccess GetCookieAccess(Handle cookie);

/**
 * Returns the last updated timestamp for a client cookie
 *
 * @param client        Client index.
 * @param cookie        Cookie handle.
 * @return              Last updated timestamp.
 */
native int GetClientCookieTime(int client, Handle cookie);

/**
 * Do not edit below this line!
 */
public Extension __ext_cprefs = 
{
	name = "Client Preferences",
	file = "clientprefs.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public void __ext_cprefs_SetNTVOptional()
{
	MarkNativeAsOptional("RegClientCookie");
	MarkNativeAsOptional("FindClientCookie");
	MarkNativeAsOptional("SetClientCookie");
	MarkNativeAsOptional("GetClientCookie");
	MarkNativeAsOptional("SetAuthIdCookie");
	MarkNativeAsOptional("AreClientCookiesCached");
	MarkNativeAsOptional("SetCookiePrefabMenu");
	MarkNativeAsOptional("SetCookieMenuItem");
	MarkNativeAsOptional("ShowCookieMenu");
	MarkNativeAsOptional("GetCookieIterator");
	MarkNativeAsOptional("ReadCookieIterator");
	MarkNativeAsOptional("GetCookieAccess");
	MarkNativeAsOptional("GetClientCookieTime");

	MarkNativeAsOptional("Cookie.Cookie");
	MarkNativeAsOptional("Cookie.Find");
	MarkNativeAsOptional("Cookie.Set");
	MarkNativeAsOptional("Cookie.Get");
	MarkNativeAsOptional("Cookie.SetByAuthId");
	MarkNativeAsOptional("Cookie.SetPrefabMenu");
	MarkNativeAsOptional("Cookie.GetClientTime");
	MarkNativeAsOptional("Cookie.AccessLevel.get");
}
#endif
