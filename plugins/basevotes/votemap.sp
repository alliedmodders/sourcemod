/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basevotes Plugin
 * Provides map functionality
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

new Handle:g_MapList = INVALID_HANDLE;
new g_mapCount;

new Handle:g_SelectedMaps;
new bool:g_VoteMapInUse;

DisplayVoteMapMenu(client, mapCount, String:maps[5][])
{
	LogAction(client, -1, "\"%L\" initiated a map vote.", client);
	ShowActivity2(client, "[SM] ", "%t", "Initiated Vote Map");
	
	g_voteType = voteType:map;
	
	g_hVoteMenu = CreateMenu(Handler_VoteCallback, MenuAction:MENU_ACTIONS_ALL);
	
	if (mapCount == 1)
	{
		strcopy(g_voteInfo[VOTE_NAME], sizeof(g_voteInfo[]), maps[0]);
			
		SetMenuTitle(g_hVoteMenu, "Change Map To");
		AddMenuItem(g_hVoteMenu, maps[0], "Yes");
		AddMenuItem(g_hVoteMenu, VOTE_NO, "No");
	}
	else
	{
		g_voteInfo[VOTE_NAME][0] = '\0';
		
		SetMenuTitle(g_hVoteMenu, "Map Vote");
		for (new i = 0; i < mapCount; i++)
		{
			AddMenuItem(g_hVoteMenu, maps[i], maps[i]);
		}	
	}
	
	SetMenuExitButton(g_hVoteMenu, false);
	VoteMenuToAll(g_hVoteMenu, 20);		
}

ResetMenu()
{
	g_VoteMapInUse = false;
	ClearArray(g_SelectedMaps);
}

ConfirmVote(client)
{
	new Handle:menu = CreateMenu(MenuHandler_Confirm);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Confirm Vote", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddMenuItem(menu, "Confirm", "Start the Vote");
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);	
}

public MenuHandler_Confirm(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_End)
	{
		CloseHandle(menu);
		g_VoteMapInUse = false;
	}
	else if (action == MenuAction_Cancel)
	{
		ResetMenu();
		
		if (param2 == MenuCancel_ExitBack && hTopMenu != INVALID_HANDLE)
		{
			DisplayTopMenu(hTopMenu, param1, TopMenuPosition_LastCategory);
		}
	}
	else if (action == MenuAction_Select)
	{
		decl String:maps[5][64];
		new selectedmaps = GetArraySize(g_SelectedMaps);
		
		for (new i = 0; i < selectedmaps; i++)
		{
			GetArrayString(g_SelectedMaps, i, maps[i], sizeof(maps[]));
		}
		
		DisplayVoteMapMenu(param1, selectedmaps, maps);
		
		ResetMenu();
	}
}

public MenuHandler_Map(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_Cancel)
	{		
		if (param2 == MenuCancel_ExitBack && hTopMenu != INVALID_HANDLE)
		{
			ConfirmVote(param1);
		}
		else // no action was selected.
		{
			/* Re-enable the menu option */
			ResetMenu();
		}
	}
	else if (action == MenuAction_DrawItem)
	{
		decl String:info[32], String:name[32];
		
		GetMenuItem(menu, param2, info, sizeof(info), _, name, sizeof(name));
		
		if (FindStringInArray(g_SelectedMaps, info) != -1)
		{
			return ITEMDRAW_IGNORE;
		}
		else
		{
			return ITEMDRAW_DEFAULT;
		}
	}
	else if (action == MenuAction_Select)
	{
		decl String:info[32], String:name[32];
		
		GetMenuItem(menu, param2, info, sizeof(info), _, name, sizeof(name));
		
		PushArrayString(g_SelectedMaps, info);
		
		/* Redisplay the list */
		if (GetArraySize(g_SelectedMaps) < 5)
		{
			DisplayMenu(g_MapList, param1, MENU_TIME_FOREVER);
		}
		else
		{
			ConfirmVote(param1);
		}
	}
	
	return 0;
}

public AdminMenu_VoteMap(Handle:topmenu, 
							  TopMenuAction:action,
							  TopMenuObject:object_id,
							  param,
							  String:buffer[],
							  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Map vote", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		if (!g_VoteMapInUse)
		{
			ResetMenu();
			g_VoteMapInUse = true;
			DisplayMenu(g_MapList, param, MENU_TIME_FOREVER);
		}
		else 
		{
			PrintToChat(param, "[SM] %T", "Map Vote In Use", param);
		}
	}
	else if (action == TopMenuAction_DrawOption)
	{	
		/* disable this option if a vote is already running, theres no maps listed or someone else has already acessed this menu */
		buffer[0] = (!IsNewVoteAllowed() || g_mapCount < 1 || g_VoteMapInUse) ? ITEMDRAW_IGNORE : ITEMDRAW_DEFAULT;
	}
}

public Action:Command_Votemap(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_votemap <mapname> [mapname2] ... [mapname5]");
		return Plugin_Handled;	
	}
	
	if (IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote in Progress");
		return Plugin_Handled;
	}
		
	if (!TestVoteDelay(client))
	{
		return Plugin_Handled;
	}
	
	decl String:text[256];
	GetCmdArgString(text, sizeof(text));

	decl String:maps[5][64];
	new mapCount;	
	new len, pos;
	
	while (pos != -1 && mapCount < 5)
	{	
		pos = BreakString(text[len], maps[mapCount], sizeof(maps[]));
		
		if (!IsMapValid(maps[mapCount]))
		{
			ReplyToCommand(client, "[SM] %t", "Map was not found", maps[mapCount]);
			return Plugin_Handled;
		}		

		mapCount++;
		
		if (pos != -1)
		{
			len += pos;
		}	
	}

	DisplayVoteMapMenu(client, mapCount, maps);
	
	return Plugin_Handled;	
}

new Handle:g_map_array = INVALID_HANDLE;
new g_map_serial = -1;

LoadMapList(Handle:menu)
{
	new Handle:map_array;
	
	if ((map_array = ReadMapList(g_map_array,
			g_map_serial,
			"sm_votemap menu",
			MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_NO_DEFAULT|MAPLIST_FLAG_MAPSFOLDER))
		!= INVALID_HANDLE)
	{
		g_map_array = map_array;
	}
	
	if (g_map_array == INVALID_HANDLE)
	{
		return 0;
	}
	
	RemoveAllMenuItems(menu);
	
	decl String:map_name[64];
	new map_count = GetArraySize(g_map_array);
	
	for (new i = 0; i < map_count; i++)
	{
		GetArrayString(g_map_array, i, map_name, sizeof(map_name));
		AddMenuItem(menu, map_name, map_name);
	}
	
	return map_count;
}
