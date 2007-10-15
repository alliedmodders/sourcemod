
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
	
		ShowActivity(param1, "%t", "Changing map", map);

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

	ShowActivity(client, "%t", "Changing map", map);

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

LoadMaps(Handle:menu)
{
	decl String:mapPath[256];
	BuildPath(Path_SM, mapPath, sizeof(mapPath), "configs/menu_maplist.ini");
	
	if (!FileExists(mapPath))
	{	
		if (g_MapList != INVALID_HANDLE)
		{
			RemoveAllMenuItems(menu);
		}
		
		return LoadMapFolder(menu);		
	}

	// If the file hasn't changed, there's no reason to reload
	// all of the maps.
	new fileTime =  GetFileTime(mapPath, FileTime_LastChange);
	if (g_mapFileTime == fileTime)
	{
		return GetMenuItemCount(menu);
	}
	
	g_mapFileTime = fileTime;
	
	// Reset the array
	if (g_MapList != INVALID_HANDLE)
	{
		RemoveAllMenuItems(menu);
	}

	LogMessage("[SM] Loading menu map list file [%s]", mapPath);

	new Handle:file = OpenFile(mapPath, "rt");
	if (file == INVALID_HANDLE)
	{
		LogError("[SM] Could not open file: %s, reverting to map folder", mapPath);
		return LoadMapFolder(menu);
	}
	
	decl String:buffer[64], len;
	while (!IsEndOfFile(file) && ReadFileLine(file, buffer, sizeof(buffer)))
	{
		TrimString(buffer);

		if ((len = StrContains(buffer, ".bsp", false)) != -1)
		{
			buffer[len] = '\0';
		}

		if (buffer[0] == '\0' || !IsValidConVarChar(buffer[0]) || !IsMapValid(buffer))
		{
			continue;
		}

		AddMenuItem(menu, buffer, buffer);
	}

	CloseHandle(file);
	
	new count = GetMenuItemCount(menu);
	
	if (!count)
		return LoadMapFolder(menu);
	else
		return count;
}

LoadMapFolder(Handle:menu)
{
	LogMessage("[SM] Loading menu map list from maps folder");
	
	new Handle:mapDir = OpenDirectory("maps/");
	
	if (mapDir == INVALID_HANDLE)
	{
		LogError("[SM] Could not open map directory for reading");
		return 0;
	}
	
	new String:mapName[64];
	new String:buffer[64];
	new FileType:fileType;
	new len;
	
	while(ReadDirEntry(mapDir, mapName, sizeof(mapName), fileType))
	{
		if(fileType == FileType_File)
		{
			len = strlen(mapName);

			if(SplitString(mapName, ".bsp", buffer, sizeof(buffer)) == len)
			{
				AddMenuItem(menu, buffer, buffer);
			}
		}
  	}
  	
  	CloseHandle(mapDir);
  	
  	return GetMenuItemCount(menu);
}