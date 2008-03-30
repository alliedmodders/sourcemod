/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Map Manager Plugin
 * Contains timer callbacks
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

public Action:Timer_ChangeMap(Handle:timer)
{
	ServerCommand("changelevel \"%s\"", g_NextMap);
	PushArrayString(g_MapHistory, g_NextMap);

	return Plugin_Stop;
}

public Action:Timer_RandomizeNextmap(Handle:timer)
{
	decl String:map[64];
	
	new exclusion = GetConVarInt(g_Cvar_ExcludeMaps);
	if (exclusion > GetArraySize(g_MapCycle))
	{
		exclusion = GetArraySize(g_MapCycle) - 1;
	}

	new b = GetRandomInt(0, GetArraySize(g_MapCycle) - 1);
	GetArrayString(g_MapCycle, b, map, sizeof(map));

	while (FindStringInArray(g_MapHistory, map) != -1)
	{
		b = GetRandomInt(0, GetArraySize(g_MapCycle) - 1);
		GetArrayString(g_MapCycle, b, map, sizeof(map));
	}
	
	SetNextmap(map);

	LogMessage("[MapManager] Randomizer has chosen '%s' as the next map.", map);	

	return Plugin_Stop;
}

public Action:Timer_DelayRTV(Handle:timer)
{
	g_RTVAllowed = true;
	g_RTVStarted = false;
	g_RTVEnded = false;	
}

public Action:Timer_StartRTV(Handle:timer)
{
	if (timer == g_RetryTimer)
	{
		g_RetryTimer = INVALID_HANDLE;
	}
	
	if (g_RetryTimer != INVALID_HANDLE)
	{
		return;
	}

	if (IsVoteInProgress())
	{
		// Can't start a vote, try again in 5 seconds.
		g_RetryTimer = CreateTimer(5.0, Timer_StartRTV, TIMER_FLAG_NO_MAPCHANGE);
		return;
	}			
	
	PrintToChatAll("[SM] %t", "RTV Vote Ready");
		
	new Handle:MapVoteMenu = CreateMenu(Handler_MapMapVoteMenu, MenuAction:MENU_ACTIONS_ALL);
	SetMenuTitle(MapVoteMenu, "Rock The Vote");
	
	new Handle:tempMaps  = CloneArray(g_MapList);
	decl String:map[32];

	GetCurrentMap(map, sizeof(map));
	new index = FindStringInArray(tempMaps, map);
	if (index != -1)
	{
		RemoveFromArray(tempMaps, index);
	}	
	
	// We assume that g_RTVMapList is within the correct limits, based on the logic for nominations
	for (new i = 0; i < GetArraySize(g_RTVMapList); i++)
	{
		GetArrayString(g_RTVMapList, i, map, sizeof(map));
		AddMenuItem(MapVoteMenu, map, map);
		
		index = FindStringInArray(tempMaps, map);
		if (index != -1)
		{
			RemoveFromArray(tempMaps, index);
		}
	}
	
	new limit = GetConVarInt(g_Cvar_Maps) - GetArraySize(g_RTVMapList);
	if (limit > GetArraySize(tempMaps))
	{
		limit = GetArraySize(tempMaps);
	}

	for (new i = 0; i < limit; i++)
	{
		new b = GetRandomInt(0, GetArraySize(tempMaps) - 1);
		GetArrayString(tempMaps, b, map, sizeof(map));		
		PushArrayString(g_RTVMapList, map);
		AddMenuItem(MapVoteMenu, map, map);			
		RemoveFromArray(tempMaps, b);
	}	
	
	CloseHandle(tempMaps);
	
	AddMenuItem(MapVoteMenu, "Don't Change", "Don't Change");
		
	SetMenuExitButton(MapVoteMenu, false);
	VoteMenuToAll(MapVoteMenu, 20);
		
	LogMessage("[SM] Rockthevote was successfully started.");
}

public Action:Timer_StartMapVote(Handle:timer)
{
	if (!GetArraySize(g_MapList))
	{
		return Plugin_Stop;
	}		
	
	if (timer == g_RetryTimer)
	{
		g_RetryTimer = INVALID_HANDLE;
	}
	else
	{
		g_VoteTimer = INVALID_HANDLE;
	}

	InitiateVote();

	return Plugin_Stop;
}