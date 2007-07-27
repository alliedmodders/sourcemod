/**
 * basefuncmds.sp
 * Implements basic punishment commands.
 * This file is part of SourceMod, Copyright (C) 2004-2007 AlliedModders LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#pragma semicolon 1

#include <sourcemod>
#include <sdktools>

public Plugin:myinfo = 
{
	name = "Basic Fun Commands",
	author = "AlliedModders LLC",
	description = "Basic Fun Commands",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

public OnPluginStart()
{
	LoadTranslations("common.phrases");

	RegAdminCmd("sm_burn", Command_Burn, ADMFLAG_SLAY, "sm_burn <#userid|name> [time]");
	RegAdminCmd("sm_slap", Command_Slap, ADMFLAG_SLAY, "sm_slap <#userid|name> [damage]");
	RegAdminCmd("sm_slay", Command_Slay, ADMFLAG_SLAY, "sm_slay <#userid|name>");
}

public Action:Command_Burn(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_burn <#userid|name> [time]");
		return Plugin_Handled;
	}	
	
	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));	
	
	new Float:seconds = 20.0;
	
	if (args > 1)
	{
		decl String:time[20];
		GetCmdArg(2, time, sizeof(time));
		if (StringToFloatEx(time, seconds) == 0)
		{
			ReplyToCommand(client, "[SM] %t", "Invalid Amount");
			return Plugin_Handled;
		}		
	}
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}
	
	GetClientName(target, arg, sizeof(arg));
	
	ShowActivity(client, "%t", "Ignited player", arg);
	LogMessage("\"%L\" ignited \"%L\" (seconds \"%f\")", client, target, seconds);
	IgniteEntity(target, seconds);
	
	return Plugin_Handled;	
}

public Action:Command_Slap(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_slap <#userid|name> [damage]");
		return Plugin_Handled;
	}	
	
	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));	
	
	new damage = 5;
	
	if (args > 1)
	{
		decl String:arg2[20];
		GetCmdArg(2, arg2, sizeof(arg2));
		if (StringToIntEx(arg2, damage) == 0)
		{
			ReplyToCommand(client, "[SM] %t", "Invalid Amount");
			return Plugin_Handled;
		}		
	}
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}
	
	GetClientName(target, arg, sizeof(arg));
	
	ShowActivity(client, "%t", "Slapped player", arg);
	LogMessage("\"%L\" slapped \"%L\" (damage \"%d\")", client, target, damage);
	SlapPlayer(target, damage, true);
	
	return Plugin_Handled;	
}

public Action:Command_Slay(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_slay <#userid|name>");
		return Plugin_Handled;
	}	
	
	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));	
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}
	
	GetClientName(target, arg, sizeof(arg));
	
	ShowActivity(client, "%t", "Slayed player", arg);
	LogMessage("\"%L\" slayed \"%L\"", client, target);
	ForcePlayerSuicide(target);
	
	return Plugin_Handled;	
}