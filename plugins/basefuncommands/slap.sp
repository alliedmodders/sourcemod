PerformSlap(client, target, damage)
{
	LogAction(client, target, "\"%L\" slapped \"%L\" (damage \"%d\")", client, target, damage);
	SlapPlayer(target, damage, true);
}

DisplaySlapDamageMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_SlapDamage);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Slap damage", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddMenuItem(menu, "0", "0");
	AddMenuItem(menu, "1", "1");
	AddMenuItem(menu, "5", "5");
	AddMenuItem(menu, "10", "10");
	AddMenuItem(menu, "20", "20");
	AddMenuItem(menu, "50", "50");
	AddMenuItem(menu, "99", "99");
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

DisplaySlapTargetMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_Slap);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Slap player", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddTargetsToMenu(menu, client, false);
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

public AdminMenu_Slap(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Slap player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplaySlapDamageMenu(param);
	}
}

public MenuHandler_SlapDamage(Handle:menu, MenuAction:action, param1, param2)
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
		
		GetMenuItem(menu, param2, info, sizeof(info));
		g_SlapDamage[param1] = StringToInt(info);
		
		DisplaySlapTargetMenu(param1);
	}
}

public MenuHandler_Slap(Handle:menu, MenuAction:action, param1, param2)
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
		else if (!IsPlayerAlive(target))
		{
			ReplyToCommand(param1, "[SM] %t", "Player has since died");
		}	
		else
		{
			decl String:name[32];
			GetClientName(target, name, sizeof(name));
			PerformSlap(param1, target, g_SlapDamage[param1]);
			ShowActivity2(param1, "[SM] ", "%t", "Slapped target", "_s", name);
		}
		
		DisplaySlapTargetMenu(param1);
	}
}

public Action:Command_Slap(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_slap <#userid|name> [damage]");
		return Plugin_Handled;
	}

	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));

	new damage = 0;
	if (args > 1)
	{
		decl String:arg2[20];
		GetCmdArg(2, arg2, sizeof(arg2));
		if (StringToIntEx(arg2, damage) == 0)
		{
			ReplyToCommand(client, "[SM] %t", "Invalid Amount");
			return Plugin_Handled;
		}
	}
	
	decl String:target_name[MAX_TARGET_LENGTH];
	decl target_list[MAXPLAYERS], target_count, bool:tn_is_ml;
	
	if ((target_count = ProcessTargetString(
			arg,
			client,
			target_list,
			MAXPLAYERS,
			COMMAND_FILTER_NO_BOTS,
			target_name,
			sizeof(target_name),
			tn_is_ml)) <= 0)
	{
		ReplyToTargetError(client, target_count);
		return Plugin_Handled;
	}
	
	for (new i = 0; i < target_count; i++)
	{
		PerformSlap(client, target_list[i], damage);
	}

	if (tn_is_ml)
	{
		ShowActivity2(client, "[SM] ", "%t", "Slapped target", target_name);
	}
	else
	{
		ShowActivity2(client, "[SM] ", "%t", "Slapped target", "_s", target_name);
	}

	return Plugin_Handled;
}
