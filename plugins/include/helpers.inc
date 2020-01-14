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
 * This function is deprecated. Use the %L format specifier instead.
 * 
 * Formats a user's info as log text.
 *
 * @param client        Client index.
 * @param buffer        Buffer for text.
 * @param maxlength     Maximum length of text.
 * @deprecated          Use the %L format specifier instead.
 */
#pragma deprecated Use the %L format specifier instead.
stock void FormatUserLogText(int client, char[] buffer, int maxlength)
{
	FormatEx(buffer, maxlength, "\"%L\"", client);
}

/**
 * Returns plugin handle from plugin filename.
 *
 * @param filename      Filename of the plugin to search for.
 * @return              Handle to plugin if found, INVALID_HANDLE otherwise.
 */
stock Handle FindPluginByFile(const char[] filename)
{
	char buffer[256];
	
	Handle iter = GetPluginIterator();
	Handle pl;
	
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
 * @deprecated          Use FindTarget() or ProcessTargetString().
 */
#pragma deprecated Use FindTarget() or ProcessTargetString()
stock int SearchForClients(const char[] pattern, int[] clients, int maxClients)
{
	int total = 0;
	
	if (maxClients == 0)
	{
		return 0;
	}
	
	if (pattern[0] == '#')
	{
		int input = StringToInt(pattern[1]);
		if (!input) {
			char name[MAX_NAME_LENGTH];
			for (int i=1; i<=MaxClients; i++)
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
		}
		else
		{
			int client = GetClientOfUserId(input);
			if (client)
			{
				clients[0] = client;
				return 1;
			}
		}
	}
	
	char name[MAX_NAME_LENGTH];
	for (int i=1; i<=MaxClients; i++)
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
 * Note that you should use LoadTranslations("common.phrases") in OnPluginStart(). 
 * "common.phrases" contains all of the translatable phrases that FindTarget() will
 * reply with in the event a target is not found (error).
 *
 * @param client        Client who issued command
 * @param target        Client's target argument
 * @param nobots        Optional. Set to true if bots should NOT be targetted
 * @param immunity      Optional. Set to false to ignore target immunity.
 * @return              Index of target client, or -1 on error.
 */
stock int FindTarget(int client, const char[] target, bool nobots = false, bool immunity = true)
{
	char target_name[MAX_TARGET_LENGTH];
	int target_list[1], target_count;
	bool tn_is_ml;
	
	int flags = COMMAND_FILTER_NO_MULTI;
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

	ReplyToTargetError(client, target_count);
	return -1;
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
 * @param array         Valid array handle, should be created with CreateArray(33) or larger. 
 * @param fileTime      Variable containing the "last changed" time of the file. Used to avoid needless reloading.
 * @param fileCvar      CVAR set to the file to be loaded. Optional.
 * @return              Number of maps loaded or 0 if in error.
 * @deprecated          Use ReadMapList() instead.
 */
#pragma deprecated Use ReadMapList() instead. 
stock int LoadMaps(Handle array, int &fileTime = 0, Handle fileCvar = INVALID_HANDLE)
{ 
	char mapPath[256], mapFile[64];
	bool fileFound = false;
	
	if (fileCvar != INVALID_HANDLE)
	{
		GetConVarString(fileCvar, mapFile, 64);
		BuildPath(Path_SM, mapPath, sizeof(mapPath), mapFile);
		fileFound = FileExists(mapPath);
	}
 
	if (!fileFound)
	{
		Handle mapCycleFile = FindConVar("mapcyclefile");
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
	int newTime =  GetFileTime(mapPath, FileTime_LastChange);
	if (fileTime == newTime)
	{
		return GetArraySize(array);
	}
	
	fileTime = newTime;
	
	ClearArray(array);
 
	File file = OpenFile(mapPath, "rt");
	if (!file) {
		LogError("Could not open file: %s", mapPath);
		return 0;
	}
 
	LogMessage("Loading maps from file: %s", mapPath);
	
	int len;
	char buffer[64];
	while (!file.EndOfFile() && file.ReadLine(buffer, sizeof(buffer)))
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
 
	file.Close();
	return GetArraySize(array);
}
