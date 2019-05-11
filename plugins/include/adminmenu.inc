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
 
#if defined _adminmenu_included
 #endinput
#endif
#define _adminmenu_included

/* Decide whether topmenus should be required */
#if !defined REQUIRE_PLUGIN
 #if defined REQUIRE_EXTENSIONS
  #define TEMP_REQUIRE_EXTENSIONS
  #undef REQUIRE_EXTENSIONS
 #endif
#endif

#include <topmenu>
#include <menu>
#include <clients>
#include <string>

/* Restore old REQUIRE_EXTENSIONS value if necessary */
#if defined TEMP_REQUIRE_EXTENSIONS
 #define REQUIRE_EXTENSIONS
 #undef TEMP_REQUIRE_EXTENSIONS
#endif

/** Category for player commands. */
#define ADMINMENU_PLAYERCOMMANDS		"PlayerCommands"
/** Category for server commands. */
#define ADMINMENU_SERVERCOMMANDS		"ServerCommands"
/** Category for voting commands. */
#define ADMINMENU_VOTINGCOMMANDS		"VotingCommands"

/**
 * Called when the admin menu is created and 3rd party plugins can grab 
 * the Handle or add categories.
 * 
 * @param topmenu		Handle to the admin menu's TopMenu.
 */
forward void OnAdminMenuCreated(Handle topmenu);

/**
 * Called when the admin menu is ready to have items added.
 * 
 * @param topmenu		Handle to the admin menu's TopMenu.
 */
forward void OnAdminMenuReady(Handle topmenu);

/**
 * Retrieves the Handle to the admin top menu.
 *
 * @return				Handle to the admin menu's TopMenu,
 *						or INVALID_HANDLE if not created yet.
 */
native TopMenu GetAdminTopMenu();

/**
 * Adds targets to an admin menu.
 *
 * Each client is displayed as: name (userid)
 * Each item contains the userid as a string for its info.
 *
 * @param menu			Menu Handle.
 * @param source_client	Source client, or 0 to ignore immunity.
 * @param in_game_only	True to only select in-game players.
 * @param alive_only 	True to only select alive players.
 * @return				Number of clients added.
 */
native int AddTargetsToMenu(Handle menu, 
						int source_client, 
						bool in_game_only=true, 
						bool alive_only=false);

/**
 * Adds targets to an admin menu.
 *
 * Each client is displayed as: name (userid)
 * Each item contains the userid as a string for its info.
 *
 * @param menu			Menu Handle.
 * @param source_client	Source client, or 0 to ignore immunity.
 * @param flags			COMMAND_FILTER flags from commandfilters.inc.
 * @return				Number of clients added.
 */
native int AddTargetsToMenu2(Handle menu, int source_client, int flags);

/**
 * Re-displays the admin menu to a client after selecting an item.
 * Auto-aborts if the Handle is invalid.
 *
 * @param topmenu		TopMenu Handle.
 * @param client		Client index.
 * @return				True on success, false on failure.
 */
stock bool RedisplayAdminMenu(Handle topmenu, int client)
{
	if (topmenu == INVALID_HANDLE)
	{
		return false;
	}

	return DisplayTopMenu(topmenu, client, TopMenuPosition_LastCategory);
}

/**
 * Used by AddTargetsToMenu* natives to add targets to a menu.
 *
 * @param menu          Menu Handle.
 * @param source_client Source client, or 0 to ignore immunity.
 * @param flags         COMMAND_FILTER flags from commandfilters.inc.
 * @return              Number of clients added to menu.
 */
stock int UTIL_AddTargetsToMenu2(Menu menu, int source_client, int flags)
{
	char user_id[12];
	char name[MAX_NAME_LENGTH];
	char display[MAX_NAME_LENGTH+12];
	
	int num_clients;
	
	for (int i = 1; i <= MaxClients; i++)
	{
		if (!IsClientConnected(i) || IsClientInKickQueue(i))
		{
			continue;
		}
		
		if (((flags & COMMAND_FILTER_NO_BOTS) == COMMAND_FILTER_NO_BOTS)
			&& IsFakeClient(i))
		{
			continue;
		}
		
		if (((flags & COMMAND_FILTER_CONNECTED) != COMMAND_FILTER_CONNECTED)
			&& !IsClientInGame(i))
		{
			continue;
		}
		
		if (((flags & COMMAND_FILTER_ALIVE) == COMMAND_FILTER_ALIVE) 
			&& !IsPlayerAlive(i))
		{
			continue;
		}
		
		if (((flags & COMMAND_FILTER_DEAD) == COMMAND_FILTER_DEAD)
			&& IsPlayerAlive(i))
		{
			continue;
		}
		
		if ((source_client && ((flags & COMMAND_FILTER_NO_IMMUNITY) != COMMAND_FILTER_NO_IMMUNITY))
			&& !CanUserTarget(source_client, i))
		{
			continue;
		}
		
		IntToString(GetClientUserId(i), user_id, sizeof(user_id));
		GetClientName(i, name, sizeof(name));
		Format(display, sizeof(display), "%s (%s)", name, user_id);
		menu.AddItem(user_id, display);
		num_clients++;
	}
	
	return num_clients;
}

/**
 * Used by AddTargetsToMenu* natives to add targets to a menu.
 *
 * Each client is displayed as: name (userid)
 * Each item contains the userid as a string for its info.
 *
 * @param menu			Menu Handle.
 * @param source_client	Source client, or 0 to ignore immunity.
 * @param in_game_only	True to only select in-game players.
 * @param alive_only 	True to only select alive players.
 * @return				Number of clients added.
 */
stock int UTIL_AddTargetsToMenu(Menu menu, int source_client, bool in_game_only = true, bool alive_only = false)
{
	int flags = 0;
	
	if (!in_game_only)
	{
		flags |= COMMAND_FILTER_CONNECTED;
	}
	
	if (alive_only)
	{
		flags |= COMMAND_FILTER_ALIVE;
	}
	
	return UTIL_AddTargetsToMenu2(menu, source_client, flags);
}

/* DO NOT EDIT BELOW THIS LINE */

public SharedPlugin __pl_adminmenu = 
{
	name = "adminmenu",
	file = "adminmenu.smx",
#if defined REQUIRE_PLUGIN
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_PLUGIN
public void __pl_adminmenu_SetNTVOptional()
{
	MarkNativeAsOptional("GetAdminTopMenu");
	MarkNativeAsOptional("AddTargetsToMenu");
	MarkNativeAsOptional("AddTargetsToMenu2");
}
#endif
