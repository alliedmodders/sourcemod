DisplayVoteFFMenu(client)
{
	if (IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote in Progress");
		return;
	}	
	
	if (!TestVoteDelay(client))
	{
		return;
	}
	
	LogAction(client, -1, "\"%L\" initiated a friendly fire vote.", client);
	ShowActivity(client, "%t", "Initiated Vote FF");
	
	g_voteType = voteType:ff;
	g_voteInfo[VOTE_NAME][0] = '\0';
	
	g_hVoteMenu = CreateMenu(Handler_VoteCallback, MenuAction:MENU_ACTIONS_ALL);
	
	if (GetConVarBool(g_Cvar_Alltalk))
	{
		SetMenuTitle(g_hVoteMenu, "Voteff Off");
	}
	else
	{
		SetMenuTitle(g_hVoteMenu, "Voteff On");
	}
	
	AddMenuItem(g_hVoteMenu, VOTE_YES, "Yes");
	AddMenuItem(g_hVoteMenu, VOTE_NO, "No");
	SetMenuExitButton(g_hVoteMenu, false);
	VoteMenuToAll(g_hVoteMenu, 20);
}

public AdminMenu_VoteFF(Handle:topmenu, 
							  TopMenuAction:action,
							  TopMenuObject:object_id,
							  param,
							  String:buffer[],
							  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Vote FF", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayVoteFFMenu(param);
	}
	else if (action == TopMenuAction_DrawOption)
	{	
		/* disable this option if a vote is already running */
		buffer[0] = IsVoteInProgress() ? ITEMDRAW_IGNORE : ITEMDRAW_DEFAULT;
	}
}

public Action:Command_VoteFF(client, args)
{
	if (args > 0)
	{
		ReplyToCommand(client, "[SM] Usage: sm_voteff");
		return Plugin_Handled;	
	}
	
	DisplayVoteFFMenu(client);
	
	return Plugin_Handled;
}