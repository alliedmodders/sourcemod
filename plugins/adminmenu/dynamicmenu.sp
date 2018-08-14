
#define NAME_LENGTH 64
#define CMD_LENGTH 255

#define ARRAY_STRING_LENGTH 32

enum GroupCommands
{
	ArrayList:groupListName,
	ArrayList:groupListCommand	
};

int g_groupList[GroupCommands];
int g_groupCount;

SMCParser g_configParser;

enum Places
{
	Place_Category,
	Place_Item,
	Place_ReplaceNum
};

char g_command[MAXPLAYERS+1][CMD_LENGTH];
int g_currentPlace[MAXPLAYERS+1][Places];

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
	ArrayList:Item_submenus
}

enum Submenu
{
	SubMenu_Type:Submenu_type,
	String:Submenu_title[32],
	PlayerMethod:Submenu_method,
	Submenu_listcount,
	DataPack:Submenu_listdata
}

ArrayList g_DataArray;

void BuildDynamicMenu()
{
	int itemInput[Item];
	g_DataArray = new ArrayList(sizeof(itemInput));
	
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
	
	char name[NAME_LENGTH];
	char buffer[NAME_LENGTH];
		
	if (!kvMenu.GotoFirstSubKey())
		return;
	
	char admin[30];
	
	TopMenuObject categoryId;
	
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

		char category_name[NAME_LENGTH];
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
				
				char temp[64];
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
  							
  			/* iterate all submenus and load data into itemInput[Item_submenus] (ArrayList) */
  			
  			int count = 1;
  			char countBuffer[10] = "1";
  			
  			char inputBuffer[48];
  			
  			while (kvMenu.JumpToKey(countBuffer))
  			{
	  			int submenuInput[Submenu];
	  			
	  			if (count == 1)
	  			{
		  			itemInput[Item_submenus] = new ArrayList(sizeof(submenuInput));	
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
					
					submenuInput[Submenu_listdata] = new DataPack();
					submenuInput[Submenu_listdata].WriteString(inputBuffer);
					submenuInput[Submenu_listdata].Reset();
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
					
					submenuInput[Submenu_listdata] = new DataPack();
					
					char temp[6];
					char value[64];
					char text[64];
					char subadm[30];	//  same as "admin", cf. line 110
					int i=1;
					bool more = true;
					
					int listcount = 0;
								
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
							submenuInput[Submenu_listdata].WriteString(value);
							submenuInput[Submenu_listdata].WriteString(text);
							submenuInput[Submenu_listdata].WriteString(subadm);
						}
						
						i++;
										
					} while (more);
					
					submenuInput[Submenu_listdata].Reset();
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
	  			
	  			itemInput[Item_submenus].PushArray(submenuInput[0]);
	  		
	  			kvMenu.GoBack();	
  			}
  			
  			/* Save this entire item into the global items array and add it to the menu */
  			
  			int location = g_DataArray.PushArray(itemInput[0]);
			
			char locString[10];
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

void ParseConfigs()
{
	if (!g_configParser)
		g_configParser = new SMCParser();
	
        g_configParser.OnEnterSection = NewSection;
        g_configParser.OnKeyValue = KeyValue;
        g_configParser.OnLeaveSection = EndSection;
	
	delete g_groupList[groupListName];
	delete g_groupList[groupListCommand];
	
	g_groupList[groupListName] = new ArrayList(ARRAY_STRING_LENGTH);
	g_groupList[groupListCommand] = new ArrayList(ARRAY_STRING_LENGTH);
	
	char configPath[256];
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
		char error[256];
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
	g_groupList[groupListName].PushString(key);
	g_groupList[groupListCommand].PushString(value);
}

public SMCResult EndSection(SMCParser smc)
{
	g_groupCount = g_groupList[groupListName].Length;
}

public void DynamicMenuCategoryHandler(TopMenu topmenu, 
						TopMenuAction action,
						TopMenuObject object_id,
						int param,
						char[] buffer,
						int maxlength)
{
	if ((action == TopMenuAction_DisplayTitle) || (action == TopMenuAction_DisplayOption))
	{
		topmenu.GetObjName(object_id, buffer, maxlength);
	}
}

public void DynamicMenuItemHandler(TopMenu topmenu, 
					  TopMenuAction action,
					  TopMenuObject object_id,
					  int param,
					  char[] buffer,
					  int maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		topmenu.GetObjName(object_id, buffer, maxlength);
	}
	else if (action == TopMenuAction_SelectOption)
	{	
		char locString[10];
		topmenu.GetInfoString(object_id, locString, sizeof(locString));
		
		int location = StringToInt(locString);
		
		int output[Item];
		g_DataArray.GetArray(location, output[0]);
		
		strcopy(g_command[param], sizeof(g_command[]), output[Item_cmd]);
					
		g_currentPlace[param][Place_Item] = location;
		g_currentPlace[param][Place_ReplaceNum] = 1;
		
		ParamCheck(param);
	}
}

public void ParamCheck(int client)
{
	char buffer[6];
	char buffer2[6];
	
	int outputItem[Item];
	int outputSubmenu[Submenu];
	
	g_DataArray.GetArray(g_currentPlace[client][Place_Item], outputItem[0]);
		
	if (g_currentPlace[client][Place_ReplaceNum] < 1)
	{
		g_currentPlace[client][Place_ReplaceNum] = 1;
	}
	
	Format(buffer, 5, "#%i", g_currentPlace[client][Place_ReplaceNum]);
	Format(buffer2, 5, "@%i", g_currentPlace[client][Place_ReplaceNum]);
	
	if (StrContains(g_command[client], buffer) != -1 || StrContains(g_command[client], buffer2) != -1)
	{
		outputItem[Item_submenus].GetArray(g_currentPlace[client][Place_ReplaceNum] - 1, outputSubmenu[0]);
		
		Menu itemMenu = new Menu(Menu_Selection);
		itemMenu.ExitBackButton = true;
			
		if ((outputSubmenu[Submenu_type] == SubMenu_Group) || (outputSubmenu[Submenu_type] == SubMenu_GroupPlayer))
		{	
			char nameBuffer[ARRAY_STRING_LENGTH];
			char commandBuffer[ARRAY_STRING_LENGTH];
		
			for (int i = 0; i<g_groupCount; i++)
			{			
				g_groupList[groupListName].GetString(i, nameBuffer, sizeof(nameBuffer));
				g_groupList[groupListCommand].GetString(i, commandBuffer, sizeof(commandBuffer));
				itemMenu.AddItem(commandBuffer, nameBuffer);
			}
		}
		
		if (outputSubmenu[Submenu_type] == SubMenu_MapCycle)
		{	
			char path[200];
			outputSubmenu[Submenu_listdata].ReadString(path, sizeof(path));
			outputSubmenu[Submenu_listdata].Reset();
		
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
			PlayerMethod playermethod = outputSubmenu[Submenu_method];
		
			char nameBuffer[MAX_NAME_LENGTH];
			char infoBuffer[32];
			char temp[4];
			
			//loop through players. Add name as text and name/userid/steamid as info
			for (int i=1; i<=MaxClients; i++)
			{
				if (IsClientInGame(i))
				{			
					GetClientName(i, nameBuffer, sizeof(nameBuffer));
					
					switch (playermethod)
					{
						case UserId:
						{
							int userid = GetClientUserId(i);
							Format(infoBuffer, sizeof(infoBuffer), "#%i", userid);
							itemMenu.AddItem(infoBuffer, nameBuffer);	
						}
						case UserId2:
						{
							int userid = GetClientUserId(i);
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
			char value[64];
			char text[64];
					
			char admin[NAME_LENGTH];
			
			for (int i=0; i<outputSubmenu[Submenu_listcount]; i++)
			{
				outputSubmenu[Submenu_listdata].ReadString(value, sizeof(value));
				outputSubmenu[Submenu_listdata].ReadString(text, sizeof(text));
				outputSubmenu[Submenu_listdata].ReadString(admin, sizeof(admin));
				
				if (CheckCommandAccess(client, admin, 0))
				{
					itemMenu.AddItem(value, text);
				}
			}
			
			outputSubmenu[Submenu_listdata].Reset();
		}
		
		itemMenu.SetTitle(outputSubmenu[Submenu_title]);
		
		itemMenu.Display(client, MENU_TIME_FOREVER);
	}
	else
	{	
		//nothing else need to be done. Run teh command.
		
		hAdminMenu.Display(client, TopMenuPosition_LastCategory);
		
		char unquotedCommand[CMD_LENGTH];
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

public int Menu_Selection(Menu menu, MenuAction action, int param1, int param2)
{
	if (action == MenuAction_End)
	{
		delete menu;
	}
	
	if (action == MenuAction_Select)
	{
		char unquotedinfo[NAME_LENGTH];
 
		/* Get item info */
		bool found = menu.GetItem(param2, unquotedinfo, sizeof(unquotedinfo));
		
		if (!found)
		{
			return 0;
		}
		
		char info[NAME_LENGTH*2+1];
		QuoteString(unquotedinfo, info, sizeof(info), "#@");
		
		
		char buffer[6];
		char infobuffer[NAME_LENGTH+2];
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

	return 0;
}

stock bool QuoteString(char[] input, char[] output, int maxlen, char[] quotechars)
{
	int count = 0;
	int len = strlen(input);
	
	for (int i=0; i<len; i++)
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

stock bool UnQuoteString(char[] input, char[] output, int maxlen, char[] quotechars)
{
	int count = 1;
	int len = strlen(input);
	
	output[0] = input[0];
	
	for (int i=1; i<len; i++)
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
