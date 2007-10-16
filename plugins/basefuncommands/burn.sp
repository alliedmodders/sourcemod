PerformBurn(client, target, Float:seconds)
{
	new String:name[32];

	GetClientName(target, name, sizeof(name));
	
	if (!IsPlayerAlive(target))
	{
		ReplyToCommand(client, "[SM] %t", "Cannot be performed on dead", name);
		return;
	}

	ShowActivity(client, "%t", "Ignited player", name);
	LogAction(client, target, "\"%L\" ignited \"%L\" (seconds \"%f\")", client, target, seconds);
	IgniteEntity(target, seconds);
}

DisplayBurnMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_Burn);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Burn player", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddTargetsToMenu(menu, client, false);
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

public AdminMenu_Burn(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Burn player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayBurnMenu(param);
	}
}

public MenuHandler_Burn(Handle:menu, MenuAction:action, param1, param2)
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
			PerformBurn(param1, target, 20.0);
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplaySlayMenu(param1);
		}
	}
}

public Action:Command_Burn(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_burn <#userid|name> [time]");
		return Plugin_Handled;
	}

	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));

	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}
	
	new Float:seconds = 20.0;
	
	if (args > 1)
	{
		decl String:time[20];
		GetCmdArg(2, time, sizeof(time));
		if (StringToFloatEx(time, seconds) == 0)
		{
			ReplyToCommand(client, "[SM] %t", "Invalid Amount");
			return Plugin_Handled;
		}
	}
	
	PerformBurn(client, target, seconds);

	return Plugin_Handled;
}