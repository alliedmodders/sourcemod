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

#pragma newdecls required

public Plugin myinfo =
{
	name = "Basic Commands",
	author = "AlliedModders LLC",
	description = "Basic Admin Commands",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

TopMenu hTopMenu;

Menu g_MapList;
StringMap g_ProtectedVars;

#include "basecommands/kick.sp"
#include "basecommands/reloadadmins.sp"
#include "basecommands/cancelvote.sp"
#include "basecommands/who.sp"
#include "basecommands/map.sp"
#include "basecommands/execcfg.sp"

public void OnPluginStart()
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
	TopMenu topmenu;
	if (LibraryExists("adminmenu") && ((topmenu = GetAdminTopMenu()) != null))
	{
		OnAdminMenuReady(topmenu);
	}
	
	g_MapList = new Menu(MenuHandler_ChangeMap, MenuAction_Display);
	g_MapList.SetTitle("%T", "Please select a map", LANG_SERVER);
	g_MapList.ExitBackButton = true;
	
	char mapListPath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, mapListPath, sizeof(mapListPath), "configs/adminmenu_maplist.ini");
	SetMapListCompatBind("sm_map menu", mapListPath);
	
	g_ProtectedVars = new StringMap();
	ProtectVar("rcon_password");
	ProtectVar("sm_show_activity");
	ProtectVar("sm_immunity_mode");
}

public void OnMapStart()
{
	ParseConfigs();
}

public void OnConfigsExecuted()
{
	LoadMapList(g_MapList);
}

void ProtectVar(const char[] cvar)
{
	g_ProtectedVars.SetValue(cvar, 1);
}

bool IsVarProtected(const char[] cvar)
{
	int dummy_value;
	return g_ProtectedVars.GetValue(cvar, dummy_value);
}

bool IsClientAllowedToChangeCvar(int client, const char[] cvarname)
{
	ConVar hndl = FindConVar(cvarname);

	bool allowed = false;
	int client_flags = client == 0 ? ADMFLAG_ROOT : GetUserFlagBits(client);
	
	if (client_flags & ADMFLAG_ROOT)
	{
		allowed = true;
	}
	else
	{
		if (hndl.Flags & FCVAR_PROTECTED)
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

public void OnAdminMenuReady(Handle aTopMenu)
{
	TopMenu topmenu = TopMenu.FromHandle(aTopMenu);

	/* Block us from being called twice */
	if (topmenu == hTopMenu)
	{
		return;
	}
	
	/* Save the Handle */
	hTopMenu = topmenu;
	
	/* Build the "Player Commands" category */
	TopMenuObject player_commands = hTopMenu.FindCategory(ADMINMENU_PLAYERCOMMANDS);
	
	if (player_commands != INVALID_TOPMENUOBJECT)
	{
		hTopMenu.AddItem("sm_kick", AdminMenu_Kick, player_commands, "sm_kick", ADMFLAG_KICK);
		hTopMenu.AddItem("sm_who", AdminMenu_Who, player_commands, "sm_who", ADMFLAG_GENERIC);
	}

	TopMenuObject server_commands = hTopMenu.FindCategory(ADMINMENU_SERVERCOMMANDS);

	if (server_commands != INVALID_TOPMENUOBJECT)
	{
		hTopMenu.AddItem("sm_reloadadmins", AdminMenu_ReloadAdmins, server_commands, "sm_reloadadmins", ADMFLAG_BAN);
		hTopMenu.AddItem("sm_map", AdminMenu_Map, server_commands, "sm_map", ADMFLAG_CHANGEMAP);
		hTopMenu.AddItem("sm_execcfg", AdminMenu_ExecCFG, server_commands, "sm_execcfg", ADMFLAG_CONFIG);		
	}

	TopMenuObject voting_commands = hTopMenu.FindCategory(ADMINMENU_VOTINGCOMMANDS);

	if (voting_commands != INVALID_TOPMENUOBJECT)
	{
		hTopMenu.AddItem("sm_cancelvote", AdminMenu_CancelVote, voting_commands, "sm_cancelvote", ADMFLAG_VOTE);
	}
}

public void OnLibraryRemoved(const char[] name)
{
	if (strcmp(name, "adminmenu") == 0)
	{
		hTopMenu = null;
	}
}

#define FLAG_STRINGS		14
char g_FlagNames[FLAG_STRINGS][20] =
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

int CustomFlagsToString(char[] buffer, int maxlength, int flags)
{
	char joins[6][6];
	int total;
	
	for (int i=view_as<int>(Admin_Custom1); i<=view_as<int>(Admin_Custom6); i++)
	{
		if (flags & (1<<i))
		{
			IntToString(i - view_as<int>(Admin_Custom1) + 1, joins[total++], 6);
		}
	}
	
	ImplodeStrings(joins, total, ",", buffer, maxlength);
	
	return total;
}

void FlagsToString(char[] buffer, int maxlength, int flags)
{
	char joins[FLAG_STRINGS+1][32];
	int total;

	for (int i=0; i<FLAG_STRINGS; i++)
	{
		if (flags & (1<<i))
		{
			strcopy(joins[total++], 32, g_FlagNames[i]);
		}
	}
	
	char custom_flags[32];
	if (CustomFlagsToString(custom_flags, sizeof(custom_flags), flags))
	{
		Format(joins[total++], 32, "custom(%s)", custom_flags);
	}

	ImplodeStrings(joins, total, ", ", buffer, maxlength);
}

public Action Command_Cvar(int client, int args)
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

	char cvarname[64];
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

	ConVar hndl = FindConVar(cvarname);
	if (hndl == null)
	{
		ReplyToCommand(client, "[SM] %t", "Unable to find cvar", cvarname);
		return Plugin_Handled;
	}

	if (!IsClientAllowedToChangeCvar(client, cvarname))
	{
		ReplyToCommand(client, "[SM] %t", "No access to cvar");
		return Plugin_Handled;
	}

	char value[255];
	if (args < 2)
	{
		hndl.GetString(value, sizeof(value));

		ReplyToCommand(client, "[SM] %t", "Value of cvar", cvarname, value);
		return Plugin_Handled;
	}

	GetCmdArg(2, value, sizeof(value));
	
	// The server passes the values of these directly into ServerCommand, following exec. Sanitize.
	if (StrEqual(cvarname, "servercfgfile", false) || StrEqual(cvarname, "lservercfgfile", false))
	{
		int pos = StrContains(value, ";", true);
		if (pos != -1)
		{
			value[pos] = '\0';
		}
	}

	if ((hndl.Flags & FCVAR_PROTECTED) != FCVAR_PROTECTED)
	{
		ShowActivity2(client, "[SM] ", "%t", "Cvar changed", cvarname, value);
	}
	else
	{
		ReplyToCommand(client, "[SM] %t", "Cvar changed", cvarname, value);
	}

	LogAction(client, -1, "\"%L\" changed cvar (cvar \"%s\") (value \"%s\")", client, cvarname, value);

	hndl.SetString(value, true);

	return Plugin_Handled;
}

public Action Command_ResetCvar(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_resetcvar <cvar>");

		return Plugin_Handled;
	}

	char cvarname[64];
	GetCmdArg(1, cvarname, sizeof(cvarname));
	
	ConVar hndl = FindConVar(cvarname);
	if (hndl == null)
	{
		ReplyToCommand(client, "[SM] %t", "Unable to find cvar", cvarname);
		return Plugin_Handled;
	}
	
	if (!IsClientAllowedToChangeCvar(client, cvarname))
	{
		ReplyToCommand(client, "[SM] %t", "No access to cvar");
		return Plugin_Handled;
	}

	hndl.RestoreDefault();

	char value[255];
	hndl.GetString(value, sizeof(value));

	if ((hndl.Flags & FCVAR_PROTECTED) != FCVAR_PROTECTED)
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

public Action Command_Rcon(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_rcon <args>");
		return Plugin_Handled;
	}

	char argstring[255];
	GetCmdArgString(argstring, sizeof(argstring));

	LogAction(client, -1, "\"%L\" console command (cmdline \"%s\")", client, argstring);

	if (client == 0) // They will already see the response in the console.
	{
		ServerCommand("%s", argstring);
	}
	else
	{
		char responseBuffer[4096];
		ServerCommandEx(responseBuffer, sizeof(responseBuffer), "%s", argstring);
		if (IsClientConnected(client))
		{
			ReplyToCommand(client, responseBuffer);
		}
	}

	return Plugin_Handled;
}

public Action Command_ReVote(int client, int args)
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
