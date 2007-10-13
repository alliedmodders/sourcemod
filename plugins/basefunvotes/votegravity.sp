
DisplayVoteGravityMenu(client,count,String:items[5][])
{
	LogAction(client, -1, "\"%L\" initiated a gravity vote.", client);
	ShowActivity(client, "%t", "Initiated Vote Gravity");
	
	g_voteType = voteType:gravity;
	
	g_hVoteMenu = CreateMenu(Handler_VoteCallback, MenuAction:MENU_ACTIONS_ALL);
	
	if (count == 1)
	{
		strcopy(g_voteInfo[VOTE_NAME], sizeof(g_voteInfo[]), items[0]);
			
		SetMenuTitle(g_hVoteMenu, "Change Gravity To");
		AddMenuItem(g_hVoteMenu, items[0], "Yes");
		AddMenuItem(g_hVoteMenu, VOTE_NO, "No");
	}
	else
	{
		g_voteInfo[VOTE_NAME][0] = '\0';
		
		SetMenuTitle(g_hVoteMenu, "Gravity Vote");
		for (new i = 0; i < count; i++)
		{
			AddMenuItem(g_hVoteMenu, items[i], items[i]);
		}	
	}
	
	SetMenuExitButton(g_hVoteMenu, false);
	VoteMenuToAll(g_hVoteMenu, 20);
}

public AdminMenu_VoteGravity(Handle:topmenu, 
							  TopMenuAction:action,
							  TopMenuObject:object_id,
							  param,
							  String:buffer[],
							  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Vote Gravity", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		/* Might need a better way of selecting the list of pre-defined gravity choices */
		new String:items[5][5] ={"200","400","800","1600","3200"};
		DisplayVoteGravityMenu(param,5, items);
	}
	else if (action == TopMenuAction_DrawOption)
	{	
		/* disable this option if a vote is already running */
		buffer[0] = IsVoteInProgress() ? ITEMDRAW_IGNORE : ITEMDRAW_DEFAULT;
	}
}

public Action:Command_VoteGravity(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_votegravity <amount> [amount2] ... [amount5]");
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

	decl String:items[5][64];
	new count;	
	new len, pos;
	
	while (pos != -1 && count < 5)
	{	
		pos = BreakString(text[len], items[count], sizeof(items[]));
		
		decl Float:temp;
		if (StringToFloatEx(items[count], temp) == 0)
		{
			ReplyToCommand(client, "[SM] %t", "Invalid Amount");
			return Plugin_Handled;
		}		

		count++;
		
		if (pos != -1)
		{
			len += pos;
		}	
	}
	
	DisplayVoteGravityMenu(client, count, items);
	
	return Plugin_Handled;	
}