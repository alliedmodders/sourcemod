/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basic Commands Plugin
 * Implements basic admin commands.
 *
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
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
 * code of this program (as well as its derivative 1works) to "Half-Life 2," the
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
#undef REQUIRE_PLUGIN
#include <adminmenu>

public Plugin:myinfo =
{
	name = "Basic Commands",
	author = "AlliedModders LLC",
	description = "Basic Admin Commands",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

new Handle:hTopMenu = INVALID_HANDLE;

new Handle:g_MapList;
new Handle:g_ProtectedVars;

#include "basecommands/kick.sp"
#include "basecommands/reloadadmins.sp"
#include "basecommands/cancelvote.sp"
#include "basecommands/who.sp"
#include "basecommands/map.sp"
#include "basecommands/execcfg.sp"

public OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("plugin.basecommands");

	RegAdminCmd("sm_kick", Command_Kick, ADMFLAG_KICK, "sm_kick <#userid|name> [reason]");
	RegAdminCmd("sm_map", Command_Map, ADMFLAG_CHANGEMAP, "sm_map <map>");
	RegAdminCmd("sm_rcon", Command_Rcon, ADMFLAG_RCON, "sm_rcon <args>");
	RegAdminCmd("sm_cvar", Command_Cvar, ADMFLAG_CONVARS, "sm_cvar <cvar> [value]");
	RegAdminCmd("sm_resetcvar", Command_ResetCvar, ADMFLAG_CONVARS, "sm_resetcvar <cvar>");
	RegAdminCmd("sm_execcfg", Command_ExecCfg, ADMFLAG_CONFIG, "sm_execcfg <filename>");
	RegAdminCmd("sm_who", Command_Who, ADMFLAG_GENERIC, "sm_who [#userid|name]");
	RegAdminCmd("sm_reloadadmins", Command_ReloadAdmins, ADMFLAG_BAN, "sm_reloadadmins");
	RegAdminCmd("sm_cancelvote", Command_CancelVote, ADMFLAG_VOTE, "sm_cancelvote");
	RegConsoleCmd("sm_revote", Command_ReVote);
	
	/* Account for late loading */
	new Handle:topmenu;
	if (LibraryExists("adminmenu") && ((topmenu = GetAdminTopMenu()) != INVALID_HANDLE))
	{
		OnAdminMenuReady(topmenu);
	}
	
	g_MapList = CreateMenu(MenuHandler_ChangeMap);
	SetMenuTitle(g_MapList, "Please select a map");
	SetMenuExitBackButton(g_MapList, true);
	
	decl String:mapListPath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, mapListPath, sizeof(mapListPath), "configs/adminmenu_maplist.ini");
	SetMapListCompatBind("sm_map menu", mapListPath);
	
	g_ProtectedVars = CreateTrie();
	ProtectVar("rcon_password");
	ProtectVar("sm_show_activity");
	ProtectVar("sm_immunity_mode");
}

public OnMapStart()
{
	ParseConfigs();
}

public OnConfigsExecuted()
{
	LoadMapList(g_MapList);
}

ProtectVar(const String:cvar[])
{
	SetTrieValue(g_ProtectedVars, cvar, 1);
}

bool:IsVarProtected(const String:cvar[])
{
	decl dummy_value;
	return GetTrieValue(g_ProtectedVars, cvar, dummy_value);
}

bool:IsClientAllowedToChangeCvar(client, const String:cvarname[])
{
	new Handle:hndl = FindConVar(cvarname);

	new bool:allowed = false;
	new client_flags = client == 0 ? ADMFLAG_ROOT : GetUserFlagBits(client);
	
	if (client_flags & ADMFLAG_ROOT)
	{
		allowed = true;
	}
	else
	{
		if (GetConVarFlags(hndl) & FCVAR_PROTECTED)
		{
			allowed = ((client_flags & ADMFLAG_PASSWORD) == ADMFLAG_PASSWORD);
		}
		else if (StrEqual(cvarname, "sv_cheats"))
		{
			allowed = ((client_flags & ADMFLAG_CHEATS) == ADMFLAG_CHEATS);
		}
		else if (!IsVarProtected(cvarname))
		{
			allowed = true;
		}
	}

	return allowed;
}

public OnAdminMenuReady(Handle:topmenu)
{
	/* Block us from being called twice */
	if (topmenu == hTopMenu)
	{
		return;
	}
	
	/* Save the Handle */
	hTopMenu = topmenu;
	
	/* Build the "Player Commands" category */
	new TopMenuObject:player_commands = FindTopMenuCategory(hTopMenu, ADMINMENU_PLAYERCOMMANDS);
	
	if (player_commands != INVALID_TOPMENUOBJECT)
	{
		AddToTopMenu(hTopMenu, 
			"sm_kick",
			TopMenuObject_Item,
			AdminMenu_Kick,
			player_commands,
			"sm_kick",
			ADMFLAG_KICK);
			
		AddToTopMenu(hTopMenu,
			"sm_who",
			TopMenuObject_Item,
			AdminMenu_Who,
			player_commands,
			"sm_who",
			ADMFLAG_GENERIC);
	}

	new TopMenuObject:server_commands = FindTopMenuCategory(hTopMenu, ADMINMENU_SERVERCOMMANDS);

	if (server_commands != INVALID_TOPMENUOBJECT)
	{
		AddToTopMenu(hTopMenu,
			"sm_reloadadmins",
			TopMenuObject_Item,
			AdminMenu_ReloadAdmins,
			server_commands,
			"sm_reloadadmins",
			ADMFLAG_BAN);
			
		AddToTopMenu(hTopMenu,
			"sm_map",
			TopMenuObject_Item,
			AdminMenu_Map,
			server_commands,
			"sm_map",
			ADMFLAG_CHANGEMAP);
			
		AddToTopMenu(hTopMenu,
			"sm_execcfg",
			TopMenuObject_Item,
			AdminMenu_ExecCFG,
			server_commands,
			"sm_execcfg",
			ADMFLAG_CONFIG);		
	}

	new TopMenuObject:voting_commands = FindTopMenuCategory(hTopMenu, ADMINMENU_VOTINGCOMMANDS);

	if (voting_commands != INVALID_TOPMENUOBJECT)
	{
		AddToTopMenu(hTopMenu,
			"sm_cancelvote",
			TopMenuObject_Item,
			AdminMenu_CancelVote,
			voting_commands,
			"sm_cancelvote",
			ADMFLAG_VOTE);
	}
}

public OnLibraryRemoved(const String:name[])
{
	if (strcmp(name, "adminmenu") == 0)
	{
		hTopMenu = INVALID_HANDLE;
	}
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

public Action:Command_Cvar(client, args)
{
	if (args < 1)
	{
		if (client == 0)
		{
			ReplyToCommand(client, "[SM] Usage: sm_cvar <cvar|protect> [value]");
		}
		else
		{
			ReplyToCommand(client, "[SM] Usage: sm_cvar <cvar> [value]");
		}
		return Plugin_Handled;
	}

	decl String:cvarname[64];
	GetCmdArg(1, cvarname, sizeof(cvarname));
	
	if (client == 0 && StrEqual(cvarname, "protect"))
	{
		if (args < 2)
		{
			ReplyToCommand(client, "[SM] Usage: sm_cvar <protect> <cvar>");
			return Plugin_Handled;
		}
		GetCmdArg(2, cvarname, sizeof(cvarname));
		ProtectVar(cvarname);
		ReplyToCommand(client, "[SM] %t", "Cvar is now protected", cvarname);
		return Plugin_Handled;
	}

	new Handle:hndl = FindConVar(cvarname);
	if (hndl == INVALID_HANDLE)
	{
		ReplyToCommand(client, "[SM] %t", "Unable to find cvar", cvarname);
		return Plugin_Handled;
	}

	if (!IsClientAllowedToChangeCvar(client, cvarname))
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
		ShowActivity2(client, "[SM] ", "%t", "Cvar changed", cvarname, value);
	}
	else
	{
		ReplyToCommand(client, "[SM] %t", "Cvar changed", cvarname, value);
	}

	LogAction(client, -1, "\"%L\" changed cvar (cvar \"%s\") (value \"%s\")", client, cvarname, value);

	SetConVarString(hndl, value, true);

	return Plugin_Handled;
}

public Action:Command_ResetCvar(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_resetcvar <cvar>");

		return Plugin_Handled;
	}

	decl String:cvarname[64];
	GetCmdArg(1, cvarname, sizeof(cvarname));
	
	new Handle:hndl = FindConVar(cvarname);
	if (hndl == INVALID_HANDLE)
	{
		ReplyToCommand(client, "[SM] %t", "Unable to find cvar", cvarname);
		return Plugin_Handled;
	}
	
	if (!IsClientAllowedToChangeCvar(client, cvarname))
	{
		ReplyToCommand(client, "[SM] %t", "No access to cvar");
		return Plugin_Handled;
	}

	ResetConVar(hndl);

	decl String:value[255];
	GetConVarString(hndl, value, sizeof(value));

	if ((GetConVarFlags(hndl) & FCVAR_PROTECTED) != FCVAR_PROTECTED)
	{
		ShowActivity2(client, "[SM] ", "%t", "Cvar changed", cvarname, value);
	}
	else
	{
		ReplyToCommand(client, "[SM] %t", "Cvar changed", cvarname, value);
	}

	LogAction(client, -1, "\"%L\" reset cvar (cvar \"%s\") (value \"%s\")", client, cvarname, value);

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

	if (client == 0) // They will already see the response in the console.
	{
		ServerCommand("%s", argstring);
	} else {
		new String:responseBuffer[4096];
		ServerCommandEx(responseBuffer, sizeof(responseBuffer), "%s", argstring);
		ReplyToCommand(client, responseBuffer);
	}

	return Plugin_Handled;
}

public Action:Command_ReVote(client, args)
{
	if (client == 0)
	{
		ReplyToCommand(client, "[SM] %t", "Command is in-game only");
		return Plugin_Handled;
	}
	
	if (!IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote Not In Progress");
		return Plugin_Handled;
	}
	
	if (!IsClientInVotePool(client))
	{
		ReplyToCommand(client, "[SM] %t", "Cannot participate in vote");
		return Plugin_Handled;
	}
	
	if (!RedrawClientVoteMenu(client))
	{
		ReplyToCommand(client, "[SM] %t", "Cannot change vote");
	}
	
	return Plugin_Handled;
}
