/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basecommands Plugin
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
 
public MenuHandler_ChangeMap(Menu menu, MenuAction action, int param1, int param2)
{
	if (action == MenuAction_Cancel)
	{
		if (param2 == MenuCancel_ExitBack && hTopMenu)
		{
			hTopMenu.Display(param1, TopMenuPosition_LastCategory);
		}
	}
	else if (action == MenuAction_Select)
	{
		decl String:map[64];
		
		menu.GetItem(param2, map, sizeof(map));
	
		ShowActivity2(param1, "[SM] ", "%t", "Changing map", map);

		LogAction(param1, -1, "\"%L\" changed map to \"%s\"", param1, map);

		new Handle:dp;
		CreateDataTimer(3.0, Timer_ChangeMap, dp);
		WritePackString(dp, map);
	}
	else if (action == MenuAction_Display)
	{
		decl String:title[128];
		Format(title, sizeof(title), "%T", "Please select a map", param1);
		SetPanelTitle(Handle:param2, title);
	}
}

public AdminMenu_Map(Handle:topmenu, 
							  TopMenuAction:action,
							  TopMenuObject:object_id,
							  param,
							  String:buffer[],
							  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Choose Map", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		g_MapList.Display(param, MENU_TIME_FOREVER);
	}
}

public Action:Command_Map(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_map <map>");
		return Plugin_Handled;
	}

	decl String:map[64];
	GetCmdArg(1, map, sizeof(map));

	if (!IsMapValid(map))
	{
		ReplyToCommand(client, "[SM] %t", "Map was not found", map);
		return Plugin_Handled;
	}

	ShowActivity2(client, "[SM] ", "%t", "Changing map", map);

	LogAction(client, -1, "\"%L\" changed map to \"%s\"", client, map);

	new Handle:dp;
	CreateDataTimer(3.0, Timer_ChangeMap, dp);
	WritePackString(dp, map);

	return Plugin_Handled;
}

public Action:Timer_ChangeMap(Handle:timer, Handle:dp)
{
	decl String:map[65];

	ResetPack(dp);
	ReadPackString(dp, map, sizeof(map));

	ForceChangeLevel(map, "sm_map Command");

	return Plugin_Stop;
}

new Handle:g_map_array = null;
new g_map_serial = -1;

int LoadMapList(Menu menu)
{
	new Handle:map_array;
	
	if ((map_array = ReadMapList(g_map_array,
			g_map_serial,
			"sm_map menu",
			MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_NO_DEFAULT|MAPLIST_FLAG_MAPSFOLDER))
		!= null)
	{
		g_map_array = map_array;
	}
	
	if (g_map_array == null)
	{
		return 0;
	}
	
	RemoveAllMenuItems(menu);
	
	char map_name[64];
	int map_count = GetArraySize(g_map_array);
	
	for (int i = 0; i < map_count; i++)
	{
		GetArrayString(g_map_array, i, map_name, sizeof(map_name));
		menu.AddItem(map_name, map_name);
	}
	
	return map_count;
}
