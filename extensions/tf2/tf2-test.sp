#include <sourcemod>
#include "tf2.inc"

public Plugin:myinfo = 
{
	name = "TF2 Test",
	author = "pRED*",
	description = "Test of Tf2 functions",
	version = "1.0",
	url = "www.sourcemod.net"
}

public OnPluginStart()
{
	RegConsoleCmd("sm_burnme", Command_Burn);
	RegConsoleCmd("sm_invuln", Command_Invuln);
}

public Action:Command_Burn(client, args)
{
	if (client == 0)
	{
		return Plugin_Continue;
	}
	
	TF2_Burn(client);
	
	return Plugin_Continue;
}

public Action:Command_Invuln(client, args)
{
	if (client == 0)
	{
		return Plugin_Continue;
	}
	
	if (args < 2)
	{
		return Plugin_Continue;	
	}
	
	decl String:text[10];
	decl String:text2[10];
	GetCmdArg(1, text, sizeof(text));
	GetCmdArg(2, text2, sizeof(text2));
	
	new bool:one = !!StringToInt(text);
	new bool:two = !!StringToInt(text2);

	TF2_Invuln(client, one, two)
	
	return Plugin_Continue;
}

