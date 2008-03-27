/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Map Manager Plugin
 * Contains event callbacks
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

public Action:UserMsg_VGUIMenu(UserMsg:msg_id, Handle:bf, const players[], playersNum, bool:reliable, bool:init)
{
	if (g_IntermissionCalled)
	{
		return Plugin_Handled;
	}
	
	decl String:type[15];

	/* If we don't get a valid string, bail out. */
	if (BfReadString(bf, type, sizeof(type)) < 0)
	{
		return Plugin_Handled;
	}
 
	if (BfReadByte(bf) == 1 && BfReadByte(bf) == 0 && (strcmp(type, "scores", false) == 0))
	{
		g_IntermissionCalled = true;
		
		decl String:map[32];
		new Float:fChatTime = GetConVarFloat(g_Cvar_Chattime);
		
		if (fChatTime < 2.0)
		{
			SetConVarFloat(g_Cvar_Chattime, 2.0);
		}
		
		LogMessage("[MapManager] Changing map to '%s' due to map ending.", g_NextMap);
		
		g_MapChanged = true;
		CreateTimer(fChatTime - 1.0, Timer_ChangeMap);		
	}
	
	return Plugin_Handled;
}

public Event_RoundEnd(Handle:event, const String:name[], bool:dontBroadcast)
{
	if (g_MapChanged)
	{
		return;
	}
	
	if (g_MapChangeSet && g_MapChangeWhen == 'r')
	{
		LogMessage("[MapManager] Changing map to '%s' due to round ending.", g_NextMap);
		
		g_MapChanged = true;
		CreateTimer(1.0, Timer_ChangeMap);
		
		return;
	}
	
	if (g_HasVoteStarted)
	{
		return;
	}

	new winner = GetEventInt(event, "winner");
	
	if (winner == 0 || winner == 1)
	{
		return;
	}

	g_TotalRounds++;
	
	new team[2], teamPos = -1;
	for (new i; i < GetArraySize(g_TeamScores); i++)
	{
		GetArrayArray(g_TeamScores, i, team);
		if (team[0] == winner)
		{
			teamPos = i;
			break;
		}		
	}
	
	if (teamPos == -1)
	{
		team[0] = winner;
		team[1] = 1;
		PushArrayArray(g_TeamScores, team);
	}
	else
	{
		team[1]++;
		SetArrayArray(g_TeamScores, teamPos, team);
	}	
	
	if (g_Cvar_Winlimit != INVALID_HANDLE)
	{
		new winlimit = GetConVarInt(g_Cvar_Winlimit);
		if (winlimit)
		{
			if (team[1] >= (winlimit - GetConVarInt(g_Cvar_StartRounds)))
			{
				InitiateVote();
			}
		}
	}
	
	if (g_Cvar_Maxrounds != INVALID_HANDLE)
	{
		new maxrounds = GetConVarInt(g_Cvar_Maxrounds);
		if (maxrounds)
		{
			if (g_TotalRounds >= (maxrounds - GetConVarInt(g_Cvar_StartRounds)))
			{
				InitiateVote();
			}			
		}
	}
}

public Event_PlayerDeath(Handle:event, const String:name[], bool:dontBroadcast)
{
	if (g_MapChanged || g_HasVoteStarted || g_Cvar_Fraglimit == INVALID_HANDLE)
	{
		return;
	}
	
	if (!GetConVarInt(g_Cvar_Fraglimit))
	{
		return;
	}

	new fragger = GetClientOfUserId(GetEventInt(event, "attacker"));
	if (fragger && GetClientFrags(fragger) >= (GetConVarInt(g_Cvar_Fraglimit) - GetConVarInt(g_Cvar_StartFrags)))
	{
		InitiateVote();
	}
}