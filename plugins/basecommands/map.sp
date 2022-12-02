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
 
public int MenuHandler_ChangeMap(Menu menu, MenuAction action, int param1, int param2)
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
		char map[PLATFORM_MAX_PATH];
		
		menu.GetItem(param2, map, sizeof(map));
	
		ShowActivity2(param1, "[SM] ", "%t", "Changing map", map);

		LogAction(param1, -1, "\"%L\" changed map to \"%s\"", param1, map);

		DataPack dp;
		CreateDataTimer(3.0, Timer_ChangeMap, dp);
		dp.WriteString(map);
	}
	else if (action == MenuAction_Display)
	{
		char title[128];
		Format(title, sizeof(title), "%T", "Please select a map", param1);

		Panel panel = view_as<Panel>(param2);
		panel.SetTitle(title);
	}

	return 0;
}

public void AdminMenu_Map(TopMenu topmenu, 
							  TopMenuAction action,
							  TopMenuObject object_id,
							  int param,
							  char[] buffer,
							  int maxlength)
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

public Action Command_Map(int client, int args)
{
	if (args < 1)
	{
		if ((GetCmdReplySource() == SM_REPLY_TO_CHAT) && (client != 0))
		{
			g_MapList.SetTitle("%T", "Choose Map", client);
			g_MapList.Display(client, MENU_TIME_FOREVER);
		}
		else 
		{
			ReplyToCommand(client, "[SM] Usage: sm_map <map>");
		}
		return Plugin_Handled;
	}

	char map[PLATFORM_MAX_PATH];
	char displayName[PLATFORM_MAX_PATH];
	GetCmdArg(1, map, sizeof(map));

	if (FindMap(map, displayName, sizeof(displayName)) == FindMap_NotFound)
	{
		ReplyToCommand(client, "[SM] %t", "Map was not found", map);
		return Plugin_Handled;
	}

	GetMapDisplayName(displayName, displayName, sizeof(displayName));

	ShowActivity2(client, "[SM] ", "%t", "Changing map", displayName);
	LogAction(client, -1, "\"%L\" changed map to \"%s\"", client, map);

	DataPack dp;
	CreateDataTimer(3.0, Timer_ChangeMap, dp);
	dp.WriteString(map);

	return Plugin_Handled;
}

public Action Timer_ChangeMap(Handle timer, DataPack dp)
{
	char map[PLATFORM_MAX_PATH];

	dp.Reset();
	dp.ReadString(map, sizeof(map));

	ForceChangeLevel(map, "sm_map Command");

	return Plugin_Stop;
}

Handle g_map_array = null;
int g_map_serial = -1;

int LoadMapList(Menu menu)
{
	Handle map_array;
	
	if ((map_array = ReadMapList(g_map_array,
			g_map_serial,
			"sm_map menu",
			MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_MAPSFOLDER))
		!= null)
	{
		g_map_array = map_array;
	}
	
	if (g_map_array == null)
	{
		return 0;
	}
	
	menu.RemoveAllItems();
	
	char map_name[PLATFORM_MAX_PATH];
	int map_count = GetArraySize(g_map_array);
	
	for (int i = 0; i < map_count; i++)
	{
		char displayName[PLATFORM_MAX_PATH];
		GetArrayString(g_map_array, i, map_name, sizeof(map_name));
		GetMapDisplayName(map_name, displayName, sizeof(displayName));
		menu.AddItem(map_name, displayName);
	}
	
	return map_count;
}
