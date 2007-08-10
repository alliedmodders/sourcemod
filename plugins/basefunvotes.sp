/**
 * basefunvotes.sp
 * Implements extra fun vote commands.
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
#include <sdktools>

#pragma semicolon 1

public Plugin:myinfo =
{
	name = "Fun Votes",
	author = "AlliedModders LLC",
	description = "Fun Vote Commands",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

#define VOTE_NO "###no###"
#define VOTE_YES "###yes###"

new Handle:g_hVoteMenu = INVALID_HANDLE;

new Handle:g_Cvar_Limits[5] = {INVALID_HANDLE, ...};
new Handle:g_Cvar_Gravity = INVALID_HANDLE;
new Handle:g_Cvar_Alltalk = INVALID_HANDLE;
new Handle:g_Cvar_FF = INVALID_HANDLE;

// new Handle:g_Cvar_Show = INVALID_HANDLE;

enum voteType
{
	gravity = 0,
	burn,
	slay,
	alltalk,
	ff
}

new voteType:g_voteType = voteType:gravity;

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

public OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("basevotes.phrases");
	LoadTranslations("basefunvotes.phrases");
	
	RegAdminCmd("sm_votegravity", Command_VoteGravity, ADMFLAG_VOTE, "sm_votegravity <amount> [amount2] ... [amount5]");
	RegAdminCmd("sm_voteburn", Command_VoteBurn, ADMFLAG_VOTE|ADMFLAG_SLAY, "sm_voteburn <player>");
	RegAdminCmd("sm_voteslay", Command_VoteSlay, ADMFLAG_VOTE|ADMFLAG_SLAY, "sm_voteslay <player>");
	RegAdminCmd("sm_votealltalk", Command_VoteAlltalk, ADMFLAG_VOTE, "sm_votealltalk");
	RegAdminCmd("sm_voteff", Command_VoteFF, ADMFLAG_VOTE, "sm_voteff");

	g_Cvar_Limits[0] = CreateConVar("sm_vote_gravity", "0.60", "percent required for successful gravity vote.", 0, true, 0.05, true, 1.0);
	g_Cvar_Limits[1] = CreateConVar("sm_vote_burn", "0.60", "percent required for successful burn vote.", 0, true, 0.05, true, 1.0);
	g_Cvar_Limits[2] = CreateConVar("sm_vote_slay", "0.60", "percent required for successful slay vote.", 0, true, 0.05, true, 1.0);
	g_Cvar_Limits[3] = CreateConVar("sm_vote_alltalk", "0.60", "percent required for successful alltalk vote.", 0, true, 0.05, true, 1.0);
	g_Cvar_Limits[4] = CreateConVar("sm_vote_ff", "0.60", "percent required for successful friendly fire vote.", 0, true, 0.05, true, 1.0);
	
	g_Cvar_Gravity = FindConVar("sv_gravity");
	g_Cvar_Alltalk = FindConVar("sv_alltalk");
	g_Cvar_FF = FindConVar("mp_friendlyfire");
	
	/*
	g_Cvar_Show = FindConVar("sm_vote_show");
	if (g_Cvar_Show == INVALID_HANDLE)
	{
		g_Cvar_Show = CreateConVar("sm_vote_show", "1", "Show player's votes? Default on.", 0, true, 0.0, true, 1.0);
	}
	*/
}

public Action:Command_VoteGravity(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_votegravity <amount> [amount2] ... [amount5]");
		return Plugin_Handled;	
	}
	
	if (IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote in Progress");
		return Plugin_Handled;
	}
	
	if (!TestVoteDelay(client))
	{
		return Plugin_Handled;
	}
	
	decl String:text[256];
	GetCmdArgString(text, sizeof(text));

	decl String:items[5][64];
	new count;	
	new len, pos;
	
	while (pos != -1 && count < 5)
	{	
		pos = BreakString(text[len], items[count], sizeof(items[]));
		
		decl Float:temp;
		if (StringToFloatEx(items[count], temp) == 0)
		{
			ReplyToCommand(client, "[SM] %t", "Invalid Amount");
			return Plugin_Handled;
		}		

		count++;
		
		if (pos != -1)
		{
			len += pos;
		}	
	}

	ShowActivity(client, "%t", "Initiated Vote Gravity");
	
	g_voteType = voteType:gravity;
	
	g_hVoteMenu = CreateMenu(Handler_VoteCallback, MenuAction:MENU_ACTIONS_ALL);
	
	if (count == 1)
	{
		strcopy(g_voteInfo[VOTE_NAME], sizeof(g_voteInfo[]), items[0]);
			
		SetMenuTitle(g_hVoteMenu, "Change Gravity To");
		AddMenuItem(g_hVoteMenu, items[0], "Yes");
		AddMenuItem(g_hVoteMenu, VOTE_NO, "No");
	}
	else
	{
		g_voteInfo[VOTE_NAME][0] = '\0';
		
		SetMenuTitle(g_hVoteMenu, "Gravity Vote");
		for (new i = 0; i < count; i++)
		{
			AddMenuItem(g_hVoteMenu, items[i], items[i]);
		}	
	}
	
	SetMenuExitButton(g_hVoteMenu, false);
	VoteMenuToAll(g_hVoteMenu, 20);		
	
	return Plugin_Handled;	
}

public Action:Command_VoteBurn(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_voteburn <player>");
		return Plugin_Handled;	
	}
	
	if (IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote in Progress");
		return Plugin_Handled;
	}	
	
	if (!TestVoteDelay(client))
	{
		return Plugin_Handled;
	}
	
	decl String:text[256], String:arg[64];
	GetCmdArgString(text, sizeof(text));
	
	BreakString(text, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	if (!IsPlayerAlive(target))
	{
		ReplyToCommand(client, "[SM] %t", "Cannot performed on dead", arg);
		return Plugin_Handled;
	}
	
	g_voteClient[VOTE_CLIENTID] = target;
	GetClientName(target, g_voteInfo[VOTE_NAME], sizeof(g_voteInfo[]));

	ShowActivity(client, "%t", "Initiated Vote Burn", g_voteInfo[VOTE_NAME]);
	
	g_voteType = voteType:burn;
	
	g_hVoteMenu = CreateMenu(Handler_VoteCallback, MenuAction:MENU_ACTIONS_ALL);
	SetMenuTitle(g_hVoteMenu, "Voteburn Player");
	AddMenuItem(g_hVoteMenu, VOTE_YES, "Yes");
	AddMenuItem(g_hVoteMenu, VOTE_NO, "No");
	SetMenuExitButton(g_hVoteMenu, false);
	VoteMenuToAll(g_hVoteMenu, 20);
	
	return Plugin_Handled;
}

public Action:Command_VoteSlay(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_voteslay <player>");
		return Plugin_Handled;	
	}
	
	if (IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote in Progress");
		return Plugin_Handled;
	}	
	
	if (!TestVoteDelay(client))
	{
		return Plugin_Handled;
	}
	
	decl String:text[256], String:arg[64];
	GetCmdArgString(text, sizeof(text));
	
	BreakString(text, arg, sizeof(arg));
	
	new target = FindTarget(client, arg);
	if (target == -1)
	{
		return Plugin_Handled;
	}

	if (!IsPlayerAlive(target))
	{
		ReplyToCommand(client, "[SM] %t", "Cannot performed on dead", arg);
		return Plugin_Handled;
	}
	
	g_voteClient[VOTE_CLIENTID] = target;
	GetClientName(target, g_voteInfo[VOTE_NAME], sizeof(g_voteInfo[]));

	ShowActivity(client, "%t", "Initiated Vote Slay", g_voteInfo[VOTE_NAME]);
	
	g_voteType = voteType:slay;
	
	g_hVoteMenu = CreateMenu(Handler_VoteCallback, MenuAction:MENU_ACTIONS_ALL);
	SetMenuTitle(g_hVoteMenu, "Voteslay Player");
	AddMenuItem(g_hVoteMenu, VOTE_YES, "Yes");
	AddMenuItem(g_hVoteMenu, VOTE_NO, "No");
	SetMenuExitButton(g_hVoteMenu, false);
	VoteMenuToAll(g_hVoteMenu, 20);
	
	return Plugin_Handled;
}

public Action:Command_VoteAlltalk(client, args)
{
	if (args > 0)
	{
		ReplyToCommand(client, "[SM] Usage: sm_votealltalk");
		return Plugin_Handled;	
	}
	
	if (IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote in Progress");
		return Plugin_Handled;
	}	
	
	if (!TestVoteDelay(client))
	{
		return Plugin_Handled;
	}
	
	ShowActivity(client, "%t", "Initiated Vote Alltalk");
	
	g_voteType = voteType:alltalk;
	g_voteInfo[VOTE_NAME][0] = '\0';

	g_hVoteMenu = CreateMenu(Handler_VoteCallback, MenuAction:MENU_ACTIONS_ALL);
	
	if (GetConVarBool(g_Cvar_Alltalk))
	{
		SetMenuTitle(g_hVoteMenu, "Votealltalk Off");
	}
	else
	{
		SetMenuTitle(g_hVoteMenu, "Votealltalk On");
	}
	
	AddMenuItem(g_hVoteMenu, VOTE_YES, "Yes");
	AddMenuItem(g_hVoteMenu, VOTE_NO, "No");
	SetMenuExitButton(g_hVoteMenu, false);
	VoteMenuToAll(g_hVoteMenu, 20);
	
	return Plugin_Handled;
}

public Action:Command_VoteFF(client, args)
{
	if (args > 0)
	{
		ReplyToCommand(client, "[SM] Usage: sm_voteff");
		return Plugin_Handled;	
	}
	
	if (IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote in Progress");
		return Plugin_Handled;
	}	
	
	if (!TestVoteDelay(client))
	{
		return Plugin_Handled;
	}
	
	ShowActivity(client, "%t", "Initiated Vote FF");
	
	g_voteType = voteType:ff;
	g_voteInfo[VOTE_NAME][0] = '\0';
	
	g_hVoteMenu = CreateMenu(Handler_VoteCallback, MenuAction:MENU_ACTIONS_ALL);
	
	if (GetConVarBool(g_Cvar_Alltalk))
	{
		SetMenuTitle(g_hVoteMenu, "Voteff Off");
	}
	else
	{
		SetMenuTitle(g_hVoteMenu, "Voteff On");
	}
	
	AddMenuItem(g_hVoteMenu, VOTE_YES, "Yes");
	AddMenuItem(g_hVoteMenu, VOTE_NO, "No");
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
	 	decl String:title[64];
		GetMenuTitle(g_hVoteMenu, title, sizeof(title));

	 	decl String:buffer[255];
		Format(buffer, sizeof(buffer), "%T", title, param1, g_voteInfo[VOTE_NAME]);

		SetMenuTitle(g_hVoteMenu, buffer);
	}
	else if (action == MenuAction_DisplayItem)
	{
		decl String:display[64];
		GetMenuItem(menu, param2, "", 0, _, display, sizeof(display));
	 
	 	if (strcmp(display, VOTE_NO) == 0 || strcmp(display, VOTE_YES) == 0)
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
	else if (action == VoteCancel_NoVotes)
	{
		PrintToChatAll("[SM] %t", "No Votes Cast");
	}
	else if (action == MenuAction_VoteEnd)
	{
		decl String:item[64];
		new Float:percent, Float:limit, votes, totalVotes;

		GetMenuVoteInfo(param2, votes, totalVotes);
		GetMenuItem(menu, param1, item, sizeof(item));
		
		if (strcmp(item, VOTE_NO) == 0 && param1 == 1)
		{
			votes = totalVotes - votes; // Reverse the votes to be in relation to the Yes option.
		}
		
		percent = GetVotePercent(votes, totalVotes);
		
		limit = GetConVarFloat(g_Cvar_Limits[g_voteType]);

		// A multi-argument vote is "always successful", but have to check if its a Yes/No vote.
		if ((strcmp(item, VOTE_YES) == 0 && FloatCompare(percent,limit) < 0 && param1 == 0) || (strcmp(item, VOTE_NO) == 0 && param1 == 1))
		{
			LogMessage("Vote failed.");
			PrintToChatAll("[SM] %t", "Vote Failed", RoundToNearest(100.0*limit), RoundToNearest(100.0*percent), totalVotes);
		}
		else
		{
			PrintToChatAll("[SM] %t", "Vote Successful", RoundToNearest(100.0*percent), totalVotes);
			
			switch (g_voteType)
			{
				case (voteType:gravity):
				{
					PrintToChatAll("[SM] %t", "Cvar changed", "sv_gravity", item);					
					LogMessage("Changing gravity to %s due to vote.", item);
					SetConVarInt(g_Cvar_Gravity, StringToInt(item));
				}
				
				case (voteType:burn):
				{
					PrintToChatAll("[SM] %t", "Ignited player", g_voteInfo[VOTE_NAME]);					
					LogMessage("Vote burn successful, igniting \"%L\"", g_voteClient[VOTE_CLIENTID]);
					
					IgniteEntity(g_voteClient[VOTE_CLIENTID], 19.8);	
				}
				
				case (voteType:slay):
				{
					PrintToChatAll("[SM] %t", "Slayed player", g_voteInfo[VOTE_NAME]);					
					LogMessage("Vote slay successful, slaying \"%L\"", g_voteClient[VOTE_CLIENTID]);
					
					ExtinguishEntity(g_voteClient[VOTE_CLIENTID]);
					ForcePlayerSuicide(g_voteClient[VOTE_CLIENTID]);
				}
				
				case (voteType:alltalk):
				{
					PrintToChatAll("[SM] %t", "Cvar changed", "sv_alltalk", (GetConVarBool(g_Cvar_Alltalk) ? "0" : "1"));
					LogMessage("Changing alltalk to %s due to vote.", (GetConVarBool(g_Cvar_Alltalk) ? "0" : "1"));
					SetConVarBool(g_Cvar_Alltalk, !GetConVarBool(g_Cvar_Alltalk));
				}
				
				case (voteType:ff):
				{
					PrintToChatAll("[SM] %t", "Cvar changed", "mp_friendlyfire", (GetConVarBool(g_Cvar_FF) ? "0" : "1"));
					LogMessage("Changing friendly fire to %s due to vote.", (GetConVarBool(g_Cvar_FF) ? "0" : "1"));
					SetConVarBool(g_Cvar_FF, !GetConVarBool(g_Cvar_FF));
				}				
			}
		}
	}
	
	return 0;
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

bool:TestVoteDelay(client)
{
 	new delay = CheckVoteDelay();
 	
 	if (delay > 0)
 	{
 		if (delay > 60)
 		{
 			ReplyToCommand(client, "[SM] %t", "Vote Delay Minutes", delay % 60);
 		}
 		else
 		{
 			ReplyToCommand(client, "[SM] %t", "Vote Delay Seconds", delay);
 		}
 		
 		return false;
 	}
 	
	return true;
}