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

#ifndef _INCLUDE_SOURCEMOD_CLIENTPREFS_MENUS_H_
#define _INCLUDE_SOURCEMOD_CLIENTPREFS_MENUS_H_

#include "extension.h"
#include "cookie.h"

enum CookieMenuAction
{
	/**
	 * An option is being drawn for a menu.
	 *
	 * INPUT : Client index and any data if available.
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

enum CookieMenu
{
	CookieMenu_YesNo,			/**< Yes/No menu with "yes"/"no" results saved into the cookie */
	CookieMenu_YesNo_Int,		/**< Yes/No menu with 1/0 saved into the cookie */
	CookieMenu_OnOff,			/**< On/Off menu with "on"/"off" results saved into the cookie */
	CookieMenu_OnOff_Int,		/**< On/Off menu with 1/0 saved into the cookie */
	CookieMenu_Elements
};

struct ItemHandler
{
	IChangeableForward *forward;
	CookieMenu autoMenuType;
	bool isAutoMenu;
};

class ClientMenuHandler : public IMenuHandler
{
	void OnMenuSelect(IBaseMenu *menu, int client, unsigned int item);
	unsigned int OnMenuDisplayItem(IBaseMenu *menu, 
										   int client, 
										   IMenuPanel *panel,
										   unsigned int item, 
										   const ItemDrawInfo &dr);
};

class AutoMenuHandler : public IMenuHandler
{
	void OnMenuSelect(IBaseMenu *menu, int client, unsigned int item);
	void OnMenuEnd(IBaseMenu *menu, MenuEndReason reason);
};

extern ClientMenuHandler g_Handler;
extern AutoMenuHandler g_AutoHandler;

/* Something went wrong with the includes and made me do this */
struct Cookie;
enum CookieMenu;
struct ItemHandler;

struct AutoMenuData
{
	ItemHandler *handler;
	Cookie *pCookie;
	cell_t datavalue;
	CookieMenu type;
};

#endif // _INCLUDE_SOURCEMOD_CLIENTPREFS_MENUS_H_
