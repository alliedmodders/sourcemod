enum CommType
{
	CommType_Mute,
	CommType_UnMute,
	CommType_Gag,
	CommType_UnGag,
	CommType_Silence,
	CommType_UnSilence

};

DisplayGagTypesMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_GagTypes);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Choose Type", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	new target = g_GagTarget[client];
	
	if (!g_Muted[target])
	{
		AddMenuItem(menu, "0", "Mute Player");
	}
	else
	{
		AddMenuItem(menu, "1", "UnMute Player");
	}
	
	if (!g_Gagged[target])
	{
		AddMenuItem(menu, "2", "Gag Player");
	}
	else
	{
		AddMenuItem(menu, "3", "UnGag Player");
	}
	
	if (!g_Muted[target] || !g_Gagged[target])
	{
		AddMenuItem(menu, "4", "Silence Player");
	}
	else
	{
		AddMenuItem(menu, "5", "UnSilence Player");
	}
		
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

DisplayGagPlayerMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_GagPlayer);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Gag/Mute Player", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddTargetsToMenu(menu, client, false);
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

public AdminMenu_Gag(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Gag/Mute Player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayGagPlayerMenu(param);
	}
}

public MenuHandler_GagPlayer(Handle:menu, MenuAction:action, param1, param2)
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
			g_GagTarget[param1] = GetClientOfUserId(userid);
			DisplayGagTypesMenu(param1);
		}
	}
}

public MenuHandler_GagTypes(Handle:menu, MenuAction:action, param1, param2)
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
		new CommType:type;
		
		GetMenuItem(menu, param2, info, sizeof(info));
		type = CommType:StringToInt(info);

		switch (type)
		{
			case CommType_Mute:	PerformMute(param1, g_GagTarget[param1]);
			case CommType_UnMute:	PerformUnMute(param1, g_GagTarget[param1]);
			case CommType_Gag:	PerformGag(param1, g_GagTarget[param1]);
			case CommType_UnGag:	PerformUnGag(param1, g_GagTarget[param1]);
			case CommType_Silence:	PerformSilence(param1, g_GagTarget[param1]);
			case CommType_UnSilence:	PerformUnSilence(param1, g_GagTarget[param1]);
		}
	}
}

PerformMute(client, target)
{
	if (g_Muted[target])
	{
		ReplyToCommand(client, "%t", "Already Muted");
		return;		
	}
		
	g_Muted[target] = true;
	SetClientListeningFlags(target, VOICE_MUTED);
	
	decl String:name[64];
	GetClientName(target, name, sizeof(name));

	ShowActivity(client, "%t", "Player Muted", name);
	ReplyToCommand(client, "%t", "Player Muted", name);
	LogAction(client, target, "\"%L\" muted \"%L\"", client, target);
}

PerformUnMute(client, target)
{
	if (!g_Muted[target])
	{
		ReplyToCommand(client, "%t", "Player Not Muted");
		return;		
	}
		
	g_Muted[target] = false;
	if (GetConVarInt(g_Cvar_Deadtalk) == 1 && !IsPlayerAlive(target))
	{
		SetClientListeningFlags(target, VOICE_LISTENALL);
	}
	else if (GetConVarInt(g_Cvar_Deadtalk) == 2 && !IsPlayerAlive(target))
	{
		SetClientListeningFlags(target, VOICE_TEAM);
	}
	else
	{
		SetClientListeningFlags(target, VOICE_NORMAL);
	}
	
	decl String:name[64];
	GetClientName(target, name, sizeof(name));

	ShowActivity(client, "%t", "Player Unmuted", name);
	ReplyToCommand(client, "%t", "Player Unmuted", name);
	LogAction(client, target, "\"%L\" unmuted \"%L\"", client, target);
}

PerformGag(client, target)
{
	if (g_Gagged[target])
	{
		ReplyToCommand(client, "%t", "Already Gagged");
		return;		
	}
		
	g_Gagged[target] = true;
	
	decl String:name[64];
	GetClientName(target, name, sizeof(name));

	ShowActivity(client, "%t", "Player Gagged", name);
	ReplyToCommand(client, "%t", "Player Gagged", name);
	LogAction(client, target, "\"%L\" gagged \"%L\"", client, target);
}

PerformUnGag(client, target)
{
	if (!g_Gagged[target])
	{
		ReplyToCommand(client, "%t", "Player Not Gagged");
		return;		
	}
		
	g_Gagged[target] = false;
	
	decl String:name[64];
	GetClientName(target, name, sizeof(name));

	ShowActivity(client, "%t", "Player Ungagged", name);
	ReplyToCommand(client, "%t", "Player Ungagged", name);
	LogAction(client, target, "\"%L\" ungagged \"%L\"", client, target);
}

PerformSilence(client, target)
{
	if (g_Gagged[target] && g_Muted[target])
	{
		ReplyToCommand(client, "%t", "Already Silenced");
		return;		
	}

	decl String:name[64];
	GetClientName(target, name, sizeof(name));
	
	if (!g_Gagged[target])
	{
		g_Gagged[target] = true;

		ShowActivity(client, "%t", "Player Gagged", name);
		ReplyToCommand(client, "%t", "Player Gagged", name);
		LogAction(client, target, "\"%L\" gagged \"%L\"", client, target);
	}
	
	if (!g_Muted[target])
	{
		g_Muted[target] = true;
		SetClientListeningFlags(target, VOICE_MUTED);
	
		ShowActivity(client, "%t", "Player Muted", name);
		ReplyToCommand(client, "%t", "Player Muted", name);
		LogAction(client, target, "\"%L\" muted \"%L\"", client, target);
	}
}

PerformUnSilence(client, target)
{
	if (!g_Gagged[target] && !g_Muted[target])
	{
		ReplyToCommand(client, "%t", "Player Not Silenced");
		return;		
	}
	
	decl String:name[64];
	GetClientName(target, name, sizeof(name));
	
	if (g_Gagged[target])
	{
		g_Gagged[target] = false;
		
		ShowActivity(client, "%t", "Player Ungagged", name);
		ReplyToCommand(client, "%t", "Player Ungagged", name);	
		LogAction(client, target, "\"%L\" ungagged \"%L\"", client, target);
	}
	
	if (g_Muted[target])
	{
		g_Muted[target] = false;
		
		if (GetConVarInt(g_Cvar_Deadtalk) == 1 && !IsPlayerAlive(target))
		{
			SetClientListeningFlags(target, VOICE_LISTENALL);
		}
		else if (GetConVarInt(g_Cvar_Deadtalk) == 2 && !IsPlayerAlive(target))
		{
			SetClientListeningFlags(target, VOICE_TEAM);
		}
		else
		{
			SetClientListeningFlags(target, VOICE_NORMAL);
		}
		
		ShowActivity(client, "%t", "Player Unmuted", name);
		ReplyToCommand(client, "%t", "Player Unmuted", name);
		LogAction(client, target, "\"%L\" unmuted \"%L\"", client, target);		
	}
}

public Action:Command_Mute(client, args)
{	
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_mute <player>");
		return Plugin_Handled;
	}
	
	decl String:arg[64];
	GetCmdArg(1, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	PerformMute(client, target);
	
	return Plugin_Handled;	
}

public Action:Command_Gag(client, args)
{	
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_gag <player>");
		return Plugin_Handled;
	}
	
	decl String:arg[64];
	GetCmdArg(1, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	PerformGag(client, target);
	
	return Plugin_Handled;	
}

public Action:Command_Silence(client, args)
{	
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_silence <player>");
		return Plugin_Handled;
	}
	
	decl String:arg[64];
	GetCmdArg(1, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	PerformSilence(client, target);
	
	return Plugin_Handled;	
}

public Action:Command_Unmute(client, args)
{	
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_unmute <player>");
		return Plugin_Handled;
	}
	
	decl String:arg[64];
	GetCmdArg(1, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	PerformUnMute(client, target);
	
	return Plugin_Handled;	
}

public Action:Command_Ungag(client, args)
{	
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_ungag <player>");
		return Plugin_Handled;
	}
	
	decl String:arg[64];
	GetCmdArg(1, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	PerformUnGag(client, target);
	
	return Plugin_Handled;	
}

public Action:Command_Unsilence(client, args)
{	
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_unsilence <player>");
		return Plugin_Handled;
	}
	
	decl String:arg[64];
	GetCmdArg(1, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	PerformUnSilence(client, target);
	
	return Plugin_Handled;	
}