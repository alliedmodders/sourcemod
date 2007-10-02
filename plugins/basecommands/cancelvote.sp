
PerformCancelVote(client)
{
	if (!IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote Not In Progress");
		return;
	}

	ShowActivity(client, "%t", "Cancelled Vote");
	
	CancelVote();
}
	
public AdminMenu_CancelVote(Handle:topmenu, 
							  TopMenuAction:action,
							  TopMenuObject:object_id,
							  param,
							  String:buffer[],
							  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Cancel vote", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		PerformCancelVote(param);
		RedisplayAdminMenu(topmenu, param);	
	}
	else if (action == TopMenuAction_DrawOption)
	{
		buffer[0] = IsVoteInProgress() ? ITEMDRAW_DEFAULT : ITEMDRAW_IGNORE;
	}
}

public Action:Command_CancelVote(client, args)
{
	PerformCancelVote(client);

	return Plugin_Handled;
}

