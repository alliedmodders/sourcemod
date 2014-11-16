
#define NAME_LENGTH 64
#define CMD_LENGTH 255

#define ARRAY_STRING_LENGTH 32

enum GroupCommands
{
	Handle:groupListName,
	Handle:groupListCommand	
};

new g_groupList[GroupCommands];
new g_groupCount;

SMCParser g_configParser;

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

enum ExecuteType
{
	Execute_Player,
	Execute_Server
}

enum SubMenu_Type
{
	SubMenu_Group,
	SubMenu_GroupPlayer,
	SubMenu_Player,
	SubMenu_MapCycle,
	SubMenu_List,
	SubMenu_OnOff	
}

enum Item
{
	String:Item_cmd[256],
	ExecuteType:Item_execute,
	Handle:Item_submenus
}

enum Submenu
{
	SubMenu_Type:Submenu_type,
	String:Submenu_title[32],
	PlayerMethod:Submenu_method,
	Submenu_listcount,
	Handle:Submenu_listdata
}

new Handle:g_DataArray;

BuildDynamicMenu()
{
	int itemInput[Item];
	g_DataArray = CreateArray(sizeof(itemInput));
	
	char executeBuffer[32];
	
	KeyValues kvMenu = new KeyValues("Commands");
	kvMenu.SetEscapeSequences(true); 
	
	char file[256];
	
	/* As a compatibility shim, we use the old file if it exists. */
	BuildPath(Path_SM, file, 255, "configs/dynamicmenu/menu.ini");
	if (FileExists(file))
	{
		LogError("Warning! configs/dynamicmenu/menu.ini is now configs/adminmenu_custom.txt.");
		LogError("Read the 1.0.2 release notes, as the dynamicmenu folder has been removed.");
	}
	else
	{
		BuildPath(Path_SM, file, 255, "configs/adminmenu_custom.txt");
	}
	
	FileToKeyValues(kvMenu, file);
	
	new String:name[NAME_LENGTH];
	new String:buffer[NAME_LENGTH];
		
	if (!kvMenu.GotoFirstSubKey())
		return;
	
	decl String:admin[30];
	
	new TopMenuObject:categoryId;
	
	do
	{		
		kvMenu.GetSectionName(buffer, sizeof(buffer));

		kvMenu.GetString("admin", admin, sizeof(admin),"sm_admin");
				
		if ((categoryId = hAdminMenu.FindCategory(buffer)) == INVALID_TOPMENUOBJECT)
		{
			categoryId = hAdminMenu.AddCategory(buffer,
							DynamicMenuCategoryHandler,
							admin,
							ADMFLAG_GENERIC,
							name);

		}

		decl String:category_name[NAME_LENGTH];
		strcopy(category_name, sizeof(category_name), buffer);
		
		if (!kvMenu.GotoFirstSubKey())
		{
			return;
		}
		
		do
		{		
			kvMenu.GetSectionName(buffer, sizeof(buffer));
			
			kvMenu.GetString("admin", admin, sizeof(admin),"");
			
			if (admin[0] == '\0')
			{
				//No 'admin' keyvalue was found
				//Use the first argument of the 'cmd' string instead
				
				decl String:temp[64];
				kvMenu.GetString("cmd", temp, sizeof(temp),"");
				
				BreakString(temp, admin, sizeof(admin));
			}
			
			
			kvMenu.GetString("cmd", itemInput[Item_cmd], sizeof(itemInput[Item_cmd]));	
			kvMenu.GetString("execute", executeBuffer, sizeof(executeBuffer));
			
			if (StrEqual(executeBuffer, "server"))
			{
				itemInput[Item_execute] = Execute_Server;
			}
			else //assume player type execute
			{
				itemInput[Item_execute] = Execute_Player;
			}
  							
  			/* iterate all submenus and load data into itemInput[Item_submenus] (adt array handle) */
  			
  			new count = 1;
  			decl String:countBuffer[10] = "1";
  			
  			decl String:inputBuffer[48];
  			
  			while (kvMenu.JumpToKey(countBuffer))
  			{
	  			new submenuInput[Submenu];
	  			
	  			if (count == 1)
	  			{
		  			itemInput[Item_submenus] = CreateArray(sizeof(submenuInput));	
	  			}
	  			
	  			kvMenu.GetString("type", inputBuffer, sizeof(inputBuffer));
	  			
	  			if (strncmp(inputBuffer,"group",5)==0)
				{	
					if (StrContains(inputBuffer, "player") != -1)
					{			
						submenuInput[Submenu_type] = SubMenu_GroupPlayer;
					}
					else
					{
						submenuInput[Submenu_type] = SubMenu_Group;
					}
				}			
				else if (StrEqual(inputBuffer,"mapcycle"))
				{
					submenuInput[Submenu_type] = SubMenu_MapCycle;
					
					kvMenu.GetString("path", inputBuffer, sizeof(inputBuffer),"mapcycle.txt");
					
					submenuInput[Submenu_listdata] = CreateDataPack();
					WritePackString(submenuInput[Submenu_listdata], inputBuffer);
					ResetPack(submenuInput[Submenu_listdata]);
				}
				else if (StrContains(inputBuffer, "player") != -1)
				{			
					submenuInput[Submenu_type] = SubMenu_Player;
				}
				else if (StrEqual(inputBuffer,"onoff"))
				{
					submenuInput[Submenu_type] = SubMenu_OnOff;
				}		
				else //assume 'list' type
				{
					submenuInput[Submenu_type] = SubMenu_List;
					
					submenuInput[Submenu_listdata] = CreateDataPack();
					
					new String:temp[6];
					new String:value[64];
					new String:text[64];
					new String:subadm[30];	//  same as "admin", cf. line 110
					new i=1;
					new bool:more = true;
					
					new listcount = 0;
								
					do
					{
						Format(temp,3,"%i",i);
						kvMenu.GetString(temp, value, sizeof(value), "");
						
						Format(temp,5,"%i.",i);
						kvMenu.GetString(temp, text, sizeof(text), value);
						
						Format(temp,5,"%i*",i);
						kvMenu.GetString(temp, subadm, sizeof(subadm),"");	
						
						if (value[0]=='\0')
						{
							more = false;
						}
						else
						{
							listcount++;
							WritePackString(submenuInput[Submenu_listdata], value);
							WritePackString(submenuInput[Submenu_listdata], text);
							WritePackString(submenuInput[Submenu_listdata], subadm);
						}
						
						i++;
										
					} while (more);
					
					ResetPack(submenuInput[Submenu_listdata]);
					submenuInput[Submenu_listcount] = listcount;
				}
				
				if ((submenuInput[Submenu_type] == SubMenu_Player) || (submenuInput[Submenu_type] == SubMenu_GroupPlayer))
				{
					kvMenu.GetString("method", inputBuffer, sizeof(inputBuffer));
					
					if (StrEqual(inputBuffer, "clientid"))
					{
						submenuInput[Submenu_method] = ClientId;
					}
					else if (StrEqual(inputBuffer, "steamid"))
					{
						submenuInput[Submenu_method] = SteamId;
					}
					else if (StrEqual(inputBuffer, "userid2"))
					{
						submenuInput[Submenu_method] = UserId2;
					}
					else if (StrEqual(inputBuffer, "userid"))
					{
						submenuInput[Submenu_method] = UserId;
					}
					else if (StrEqual(inputBuffer, "ip"))
					{
						submenuInput[Submenu_method] = IpAddress;
					}
					else
					{
						submenuInput[Submenu_method] = Name;
					}				
				}
				
				kvMenu.GetString("title", inputBuffer, sizeof(inputBuffer));
				strcopy(submenuInput[Submenu_title], sizeof(submenuInput[Submenu_title]), inputBuffer);
	  			
	  			count++;
	  			Format(countBuffer, sizeof(countBuffer), "%i", count);
	  			
	  			PushArrayArray(itemInput[Item_submenus], submenuInput[0]);
	  		
	  			kvMenu.GoBack();	
  			}
  			
  			/* Save this entire item into the global items array and add it to the menu */
  			
  			new location = PushArrayArray(g_DataArray, itemInput[0]);
			
			decl String:locString[10];
			IntToString(location, locString, sizeof(locString));

			if (hAdminMenu.AddItem(buffer,
				DynamicMenuItemHandler,
  				categoryId,
  				admin,
  				ADMFLAG_GENERIC,
  				locString) == INVALID_TOPMENUOBJECT)
			{
				LogError("Duplicate command name \"%s\" in adminmenu_custom.txt category \"%s\"", buffer, category_name);
			}
		
		} while (kvMenu.GotoNextKey());
		
		kvMenu.GoBack();
		
	} while (kvMenu.GotoNextKey());
	
	delete kvMenu;
}

ParseConfigs()
{
	if (!g_configParser)
		g_configParser = new SMCParser();
	
        g_configParser.OnEnterSection = NewSection;
        g_configParser.OnKeyValue = KeyValue;
        g_configParser.OnLeaveSection = EndSection;
	
	if (g_groupList[groupListName] != INVALID_HANDLE)
	{
		CloseHandle(g_groupList[groupListName]);
	}
	
	if (g_groupList[groupListCommand] != null)
	{
		CloseHandle(g_groupList[groupListCommand]);
	}
	
	g_groupList[groupListName] = CreateArray(ARRAY_STRING_LENGTH);
	g_groupList[groupListCommand] = CreateArray(ARRAY_STRING_LENGTH);
	
	decl String:configPath[256];
	BuildPath(Path_SM, configPath, sizeof(configPath), "configs/dynamicmenu/adminmenu_grouping.txt");
	if (FileExists(configPath))
	{
		LogError("Warning! configs/dynamicmenu/adminmenu_grouping.txt is now configs/adminmenu_grouping.txt.");
		LogError("Read the 1.0.2 release notes, as the dynamicmenu folder has been removed.");
	}
	else
	{
		BuildPath(Path_SM, configPath, sizeof(configPath), "configs/adminmenu_grouping.txt");
	}
	
	if (!FileExists(configPath))
	{
		LogError("Unable to locate admin menu groups file: %s", configPath);
			
		return;		
	}
	
	int line;
	SMCError err = g_configParser.ParseFile(configPath, line);
	if (err != SMCError_Okay)
	{
		decl String:error[256];
		SMC_GetErrorString(err, error, sizeof(error));
		LogError("Could not parse file (line %d, file \"%s\"):", line, configPath);
		LogError("Parser encountered error: %s", error);
	}
	
	return;
}

public SMCResult NewSection(SMCParser smc, const char[] name, bool opt_quotes)
{

}

public SMCResult KeyValue(SMCParser smc, const char[] key, const char[] value, bool key_quotes, bool value_quotes)
{
	PushArrayString(g_groupList[groupListName], key);
	PushArrayString(g_groupList[groupListCommand], value);
}

public SMCResult EndSection(SMCParser smc)
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
		new String:locString[10];
		GetTopMenuInfoString(topmenu, object_id, locString, sizeof(locString));
		
		new location = StringToInt(locString);
		
		new output[Item];
		GetArrayArray(g_DataArray, location, output[0]);
		
		strcopy(g_command[param], sizeof(g_command[]), output[Item_cmd]);
					
		g_currentPlace[param][Place_Item] = location;
		g_currentPlace[param][Place_ReplaceNum] = 1;
		
		ParamCheck(param);
	}
}

public ParamCheck(client)
{
	new String:buffer[6];
	new String:buffer2[6];
	
	new outputItem[Item];
	new outputSubmenu[Submenu];
	
	GetArrayArray(g_DataArray, g_currentPlace[client][Place_Item], outputItem[0]);
		
	if (g_currentPlace[client][Place_ReplaceNum] < 1)
	{
		g_currentPlace[client][Place_ReplaceNum] = 1;
	}
	
	Format(buffer, 5, "#%i", g_currentPlace[client][Place_ReplaceNum]);
	Format(buffer2, 5, "@%i", g_currentPlace[client][Place_ReplaceNum]);
	
	if (StrContains(g_command[client], buffer) != -1 || StrContains(g_command[client], buffer2) != -1)
	{
		GetArrayArray(outputItem[Item_submenus], g_currentPlace[client][Place_ReplaceNum] - 1, outputSubmenu[0]);
		
		Menu itemMenu = CreateMenu(Menu_Selection);
		itemMenu.ExitBackButton = true;
			
		if ((outputSubmenu[Submenu_type] == SubMenu_Group) || (outputSubmenu[Submenu_type] == SubMenu_GroupPlayer))
		{	
			decl String:nameBuffer[ARRAY_STRING_LENGTH];
			decl String:commandBuffer[ARRAY_STRING_LENGTH];
		
			for (new i = 0; i<g_groupCount; i++)
			{			
				GetArrayString(g_groupList[groupListName], i, nameBuffer, sizeof(nameBuffer));
				GetArrayString(g_groupList[groupListCommand], i, commandBuffer, sizeof(commandBuffer));
				itemMenu.AddItem(commandBuffer, nameBuffer);
			}
		}
		
		if (outputSubmenu[Submenu_type] == SubMenu_MapCycle)
		{	
			decl String:path[200];
			ReadPackString(outputSubmenu[Submenu_listdata], path, sizeof(path));
			ResetPack(outputSubmenu[Submenu_listdata]);
		
			File file = OpenFile(path, "rt");
			char readData[128];
			
			if (file)
			{
				while (!file.EndOfFile() && file.ReadLine(readData, sizeof(readData)))
				{
					TrimString(readData);
					
					if (IsMapValid(readData))
					{
						itemMenu.AddItem(readData, readData);
					}
				}
			}
		}
		else if ((outputSubmenu[Submenu_type] == SubMenu_Player) || (outputSubmenu[Submenu_type] == SubMenu_GroupPlayer))
		{
			new PlayerMethod:playermethod = outputSubmenu[Submenu_method];
		
			new String:nameBuffer[32];
			new String:infoBuffer[32];
			new String:temp[4];
			
			//loop through players. Add name as text and name/userid/steamid as info
			for (new i=1; i<=MaxClients; i++)
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
							itemMenu.AddItem(infoBuffer, nameBuffer);	
						}
						case UserId2:
						{
							new userid = GetClientUserId(i);
							Format(infoBuffer, sizeof(infoBuffer), "%i", userid);
							itemMenu.AddItem(infoBuffer, nameBuffer);							
						}
						case SteamId:
						{
							if (GetClientAuthId(i, AuthId_Steam2, infoBuffer, sizeof(infoBuffer)))
								itemMenu.AddItem(infoBuffer, nameBuffer);
						}	
						case IpAddress:
						{
							GetClientIP(i, infoBuffer, sizeof(infoBuffer));
							itemMenu.AddItem(infoBuffer, nameBuffer);							
						}
						case Name:
						{
							itemMenu.AddItem(nameBuffer, nameBuffer);
						}	
						default: //assume client id
						{
							Format(temp,3,"%i",i);
							itemMenu.AddItem(temp, nameBuffer);						
						}								
					}
				}
			}
		}
		else if (outputSubmenu[Submenu_type] == SubMenu_OnOff)
		{
			itemMenu.AddItem("1", "On");
			itemMenu.AddItem("0", "Off");
		}		
		else
		{
			new String:value[64];
			new String:text[64];
					
			new String:admin[NAME_LENGTH];
			
			for (new i=0; i<outputSubmenu[Submenu_listcount]; i++)
			{
				ReadPackString(outputSubmenu[Submenu_listdata], value, sizeof(value));
				ReadPackString(outputSubmenu[Submenu_listdata], text, sizeof(text));
				ReadPackString(outputSubmenu[Submenu_listdata], admin, sizeof(admin));
				
				if (CheckCommandAccess(client, admin, 0))
				{
					itemMenu.AddItem(value, text);
				}
			}
			
			ResetPack(outputSubmenu[Submenu_listdata]);	
		}
		
		itemMenu.SetTitle(outputSubmenu[Submenu_title]);
		
		itemMenu.Display(client, MENU_TIME_FOREVER);
	}
	else
	{	
		//nothing else need to be done. Run teh command.
		
		hAdminMenu.Display(client, TopMenuPosition_LastCategory);
		
		decl String:unquotedCommand[CMD_LENGTH];
		UnQuoteString(g_command[client], unquotedCommand, sizeof(unquotedCommand), "#@");
		
		if (outputItem[Item_execute] == Execute_Player) // assume 'player' type execute option
		{
			FakeClientCommand(client, unquotedCommand);
		}
		else // assume 'server' type execute option
		{
			InsertServerCommand(unquotedCommand);
			ServerExecute();
		}

		g_command[client][0] = '\0';
		g_currentPlace[client][Place_ReplaceNum] = 1;
	}
}

public Menu_Selection(Menu menu, MenuAction action, int param1, int param2)
{
	if (action == MenuAction_End)
	{
		delete menu;
	}
	
	if (action == MenuAction_Select)
	{
		new String:unquotedinfo[NAME_LENGTH];
 
		/* Get item info */
		new bool:found = menu.GetItem(param2, unquotedinfo, sizeof(unquotedinfo));
		
		if (!found)
		{
			return;
		}
		
		new String:info[NAME_LENGTH*2+1];
		QuoteString(unquotedinfo, info, sizeof(info), "#@");
		
		
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
	
	if (action == MenuAction_Cancel && param2 == MenuCancel_ExitBack)
	{
		//client exited we should go back to submenu i think
		hAdminMenu.Display(param1, TopMenuPosition_LastCategory);
	}
}


stock bool:QuoteString(String:input[], String:output[], maxlen, String:quotechars[])
{
	new count = 0;
	new len = strlen(input);
	
	for (new i=0; i<len; i++)
	{
		output[count] = input[i];
		count++;
		
		if (count >= maxlen)
		{
			/* Null terminate for safety */
			output[maxlen-1] = 0;
			return false;	
		}
		
		if (FindCharInString(quotechars, input[i]) != -1 || input[i] == '\\')
		{
			/* This char needs escaping */
			output[count] = '\\';
			count++;
			
			if (count >= maxlen)
			{
				/* Null terminate for safety */
				output[maxlen-1] = 0;
				return false;	
			}		
		}
	}
	
	output[count] = 0;
	
	return true;
}

stock bool:UnQuoteString(String:input[], String:output[], maxlen, String:quotechars[])
{
	new count = 1;
	new len = strlen(input);
	
	output[0] = input[0];
	
	for (new i=1; i<len; i++)
	{
		output[count] = input[i];
		count++;
		
		if (input[i+1] == '\\' && (input[i] == '\\' || FindCharInString(quotechars, input[i]) != -1))
		{
			/* valid quotechar followed by a backslash - Skip */
			i++; 
		}
		
		if (count >= maxlen)
		{
			/* Null terminate for safety */
			output[maxlen-1] = 0;
			return false;	
		}
	}
	
	output[count] = 0;
	
	return true;
}
