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

#pragma newdecls required

public Plugin myinfo =
{
	name = "Fun Votes",
	author = "AlliedModders LLC",
	description = "Fun Vote Commands",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

#define VOTE_NO "###no###"
#define VOTE_YES "###yes###"

Menu g_hVoteMenu = null;

ConVar g_Cvar_Limits[5] = {null, ...};
ConVar g_Cvar_Gravity;
ConVar g_Cvar_Alltalk;
ConVar g_Cvar_FF;

// ConVar g_Cvar_Show = null;

enum voteType
{
	gravity = 0,
	burn,
	slay,
	alltalk,
	ff
};

voteType g_voteType = gravity;

// Menu API does not provide us with a way to pass multiple peices of data with a single
// choice, so some globals are used to hold stuff.
//
int g_voteTarget;		/* Holds the target's user id */

#define VOTE_NAME	0
#define VOTE_AUTHID	1
#define	VOTE_IP		2
char g_voteInfo[3][65];		/* Holds the target's name, authid, and IP */

TopMenu hTopMenu;

#include "funvotes/votegravity.sp"
#include "funvotes/voteburn.sp"
#include "funvotes/voteslay.sp"
#include "funvotes/votealltalk.sp"
#include "funvotes/voteff.sp"

public void OnPluginStart()
{
	if (FindPluginByFile("basefunvotes.smx") != null)
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
	if (g_Cvar_Show == null)
	{
		g_Cvar_Show = CreateConVar("sm_vote_show", "1", "Show player's votes? Default on.", 0, true, 0.0, true, 1.0);
	}
	*/

	AutoExecConfig(true, "funvotes");
	
	/* Account for late loading */
	TopMenu topmenu;
	if (LibraryExists("adminmenu") && ((topmenu = GetAdminTopMenu()) != null))
	{
		OnAdminMenuReady(topmenu);
	}
}

public void OnAdminMenuReady(Handle aTopMenu)
{
	TopMenu topmenu = TopMenu.FromHandle(aTopMenu);

	/* Block us from being called twice */
	if (topmenu == hTopMenu)
	{
		return;
	}
	
	/* Save the Handle */
	hTopMenu = topmenu;
	
	/* Build the "Voting Commands" category */
	TopMenuObject voting_commands = hTopMenu.FindCategory(ADMINMENU_VOTINGCOMMANDS);

	if (voting_commands != INVALID_TOPMENUOBJECT)
	{
		hTopMenu.AddItem("sm_votegravity", AdminMenu_VoteGravity, voting_commands, "sm_votegravity", ADMFLAG_VOTE);
		hTopMenu.AddItem("sm_voteburn", AdminMenu_VoteBurn, voting_commands, "sm_voteburn", ADMFLAG_VOTE|ADMFLAG_SLAY);
		hTopMenu.AddItem("sm_voteslay", AdminMenu_VoteSlay, voting_commands, "sm_voteslay", ADMFLAG_VOTE|ADMFLAG_SLAY);
		hTopMenu.AddItem("sm_votealltalk", AdminMenu_VoteAllTalk, voting_commands, "sm_votealltalk", ADMFLAG_VOTE);
		hTopMenu.AddItem("sm_voteff", AdminMenu_VoteFF, voting_commands, "sm_voteff", ADMFLAG_VOTE);
	}
}

public int Handler_VoteCallback(Menu menu, MenuAction action, int param1, int param2)
{
	if (action == MenuAction_End)
	{
		VoteMenuClose();
	}
	else if (action == MenuAction_Display)
	{
	 	char title[64];
		menu.GetTitle(title, sizeof(title));

	 	char buffer[255];
		Format(buffer, sizeof(buffer), "%T", title, param1, g_voteInfo[VOTE_NAME]);

		Panel panel = view_as<Panel>(param2);
		panel.SetTitle(buffer);
	}
	else if (action == MenuAction_DisplayItem)
	{
		char display[64];
		menu.GetItem(param2, "", 0, _, display, sizeof(display));
	 
	 	if (strcmp(display, VOTE_NO) == 0 || strcmp(display, VOTE_YES) == 0)
	 	{
			char buffer[255];
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
		char item[64];
		float percent, limit;
		int votes, totalVotes;

		GetMenuVoteInfo(param2, votes, totalVotes);
		menu.GetItem(param1, item, sizeof(item));
		
		if (strcmp(item, VOTE_NO) == 0 && param1 == 1)
		{
			votes = totalVotes - votes; // Reverse the votes to be in relation to the Yes option.
		}
		
		percent = GetVotePercent(votes, totalVotes);
		
		limit = g_Cvar_Limits[g_voteType].FloatValue;
		
		// A multi-argument vote is "always successful", but have to check if its a Yes/No vote.
		if ((strcmp(item, VOTE_YES) == 0 && FloatCompare(percent,limit) < 0 && param1 == 0) || (strcmp(item, VOTE_NO) == 0 && param1 == 1))
		{
			/* :TODO: g_voteTarget should be used here and set to -1 if not applicable.
			 */
			LogAction(-1, -1, "Vote failed.");
			PrintToChatAll("[SM] %t", "Vote Failed", RoundToNearest(100.0*limit), RoundToNearest(100.0*percent), totalVotes);
		}
		else
		{
			PrintToChatAll("[SM] %t", "Vote Successful", RoundToNearest(100.0*percent), totalVotes);
			
			switch (g_voteType)
			{
				case (gravity):
				{
					PrintToChatAll("[SM] %t", "Cvar changed", "sv_gravity", item);					
					LogAction(-1, -1, "Changing gravity to %s due to vote.", item);
					g_Cvar_Gravity.IntValue = StringToInt(item);
				}
				
				case (burn):
				{
					int voteTarget;
					if((voteTarget = GetClientOfUserId(g_voteTarget)) == 0)
					{
						LogAction(-1, -1, "Vote burn failed, unable to burn \"%s\" (reason \"%s\")", g_voteInfo[VOTE_NAME], "Player no longer available");
					}
					else
					{
						PrintToChatAll("[SM] %t", "Set target on fire", "_s", g_voteInfo[VOTE_NAME]);					
						LogAction(-1, voteTarget, "Vote burn successful, igniting \"%L\"", voteTarget);
						
						IgniteEntity(voteTarget, 19.8);	
					}
				}
				
				case (slay):
				{
					int voteTarget;
					if((voteTarget = GetClientOfUserId(g_voteTarget)) == 0)
					{
						LogAction(-1, -1, "Vote slay failed, unable to slay \"%s\" (reason \"%s\")", g_voteInfo[VOTE_NAME], "Player no longer available");
					}
					else
					{
						PrintToChatAll("[SM] %t", "Slayed player", g_voteInfo[VOTE_NAME]);					
						LogAction(-1, voteTarget, "Vote slay successful, slaying \"%L\"", voteTarget);
						
						ExtinguishEntity(voteTarget);
						ForcePlayerSuicide(voteTarget);
					}
				}
				
				case (alltalk):
				{
					PrintToChatAll("[SM] %t", "Cvar changed", "sv_alltalk", (g_Cvar_Alltalk.BoolValue ? "0" : "1"));
					LogAction(-1, -1, "Changing alltalk to %s due to vote.", (g_Cvar_Alltalk.BoolValue ? "0" : "1"));
					g_Cvar_Alltalk.BoolValue = !g_Cvar_Alltalk.BoolValue;
				}
				
				case (ff):
				{
					PrintToChatAll("[SM] %t", "Cvar changed", "mp_friendlyfire", (g_Cvar_FF.BoolValue ? "0" : "1"));
					LogAction(-1, -1, "Changing friendly fire to %s due to vote.", (g_Cvar_FF.BoolValue ? "0" : "1"));
					g_Cvar_FF.BoolValue = !g_Cvar_FF.BoolValue;
				}				
			}
		}
	}
	
	return 0;
}

/*
void VoteSelect(Menu menu, int param1, int param2 = 0)
{
	if (g_Cvar_VoteShow.IntValue == 1)
	{
		char voter[MAX_NAME_LENGTH], junk[64], choice[64];
		GetClientName(param1, voter, sizeof(voter));
		menu.GetItem(param2, junk, sizeof(junk), _, choice, sizeof(choice));
		PrintToChatAll("[SM] %T", "Vote Select", LANG_SERVER, voter, choice);
	}
}
*/

void VoteMenuClose()
{
	delete g_hVoteMenu;
}

float GetVotePercent(int votes, int totalVotes)
{
	return float(votes) / float(totalVotes);
}

bool TestVoteDelay(int client)
{
 	int delay = CheckVoteDelay();
 	
 	if (delay > 0)
 	{
 		if (delay > 60)
 		{
 			ReplyToCommand(client, "[SM] %t", "Vote Delay Minutes", (delay / 60));
 		}
 		else
 		{
 			ReplyToCommand(client, "[SM] %t", "Vote Delay Seconds", delay);
 		}
 		
 		return false;
 	}
 	
	return true;
}
