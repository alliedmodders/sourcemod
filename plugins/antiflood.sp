/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Anti-Flood Plugin
 * Protects against chat flooding.
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

#pragma newdecls required

public Plugin myinfo = 
{
	name = "Anti-Flood",
	author = "AlliedModders LLC",
	description = "Protects against chat flooding",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

enum struct PlayerInfo {
	float lastTime; /* Last time player used say or say_team */
	int tokenCount; /* Number of flood tokens player has */
}

PlayerInfo playerinfo[MAXPLAYERS+1];

ConVar sm_flood_time;									/* Handle to sm_flood_time convar */
float max_chat;
public void OnPluginStart()
{
	sm_flood_time = CreateConVar("sm_flood_time", "0.75", "Amount of time allowed between chat messages");
}

public void OnClientPutInServer(int client)
{
	playerinfo[client].lastTime = 0.0;
	playerinfo[client].tokenCount = 0;
}


public bool OnClientFloodCheck(int client)
{
	max_chat = sm_flood_time.FloatValue;
	
	if (max_chat <= 0.0 
 		|| CheckCommandAccess(client, "sm_flood_access", ADMFLAG_ROOT, true))
	{
		return false;
	}
	
	if (playerinfo[client].lastTime >= GetGameTime())
	{
		/* If player has 3 or more flood tokens, block their message */
		if (playerinfo[client].tokenCount >= 3)
		{
			return true;
		}
	}
	
	return false;
}

public void OnClientFloodResult(int client, bool blocked)
{
	if (max_chat <= 0.0 
 		|| CheckCommandAccess(client, "sm_flood_access", ADMFLAG_ROOT, true))
	{
		return;
	}
	
	float curTime = GetGameTime();
	float newTime = curTime + max_chat;
	
	if (playerinfo[client].lastTime >= curTime)
	{
		/* If the last message was blocked, update their time limit */
		if (blocked)
		{
			newTime += 3.0;
		}
		/* Add one flood token when player goes over chat time limit */
		else if (playerinfo[client].tokenCount < 3)
		{
			playerinfo[client].tokenCount++;
		}
	}
	else if (playerinfo[client].tokenCount > 0)
	{
		/* Remove one flood token when player chats within time limit (slow decay) */
		playerinfo[client].tokenCount--;
	}
	
	playerinfo[client].lastTime = newTime;
}
