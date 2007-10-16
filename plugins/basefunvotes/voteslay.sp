
DisplayVoteSlayMenu(client,target,String:name[])
{
	if (!IsPlayerAlive(target))
	{
		ReplyToCommand(client, "[SM] %t", "Cannot be performed on dead", name);
		return;
	}
	
	g_voteClient[VOTE_CLIENTID] = target;
	GetClientName(target, g_voteInfo[VOTE_NAME], sizeof(g_voteInfo[]));

	LogAction(client, target, "\"%L\" initiated a slay vote against \"%L\"", client, target);
	ShowActivity(client, "%t", "Initiated Vote Slay", g_voteInfo[VOTE_NAME]);
	
	g_voteType = voteType:slay;
	
	g_hVoteMenu = CreateMenu(Handler_VoteCallback, MenuAction:MENU_ACTIONS_ALL);
	SetMenuTitle(g_hVoteMenu, "Voteslay Player");
	AddMenuItem(g_hVoteMenu, VOTE_YES, "Yes");
	AddMenuItem(g_hVoteMenu, VOTE_NO, "No");
	SetMenuExitButton(g_hVoteMenu, false);
	VoteMenuToAll(g_hVoteMenu, 20);
}

DisplaySlayTargetMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_Slay);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Slay vote", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddTargetsToMenu(menu, client, false);
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

public AdminMenu_VoteSlay(Handle:topmenu, 
							  TopMenuAction:action,
							  TopMenuObject:object_id,
							  param,
							  String:buffer[],
							  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Slay vote", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplaySlayTargetMenu(param);
	}
	else if (action == TopMenuAction_DrawOption)
	{	
		/* disable this option if a vote is already running */
		buffer[0] = IsVoteInProgress() ? ITEMDRAW_IGNORE : ITEMDRAW_DEFAULT;
	}
}

public MenuHandler_Slay(Handle:menu, MenuAction:action, param1, param2)
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
		else if (!IsPlayerAlive(target))
		{
			PrintToChat(param1, "[SM] %t", "Player has since died");
		}
		else
		{
			DisplayVoteSlayMenu(param1, target, name);
		}
	}
}

public Action:Command_VoteSlay(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_voteslay <player>");
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
	
	decl String:text[256], String:arg[64];
	GetCmdArgString(text, sizeof(text));
	
	BreakString(text, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	DisplayVoteSlayMenu(client, target, arg);
	
	return Plugin_Handled;
}