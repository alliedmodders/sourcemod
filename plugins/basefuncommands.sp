/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basic Fun Commands Plugin
 * Implements basic punishment commands.
 *
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
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
	RegAdminCmd("sm_play", Command_Play, ADMFLAG_GENERIC, "sm_play <#userid|name> <filename>");
}

public Action:Command_Play(client, args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_play <#userid|name> <filename>");
	}

	new String:Arguments[PLATFORM_MAX_PATH + 65];
	GetCmdArgString(Arguments, sizeof(Arguments));

 	decl String:Arg[65];
	new len = BreakString(Arguments, Arg, sizeof(Arg));

	new target = FindTarget(client, Arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	/* Make sure it does not go out of bound by doing "sm_play user  "*/
	if (len == -1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_play <#userid|name> <filename>");
		return Plugin_Handled;
	}

	/* Incase they put quotes and white spaces after the quotes */
	if (Arguments[len] == '"')
	{
		len++;
		new FileLen = TrimString(Arguments[len]) + len;

		if (Arguments[FileLen - 1] == '"')
		{
			Arguments[FileLen - 1] = '\0';
		}
	}

	GetClientName(target, Arg, sizeof(Arg));
	ShowActivity(client, "%t", "Played Sound", Arg);
	LogMessage("\"%L\" played sound on \"%L\" (file \"%s\")", client, target, Arguments[len]);

	ClientCommand(target, "playgamesound \"%s\"", Arguments[len]);

	return Plugin_Handled;
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

	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	GetClientName(target, arg, sizeof(arg));
	
	if (!IsPlayerAlive(target))
	{
		ReplyToCommand(client, "[SM] %t", "Cannot performed on dead", arg);
		return Plugin_Handled;
	}
	
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

	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	GetClientName(target, arg, sizeof(arg));

	if (!IsPlayerAlive(target))
	{
		ReplyToCommand(client, "[SM] %t", "Cannot performed on dead", arg);
		return Plugin_Handled;
	}	
	
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

	if (!IsPlayerAlive(target))
	{
		ReplyToCommand(client, "[SM] %t", "Cannot performed on dead", arg);
		return Plugin_Handled;
	}	

	ShowActivity(client, "%t", "Slayed player", arg);
	LogMessage("\"%L\" slayed \"%L\"", client, target);
	ForcePlayerSuicide(target);

	return Plugin_Handled;
}
