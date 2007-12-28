
#define NAME_LENGTH 32
#define CMD_LENGTH 255

#define ARRAY_STRING_LENGTH 32

new Handle:g_kvMenu;

enum GroupCommands
{
	Handle:groupListName,
	Handle:groupListCommand	
};

new g_groupList[GroupCommands];
new g_groupCount;

new Handle:g_configParser = INVALID_HANDLE;

enum Places
{
	Place_Category,
	Place_Item,
	Place_ReplaceNum	
};

new String:g_command[MAXPLAYERS+1][CMD_LENGTH];
new g_currentPlace[MAXPLAYERS+1][Places];

/**
 * What to put in the 'info' menu field (for PlayerList and Player_Team menus only)
 */
enum PlayerMethod
{		
	ClientId,				/** Client id number ( 1 - Maxplayers) */
	UserId,					/** Client userid */
	Name,					/** Client Name */
	SteamId,				/** Client Steamid */
	IpAddress,				/** Client's Ip Address */
	UserId2					/** Userid (not prefixed with #) */
};

BuildDynamicMenu()
{
	if (g_kvMenu != INVALID_HANDLE)
	{
		CloseHandle(g_kvMenu);	
	}
	
	g_kvMenu = CreateKeyValues("Commands");
	new String:file[256];
	BuildPath(Path_SM, file, 255, "configs/dynamicmenu/menu.ini");
	FileToKeyValues(g_kvMenu, file);
	
	new String:name[NAME_LENGTH];
	new String:buffer[NAME_LENGTH];
	
	
	if (!KvGotoFirstSubKey(g_kvMenu))
	{
		return;
	}
	
	decl String:admin[30];
	
	new TopMenuObject:categoryId;
	
	new catId;
	new id;
	
	do
	{		
		KvGetSectionName(g_kvMenu, buffer, sizeof(buffer));

		KvGetString(g_kvMenu, "admin", admin, sizeof(admin),"sm_admin");
				
		if ((categoryId =FindTopMenuCategory(hAdminMenu, buffer)) == INVALID_TOPMENUOBJECT)
		{
			categoryId = AddToTopMenu(hAdminMenu,
							buffer,
							TopMenuObject_Category,
							DynamicMenuCategoryHandler,
							INVALID_TOPMENUOBJECT,
							admin,
							ADMFLAG_GENERIC,
							name);

		}
		
		if (!KvGetSectionSymbol(g_kvMenu, catId))
		{
			LogError("Key Id not found for section: %s", buffer);
			break;
		}
		
		if (!KvGotoFirstSubKey(g_kvMenu))
		{
			return;
		}
		
		do
		{		
			KvGetSectionName(g_kvMenu, buffer, sizeof(buffer));

			KvGetString(g_kvMenu, "admin", admin, sizeof(admin),"");
			
			if (admin[0] == '\0')
			{
				//No 'admin' keyvalue was found
				//Use the first argument of the 'cmd' string instead
				
				decl String:temp[64];
				KvGetString(g_kvMenu, "cmd", temp, sizeof(temp),"");
				
				BreakString(temp, admin, sizeof(admin));
			}
							  
			if (!KvGetSectionSymbol(g_kvMenu, id))
			{
				LogError("Key Id not found for section: %s");
				break;
			}
			
			decl String:keyId[64];
			
			Format(keyId, sizeof(keyId), "%i %i", catId, id);
		
			AddToTopMenu(hAdminMenu,
							buffer,
							TopMenuObject_Item,
							DynamicMenuItemHandler,
  							categoryId,
  							admin,
  							ADMFLAG_GENERIC,
  							keyId);
		
		} while (KvGotoNextKey(g_kvMenu));
		
		KvGoBack(g_kvMenu);
		
	} while (KvGotoNextKey(g_kvMenu));
	
	KvRewind(g_kvMenu);	
}

ParseConfigs()
{
	if (g_configParser == INVALID_HANDLE)
	{
		g_configParser = SMC_CreateParser();
	}
	
	SMC_SetReaders(g_configParser, NewSection, KeyValue, EndSection);
	
	if (g_groupList[groupListName] != INVALID_HANDLE)
	{
		CloseHandle(g_groupList[groupListName]);
	}
	
	if (g_groupList[groupListCommand] != INVALID_HANDLE)
	{
		CloseHandle(g_groupList[groupListCommand]);
	}
	
	g_groupList[groupListName] = CreateArray(ARRAY_STRING_LENGTH);
	g_groupList[groupListCommand] = CreateArray(ARRAY_STRING_LENGTH);
	
	decl String:configPath[256];
	BuildPath(Path_SM, configPath, sizeof(configPath), "configs/dynamicmenu/adminmenu_grouping.txt");
	
	if (!FileExists(configPath))
	{
		LogError("Unable to locate admin menu groups file, no groups loaded.");
			
		return;		
	}
	
	new line;
	new SMCError:err = SMC_ParseFile(g_configParser, configPath, line);
	if (err != SMCError_Okay)
	{
		decl String:error[256];
		SMC_GetErrorString(err, error, sizeof(error));
		LogError("Could not parse file (line %d, file \"%s\"):", line, configPath);
		LogError("Parser encountered error: %s", error);
	}
	
	return;
}

public SMCResult:NewSection(Handle:smc, const String:name[], bool:opt_quotes)
{

}

public SMCResult:KeyValue(Handle:smc, const String:key[], const String:value[], bool:key_quotes, bool:value_quotes)
{
	PushArrayString(g_groupList[groupListName], key);
	PushArrayString(g_groupList[groupListCommand], value);
}

public SMCResult:EndSection(Handle:smc)
{
	g_groupCount = GetArraySize(g_groupList[groupListName]);
}

public DynamicMenuCategoryHandler(Handle:topmenu, 
						TopMenuAction:action,
						TopMenuObject:object_id,
						param,
						String:buffer[],
						maxlength)
{
	if ((action == TopMenuAction_DisplayTitle) || (action == TopMenuAction_DisplayOption))
	{
		GetTopMenuObjName(topmenu, object_id, buffer, maxlength);
	}
}

public DynamicMenuItemHandler(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		GetTopMenuObjName(topmenu, object_id, buffer, maxlength);
	}
	else if (action == TopMenuAction_SelectOption)
	{	
		new String:keyId[64];
		new String:catId[64];
		GetTopMenuInfoString(topmenu, object_id, keyId, sizeof(keyId));
		
		new start = BreakString(keyId, catId, sizeof(catId));
		
		new id = StringToInt(keyId[start]);
		new category = StringToInt(catId);
		
		KvJumpToKeySymbol(g_kvMenu, category);
		KvJumpToKeySymbol(g_kvMenu, id);
		
		KvGetString(g_kvMenu, "cmd", g_command[param], sizeof(g_command[]),"");
		KvRewind(g_kvMenu);
					
		g_currentPlace[param][Place_Category] = category;
		g_currentPlace[param][Place_Item] = id;
		
		ParamCheck(param);
	}
}

public ParamCheck(client)
{
	new String:buffer[6];
	new String:buffer2[6];

	KvJumpToKeySymbol(g_kvMenu, g_currentPlace[client][Place_Category]);
	KvJumpToKeySymbol(g_kvMenu, g_currentPlace[client][Place_Item]);
	
	new String:type[NAME_LENGTH];
		
	if (g_currentPlace[client][Place_ReplaceNum] < 1)
	{
		g_currentPlace[client][Place_ReplaceNum] = 1;
	}
	
	Format(buffer, 5, "#%i", g_currentPlace[client][Place_ReplaceNum]);
	Format(buffer2, 5, "@%i", g_currentPlace[client][Place_ReplaceNum]);
	
	if (StrContains(g_command[client], buffer) != -1 || StrContains(g_command[client], buffer2) != -1)
	{
		//user has a parameter to fill. lets do it.	
		KvJumpToKey(g_kvMenu, buffer[1]); // Jump to current param
		KvGetString(g_kvMenu, "type", type, sizeof(type),"list");
		
		PrintToChatAll("Checking param %s - type %s", buffer[1], type);
		
		new Handle:itemMenu = CreateMenu(Menu_Selection);
		
		new String:title[NAME_LENGTH];
			
		if (strncmp(type,"group",5)==0 && g_groupCount)
		{	
			decl String:nameBuffer[ARRAY_STRING_LENGTH];
			decl String:commandBuffer[ARRAY_STRING_LENGTH];
		
			for (new i = 0; i<g_groupCount; i++)
			{			
				GetArrayString(g_groupList[groupListName], i, nameBuffer, sizeof(nameBuffer));
				GetArrayString(g_groupList[groupListCommand], i, commandBuffer, sizeof(commandBuffer));
				AddMenuItem(itemMenu, commandBuffer, nameBuffer);
			}
		}
		
		if (strncmp(type,"mapcycle",8) == 0)
		{
			decl String:path[200];
			KvGetString(g_kvMenu, "path", path, sizeof(path),"mapcycle.txt");
		
			new Handle:file = OpenFile(path, "rt");
			new String:readData[128];
			
			if(file != INVALID_HANDLE)
			{
				while(!IsEndOfFile(file) && ReadFileLine(file, readData, sizeof(readData)))
				{
					TrimString(readData);
					
					if (IsMapValid(readData))
					{
						AddMenuItem(itemMenu, readData, readData);
					}
				}
			}
		}
		else if (StrContains(type, "player") != -1)
		{
			PrintToChatAll("Building Player List");
			
			new PlayerMethod:playermethod;
			new String:method[NAME_LENGTH];
			
			KvGetString(g_kvMenu, "method", method, sizeof(method),"name");
			
			if (strncmp(method,"clientid",8)==0)
			{
				playermethod = ClientId;
			}
			else if (strncmp(method,"steamid",7)==0)
			{
				playermethod = SteamId;
			}
			else if (strncmp(method,"userid2",7)==0)
			{
				playermethod = UserId2;
			}
			else if (strncmp(method,"userid",6)==0)
			{
				playermethod = UserId;
			}
			else if (strncmp(method,"ip",2)==0)
			{
				playermethod = IpAddress;
			}
			else
			{
				playermethod = Name;
			}
	
			
			new String:nameBuffer[32];
			new String:infoBuffer[32];
			new String:temp[4];
			
			//loop through players. Add name as text and name/userid/steamid as info
			for (new i=1; i<=g_maxPlayers; i++)
			{
				if (IsClientInGame(i))
				{			
					GetClientName(i, nameBuffer, 31);
					
					switch (playermethod)
					{
						case UserId:
						{
							new userid = GetClientUserId(i);
							Format(infoBuffer, sizeof(infoBuffer), "#%i", userid);
							AddMenuItem(itemMenu, infoBuffer, nameBuffer);	
						}
						case UserId2:
						{
							new userid = GetClientUserId(i);
							Format(infoBuffer, sizeof(infoBuffer), "%i", userid);
							AddMenuItem(itemMenu, infoBuffer, nameBuffer);							
						}
						case SteamId:
						{
							GetClientAuthString(i, infoBuffer, sizeof(infoBuffer));
							AddMenuItem(itemMenu, infoBuffer, nameBuffer);							
						}	
						case IpAddress:
						{
							GetClientIP(i, infoBuffer, sizeof(infoBuffer));
							AddMenuItem(itemMenu, infoBuffer, nameBuffer);							
						}
						case Name:
						{
							AddMenuItem(itemMenu, nameBuffer, nameBuffer);
						}	
						default: //assume client id
						{
							Format(temp,3,"%i",i);
							AddMenuItem(itemMenu, temp, nameBuffer);						
						}								
					}
				}
			}
		}
		else if (strncmp(type,"onoff",5) == 0)
		{
			AddMenuItem(itemMenu, "1", "On");
			AddMenuItem(itemMenu, "0", "Off");
		}		
		else
		{
			//list menu
			
			PrintToChatAll("Building List Menu");

			new String:temp[6];
			new String:value[64];
			new String:text[64];
			new i=1;
			new bool:more = true;
					
			new String:admin[NAME_LENGTH];
						
			do
			{
				// load the i and i. options from kv and make a menu from them (i* = required admin level to view)
				Format(temp,3,"%i",i);
				KvGetString(g_kvMenu, temp, value, sizeof(value), "");
				
				Format(temp,5,"%i.",i);
				KvGetString(g_kvMenu, temp, text, sizeof(text), value);
				
				Format(temp,5,"%i*",i);
				KvGetString(g_kvMenu, temp, admin, sizeof(admin),"");	
				
				if (value[0]=='\0')
				{
					more = false;
				}
				else if (CheckCommandAccess(client, admin, 0))
				{
					AddMenuItem(itemMenu, value, text);
				}
				
				i++;
								
			} while (more);
		
		}
		
		KvGetString(g_kvMenu, "title", title, sizeof(title),"Choose an Option");
		SetMenuTitle(itemMenu, title);
		
		DisplayMenu(itemMenu, client, MENU_TIME_FOREVER);
	}
	else
	{	
		//nothing else need to be done. Run teh command.
		new String:execute[7];
		KvGetString(g_kvMenu, "execute", execute, sizeof(execute), "player");
		
		DisplayTopMenu(hAdminMenu, client, TopMenuPosition_LastCategory);
		
		if (execute[0]=='p') // assume 'player' type execute option
		{
			FakeClientCommand(client, g_command[client]);
		}
		else // assume 'server' type execute option
		{
			InsertServerCommand(g_command[client]);
			ServerExecute();
		}

		g_command[client][0] = '\0';
		g_currentPlace[client][Place_ReplaceNum] = 1;
	}
	
	KvRewind(g_kvMenu);
}

public Menu_Selection(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_End)
	{
		CloseHandle(menu);
	}
	
	if (action == MenuAction_Select)
	{
		new String:info[NAME_LENGTH];
 
		/* Get item info */
		new bool:found = GetMenuItem(menu, param2, info, sizeof(info));
		
		if (!found)
		{
			return;
		}
		
		new String:buffer[6];
		new String:infobuffer[NAME_LENGTH+2];
		Format(infobuffer, sizeof(infobuffer), "\"%s\"", info);
		
		Format(buffer, 5, "#%i", g_currentPlace[param1][Place_ReplaceNum]);
		ReplaceString(g_command[param1], sizeof(g_command[]), buffer, infobuffer);
		//replace #num with the selected option (quoted)
		
		Format(buffer, 5, "@%i", g_currentPlace[param1][Place_ReplaceNum]);
		ReplaceString(g_command[param1], sizeof(g_command[]), buffer, info);
		//replace @num with the selected option (unquoted)
		
		// Increment the parameter counter.
		g_currentPlace[param1][Place_ReplaceNum]++;
		
		ParamCheck(param1);
	}
	
	if (action == MenuAction_Cancel && param2 == MenuCancel_Exit)
	{
		//client exited we should go back to submenu i think
		DisplayTopMenu(hAdminMenu, param1, TopMenuPosition_LastCategory);
	}
}