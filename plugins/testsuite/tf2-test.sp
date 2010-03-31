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
	RegConsoleCmd("sm_respawn", Command_Respawn);
	RegConsoleCmd("sm_disguise", Command_Disguise);
	RegConsoleCmd("sm_remdisguise", Command_RemDisguise);
	RegConsoleCmd("sm_class", Command_Class);
	RegConsoleCmd("sm_remove", Command_Remove);
	RegConsoleCmd("sm_changeclass", Command_ChangeClass);
	RegConsoleCmd("sm_regenerate", Command_Regenerate);
	RegConsoleCmd("sm_uberme", Command_UberMe);
	RegConsoleCmd("sm_unuberme", Command_UnUberMe);
	RegConsoleCmd("sm_setpowerplay", Command_SetPowerPlay);
	RegConsoleCmd("sm_panic", Command_Panic);
	RegConsoleCmd("sm_bighit", Command_BigHit);
	RegConsoleCmd("sm_frighten", Command_Frighten);
}

public Action:Command_Class(client, args)
{
	TF2_RemoveAllWeapons(client);
	
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

public Action:Command_Regenerate(client, args)
{
	if (client == 0)
	{
		return Plugin_Handled;
	}
	
	TF2_RegeneratePlayer(client);
	
	return Plugin_Handled;
}

public Action:Command_UberMe(client, args)
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
	
	new Float:one = StringToFloat(text);
	
	TF2_AddCondition(client, TFCond_Ubercharged, one);
	
	return Plugin_Handled;
}

public Action:Command_UnUberMe(client, args)
{
	if (client == 0)
	{
		return Plugin_Handled;
	}
	
	TF2_RemoveCondition(client, TFCond_Ubercharged);
	
	return Plugin_Handled;
}

public Action:Command_SetPowerPlay(client, args)
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
	
	new bool:one = bool:StringToInt(text);
	
	TF2_SetPlayerPowerPlay(client, one);
	
	return Plugin_Handled;
}

public Action:Command_Panic(client, args)
{
	if (client == 0)
	{
		return Plugin_Handled;
	}
	
	TF2_StunPlayer(client, 15.0, 0.25, TF_STUNFLAGS_LOSERSTATE);
	
	return Plugin_Handled;
}

public Action:Command_BigHit(client, args)
{
	if (client == 0)
	{
		return Plugin_Handled;
	}
	
	TF2_StunPlayer(client, 5.0, _, TF_STUNFLAGS_BIGBONK, client);
	
	return Plugin_Handled;
}

public Action:Command_Frighten(client, args)
{
	if (client == 0)
	{
		return Plugin_Handled;
	}
	
	TF2_StunPlayer(client, 5.0, _, TF_STUNFLAGS_GHOSTSCARE);
	
	return Plugin_Handled;
}