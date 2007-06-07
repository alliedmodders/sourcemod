/**
 * admincmds-basic.sp
 * Manages the standard flat files for admins.  This is the file to compile.
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
 */

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
	LoadTranslations("core.cfg");
	RegAdminCmd("sm_kick", Command_Kick, ADMFLAG_KICK, "sm_kick <#userid|name> [reason]");
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
	}
	
	new userid = GetClientUserId(clients[0]);
	new String:name[65];
	
	GetClientName(clients[0], name, sizeof(name));
	
	decl String:reason[256]
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
	
	ReplyToCommand(client, "[SM] Client \"%s\" kicked.", name);
	
	return Plugin_Handled;
}
