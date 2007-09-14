/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basic Commands Plugin
 * Implements basic admin commands.
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
	LoadTranslations("common.phrases");

	RegAdminCmd("sm_kick", Command_Kick, ADMFLAG_KICK, "sm_kick <#userid|name> [reason]");
	RegAdminCmd("sm_map", Command_Map, ADMFLAG_CHANGEMAP, "sm_map <map>");
	RegAdminCmd("sm_rcon", Command_Rcon, ADMFLAG_RCON, "sm_rcon <args>");
	RegAdminCmd("sm_cvar", Command_Cvar, ADMFLAG_CONVARS, "sm_cvar <cvar> [value]");
	RegAdminCmd("sm_execcfg", Command_ExecCfg, ADMFLAG_CONFIG, "sm_execcfg <filename>");
	RegAdminCmd("sm_who", Command_Who, ADMFLAG_GENERIC, "sm_who [#userid|name]");
	RegAdminCmd("sm_reloadadmins", Command_ReloadAdmins, ADMFLAG_BAN, "sm_reloadadmins");
	RegAdminCmd("sm_cancelvote", Command_CancelVote, ADMFLAG_VOTE, "sm_cancelvote");
}

public Action:Command_ReloadAdmins(client, args)
{
	/* Dump it all! */
	DumpAdminCache(AdminCache_Groups, true);
	DumpAdminCache(AdminCache_Overrides, true);

	LogAction(client, -1, "\"%L\" refreshed the admin cache.", client);
	ReplyToCommand(client, "[SM] %t", "Admin cache refreshed");

	return Plugin_Handled;
}

#define FLAG_STRINGS		14
new String:g_FlagNames[FLAG_STRINGS][20] =
{
	"res",
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

CustomFlagsToString(String:buffer[], maxlength, flags)
{
	decl String:joins[6][6];
	new total;
	
	for (new i=_:Admin_Custom1; i<=_:Admin_Custom6; i++)
	{
		if (flags & (1<<i))
		{
			IntToString(i - _:Admin_Custom1 + 1, joins[total++], 6);
		}
	}
	
	ImplodeStrings(joins, total, ",", buffer, maxlength);
	
	return total;
}

FlagsToString(String:buffer[], maxlength, flags)
{
	decl String:joins[FLAG_STRINGS+1][32];
	new total;

	for (new i=0; i<FLAG_STRINGS; i++)
	{
		if (flags & (1<<i))
		{
			strcopy(joins[total++], 32, g_FlagNames[i]);
		}
	}
	
	decl String:custom_flags[32];
	if (CustomFlagsToString(custom_flags, sizeof(custom_flags), flags))
	{
		Format(joins[total++], 32, "custom(%s)", custom_flags);
	}

	ImplodeStrings(joins, total, ", ", buffer, maxlength);
}

public Action:Command_Who(client, args)
{
	if (args < 1)
	{
		/* Display header */
		decl String:t_access[16], String:t_name[16];
		Format(t_access, sizeof(t_access), "%t", "Access", client);
		Format(t_name, sizeof(t_name), "%t", "Name", client);

		PrintToConsole(client, "%-24.23s %s", t_name, t_access);

		/* List all players */
		new maxClients = GetMaxClients();
		decl String:flagstring[255];

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

	decl String:arg[65];
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
	decl String:flagstring[255];
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

	ShowActivity(client, "%t", "Executed config", path[4]);

	LogAction(client, -1, "\"%L\" executed config (file \"%s\")", client, path[4]);

	ServerCommand("exec \"%s\"", path[4]);

	return Plugin_Handled;
}

public Action:Command_Cvar(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_cvar <cvar> [value]");
		return Plugin_Handled;
	}

	decl String:cvarname[33];
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
		if (GetUserFlagBits(client) & ADMFLAG_CHEATS
			|| GetUserFlagBits(client) & ADMFLAG_ROOT)
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

	if ((GetConVarFlags(hndl) & FCVAR_PROTECTED) != FCVAR_PROTECTED)
	{
		ShowActivity(client, "%t", "Cvar changed", cvarname, value);
	} else {
		ReplyToCommand(client, "[SM] %t", "Cvar changed", cvarname, value);
	}

	LogAction(client, -1, "\"%L\" changed cvar (cvar \"%s\") (value \"%s\")", client, cvarname, value);

	SetConVarString(hndl, value, true);

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

	LogAction(client, -1, "\"%L\" console command (cmdline \"%s\")", client, argstring);

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

	decl String:map[64];
	GetCmdArg(1, map, sizeof(map));

	if (!IsMapValid(map))
	{
		ReplyToCommand(client, "[SM] %t", "Map was not found", map);
		return Plugin_Handled;
	}

	ShowActivity(client, "%t", "Changing map", map);

	LogAction(client, -1, "\"%L\" changed map to \"%s\"", client, map);

	new Handle:dp;
	CreateDataTimer(3.0, Timer_ChangeMap, dp);
	WritePackString(dp, map);

	return Plugin_Handled;
}

public Action:Timer_ChangeMap(Handle:timer, Handle:dp)
{
	decl String:map[65];

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


	decl String:Arguments[256];
	GetCmdArgString(Arguments, sizeof(Arguments));

	decl String:arg[65];
	new len = BreakString(Arguments, arg, sizeof(arg));

	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	GetClientName(target, arg, sizeof(arg));

	if (len == -1)
	{
		/* Safely null terminate */
		len = 0;
		Arguments[0] = '\0';
	}

	ShowActivity(client, "%t", "Kicked player", arg);
	LogAction(client, target, "\"%L\" kicked \"%L\" (reason \"%s\")", client, target, Arguments[len]);

	if (Arguments[0] == '\0')
	{
		KickClient(target, "%t", "Kicked by admin");
	}
	else
	{
		KickClient(target, "%s", Arguments[len]);
	}

	return Plugin_Handled;
}

public Action:Command_CancelVote(client, args)
{
	if (!IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote Not In Progress");
		return Plugin_Handled;
	}

	ShowActivity(client, "%t", "Cancelled Vote");
	
	CancelVote();
	
	return Plugin_Handled;
}
