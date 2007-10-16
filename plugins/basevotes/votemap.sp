
DisplayVoteMapMenu(client, mapCount, String:maps[5][])
{
	LogAction(client, -1, "\"%L\" initiated a map vote.", client);
	ShowActivity(client, "%t", "Initiated Vote Map");
	
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
	}
	else if (action == MenuAction_Cancel)
	{
		if (param2 == MenuCancel_ExitBack && hTopMenu != INVALID_HANDLE)
		{
			DisplayTopMenu(hTopMenu, param1, TopMenuPosition_LastCategory);
		}
	}
	else if (action == MenuAction_Select)
	{
		decl String:maps[5][64];
		new selectedmaps = GetArraySize(g_SelectedMaps);
		
		for (new i=0; i<selectedmaps; i++)
		{
			GetArrayString(g_SelectedMaps, i, maps[i], sizeof(maps[]));
		}
		
		DisplayVoteMapMenu(param1, selectedmaps, maps);
		
		/* Re-enable the menu option */
		g_VoteMapInUse = false;	
	}
}

public MenuHandler_Map(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_Cancel)
	{
		/* Add the removed maps back to the menu */
		new selectedmaps = GetArraySize(g_SelectedMaps);
		decl String:mapname[64];
			
		for (new i=0; i<selectedmaps; i++)
		{
			GetArrayString(g_SelectedMaps, i, mapname, sizeof(mapname))	;
			AddMenuItem(menu, mapname, mapname);
		}
		
		if (param2 == MenuCancel_ExitBack && hTopMenu != INVALID_HANDLE)
		{
			ConfirmVote(param1);
		}
		else // no action was selected.
		{
			/* Re-enable the menu option */
			g_VoteMapInUse = false;	
		}
	}
	else if (action == MenuAction_Select)
	{
		decl String:info[32], String:name[32];
		
		GetMenuItem(menu, param2, info, sizeof(info), _, name, sizeof(name));
		
		/* Remove selected map from global menu and add it to the selected array */
		RemoveMenuItem(menu, param2);
		PushArrayString(g_SelectedMaps, info);
		g_SelectedCount++;
		
		
		/* Redisplay the list */
		if (g_SelectedCount < 5)
		{
			DisplayMenu(g_MapList, param1, MENU_TIME_FOREVER);
		}
		else
		{
			ConfirmVote(param1);
		}
	}
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
		g_VoteMapInUse = true;
		ClearArray(g_SelectedMaps);
		g_SelectedCount = 0;
		DisplayMenu(g_MapList, param, MENU_TIME_FOREVER);
	}
	else if (action == TopMenuAction_DrawOption)
	{	
		/* disable this option if a vote is already running, theres no maps listed or someone else has already acessed this menu */
		buffer[0] = (IsVoteInProgress() || g_mapCount < 1 || g_VoteMapInUse) ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT;
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

LoadMaps(Handle:menu)
{
	decl String:mapPath[256];
	BuildPath(Path_SM, mapPath, sizeof(mapPath), "configs/adminmenu_maplist.ini");
	
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
