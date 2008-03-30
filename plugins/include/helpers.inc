/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
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
 * @deprecated Use FindTarget() or ProcessTargetString().
 */
#pragma deprecated Use FindTarget() or ProcessTargetString()
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
 * Wraps ProcessTargetString() and handles producing error messages for
 * bad targets.
 *
 * @param client	Client who issued command
 * @param target	Client's target argument
 * @param nobots	Optional. Set to true if bots should NOT be targetted
 * @param immunity	Optional. Set to false to ignore target immunity.
 * @return			Index of target client, or -1 on error.
 */
stock FindTarget(client, const String:target[], bool:nobots = false, bool:immunity = true)
{
	decl String:target_name[MAX_TARGET_LENGTH];
	decl target_list[1], target_count, bool:tn_is_ml;
	
	new flags = COMMAND_FILTER_NO_MULTI;
	if (nobots)
	{
		flags |= COMMAND_FILTER_NO_BOTS;
	}
	if (!immunity)
	{
		flags |= COMMAND_FILTER_NO_IMMUNITY;
	}
	
	if ((target_count = ProcessTargetString(
			target,
			client, 
			target_list, 
			1, 
			flags,
			target_name,
			sizeof(target_name),
			tn_is_ml)) > 0)
	{
		return target_list[0];
	}
	else
	{
		ReplyToTargetError(client, target_count);
		return -1;
	}
}

/**
 * This function is no longer supported.  It has been replaced with ReadMapList(), 
 * which uses a more unified caching and configuration mechanism.  This function also 
 * has a bug where if the cvar contents changes, the fileTime change won't be recognized.
 * 
 * Loads a specified array with maps. The maps will be either loaded from mapcyclefile, or if supplied
 * a cvar containing a file name. If the file in the cvar is bad, it will use mapcyclefile. The fileTime
 * parameter is used to store a timestamp of the file. If specified, the file will only be reloaded if it
 * has changed.
 *
 * @param array		Valid array handle, should be created with CreateArray(33) or larger. 
 * @param fileTime	Variable containing the "last changed" time of the file. Used to avoid needless reloading.
 * @param fileCvar	CVAR set to the file to be loaded. Optional.
 * @return			Number of maps loaded or 0 if in error.
 */
#pragma deprecated Use ReadMapList() instead. 
 stock LoadMaps(Handle:array, &fileTime = 0, Handle:fileCvar = INVALID_HANDLE)
 { 
 	decl String:mapPath[256], String:mapFile[64];
 	new bool:fileFound = false;
 	
 	if (fileCvar != INVALID_HANDLE)
 	{
 		GetConVarString(fileCvar, mapFile, 64);
	 	BuildPath(Path_SM, mapPath, sizeof(mapPath), mapFile);
 		fileFound = FileExists(mapPath);
 	}
 
 	if (!fileFound)
 	{
 		new Handle:mapCycleFile = FindConVar("mapcyclefile");
 		GetConVarString(mapCycleFile, mapPath, sizeof(mapPath));
 		fileFound = FileExists(mapPath);
 	}
 	
 	if (!fileFound)
 	{
 		LogError("Failed to find a file to load maps from. No maps loaded.");
 		ClearArray(array);
 		
 		return 0;		
 	}
 
 	// If the file hasn't changed, there's no reason to reload
 	// all of the maps.
 	new newTime =  GetFileTime(mapPath, FileTime_LastChange);
 	if (fileTime == newTime)
 	{
 		return GetArraySize(array);
 	}
 	
 	fileTime = newTime;
 	
 	ClearArray(array);
 
 	new Handle:file = OpenFile(mapPath, "rt");
 	if (file == INVALID_HANDLE)
 	{
 		LogError("Could not open file: %s", mapPath);
 		
 		return 0;
 	}
 
	LogMessage("Loading maps from file: %s", mapPath);
	
 	decl String:buffer[64], len;
 	while (!IsEndOfFile(file) && ReadFileLine(file, buffer, sizeof(buffer)))
 	{
 		TrimString(buffer);
 
 		if ((len = StrContains(buffer, ".bsp", false)) != -1)
 		{
 			buffer[len] = '\0';
 		}
 
 		if (buffer[0] == '\0' || !IsValidConVarChar(buffer[0]) || !IsMapValid(buffer))
 		{
 			continue;
 		}
 		
 		if (FindStringInArray(array, buffer) != -1)
 		{
 			continue;
 		}
 
 		PushArrayString(array, buffer);
 	}
 
 	CloseHandle(file);
 	return GetArraySize(array);
}
