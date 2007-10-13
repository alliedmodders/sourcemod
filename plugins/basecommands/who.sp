
PerformWho(client, target, ReplySource:reply)
{
	new flags = GetUserFlagBits(target);
	decl String:flagstring[255];
	if (flags == 0)
	{
		strcopy(flagstring, sizeof(flagstring), "none");
	} else if (flags & ADMFLAG_ROOT) {
		strcopy(flagstring, sizeof(flagstring), "root");
	} else {
		FlagsToString(flagstring, sizeof(flagstring), flags);
	}

	if (reply == SM_REPLY_TO_CHAT)
		PrintToChat(client, "[SM] %t: %s", "Access", flagstring);
	else
		PrintToConsole(client, "[SM] %t: %s", "Access", flagstring);
}

DisplayWhoMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_Who);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Check player access", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddTargetsToMenu(menu, client, false);
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

public AdminMenu_Who(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Check player access", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayWhoMenu(param);
	}
}

public MenuHandler_Who(Handle:menu, MenuAction:action, param1, param2)
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
		decl String:info[32];
		new userid, target;
		
		GetMenuItem(menu, param2, info, sizeof(info));
		userid = StringToInt(info);

		if ((target = GetClientOfUserId(userid)) == 0)
		{
			PrintToChat(param1, "[SM] %t", "Player no longer available");
		}
		else if (!CanUserTarget(param1, target))
		{
			PrintToChat(param1, "[SM] %t", "Unable to target");
		}
		else
		{
			PerformWho(param1, target, SM_REPLY_TO_CHAT);
		}
		
		/* Re-draw the menu if they're still valid */
		
		/* - Close the menu? redisplay? jump back up to the category?
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayWhoMenu(param1);
		}
		*/
	}
}
public Action:Command_Who(client, args)
{
	if (args < 1)
	{
		/* Display header */
		decl String:t_access[16], String:t_name[16];
		Format(t_access, sizeof(t_access), "%t", "Access", client);
		Format(t_name, sizeof(t_name), "%t", "Name", client);

		PrintToConsole(client, "%-24.23s %s", t_name, t_access);

		/* List all players */
		new maxClients = GetMaxClients();
		decl String:flagstring[255];

		for (new i=1; i<=maxClients; i++)
		{
			if (!IsClientInGame(i))
			{
				continue;
			}
			new flags = GetUserFlagBits(i);
			if (flags == 0)
			{
				strcopy(flagstring, sizeof(flagstring), "none");
			} else if (flags & ADMFLAG_ROOT) {
				strcopy(flagstring, sizeof(flagstring), "root");
			} else {
				FlagsToString(flagstring, sizeof(flagstring), flags);
			}
			decl String:name[65];
			GetClientName(i, name, sizeof(name));
			PrintToConsole(client, "%d. %-24.23s %s", i, name, flagstring);
		}

		if (GetCmdReplySource() == SM_REPLY_TO_CHAT)
		{
			ReplyToCommand(client, "[SM] %t", "See console for output");
		}

		return Plugin_Handled;
	}

	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));

	new clients[2];
	new numClients = SearchForClients(arg, clients, 2);

	if (numClients == 0)
	{
		ReplyToCommand(client, "[SM] %t", "No matching client");
		return Plugin_Handled;
	} else if (numClients > 1) {
		ReplyToCommand(client, "[SM] %t", "More than one client matches", arg);
		return Plugin_Handled;
	}
	
	PerformWho(client, clients[0], GetCmdReplySource());

	return Plugin_Handled;
}
