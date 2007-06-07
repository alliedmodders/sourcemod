/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be used 
 * or modified under the Terms and Conditions of its License Agreement, which is found 
 * in LICENSE.txt.  The Terms and Conditions for making SourceMod extensions/plugins 
 * may change at any time.  To view the latest information, see:
 *   http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */
 
#if defined _helpers_included
 #endinput
#endif
#define _helpers_included

/**
 * Formats a user's info as log text.
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
		StrCopy(auth, sizeof(auth), "UNKNOWN");
	}
	if (!GetClientName(client, name, sizeof(name)))
	{
		StrCopy(name, sizeof(name), "UNKNOWN");
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
		if (StrCompare(buffer, filename) == 0)
		{
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
				if (StrEqual(name, pattern, false))
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
