/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Admin Menu Plugin
 * Creates the base admin menu, for plugins to add items to.
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
#include <topmenus>

#pragma newdecls required

public Plugin myinfo = 
{
	name = "Admin Menu",
	author = "AlliedModders LLC",
	description = "Administration Menu",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

/* Forwards */
GlobalForward hOnAdminMenuReady;
GlobalForward hOnAdminMenuCreated;

/* Menus */
TopMenu hAdminMenu;

/* Top menu objects */
TopMenuObject obj_playercmds = INVALID_TOPMENUOBJECT;
TopMenuObject obj_servercmds = INVALID_TOPMENUOBJECT;
TopMenuObject obj_votingcmds = INVALID_TOPMENUOBJECT;

#include "adminmenu/dynamicmenu.sp"

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	CreateNative("GetAdminTopMenu", __GetAdminTopMenu);
	CreateNative("AddTargetsToMenu", __AddTargetsToMenu);
	CreateNative("AddTargetsToMenu2", __AddTargetsToMenu2);
	RegPluginLibrary("adminmenu");
	return APLRes_Success;
}

public void OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("adminmenu.phrases");
	
	hOnAdminMenuCreated = new GlobalForward("OnAdminMenuCreated", ET_Ignore, Param_Cell);
	hOnAdminMenuReady = new GlobalForward("OnAdminMenuReady", ET_Ignore, Param_Cell);

	RegAdminCmd("sm_admin", Command_DisplayMenu, ADMFLAG_GENERIC, "Displays the admin menu");
}

public void OnConfigsExecuted()
{
	char path[PLATFORM_MAX_PATH];
	char error[256];
	
	BuildPath(Path_SM, path, sizeof(path), "configs/adminmenu_sorting.txt");
	
	if (!hAdminMenu.LoadConfig(path, error, sizeof(error)))
	{
		LogError("Could not load admin menu config (file \"%s\": %s)", path, error);
		return;
	}
}

public void OnMapStart()
{
	ParseConfigs();
}

public void OnAllPluginsLoaded()
{
	hAdminMenu = new TopMenu(DefaultCategoryHandler);
	
	obj_playercmds = hAdminMenu.AddCategory("PlayerCommands", DefaultCategoryHandler);
	obj_servercmds = hAdminMenu.AddCategory("ServerCommands", DefaultCategoryHandler);
	obj_votingcmds = hAdminMenu.AddCategory("VotingCommands", DefaultCategoryHandler);
		
	BuildDynamicMenu();
	
	Call_StartForward(hOnAdminMenuCreated);
	Call_PushCell(hAdminMenu);
	Call_Finish();
	
	Call_StartForward(hOnAdminMenuReady);
	Call_PushCell(hAdminMenu);
	Call_Finish();
}

public void DefaultCategoryHandler(TopMenu topmenu, 
						TopMenuAction action,
						TopMenuObject object_id,
						int param,
						char[] buffer,
						int maxlength)
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

public any __GetAdminTopMenu(Handle plugin, int numParams)
{
	return hAdminMenu;
}

public int __AddTargetsToMenu(Handle plugin, int numParams)
{
	bool alive_only = false;
	
	if (numParams >= 4)
	{
		alive_only = GetNativeCell(4);
	}
	
	return UTIL_AddTargetsToMenu(GetNativeCell(1), GetNativeCell(2), GetNativeCell(3), alive_only);
}

public int __AddTargetsToMenu2(Handle plugin, int numParams)
{
	return UTIL_AddTargetsToMenu2(GetNativeCell(1), GetNativeCell(2), GetNativeCell(3));
}

public Action Command_DisplayMenu(int client, int args)
{
	if (client == 0)
	{
		ReplyToCommand(client, "[SM] %t", "Command is in-game only");
		return Plugin_Handled;
	}
	
	hAdminMenu.Display(client, TopMenuPosition_Start);
	return Plugin_Handled;
}

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

stock int UTIL_AddTargetsToMenu(Menu menu, int source_client, bool in_game_only, bool alive_only)
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
