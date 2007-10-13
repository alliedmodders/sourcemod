PrepareBan(client, target, time, String:reason[], size)
{
	decl String:authid[64], String:name[32];
	GetClientAuthString(target, authid, sizeof(authid));
	GetClientName(target, name, sizeof(name));

	if (!time)
	{
		if (reason[0] == '\0')
		{
			ShowActivity(client, "%t", "Permabanned player", name);
		} else {
			ShowActivity(client, "%t", "Permabanned player reason", name, reason);
		}
	} else {
		if (reason[0] == '\0')
		{
			ShowActivity(client, "%t", "Banned player", name, time);
		} else {
			ShowActivity(client, "%t", "Banned player reason", name, time, reason);
		}
	}

	LogAction(client, target, "\"%L\" banned \"%L\" (minutes \"%d\") (reason \"%s\")", client, target, time, reason);

	if (reason[0] == '\0')
	{
		strcopy(reason, size, "Banned");
	}

	BanClient(target, time, BANFLAG_AUTO, reason, reason, "sm_ban", client);
}

DisplayBanTargetMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_BanPlayerList);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Ban Player", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddTargetsToMenu(menu, client, false);
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

DisplayBanTimeMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_BanTimeList);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Ban Player", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddMenuItem(menu, "0", "Permanent");
	AddMenuItem(menu, "10", "10 Minutes");
	AddMenuItem(menu, "30", "30 Minutes");
	AddMenuItem(menu, "60", "1 Hour");
	AddMenuItem(menu, "240", "4 Hours");
	AddMenuItem(menu, "1440", "1 Day");
	AddMenuItem(menu, "10080", "1 Week");
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

DisplayBanReasonMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_BanReasonList);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Ban Reason", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddMenuItem(menu, "Abusive", "Abusive");
	AddMenuItem(menu, "Racism", "Racism");
	AddMenuItem(menu, "General cheating/exploits", "General cheating/exploits");
	AddMenuItem(menu, "Wallhack", "Wallhack");
	AddMenuItem(menu, "Aimbot", "Aimbot");
	AddMenuItem(menu, "Speedhacking", "Speedhacking");
	AddMenuItem(menu, "Mic spamming", "Mic spamming");
	AddMenuItem(menu, "Admin disrepect", "Admin disrepect");
	AddMenuItem(menu, "Camping", "Camping");
	AddMenuItem(menu, "Team killing", "Team killing");
	AddMenuItem(menu, "Unacceptable Spray", "Unacceptable Spray");
	AddMenuItem(menu, "Breaking Server Rules", "Breaking Server Rules");
	AddMenuItem(menu, "Other", "Other");
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

public AdminMenu_Ban(Handle:topmenu, 
							  TopMenuAction:action,
							  TopMenuObject:object_id,
							  param,
							  String:buffer[],
							  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Ban Player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayBanTargetMenu(param);
	}
}

public MenuHandler_BanReasonList(Handle:menu, MenuAction:action, param1, param2)
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
		decl String:info[64];
		
		GetMenuItem(menu, param2, info, sizeof(info));
	
		PrepareBan(param1, g_BanTarget[param1], g_BanTime[param1], info, sizeof(info));
	}
}

public MenuHandler_BanPlayerList(Handle:menu, MenuAction:action, param1, param2)
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
		decl String:info[32], String:name[32];
		new userid, target;
		
		GetMenuItem(menu, param2, info, sizeof(info), _, name, sizeof(name));
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
			g_BanTarget[param1] = target;
			DisplayBanTimeMenu(param1);
		}
	}
}

public MenuHandler_BanTimeList(Handle:menu, MenuAction:action, param1, param2)
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
		g_BanTime[param1] = StringToInt(info);
		
		DisplayBanReasonMenu(param1);
	}
}


public Action:Command_Ban(client, args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_ban <#userid|name> <minutes|0> [reason]");
		return Plugin_Handled;
	}

	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));

	new target = FindTarget(client, arg, true);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	decl String:s_time[12];
	GetCmdArg(2, s_time, sizeof(s_time));

	new time = StringToInt(s_time);

	decl String:reason[128];
	if (args >= 3)
	{
		GetCmdArg(3, reason, sizeof(reason));
	} else {
		reason[0] = '\0';
	}

	PrepareBan(client, target, time, reason, sizeof(reason));

	return Plugin_Handled;
}