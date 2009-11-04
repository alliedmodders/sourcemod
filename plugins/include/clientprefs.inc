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
 * Cookie access types for client viewing
 */
enum CookieAccess
{
	CookieAccess_Public,			/**< Visible and Changeable by users */
	CookieAccess_Protected,			/**< Read only to users */
	CookieAccess_Private,			/**< Completely hidden cookie */
};

/**
 * Cookie Prefab menu types
 */
enum CookieMenu
{
	CookieMenu_YesNo,			/**< Yes/No menu with "yes"/"no" results saved into the cookie */
	CookieMenu_YesNo_Int,		/**< Yes/No menu with 1/0 saved into the cookie */
	CookieMenu_OnOff,			/**< On/Off menu with "on"/"off" results saved into the cookie */
	CookieMenu_OnOff_Int,		/**< On/Off menu with 1/0 saved into the cookie */
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
	CookieMenuAction_SelectOption = 1,
};

/**
 * Note:
 * 
 * A successful return value/result on any client prefs native only guarantees that the local cache has been updated.
 * Database connection problems can still prevent the data from being permanently saved. Connection problems will be logged as
 * errors by the clientprefs extension.
 */

/**
 * Creates a new Client preference cookie.
 *
 * Handles returned by RegClientCookie can be closed via CloseHandle() when
 * no longer needed.
 *
 * @param name			Name of the new preference cookie.
 * @param description	Optional description of the preference cookie.
 * @param access		What CookieAccess level to assign to this cookie.
 * @return				A handle to the newly created cookie. If the cookie already
 *                      exists, a handle to it will still be returned.
 * @error				Cookie name is blank.
 */
native Handle:RegClientCookie(const String:name[], const String:description[], CookieAccess:access);

/**
 * Searches for a Client preference cookie.
 *
 * Handles returned by FindClientCookie can be closed via CloseHandle() when
 * no longer needed.
 *
 * @param name			Name of cookie to find.
 * @return				A handle to the cookie if it is found. INVALID_HANDLE otherwise.
 */
native Handle:FindClientCookie(const String:name[]);

/**
 * Set the value of a Client preference cookie.
 *
 * @param client		Client index.
 * @param cookie		Client preference cookie handle.
 * @param value			String value to set.
 * @noreturn			
 * @error				Invalid cookie handle or invalid client index.
 */
native SetClientCookie(client, Handle:cookie, const String:value[]);

/**
 * Retrieve the value of a Client preference cookie.
 *
 * @param client		Client index.
 * @param cookie		Client preference cookie handle.
 * @param buffer		Copyback buffer for value.
 * @param maxlen		Maximum length of the buffer.
 * @noreturn
 * @error				Invalid cookie handle or invalid client index.
 */
native GetClientCookie(client, Handle:cookie, String:buffer[], maxlen);

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
forward OnClientCookiesCached(client);

/**
 * Cookie Menu Callback prototype
 *
 * @param client		Client index.
 * @param action		CookeMenuAction being performed.
 * @param data			Info data passed.
 * @param buffer		Outbut buffer.
 * @param maxlen		Max length of the output buffer.			
 */
functag public CookieMenuHandler(client, CookieMenuAction:action, any:info, String:buffer[], maxlen);

/**
 * Add a new prefab item to the client cookie settings menu.
 *
 * Note: This handles everything automatically and does not require a callback
 *
 * @param cookie		Client preference cookie handle.
 * @param type			A CookieMenu prefab menu type.
 * @param display		Text to show on the menu.
 * @param handler		Optional handler callback for translations and output on selection
 * @param info			Info data to pass to the callback.
 * @noreturn
 * @error				Invalid cookie handle.
 */
native SetCookiePrefabMenu(Handle:cookie, CookieMenu:type, const String:display[], CookieMenuHandler:handler=CookieMenuHandler:-1, info=0);

/**
 * Adds a new item to the client cookie settings menu.
 *
 * Note: This only adds the top level menu item. You need to handle any submenus from the callback.
 *
 * @param handler		A MenuHandler callback function.
 * @param info			Data to pass to the callback.
 * @param display		Text to show on the menu.
 * @noreturn
 * @error				Invalid cookie handle.
 */
native SetCookieMenuItem(CookieMenuHandler:handler, any:info, const String:display[]);

/**
 * Displays the settings menu to a client.
 *
 * @param client		Client index.
 * @noreturn
 */
native ShowCookieMenu(client);

/**
 * Gets a cookie iterator.  Must be freed with CloseHandle().
 *
 * @return				A new cookie iterator.
 */
native Handle:GetCookieIterator();

/**
 * Reads a cookie iterator, then advances to the next cookie if any.
 *
 * @param iter			Cookie iterator Handle.
 * @param name			Name buffer.
 * @param nameLen		Name buffer size.
 * @param access		Access level of the cookie.
 * @param desc			Cookie description buffer.
 * @param descLen		Cookie description buffer size.
 * @param 
 * @return				True on success, false if there are no more commands.
 */
native bool:ReadCookieIterator(Handle:iter, 
								String:name[], 
								nameLen,
								&CookieAccess:access, 
								String:desc[]="", 
								descLen=0);
								
/**
 * Returns the access level of a cookie
 *
 * @param cookie		Client preference cookie handle.
 * @return				CookieAccess access level.
 * @error				Invalid cookie handle.	
 */
native CookieAccess:GetCookieAccess(Handle:cookie);

/**
 * Returns the last updated timestamp for a client cookie
 *
 * @param client		Client index.
 * @param cookie		Cookie handle.
 * @return			Last updated timestamp.
 */
native GetClientCookieTime(client, Handle:cookie);

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
