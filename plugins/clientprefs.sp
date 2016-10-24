/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Map Management Plugin
 * Provides all map related functionality, including map changing, map voting,
 * and nextmap.
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
#include <clientprefs>

#pragma newdecls required

public Plugin myinfo =
{
	name = "Client Preferences",
	author = "AlliedModders LLC",
	description = "Client peferences and settings menu",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

public void OnPluginStart()
{
	LoadTranslations("clientprefs.phrases");
	
	RegConsoleCmd("sm_cookies", Command_Cookie, "sm_cookies <name> [value]");
	RegConsoleCmd("sm_settings", Command_Settings);	
}

public Action Command_Cookie(int client, int args)
{
	if (args == 0)
	{
		ReplyToCommand(client, "[SM] Usage: sm_cookies <name> [value]");
		ReplyToCommand(client, "[SM] %t", "Printing Cookie List");
		
		/* Show list of cookies */
		Handle iter = GetCookieIterator();
		
		char name[30];
		name[0] = '\0';
		char description[255];
		description[0] = '\0';
		
		PrintToConsole(client, "%t:", "Cookie List");
		
		CookieAccess access;
		
		while (ReadCookieIterator(iter, 
								name, 
								sizeof(name),
								access, 
								description, 
								sizeof(description)) != false)
		{
			if (access < CookieAccess_Private)
			{
				PrintToConsole(client, "%s - %s", name, description);
			}
		}
		
		delete iter;		
		return Plugin_Handled;
	}
	
	if (client == 0)
	{
		PrintToServer("%T", "No Console", LANG_SERVER);
		return Plugin_Handled;	
	}
	
	char name[30];
	name[0] = '\0';
	GetCmdArg(1, name, sizeof(name));
	
	Handle cookie = FindClientCookie(name);
	
	if (cookie == null)
	{
		ReplyToCommand(client, "[SM] %t", "Cookie not Found", name);
		return Plugin_Handled;
	}
	
	CookieAccess access = GetCookieAccess(cookie);
	
	if (access == CookieAccess_Private)
	{
		ReplyToCommand(client, "[SM] %t", "Cookie not Found", name);
		delete cookie;
		return Plugin_Handled;
	}
	
	char value[100];
	value[0] = '\0';
	
	if (args == 1)
	{
		GetClientCookie(client, cookie, value, sizeof(value));
		ReplyToCommand(client, "[SM] %t", "Cookie Value", name, value);
		
		delete cookie;
		return Plugin_Handled;
	}
	
	if (access == CookieAccess_Protected)
	{
		ReplyToCommand(client, "[SM] %t", "Protected Cookie", name);
		delete cookie;
		return Plugin_Handled;
	}
	
	/* Set the new value of the cookie */
	
	GetCmdArg(2, value, sizeof(value));
	
	SetClientCookie(client, cookie, value);
	delete cookie;
	ReplyToCommand(client, "[SM] %t", "Cookie Changed Value", name, value);
	
	return Plugin_Handled;
}

public Action Command_Settings(int client, int args)
{
	if (client == 0)
	{
		PrintToServer("%T", "No Console", LANG_SERVER);
		return Plugin_Handled;	
	}
	
	ShowCookieMenu(client);
	
	return Plugin_Handled;
}
