
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
	if (action == TopMenuAction_DrawOption)
	{
		Format(buffer, maxlength, "%T", "Cancel vote", param);

		if (IsVoteInProgress())
		{
			return ITEMDRAW_DEFAULT;
		}
		else
		{
			return ITEMDRAW_IGNORE;
		}
	}
	else if (action == TopMenuAction_SelectOption)
	{
		PerformCancelVote(param);
		RedisplayAdminMenu(topmenu, param);	
	}

	return 0;
}

public Action:Command_CancelVote(client, args)
{
	PerformCancelVote(client);

	return Plugin_Handled;
}

