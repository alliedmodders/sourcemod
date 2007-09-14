/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
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
 
#if defined _helpers_included
 #endinput
#endif
#define _helpers_included

/**
 * Formats a user's info as log text.  This is usually not needed because
 * %L can be used to auto-format client information into a string.
 *
 * @param client		Client index.
 * @param buffer		Buffer for text.
 * @param maxlength		Maximum length of text.
 */
stock FormatUserLogText(client, String:buffer[], maxlength)
{
	decl String:auth[32];
	decl String:name[40];
	
	new userid = GetClientUserId(client);
	if (!GetClientAuthString(client, auth, sizeof(auth)))
	{
		strcopy(auth, sizeof(auth), "UNKNOWN");
	}
	if (!GetClientName(client, name, sizeof(name)))
	{
		strcopy(name, sizeof(name), "UNKNOWN");
	}
	
	/** Currently, no team stuff ... */
	
	Format(buffer, maxlength, "\"%s<%d><%s><>\"", name, userid, auth);
}

/**
 * Returns plugin handle from plugin filename.
 *
 * @param filename		Filename of the plugin to search for.
 * @return				Handle to plugin if found, INVALID_HANDLE otherwise.
 */
stock Handle:FindPluginByFile(const String:filename[])
{
	decl String:buffer[256];
	
	new Handle:iter = GetPluginIterator();
	new Handle:pl;
	
	while (MorePlugins(iter))
	{
		pl = ReadPlugin(iter);
		
		GetPluginFilename(pl, buffer, sizeof(buffer));
		if (strcmp(buffer, filename, false) == 0)
		{
			CloseHandle(iter);
			return pl;
		}
	}
	
	CloseHandle(iter);
	
	return INVALID_HANDLE;
}

/**
 * Searches for clients that match an input string.
 *
 * Allowed patterns: 
 *  1) #<userid> or #<exact name>
 *  2) <partial or full name>
 *
 * @param pattern		Pattern to search for.
 * @param clients		Array to store matching clients in.
 * @param maxClients	Maximum clients in the array.
 * @return				Number of clients found.
 */
stock SearchForClients(const String:pattern[], clients[], maxClients)
{
	new maxclients = GetMaxClients();
	new total = 0;
	
	if (maxClients == 0)
	{
		return 0;
	}
	
	if (pattern[0] == '#')
	{
		new input = StringToInt(pattern[1]);
		if (!input)
		{
			decl String:name[65]
			for (new i=1; i<=maxclients; i++)
			{
				if (!IsClientInGame(i))
				{
					continue;
				}
				GetClientName(i, name, sizeof(name));
				if (strcmp(name, pattern, false) == 0)
				{
					clients[0] = i;
					return 1;
				}
			}
		} else {
			new client = GetClientOfUserId(input);
			if (client)
			{
				clients[0] = client;
				return 1;
			}
		}
	}
	
	decl String:name[65]
	for (new i=1; i<=maxclients; i++)
	{
		if (!IsClientInGame(i))
		{
			continue;
		}
		GetClientName(i, name, sizeof(name));
		if (StrContains(name, pattern, false) != -1)
		{
			clients[total++] = i;
			if (total >= maxClients)
			{
				break;
			}
		}
	}
	
	return total;
}

/**
 * Wraps SearchForClients and handles producing error messages for
 * bad targets.
 *
 * @param client	Client who issued command
 * @param target	Client's target argument
 * @param nobots	Optional. Set to true if bots should NOT be targetted
 * @param immunity	Optional. Set to false to ignore target immunity.
 * @return		Index of target client, or -1 on error.
 */
stock FindTarget(client, const String:target[], bool:nobots = false, bool:immunity = true)
{
	new clients[2];
	new numClients = SearchForClients(target, clients, 2);
	
	if (numClients == 0)
	{
		ReplyToCommand(client, "[SM] %t", "No matching client");
		return -1;
	}
	else if (numClients > 1)
	{
		ReplyToCommand(client, "[SM] %t", "More than one client matches", target);
		return -1;
	}
	else if (immunity && !CanUserTarget(client, clients[0]))
	{
		ReplyToCommand(client, "[SM] %t", "Unable to target");
		return -1;
	}
	else if (nobots && IsFakeClient(clients[0]))
	{
		ReplyToCommand(client, "[SM] %t", "Cannot target bot");
		return -1;
	}
	
	return clients[0];
}
