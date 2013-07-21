#include <sourcemod>
#include <cstrike>

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
