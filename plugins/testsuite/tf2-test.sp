#include <sourcemod>
#include <sdktools>
#include <tf2>
#include <tf2_stocks>

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
	RegConsoleCmd("sm_respawn", Command_Respawn);
	RegConsoleCmd("sm_disguise", Command_Disguise);
	RegConsoleCmd("sm_remdisguise", Command_RemDisguise);
	RegConsoleCmd("sm_class", Command_Class);
	RegConsoleCmd("sm_remove", Command_Remove);
	RegConsoleCmd("sm_changeclass", Command_ChangeClass);
}

public Action:Command_Class(client, args)
{
	TF2_RemoveAllWeapons(client);

	decl String:text[10];
	GetCmdArg(1, text, sizeof(text));
	
	new one = StringToInt(text);
	
	TF2_EquipPlayerClassWeapons(client, TFClassType:one);
	
	PrintToChat(client, "Test: sniper's classnum is %i (should be %i)", TF2_GetClass("sniper"), TFClass_Sniper);
		
	return Plugin_Handled;
}

public Action:Command_Remove(client, args)
{
	decl String:text[10];
	GetCmdArg(1, text, sizeof(text));
	
	new one = StringToInt(text);
	
	TF2_RemoveWeaponSlot(client, one);
	
	PrintToChat(client, "Test: heavy's classnum is %i (should be %i)", TF2_GetClass("heavy"), TFClass_Heavy);
	
	new doms = TF2_GetPlayerResourceData(client, TFResource_Dominations);
	PrintToChat(client, "Dominations read test: %i", doms);
	
	TF2_SetPlayerResourceData(client, TFResource_Dominations, doms + 10);
	doms = TF2_GetPlayerResourceData(client, TFResource_Dominations);
	PrintToChat(client, "Dominations write test: %i", doms);
	
	/* Note: This didn't appear to change my dominations value when I pressed tab. */
	
	return Plugin_Handled;
}

public Action:Command_ChangeClass(client, args)
{
	decl String:text[10];
	GetCmdArg(1, text, sizeof(text));
	
	new one = StringToInt(text);
	
	PrintToChat(client, "Current class is :%i", TF2_GetPlayerClass(client));
	
	TF2_SetPlayerClass(client, TFClassType:one);
	
	PrintToChat(client, "New class is :%i", TF2_GetPlayerClass(client));
		
	return Plugin_Handled;
}



public Action:Command_Burn(client, args)
{
	if (client == 0)
	{
		return Plugin_Handled;
	}
	
	TF2_IgnitePlayer(client, client);
	
	return Plugin_Handled;
}

public Action:Command_Invuln(client, args)
{
	if (client == 0)
	{
		return Plugin_Handled;
	}
	
	if (args < 1)
	{
		return Plugin_Handled;	
	}
	
	decl String:text[10];
	GetCmdArg(1, text, sizeof(text));
	
	new bool:one = !!StringToInt(text);

	TF2_SetPlayerInvuln(client, one)
	
	return Plugin_Handled;
}

public Action:Command_Disguise(client, args)
{
	if (client == 0)
	{
		return Plugin_Handled;
	}
	
	if (args < 2)
	{
		return Plugin_Handled;	
	}
	
	decl String:text[10];
	decl String:text2[10];
	GetCmdArg(1, text, sizeof(text));
	GetCmdArg(2, text2, sizeof(text2));
	
	new one = StringToInt(text);
	new two = StringToInt(text2);

	TF2_DisguisePlayer(client, TFTeam:one, TFClassType:two);
	
	return Plugin_Handled;
}

public Action:Command_RemDisguise(client, args)
{
	if (client == 0)
	{
		return Plugin_Handled;
	}
	
	TF2_RemovePlayerDisguise(client);
	
	return Plugin_Handled;
}


public Action:Command_Respawn(client, args)
{
	if (client == 0)
	{
		return Plugin_Handled;
	}
	

	TF2_RespawnPlayer(client);
	
	return Plugin_Handled;
}
