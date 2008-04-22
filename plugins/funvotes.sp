/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Fun Votes Plugin
 * Implements extra fun vote commands.
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
#include <sdktools>

#undef REQUIRE_PLUGIN
#include <adminmenu>

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
};

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

new Handle:hTopMenu = INVALID_HANDLE;

#include "funvotes/votegravity.sp"
#include "funvotes/voteburn.sp"
#include "funvotes/voteslay.sp"
#include "funvotes/votealltalk.sp"
#include "funvotes/voteff.sp"

public OnPluginStart()
{
	if (FindPluginByFile("basefunvotes.smx") != INVALID_HANDLE)
	{
		ThrowError("This plugin replaces basefuncommands.  You cannot run both at once.");
	}
	
	LoadTranslations("common.phrases");
	LoadTranslations("basevotes.phrases");
	LoadTranslations("funvotes.phrases");
	LoadTranslations("funcommands.phrases");
	
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
	
	/* Account for late loading */
	new Handle:topmenu;
	if (LibraryExists("adminmenu") && ((topmenu = GetAdminTopMenu()) != INVALID_HANDLE))
	{
		OnAdminMenuReady(topmenu);
	}
}

public OnAdminMenuReady(Handle:topmenu)
{
	/* Block us from being called twice */
	if (topmenu == hTopMenu)
	{
		return;
	}
	
	/* Save the Handle */
	hTopMenu = topmenu;
	
	/* Build the "Voting Commands" category */
	new TopMenuObject:voting_commands = FindTopMenuCategory(hTopMenu, ADMINMENU_VOTINGCOMMANDS);

	if (voting_commands != INVALID_TOPMENUOBJECT)
	{
		AddToTopMenu(hTopMenu,
			"sm_votegravity",
			TopMenuObject_Item,
			AdminMenu_VoteGravity,
			voting_commands,
			"sm_votegravity",
			ADMFLAG_VOTE);
			
		AddToTopMenu(hTopMenu,
			"sm_voteburn",
			TopMenuObject_Item,
			AdminMenu_VoteBurn,
			voting_commands,
			"sm_voteburn",
			ADMFLAG_VOTE|ADMFLAG_SLAY);
			
		AddToTopMenu(hTopMenu,
			"sm_voteslay",
			TopMenuObject_Item,
			AdminMenu_VoteSlay,
			voting_commands,
			"sm_voteslay",
			ADMFLAG_VOTE|ADMFLAG_SLAY);
			
		AddToTopMenu(hTopMenu,
			"sm_votealltalk",
			TopMenuObject_Item,
			AdminMenu_VoteAllTalk,
			voting_commands,
			"sm_votealltalk",
			ADMFLAG_VOTE);
			
		AddToTopMenu(hTopMenu,
			"sm_voteff",
			TopMenuObject_Item,
			AdminMenu_VoteFF,
			voting_commands,
			"sm_voteff",
			ADMFLAG_VOTE);
	}
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
		GetMenuTitle(menu, title, sizeof(title));

	 	decl String:buffer[255];
		Format(buffer, sizeof(buffer), "%T", title, param1, g_voteInfo[VOTE_NAME]);

		new Handle:panel = Handle:param2;
		SetPanelTitle(panel, buffer);
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
	else if (action == MenuAction_VoteCancel && param1 == VoteCancel_NoVotes)
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
		
		/* :TODO: g_voteClient[userid] needs to be checked.
		 */

		// A multi-argument vote is "always successful", but have to check if its a Yes/No vote.
		if ((strcmp(item, VOTE_YES) == 0 && FloatCompare(percent,limit) < 0 && param1 == 0) || (strcmp(item, VOTE_NO) == 0 && param1 == 1))
		{
			/* :TODO: g_voteClient[userid] should be used here and set to -1 if not applicable.
			 */
			LogAction(-1, -1, "Vote failed.");
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
					LogAction(-1, -1, "Changing gravity to %s due to vote.", item);
					SetConVarInt(g_Cvar_Gravity, StringToInt(item));
				}
				
				case (voteType:burn):
				{
					PrintToChatAll("[SM] %t", "Set target on fire", "_s", g_voteInfo[VOTE_NAME]);					
					LogAction(-1, g_voteClient[VOTE_CLIENTID], "Vote burn successful, igniting \"%L\"", g_voteClient[VOTE_CLIENTID]);
					
					IgniteEntity(g_voteClient[VOTE_CLIENTID], 19.8);	
				}
				
				case (voteType:slay):
				{
					PrintToChatAll("[SM] %t", "Slayed player", g_voteInfo[VOTE_NAME]);					
					LogAction(-1, g_voteClient[VOTE_CLIENTID], "Vote slay successful, slaying \"%L\"", g_voteClient[VOTE_CLIENTID]);
					
					ExtinguishEntity(g_voteClient[VOTE_CLIENTID]);
					ForcePlayerSuicide(g_voteClient[VOTE_CLIENTID]);
				}
				
				case (voteType:alltalk):
				{
					PrintToChatAll("[SM] %t", "Cvar changed", "sv_alltalk", (GetConVarBool(g_Cvar_Alltalk) ? "0" : "1"));
					LogAction(-1, -1, "Changing alltalk to %s due to vote.", (GetConVarBool(g_Cvar_Alltalk) ? "0" : "1"));
					SetConVarBool(g_Cvar_Alltalk, !GetConVarBool(g_Cvar_Alltalk));
				}
				
				case (voteType:ff):
				{
					PrintToChatAll("[SM] %t", "Cvar changed", "mp_friendlyfire", (GetConVarBool(g_Cvar_FF) ? "0" : "1"));
					LogAction(-1, -1, "Changing friendly fire to %s due to vote.", (GetConVarBool(g_Cvar_FF) ? "0" : "1"));
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
