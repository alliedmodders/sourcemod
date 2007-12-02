
public MenuHandler_ChangeMap(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_Cancel)
	{
		if (param2 == MenuCancel_ExitBack && hTopMenu != INVALID_HANDLE)
		{
			DisplayTopMenu(hTopMenu, param1, TopMenuPosition_LastCategory);
		}
	}
	else if (action == MenuAction_Select)
	{
		decl String:map[64];
		
		GetMenuItem(menu, param2, map, sizeof(map));
	
		ShowActivity2(param1, "[SM] ", "%t", "Changing map", map);

		LogAction(param1, -1, "\"%L\" changed map to \"%s\"", param1, map);

		new Handle:dp;
		CreateDataTimer(3.0, Timer_ChangeMap, dp);
		WritePackString(dp, map);

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
		DisplayMenu(g_MapList, param, MENU_TIME_FOREVER);
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

	ServerCommand("changelevel \"%s\"", map);

	return Plugin_Stop;
}

new Handle:g_map_array = INVALID_HANDLE;
new g_map_serial = -1;

LoadMapList(Handle:menu)
{
	new Handle:map_array;
	
	if ((map_array = ReadMapList(g_map_array,
			g_map_serial,
			"sm_map menu",
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
