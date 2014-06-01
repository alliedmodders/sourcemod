#include <sourcemod>
#include <cstrike>
#include <sdktools>

stock GET_ARG_INT( arg, maxSize=64 )
{
	decl String:tempvar[maxSize];
	GetCmdArg( arg, tempvar, maxSize );
	return StringToInt( tempvar );
}

public OnPluginStart()
{
	RegConsoleCmd( "get_mvps",    get_mvps    );
	RegConsoleCmd( "set_mvps",    set_mvps    );
	RegConsoleCmd( "get_score",   get_score   );
	RegConsoleCmd( "set_score",   set_score   );
	RegConsoleCmd( "get_assists", get_assists );
	RegConsoleCmd( "set_assists", set_assists );
	RegConsoleCmd( "get_clantag", get_clantag );
	RegConsoleCmd( "set_clantag", set_clantag );
	RegConsoleCmd( "get_teamscore", get_teamscore );
	RegConsoleCmd( "set_teamscore", set_teamscore );
}

public Action:get_mvps( client, argc )
{
	ReplyToCommand( client, "Your MVP count is %d", CS_GetMVPCount( client ) );
	
	return Plugin_Handled;
}

public Action:set_mvps( client, argc )
{
	new count = GET_ARG_INT( 1 );
	
	CS_SetMVPCount( client, count );
	ReplyToCommand( client, "Set your MVP count to %d", count );
	
	return Plugin_Handled;
}

public Action:get_score( client, argc )
{
	if( GetEngineVersion() != Engine_CSGO )
	{
		ReplyToCommand( client, "This command is only intended for CS:GO" );
		return Plugin_Handled;
	}
	
	ReplyToCommand( client, "Your contribution score is %d", CS_GetClientContributionScore( client ) );

	return Plugin_Handled;
}

public Action:set_score( client, argc )
{
	if( GetEngineVersion() != Engine_CSGO )
	{
		ReplyToCommand( client, "This command is only intended for CS:GO" );
		return Plugin_Handled;
	}
	
	new count = GET_ARG_INT( 1 );
	
	CS_SetClientContributionScore( client, count );
	ReplyToCommand( client, "Set your contribution score to %d", count );

	return Plugin_Handled;
}

public Action:get_assists( client, argc )
{
	if( GetEngineVersion() != Engine_CSGO )
	{
		ReplyToCommand( client, "This command is only intended for CS:GO" );
		return Plugin_Handled;
	}
	
	ReplyToCommand( client, "Your assist count is %d", CS_GetClientAssists( client ) );

	return Plugin_Handled;
}

public Action:set_assists( client, argc )
{
	if( GetEngineVersion() != Engine_CSGO )
	{
		ReplyToCommand( client, "This command is only intended for CS:GO" );
		return Plugin_Handled;
	}
	
	new count = GET_ARG_INT( 1 );
	
	CS_SetClientAssists( client, count );
	ReplyToCommand( client, "Set your assist count to %d", count );

	return Plugin_Handled;
}

public Action:get_clantag( client, argc )
{
	decl String:tag[64];

	CS_GetClientClanTag( client, tag, sizeof(tag) );
	ReplyToCommand( client, "Your clan tag is: %s", tag );

	return Plugin_Handled;
}

public Action:set_clantag( client, argc )
{
	decl String:tag[64];
	GetCmdArg( 1, tag, sizeof(tag) );

	CS_SetClientClanTag( client, tag );
	ReplyToCommand( client, "Set your clan tag to: %s", tag );

	return Plugin_Handled;
}

public Action:get_teamscore( client, argc )
{
	new tscore = CS_GetTeamScore( CS_TEAM_T );
	new ctscore = CS_GetTeamScore( CS_TEAM_CT );

	ReplyToCommand( client, "Team Scores: T = %d, CT = %d", tscore, ctscore );

	return Plugin_Handled;
}

public Action:set_teamscore( client, argc )
{
	new team = GET_ARG_INT( 1 );
	new score = GET_ARG_INT( 2 );

	if ( team != CS_TEAM_T && team != CS_TEAM_CT )
	{
		ReplyToCommand( client, "Team number must be %d or %d", CS_TEAM_T, CS_TEAM_CT );
		return Plugin_Handled;
	}

	CS_SetTeamScore( team, score );
	SetTeamScore( team, score );
	ReplyToCommand( client, "Score for team %d has been set to %d", team, score );

	return Plugin_Handled;
}

