/**
 * basecommands.sp
 * Implements basic admin commands.
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

#include <sourcemod>

#pragma semicolon 1

public Plugin:myinfo = 
{
	name = "Basic Commands",
	author = "AlliedModders LLC",
	description = "Basic Admin Commands",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

public OnPluginStart()
{
	LoadTranslations("common.cfg");
	LoadTranslations("plugin.basecommands.cfg");
	RegAdminCmd("sm_kick", Command_Kick, ADMFLAG_KICK, "sm_kick <#userid|name> [reason]");
	RegAdminCmd("sm_map", Command_Map, ADMFLAG_CHANGEMAP, "sm_map <map>");
	RegAdminCmd("sm_rcon", Command_Rcon, ADMFLAG_RCON, "sm_rcon <args>");
	RegAdminCmd("sm_cvar", Command_Cvar, ADMFLAG_CONVARS, "sm_cvar <cvar> [value]");
	RegAdminCmd("sm_execcfg", Command_ExecCfg, ADMFLAG_CONFIG, "sm_execcfg <filename>");
	RegAdminCmd("sm_who", Command_Who, ADMFLAG_GENERIC, "sm_who [#userid|name]");
}

#define FLAG_STRINGS		14
new String:g_FlagNames[FLAG_STRINGS][20] =
{
	"reservation",
	"admin",
	"kick",
	"ban",
	"unban",
	"slay",
	"map",
	"cvars",
	"cfg",
	"chat",
	"vote",
	"pass",
	"rcon",
	"cheat"
};

FlagsToString(String:buffer[], maxlength, flags)
{
	new String:joins[FLAG_STRINGS][20];
	new total;
	
	for (new i=0; i<FLAG_STRINGS; i++)
	{
		if (flags & (1<<i))
		{
			strcopy(joins[total++], 20, g_FlagNames[i]);
		}
	}
	
	ImplodeStrings(joins, total, ", ", buffer, maxlength);
}

public Action:Command_Who(client, args)
{
	if (args < 1)
	{
		/* Display header */
		new String:t_access[16], String:t_name[16];
		Format(t_access, sizeof(t_access), "%t", "Access", client);
		Format(t_name, sizeof(t_name), "%t", "Name", client);
		
		PrintToConsole(client, "%-24.23s %s", t_name, t_access);
		
		/* List all players */
		new maxClients = GetMaxClients();
		new String:flagstring[255];
				
		for (new i=1; i<=maxClients; i++)
		{
			if (!IsClientInGame(i))
			{
				continue;
			}
			new flags = GetUserFlagBits(i);
			if (flags == 0)
			{
				strcopy(flagstring, sizeof(flagstring), "none");
			} else if (flags & ADMFLAG_ROOT) {
				strcopy(flagstring, sizeof(flagstring), "root");
			} else {
				FlagsToString(flagstring, sizeof(flagstring), flags);
			}
			decl String:name[65];
			GetClientName(i, name, sizeof(name));
			PrintToConsole(client, "%d. %-24.23s %s", i, name, flagstring);
		}
		
		if (GetCmdReplySource() == SM_REPLY_TO_CHAT)
		{
			ReplyToCommand(client, "[SM] %t", "See console for output");
		}
		
		return Plugin_Handled;
	}
	
	new String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));
	
	new clients[2];
	new numClients = SearchForClients(arg, clients, 2);
	
	if (numClients == 0)
	{
		ReplyToCommand(client, "[SM] %t", "No matching client");
		return Plugin_Handled;
	} else if (numClients > 1) {
		ReplyToCommand(client, "[SM] %t", "More than one client matches", arg);
		return Plugin_Handled;
	}
	
	new flags = GetUserFlagBits(clients[0]);
	new String:flagstring[255];
	if (flags == 0)
	{
		strcopy(flagstring, sizeof(flagstring), "none");
	} else if (flags & ADMFLAG_ROOT) {
		strcopy(flagstring, sizeof(flagstring), "root");
	} else {
		FlagsToString(flagstring, sizeof(flagstring), flags);
	}
	
	ReplyToCommand(client, "[SM] %t: %s", "Access", flagstring);
	
	return Plugin_Handled;
}

public Action:Command_ExecCfg(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_execcfg <filename>");
		return Plugin_Handled;
	}
	
	new String:path[64] = "cfg/";
	GetCmdArg(1, path[4], sizeof(path)-4);
	
	if (!FileExists(path))
	{
		ReplyToCommand(client, "[SM] %t", "Config not found", path[4]);
		return Plugin_Handled;
	}
	
	LogMessage("\"%L\" executed config (file \"%s\")", client, path[4]);
	ShowActivity(client, "%t", "Executed config", path[4]);
		
	ServerCommand("exec \"%s\"", path[4]);
	
	ReplyToCommand(client, "[SM] %t", "Executed config", path[4]);
	
	return Plugin_Handled;
}

public Action:Command_Cvar(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_cvar <cvar> [value]");
		return Plugin_Handled;
	}
	
	new String:cvarname[33];
	GetCmdArg(1, cvarname, sizeof(cvarname));
	
	new Handle:hndl = FindConVar(cvarname);
	if (hndl == INVALID_HANDLE)
	{
		ReplyToCommand(client, "[SM] %t", "Unable to find cvar", cvarname);
		return Plugin_Handled;
	}
	
	new bool:allowed = false;
	if (GetConVarFlags(hndl) & FCVAR_PROTECTED)
	{
		/* If they're root, allow anything */
		if ((GetUserFlagBits(client) & ADMFLAG_ROOT) == ADMFLAG_ROOT)
		{
			allowed = true;
		}
		/* If they're not root, and getting sv_password, see if they have ADMFLAG_PASSWORD */
		else if (StrEqual(cvarname, "sv_password") && (GetUserFlagBits(client) & ADMFLAG_PASSWORD))
		{
			allowed = true;
		}
	} 
	/* Do a check for the cheat cvar */
	else if (StrEqual(cvarname, "sv_cheats"))
	{
		if (GetUserFlagBits(client) & ADMFLAG_CHEATS)
		{
			allowed = true;
		}
	}
	/* If we drop down to here, it was a normal cvar. */
	else
	{
		allowed = true;
	}
	
	if (!allowed)
	{
		ReplyToCommand(client, "[SM] %t", "No access to cvar");
		return Plugin_Handled;
	}
	
	decl String:value[255];
	if (args < 2)
	{
		GetConVarString(hndl, value, sizeof(value));
		
		ReplyToCommand(client, "[SM] %t", "Value of cvar", cvarname, value);
		return Plugin_Handled;
	}
	
	GetCmdArg(2, value, sizeof(value));
	
	LogMessage("\"%L\" changed cvar (cvar \"%s\") (value \"%s\")", client, cvarname, value);
	if ((GetConVarFlags(hndl) & FCVAR_PROTECTED) != FCVAR_PROTECTED)
	{
		ShowActivity(client, "%t", "Cvar changed", cvarname, value);
	}
	
	SetConVarString(hndl, value);
	ReplyToCommand(client, "[SM] %t", "Cvar changed", cvarname, value);
		
	return Plugin_Handled;
}

public Action:Command_Rcon(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_rcon <args>");
		return Plugin_Handled;
	}
	
	decl String:argstring[255];
	GetCmdArgString(argstring, sizeof(argstring));
	
	LogMessage("\"%L\" console command (cmdline \"%s\")", client, argstring);
	ServerCommand("%s", argstring);
	
	return Plugin_Handled;
}

public Action:Command_Map(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_map <map>");
		return Plugin_Handled;
	}
	
	new String:map[64];
	GetCmdArg(1, map, sizeof(map));
	
	if (!IsMapValid(map))
	{
		ReplyToCommand(client, "[SM] %t", "Map was not found", map);
		return Plugin_Handled;
	}
	
	LogMessage("\"%L\" changed map to \"%s\"", client, map);
	ShowActivity(client, "%t", "Changing map", map);
	ReplyToCommand(client, "%t", "Changing map", map);
	
	new Handle:dp;
	CreateDataTimer(3.0, Timer_ChangeMap, dp);
	WritePackString(dp, map);
	
	return Plugin_Handled;
}

public Action:Timer_ChangeMap(Handle:timer, Handle:dp)
{
	new String:map[65];
	
	ResetPack(dp);
	ReadPackString(dp, map, sizeof(map));
	
	ServerCommand("changelevel \"%s\"", map);
	
	return Plugin_Stop;
}

public Action:Command_Kick(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_kick <#userid|name> [reason]");
		return Plugin_Handled;
	}
	
	new String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));
	
	new clients[2];
	new numClients = SearchForClients(arg, clients, 2);
	
	if (numClients == 0)
	{
		ReplyToCommand(client, "[SM] %t", "No matching client");
		return Plugin_Handled;
	} else if (numClients > 1) {
		ReplyToCommand(client, "[SM] %t", "More than one client matches", arg);
		return Plugin_Handled;
	} else if (!CanUserTarget(client, clients[0])) {
		ReplyToCommand(client, "[SM] %t", "Unable to target");
	}
	
	new userid = GetClientUserId(clients[0]);
	new String:name[65];
	
	GetClientName(clients[0], name, sizeof(name));
	
	decl String:reason[256];
	if (args < 2)
	{
		/* Safely null terminate */
		reason[0] = '\0';
	} else {
		GetCmdArg(2, reason, sizeof(reason));
	}
	
	LogMessage("\"%L\" kicked \"%L\" (reason \"%s\")", client, clients[0], reason);
	ShowActivity(client, "%t", "Kicked player", name);
	
	if (args < 2)
	{
		ServerCommand("kickid %d", userid);
	} else {
		ServerCommand("kickid %d \"%s\"", userid, reason);
	}
	
	ReplyToCommand(client, "[SM] %t", "Client kicked", name);
	
	return Plugin_Handled;
}
