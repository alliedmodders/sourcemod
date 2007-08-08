/**
 * basevotes.sp
 * Implements basic vote commands.
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
 *
 * Version: $Id$
 */

#include <sourcemod>

#pragma semicolon 1

public Plugin:myinfo =
{
	name = "Basic Votes",
	author = "AlliedModders LLC",
	description = "Basic Vote Commands",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

new Handle:g_hVoteMenu = INVALID_HANDLE;

new Handle:g_hBanForward = INVALID_HANDLE;

new Handle:g_Cvar_VoteMap = INVALID_HANDLE;
new Handle:g_Cvar_VoteKick = INVALID_HANDLE;
new Handle:g_Cvar_VoteBan = INVALID_HANDLE;
//new Handle:g_Cvar_VoteSay = INVALID_HANDLE;

enum voteType
{
	question = 0,
	map,
	kick,
	ban
}

new voteType:g_voteType = voteType:question;

// Menu API does not provide us with a way to pass multiple peices of data with a single
// choice, so some globals are used to hold stuff.
//
#define VOTE_CLIENTID	0
#define VOTE_USERID	1
new g_voteClient[2];		/* Holds the target's client id and user id */

#define VOTE_NAME	0
#define VOTE_AUTHID	1
#define	VOTE_IP		2
new String:g_voteInfo[3][65];	/* Holds the target's name, authid, and IP */

new String:g_voteArg[256];	/* Used to hold ban/kick reasons or vote questions */


public OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("plugin.basevotes");
	
	RegAdminCmd("sm_votemap", Command_Votemap, ADMFLAG_VOTE|ADMFLAG_CHANGEMAP, "sm_votemap <mapname> [mapname2] ... [mapname5] ");
	RegAdminCmd("sm_votekick", Command_Votekick, ADMFLAG_VOTE|ADMFLAG_KICK, "sm_votekick <player> [reason]");
	RegAdminCmd("sm_voteban", Command_Voteban, ADMFLAG_VOTE|ADMFLAG_BAN, "sm_voteban <player> [reason]");
	RegAdminCmd("sm_vote", Command_Vote, ADMFLAG_VOTE, "sm_vote <question> [Answer1] [Answer2] ... [Answer5]");

	//g_Cvar_VoteShow = CreateConVar("sm_vote_show", "1", "Show player's votes? Default on.", 0, true, 0.0, true, 1.0);	

	g_Cvar_VoteMap = CreateConVar("sm_vote_map", "0.60", "percent required for successful map vote.", 0, true, 0.05, true, 1.0);
	g_Cvar_VoteKick = CreateConVar("sm_vote_kick", "0.60", "percent required for successful kick vote.", 0, true, 0.05, true, 1.0);	
	g_Cvar_VoteBan = CreateConVar("sm_vote_ban", "0.60", "percent required for successful ban vote.", 0, true, 0.05, true, 1.0);	
	
	g_hBanForward = CreateGlobalForward("OnClientBanned", ET_Ignore, Param_Cell, Param_Cell, Param_Cell, Param_String);
}

public Action:Command_Votemap(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_votemap <mapname> [mapname2] ... [mapname5]");
		return Plugin_Handled;	
	}
	
	if (IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote in Progress");
		return Plugin_Handled;
	}
	
	decl String:text[256];
	GetCmdArgString(text, sizeof(text));

	decl String:maps[5][64];
	new mapCount;	
	new len, pos;
	
	while (pos != -1 && mapCount < 5)
	{	
		pos = BreakString(text[len], maps[mapCount], sizeof(maps[]));
		
		if (!IsMapValid(maps[mapCount]))
		{
			ReplyToCommand(client, "[SM] %t", "Map was not found", maps[mapCount]);
			return Plugin_Handled;
		}		

		mapCount++;
		
		if (pos != -1)
		{
			len += pos;
		}	
	}

	ShowActivity(client, "%t", "Initiated Vote Map");
	
	g_voteType = voteType:map;
	
	g_hVoteMenu = CreateMenu(Handler_VoteCallback);
	
	if (mapCount == 1)
	{
		strcopy(g_voteInfo[VOTE_NAME], sizeof(g_voteInfo[]), maps[0]);
		SetMenuTitle(g_hVoteMenu, "Change Map To");
		AddMenuItem(g_hVoteMenu, maps[0], "Yes");
		AddMenuItem(g_hVoteMenu, "no", "No");
	}
	else
	{
		SetMenuTitle(g_hVoteMenu, "Map Vote");
		for (new i = 0; i < mapCount; i++)
		{
			AddMenuItem(g_hVoteMenu, maps[i], maps[i]);
		}	
	}
	
	SetMenuExitButton(g_hVoteMenu, false);
	VoteMenuToAll(g_hVoteMenu, 20);		
	
	return Plugin_Handled;	
}

public Action:Command_Vote(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_vote <question> [Answer1] [Answer2] ... [Answer5]");
		return Plugin_Handled;	
	}
	
	if (IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote in Progress");
		return Plugin_Handled;
	}
	
	decl String:text[256];
	GetCmdArgString(text, sizeof(text));

	decl String:answers[5][64];
	new answerCount;	
	new len = BreakString(text, g_voteArg, sizeof(g_voteArg));
	new pos = len;
	
	while (pos != -1 && answerCount < 5)
	{	
		pos = BreakString(text[len], answers[answerCount], sizeof(answers[]));
		answerCount++;
		
		if (pos != -1)
		{
			len += pos;
		}	
	}

	ShowActivity(client, "%t", "Initiate Vote", g_voteArg);
	
	g_voteType = voteType:question;
	
	g_hVoteMenu = CreateMenu(Handler_VoteCallback);
	SetMenuTitle(g_hVoteMenu, "%s?", g_voteArg);
	
	if (answerCount == 1)
	{
		AddMenuItem(g_hVoteMenu, "Yes", "Yes");
		AddMenuItem(g_hVoteMenu, "No", "No");
	}
	else
	{
		for (new i = 0; i < answerCount; i++)
		{
			AddMenuItem(g_hVoteMenu, answers[i], answers[i]);
		}	
	}
	
	SetMenuExitButton(g_hVoteMenu, false);
	VoteMenuToAll(g_hVoteMenu, 20);		
	
	return Plugin_Handled;	
}

public Action:Command_Votekick(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_votekick <player> [reason]");
		return Plugin_Handled;	
	}
	
	if (IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote in Progress");
		return Plugin_Handled;
	}	

	decl String:text[256], String:arg[64];
	GetCmdArgString(text, sizeof(text));
	
	new len = BreakString(text, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}
	
	if (len != -1)
	{
		strcopy(g_voteArg, sizeof(g_voteArg), text[len]);
	}
	else
	{
		g_voteArg[0] = '\0';
	}
	
	g_voteClient[VOTE_CLIENTID] = target;
	g_voteClient[VOTE_USERID] = GetClientUserId(target);

	GetClientName(target, g_voteInfo[VOTE_NAME], sizeof(g_voteInfo[]));

	ShowActivity(client, "%t", "Initiated Vote Kick", g_voteInfo[VOTE_NAME]);
	
	g_voteType = voteType:kick;
	
	g_hVoteMenu = CreateMenu(Handler_VoteCallback);
	SetMenuTitle(g_hVoteMenu, "Votekick Player");
	AddMenuItem(g_hVoteMenu, "No", "Yes");
	AddMenuItem(g_hVoteMenu, "No", "No");
	SetMenuExitButton(g_hVoteMenu, false);
	VoteMenuToAll(g_hVoteMenu, 20);
	
	return Plugin_Handled;
}

public Action:Command_Voteban(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_voteban <player> [reason]");
		return Plugin_Handled;	
	}
	
	if (IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote in Progress");
		return Plugin_Handled;
	}	

	decl String:text[256], String:arg[64];
	GetCmdArgString(text, sizeof(text));
	
	new len = BreakString(text, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}
	
	if (len != -1)
	{
		strcopy(g_voteArg, sizeof(g_voteArg), text[len]);
	}
	else
	{
		g_voteArg[0] = '\0';
	}
	
	g_voteClient[VOTE_CLIENTID] = target;
	g_voteClient[VOTE_USERID] = GetClientUserId(target);

	GetClientName(target, g_voteInfo[VOTE_NAME], sizeof(g_voteInfo[]));
	GetClientAuthString(target, g_voteInfo[VOTE_AUTHID], sizeof(g_voteInfo[]));
	GetClientIP(target, g_voteInfo[VOTE_IP], sizeof(g_voteInfo[]));

	ShowActivity(client, "%t", "Initiated Vote Ban", g_voteInfo[VOTE_NAME]);

	g_voteType = voteType:ban;
	
	g_hVoteMenu = CreateMenu(Handler_VoteCallback);
	SetMenuTitle(g_hVoteMenu, "Voteban Player");
	AddMenuItem(g_hVoteMenu, "No", "Yes");
	AddMenuItem(g_hVoteMenu, "No", "No");
	SetMenuExitButton(g_hVoteMenu, false);
	VoteMenuToAll(g_hVoteMenu, 20);
	
	return Plugin_Handled;
}

public Handler_VoteCallback(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_End)
	{
		VoteMenuClose();
	}
	else if (action == MenuAction_Display)
	{
	}
	else if (action == MenuAction_DisplayItem)
	{
		decl String:display[64];
		GetMenuItem(menu, param2, "", 0, _, display, sizeof(display));
	 
	 	if(strcmp(display, "No") == 0 || strcmp(display, "Yes") == 0)
	 	{
			decl String:buffer[255];
			Format(buffer, sizeof(buffer), "%T", display, param1);

			return RedrawMenuItem(buffer);
		}
	}
	/* else if (action == MenuAction_Select)
	{
		VoteSelect(menu, param1, param2);
	}*/
	else if (action == MenuAction_VoteEnd)
	{
		decl String:item[64], String:display[64];
		new Float:percent, Float:limit, votes, totalVotes;
		GetMenuVoteInfo(param2, votes, totalVotes);
		
		if (totalVotes < 1)
		{
			PrintToChatAll("[SM] %t", "No Votes Cast");
			return;
		}
		
		GetMenuItem(menu, param1, item, sizeof(item), _, display, sizeof(display));
		
		if (strcmp(display,"No") == 0 && param1 == 1)
		{
			votes = totalVotes - votes; // Reverse the votes to be in relation to the Yes option.
		}
		
		percent = GetVotePercent(votes, totalVotes);
		
		switch(g_voteType)
		{
			case (voteType:map):
				limit = GetConVarFloat(g_Cvar_VoteMap);
				
			case (voteType:kick):
				limit = GetConVarFloat(g_Cvar_VoteKick);
				
			case (voteType:ban):
				limit = GetConVarFloat(g_Cvar_VoteBan);
		}

		// A multi-argument vote is "always successful", but have to check if its a Yes/No vote.
		if ((strcmp(display,"Yes") == 0 && FloatCompare(percent,limit) < 0 && param1 == 0) || (strcmp(display,"No") == 0 && param1 == 1))
		{
			LogMessage("Vote failed.");
			PrintToChatAll("[SM] %t", "Vote Failed", RoundToNearest(100.0*limit), RoundToNearest(100.0*percent), totalVotes);
		}
		else
		{
			PrintToChatAll("[SM] %t", "Vote Successful", RoundToNearest(100.0*percent), totalVotes);
			
			switch(g_voteType)
			{
				case (voteType:question):
				{
					PrintToChatAll("[SM] %t", "Vote End", g_voteArg, item, RoundToNearest(100.0*percent), totalVotes);
				}
				
				case (voteType:map):
				{
					LogMessage("Changing map to %s due to vote.", item);
					PrintToChatAll("[SM] %t", "Changing map", item);
					new Handle:dp;
					CreateDataTimer(5.0, Timer_ChangeMap, dp);
					WritePackString(dp, item);		
				}
					
				case (voteType:kick):
				{
					if (g_voteArg[0] == '\0')
					{
						strcopy(g_voteArg, sizeof(g_voteArg), "Votekicked");
					}
					
					PrintToChatAll("[SM] %t", "Kicked player", g_voteInfo[VOTE_NAME]);					
					LogMessage("Vote kick successful, kicked \"%L\" (reason \"%s\")", g_voteClient[VOTE_USERID], g_voteArg);
					
					ServerCommand("kickid %d \"%s\"", g_voteClient[VOTE_USERID], g_voteArg);					
				}
					
				case (voteType:ban):
				{
					/* Fire the ban forward */
					Call_StartForward(g_hBanForward);
					Call_PushCell(0);
					Call_PushCell(g_voteClient[VOTE_USERID]);
					Call_PushCell(30);
					Call_PushString(g_voteArg);
					Call_Finish();
				
					if (g_voteArg[0] == '\0')
					{
						strcopy(g_voteArg, sizeof(g_voteArg), "Votebanned");
					}
					
					PrintToChatAll("[SM] %t", "Banned player", g_voteInfo[VOTE_NAME], 30);
					LogMessage("Vote ban successful, banned \"%L\" (minutes \"30\") (reason \"%s\")", g_voteClient[VOTE_USERID], g_voteArg);
					
					ServerCommand("banid %d %s", 30, g_voteClient[VOTE_AUTHID]);
					ServerCommand("kickid %d \"%s\"", g_voteClient[VOTE_USERID], g_voteArg);				
				}
			}
		}
	}
}

/*
VoteSelect(Handle:menu, param1, param2 = 0)
{
	if (GetConVarInt(g_Cvar_VoteShow) == 1)
	{
		decl String:voter[64], String:junk[64], String:choice[64];
		GetClientName(param1, voter, sizeof(voter));
		GetMenuItem(menu, param2, junk, sizeof(junk), _, choice, sizeof(choice));
		PrintToChatAll("[SM] %T", "Vote Select", LANG_SERVER, voter, choice);
	}
}
*/

VoteMenuClose()
{
	CloseHandle(g_hVoteMenu);
	g_hVoteMenu = INVALID_HANDLE;
}

Float:GetVotePercent(votes, totalVotes)
{
	return FloatDiv(float(votes),float(totalVotes));
}

public Action:Timer_ChangeMap(Handle:timer, Handle:dp)
{
	decl String:mapname[65];
	
	ResetPack(dp);
	ReadPackString(dp, mapname, sizeof(mapname));
	
	ServerCommand("changelevel \"%s\"", mapname);
	
	return Plugin_Stop;
}