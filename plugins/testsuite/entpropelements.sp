#include <sourcemod>
#include <sdktools>

// Coverage:
// GetEntPropArraySize - PropSend/PropData

// GetEntPropString - PropData

// GetEntProp    - PropSend/PropData
// GetEntPropEnt - PropSend/PropData

// SetEntProp    - PropSend/PropData

public OnPluginStart()
{
	RegConsoleCmd("sm_listammo", sm_listammo, "sm_listammo <send|data> [target] - Lists current ammo for self or specified player");
	RegConsoleCmd("sm_listweapons", sm_listweapons, "sm_listweapons <send|data> [target] - Lists current weapons for self or specified player");
	RegConsoleCmd("sm_setammo", sm_setammo, "sm_setammo <ammotype> <amount> <send|data> [target]");
	RegConsoleCmd("sm_teststrings", sm_teststrings);
}

public Action:sm_listammo(client, argc)
{
	decl String:buffer[64];
	new target = client;
	if (argc >= 2)
	{
		GetCmdArg(2, buffer, sizeof(buffer));
		target = FindTarget(client, buffer, false, false);
		if (target <= 0)
		{
			ReplyToCommand(client, "Bad client");
			return Plugin_Handled;
		}
	}
	else if (client == 0)
	{
		ReplyToCommand(client, "Dillweed");
		return Plugin_Handled;
	}
	
	new PropType:proptype = PropType:-1;
	GetCmdArg(1, buffer, sizeof(buffer));
	if (!strcmp(buffer, "send"))
		proptype = Prop_Send;
	else if (!strcmp(buffer, "data"))
		proptype = Prop_Data;
	
	new ammo;
	new max = GetEntPropArraySize(target, proptype, "m_iAmmo");
	for (new i = 0; i < max; i++)
	{
		if ((ammo = GetEntProp(target, proptype, "m_iAmmo", _, i)) > 0)
			ReplyToCommand(client, "Slot %d, Ammo %d", i, ammo);
	}
		
	return Plugin_Handled;
}

public Action:sm_listweapons(client, argc)
{
	decl String:buffer[64];
	new target = client;
	if (argc >= 2)
	{
		GetCmdArg(2, buffer, sizeof(buffer));
		target = FindTarget(client, buffer, false, false);
		if (target <= 0)
		{
			ReplyToCommand(client, "Bad client");
			return Plugin_Handled;
		}
	}
	else if (client == 0)
	{
		ReplyToCommand(client, "Dillweed");
		return Plugin_Handled;
	}
	
	new PropType:proptype = PropType:-1;
	GetCmdArg(1, buffer, sizeof(buffer));
	if (!strcmp(buffer, "send"))
		proptype = Prop_Send;
	else if (!strcmp(buffer, "data"))
		proptype = Prop_Data;
	
	new weapon, String:classname[64];
	new max = GetEntPropArraySize(target, proptype, "m_hMyWeapons");
	for (new i = 0; i < max; i++)
	{
		if ((weapon = GetEntPropEnt(target, proptype, "m_hMyWeapons", i)) == -1)
			continue;
		
		GetEdictClassname(weapon, classname, sizeof(classname));
		ReplyToCommand(client, "Slot %d - \"%s\"", i, classname);
	}
		
	return Plugin_Handled;
}

public Action:sm_setammo(client, argc)
{
	decl String:buffer[64];
	new target = client;
	if (argc >= 4)
	{
		GetCmdArg(4, buffer, sizeof(buffer));
		target = FindTarget(client, buffer, false, false);
		if (target <= 0)
		{
			ReplyToCommand(client, "Bad client");
			return Plugin_Handled;
		}
	}
	else if (client == 0)
	{
		ReplyToCommand(client, "Dillweed");
		return Plugin_Handled;
	}
	
	new PropType:proptype = PropType:-1;
	GetCmdArg(3, buffer, sizeof(buffer));
	if (!strcmp(buffer, "send"))
		proptype = Prop_Send;
	else if (!strcmp(buffer, "data"))
		proptype = Prop_Data;
	
	new max = GetEntPropArraySize(target, proptype, "m_iAmmo");
	
	GetCmdArg(1, buffer, sizeof(buffer));
	new ammotype = StringToInt(buffer);
	
	if (ammotype >= max)
	{
		ReplyToCommand(client, "Ammotype out of bounds");
		return Plugin_Handled;
	}
	
	GetCmdArg(2, buffer, sizeof(buffer));
	new amount = StringToInt(buffer);
	
	SetEntProp(target, proptype, "m_iAmmo", amount, _, ammotype);
	
	return Plugin_Handled;
}

public Action:sm_teststrings(client, argc)
{
	new gpe = CreateEntityByName("game_player_equip");
	DispatchKeyValue(gpe, "tf_weapon_rocketlauncher", "2");
	DispatchKeyValue(gpe, "tf_weapon_pistol",         "1");
	
	decl String:value[64];
	for (new i = 0; i < 2; i++)
	{
		GetEntPropString(gpe, Prop_Data, "m_weaponNames", value, sizeof(value), i);
		ReplyToCommand(client, "At %d, I see a \"%s\"", i, value);
	}
	
	AcceptEntityInput(gpe, "Kill");
	
	return Plugin_Handled;
}
