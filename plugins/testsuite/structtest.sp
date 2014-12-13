#include <sourcemod>
#include <structs>

public Plugin:myinfo = 
{
	name = "Struct Abstraction Test",
	author = "pRED*",
	description = "",
	version = "1.0",
	url = "http://www.sourcemod.net/"
};

public OnPluginStart()
{
	RegServerCmd("sm_getstructstring", Command_GetString);
	RegServerCmd("sm_setstructstring", Command_SetString);
	
	RegServerCmd("sm_getstructint", Command_GetInt);
	RegServerCmd("sm_setstructint", Command_SetInt);
	
	RegServerCmd("sm_getstructfloat", Command_GetFloat);
	RegServerCmd("sm_setstructfloat", Command_SetFloat);

}

public Action:Command_GetString(args)
{
	if (args != 2)
	{
		ReplyToCommand(0, "[SM] Usage: sm_getstructstring <struct> <string>");
		return Plugin_Handled;
	}

	decl String:arg1[32], String:arg2[32], String:value[32];
	GetCmdArg(1, arg1, sizeof(arg1));
	GetCmdArg(2, arg2, sizeof(arg2));

	new Handle:strct = GetWeaponStruct(arg1);
	GetStructString(strct, arg2, value, sizeof(value));

	LogMessage("Value of %s: %s", arg2, value);

	delete strct;

	return Plugin_Handled;
}

public Action:Command_SetString(args)
{
	if (args != 3)
	{
		ReplyToCommand(0, "[SM] Usage: sm_setstructstring <struct> <string> <value>");
		return Plugin_Handled;
	}

	decl String:arg1[32], String:arg2[32], String:value[32];
	GetCmdArg(1, arg1, sizeof(arg1));
	GetCmdArg(2, arg2, sizeof(arg2));
	GetCmdArg(3, value, sizeof(value));

	new Handle:strct = GetWeaponStruct(arg1);
	SetStructString(strct, arg2, value);

	delete strct;

	return Plugin_Handled;	
	
}

public Action:Command_GetInt(args)
{
	if (args != 2)
	{
		ReplyToCommand(0, "[SM] Usage: sm_getstructint <struct> <string>");
		return Plugin_Handled;
	}

	decl String:arg1[32], String:arg2[32];
	GetCmdArg(1, arg1, sizeof(arg1));
	GetCmdArg(2, arg2, sizeof(arg2));

	new Handle:strct = GetWeaponStruct(arg1);
	new value = GetStructInt(strct, arg2);

	LogMessage("Value of %s: %i", arg2, value);

	delete strct;

	return Plugin_Handled;
}

public Action:Command_SetInt(args)
{
	if (args != 3)
	{
		ReplyToCommand(0, "[SM] Usage: sm_setstructint <struct> <string> <value>");
		return Plugin_Handled;
	}

	decl String:arg1[32], String:arg2[32], String:value[32];
	GetCmdArg(1, arg1, sizeof(arg1));
	GetCmdArg(2, arg2, sizeof(arg2));
	GetCmdArg(3, value, sizeof(value));

	new Handle:strct = GetWeaponStruct(arg1);
	SetStructInt(strct, arg2, StringToInt(value));


	delete strct;

	return Plugin_Handled;
}

public Action:Command_GetFloat(args)
{
	if (args != 2)
	{
		ReplyToCommand(0, "[SM] Usage: sm_getstructint <struct> <string>");
		return Plugin_Handled;
	}

	decl String:arg1[32], String:arg2[32];
	GetCmdArg(1, arg1, sizeof(arg1));
	GetCmdArg(2, arg2, sizeof(arg2));

	new Handle:strct = GetWeaponStruct(arg1);
	new Float:value = GetStructFloat(strct, arg2);

	LogMessage("Value of %s: %f", arg2, value);

	delete strct;

	return Plugin_Handled;
}

public Action:Command_SetFloat(args)
{
	if (args != 3)
	{
		ReplyToCommand(0, "[SM] Usage: sm_setstructint <struct> <string> <value>");
		return Plugin_Handled;
	}

	decl String:arg1[32], String:arg2[32], String:value[32];
	GetCmdArg(1, arg1, sizeof(arg1));
	GetCmdArg(2, arg2, sizeof(arg2));
	GetCmdArg(3, value, sizeof(value));

	new Handle:strct = GetWeaponStruct(arg1);
	SetStructFloat(strct, arg2, StringToFloat(value));


	delete strct;

	return Plugin_Handled;
}
