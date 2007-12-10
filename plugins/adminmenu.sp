/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Admin Menu Plugin
 * Creates the base admin menu, for plugins to add items to.
 *
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
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
#include <topmenus>

public Plugin:myinfo = 
{
	name = "Admin Menu",
	author = "AlliedModders LLC",
	description = "Administration Menu",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

new g_maxPlayers;

/* Forwards */
new Handle:hOnAdminMenuReady = INVALID_HANDLE;
new Handle:hOnAdminMenuCreated = INVALID_HANDLE;

/* Menus */
new Handle:hAdminMenu = INVALID_HANDLE;

/* Top menu objects */
new TopMenuObject:obj_playercmds = INVALID_TOPMENUOBJECT;
new TopMenuObject:obj_servercmds = INVALID_TOPMENUOBJECT;
new TopMenuObject:obj_votingcmds = INVALID_TOPMENUOBJECT;

#include "adminmenu/dynamicmenu.sp"

public bool:AskPluginLoad(Handle:myself, bool:late, String:error[], err_max)
{
	CreateNative("GetAdminTopMenu", __GetAdminTopMenu);
	CreateNative("AddTargetsToMenu", __AddTargetsToMenu);
	RegPluginLibrary("adminmenu");
	return true;
}

public OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("adminmenu.phrases");
	
	hOnAdminMenuCreated = CreateGlobalForward("OnAdminMenuCreated", ET_Ignore, Param_Cell);
	hOnAdminMenuReady = CreateGlobalForward("OnAdminMenuReady", ET_Ignore, Param_Cell);

	RegAdminCmd("sm_admin", Command_DisplayMenu, ADMFLAG_GENERIC, "Displays the admin menu");
}

public OnConfigsExecuted()
{
	decl String:path[PLATFORM_MAX_PATH];
	decl String:error[256];
	
	BuildPath(Path_SM, path, sizeof(path), "configs/dynamicmenu/adminmenu_sorting.txt");
	
	if (!LoadTopMenuConfig(hAdminMenu, path, error, sizeof(error)))
	{
		LogError("Could not load admin menu config (file \"%s\": %s)", path, error);
		return;
	}
}

public OnMapStart()
{
	g_maxPlayers = GetMaxClients();
	
	ParseConfigs();
}

public OnAllPluginsLoaded()
{
	hAdminMenu = CreateTopMenu(DefaultCategoryHandler);
	
	obj_playercmds = AddToTopMenu(hAdminMenu, 
		"PlayerCommands",
		TopMenuObject_Category,
		DefaultCategoryHandler,
		INVALID_TOPMENUOBJECT);

	obj_servercmds = AddToTopMenu(hAdminMenu,
		"ServerCommands",
		TopMenuObject_Category,
		DefaultCategoryHandler,
		INVALID_TOPMENUOBJECT);

	obj_votingcmds = AddToTopMenu(hAdminMenu,
		"VotingCommands",
		TopMenuObject_Category,
		DefaultCategoryHandler,
		INVALID_TOPMENUOBJECT);
		
	BuildDynamicMenu();
	
	Call_StartForward(hOnAdminMenuCreated);
	Call_PushCell(hAdminMenu);
	Call_Finish();
	
	Call_StartForward(hOnAdminMenuReady);
	Call_PushCell(hAdminMenu);
	Call_Finish();
}

public DefaultCategoryHandler(Handle:topmenu, 
						TopMenuAction:action,
						TopMenuObject:object_id,
						param,
						String:buffer[],
						maxlength)
{
	if (action == TopMenuAction_DisplayTitle)
	{
		if (object_id == INVALID_TOPMENUOBJECT)
		{
			Format(buffer, maxlength, "%T:", "Admin Menu", param);
		}
		else if (object_id == obj_playercmds)
		{
			Format(buffer, maxlength, "%T:", "Player Commands", param);
		}
		else if (object_id == obj_servercmds)
		{
			Format(buffer, maxlength, "%T:", "Server Commands", param);
		}
		else if (object_id == obj_votingcmds)
		{
			Format(buffer, maxlength, "%T:", "Voting Commands", param);
		}
	}
	else if (action == TopMenuAction_DisplayOption)
	{
		if (object_id == obj_playercmds)
		{
			Format(buffer, maxlength, "%T", "Player Commands", param);
		}
		else if (object_id == obj_servercmds)
		{
			Format(buffer, maxlength, "%T", "Server Commands", param);
		}
		else if (object_id == obj_votingcmds)
		{
			Format(buffer, maxlength, "%T", "Voting Commands", param);
		}
	}
}

public __GetAdminTopMenu(Handle:plugin, numParams)
{
	return _:hAdminMenu;
}

public __AddTargetsToMenu(Handle:plugin, numParams)
{
	new bool:alive_only = false;
	
	if (numParams >= 4)
	{
		alive_only = GetNativeCell(4);
	}
	
	return UTIL_AddTargetsToMenu(GetNativeCell(1), GetNativeCell(2), GetNativeCell(3), alive_only);
}

public Action:Command_DisplayMenu(client, args)
{
	if (client == 0)
	{
		ReplyToCommand(client, "[SM] %t", "Command is in-game only");
		return Plugin_Handled;
	}
	
	DisplayTopMenu(hAdminMenu, client, TopMenuPosition_Start);
	
	return Plugin_Handled;
}

stock UTIL_AddTargetsToMenu(Handle:menu, source_client, bool:in_game_only, bool:alive_only)
{
	new max_clients = GetMaxClients();
	decl String:user_id[12];
	decl String:name[MAX_NAME_LENGTH];
	decl String:display[MAX_NAME_LENGTH+12];
	
	new num_clients;
	
	for (new i = 1; i <= max_clients; i++)
	{
		if (!IsClientConnected(i) || IsClientInKickQueue(i))
		{
			continue;
		}
		
		if (in_game_only && !IsClientInGame(i))
		{
			continue;
		}
		
		if (alive_only && !IsPlayerAlive(i))
		{
			continue;
		}
		
		if (source_client && !CanUserTarget(source_client, i))
		{
			continue;
		}
		
		IntToString(GetClientUserId(i), user_id, sizeof(user_id));
		GetClientName(i, name, sizeof(name));
		Format(display, sizeof(display), "%s (%s)", name, user_id);
		AddMenuItem(menu, user_id, display);
		num_clients++;
	}
	
	return num_clients;
}

