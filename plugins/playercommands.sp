/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Player Commands Plugin
 * Implements slap and slay commands
 *
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
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

#pragma semicolon 1

#include <sourcemod>
#include <sdktools>
#undef REQUIRE_PLUGIN
#include <adminmenu>

public Plugin:myinfo =
{
	name = "Player Commands",
	author = "AlliedModders LLC",
	description = "Misc. Player Commands",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

new Handle:hTopMenu = INVALID_HANDLE;

/* Used to get the SDK / Engine version. */
/* This is used in sm_rename and sm_changeteam */
new g_ModVersion = 0;

#include "playercommands/slay.sp"
#include "playercommands/slap.sp"
#include "playercommands/rename.sp"

public OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("playercommands.phrases");

	RegAdminCmd("sm_slap", Command_Slap, ADMFLAG_SLAY, "sm_slap <#userid|name> [damage]");
	RegAdminCmd("sm_slay", Command_Slay, ADMFLAG_SLAY, "sm_slay <#userid|name>");
	RegAdminCmd("sm_rename", Command_Rename, ADMFLAG_SLAY, "sm_rename <#userid|name>");

	g_ModVersion = GuessSDKVersion();
	
	/* Account for late loading */
	new Handle:topmenu;
	if (LibraryExists("adminmenu") && ((topmenu = GetAdminTopMenu()) != INVALID_HANDLE))
	{
		OnAdminMenuReady(topmenu);
	}
}

public OnAdminMenuReady(Handle:topmenu)
{
	/* Block us from being called twice */
	if (topmenu == hTopMenu)
	{
		return;
	}
	
	/* Save the Handle */
	hTopMenu = topmenu;
	
	/* Find the "Player Commands" category */
	new TopMenuObject:player_commands = FindTopMenuCategory(hTopMenu, ADMINMENU_PLAYERCOMMANDS);

	if (player_commands != INVALID_TOPMENUOBJECT)
	{
		AddToTopMenu(hTopMenu,
			"sm_slay",
			TopMenuObject_Item,
			AdminMenu_Slay,
			player_commands,
			"sm_slay",
			ADMFLAG_SLAY);
			
		AddToTopMenu(hTopMenu,
			"sm_slap",
			TopMenuObject_Item,
			AdminMenu_Slap,
			player_commands,
			"sm_slap",
			ADMFLAG_SLAY);

		AddToTopMenu(hTopMenu,
			"sm_rename",
			TopMenuObject_Item,
			AdminMenu_Rename,
			player_commands,
			"sm_rename",
			ADMFLAG_SLAY);
	}
}
