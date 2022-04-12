/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basic Votes Plugin
 * Implements basic vote commands.
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
#undef REQUIRE_PLUGIN
#include <adminmenu>

#pragma newdecls required

public Plugin myinfo =
{
	name = "Basic Votes",
	author = "AlliedModders LLC",
	description = "Basic Vote Commands",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

#define VOTE_NO "###no###"
#define VOTE_YES "###yes###"

#define GENERIC_COUNT 5
#define ANSWER_SIZE 64

Menu g_hVoteMenu = null;

ConVar g_Cvar_Limits[3] = {null, ...};
ConVar g_Cvar_Voteban = null;
//ConVar g_Cvar_VoteSay = null;

enum VoteType
{
	VoteType_Map,
	VoteType_Kick,
	VoteType_Ban,
	VoteType_Question
}

VoteType g_voteType = VoteType_Question;

// Menu API does not provide us with a way to pass multiple peices of data with a single
// choice, so some globals are used to hold stuff.
//
int g_voteTarget;		/* Holds the target's user id */

#define VOTE_NAME	0
#define VOTE_AUTHID	1
#define	VOTE_IP		2
char g_voteInfo[3][65];	/* Holds the target's name, authid, and IP */

char g_voteArg[256];	/* Used to hold ban/kick reasons or vote questions */


TopMenu hTopMenu;

#include "basevotes/votekick.sp"
#include "basevotes/voteban.sp"
#include "basevotes/votemap.sp"

public void OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("basevotes.phrases");
	LoadTranslations("plugin.basecommands");
	LoadTranslations("basebans.phrases");
	
	RegAdminCmd("sm_votemap", Command_Votemap, ADMFLAG_VOTE|ADMFLAG_CHANGEMAP, "sm_votemap <mapname> [mapname2] ... [mapname5] ");
	RegAdminCmd("sm_votekick", Command_Votekick, ADMFLAG_VOTE|ADMFLAG_KICK, "sm_votekick <player> [reason]");
	RegAdminCmd("sm_voteban", Command_Voteban, ADMFLAG_VOTE|ADMFLAG_BAN, "sm_voteban <player> [reason]");
	RegAdminCmd("sm_vote", Command_Vote, ADMFLAG_VOTE, "sm_vote <question> [Answer1] [Answer2] ... [Answer5]");

	/*
	g_Cvar_Show = FindConVar("sm_vote_show");
	if (g_Cvar_Show == null)
	{
		g_Cvar_Show = CreateConVar("sm_vote_show", "1", "Show player's votes? Default on.", 0, true, 0.0, true, 1.0);
	}
	*/

	g_Cvar_Limits[0] = CreateConVar("sm_vote_map", "0.60", "percent required for successful map vote.", 0, true, 0.05, true, 1.0);
	g_Cvar_Limits[1] = CreateConVar("sm_vote_kick", "0.60", "percent required for successful kick vote.", 0, true, 0.05, true, 1.0);	
	g_Cvar_Limits[2] = CreateConVar("sm_vote_ban", "0.60", "percent required for successful ban vote.", 0, true, 0.05, true, 1.0);		
	g_Cvar_Voteban = CreateConVar("sm_voteban_time", "30", "length of ban in minutes.", 0, true, 0.0);	

	AutoExecConfig(true, "basevotes");
	
	/* Account for late loading */
	TopMenu topmenu;
	if (LibraryExists("adminmenu") && ((topmenu = GetAdminTopMenu()) != null))
	{
		OnAdminMenuReady(topmenu);
	}
	
	g_SelectedMaps = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH));
	
	g_MapList = new Menu(MenuHandler_Map, MenuAction_DrawItem|MenuAction_Display);
	g_MapList.SetTitle("%T", "Please select a map", LANG_SERVER);
	g_MapList.ExitBackButton = true;
	
	char mapListPath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, mapListPath, sizeof(mapListPath), "configs/adminmenu_maplist.ini");
	SetMapListCompatBind("sm_votemap menu", mapListPath);
}

public void OnConfigsExecuted()
{
	g_mapCount = LoadMapList(g_MapList);
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
		hTopMenu.AddItem("sm_votekick", AdminMenu_VoteKick, voting_commands, "sm_votekick", ADMFLAG_VOTE|ADMFLAG_KICK);
		hTopMenu.AddItem("sm_voteban", AdminMenu_VoteBan, voting_commands, "sm_voteban", ADMFLAG_VOTE|ADMFLAG_BAN);
		hTopMenu.AddItem("sm_votemap", AdminMenu_VoteMap, voting_commands, "sm_votemap", ADMFLAG_VOTE|ADMFLAG_CHANGEMAP);
	}
}

public Action Command_Vote(int client, int args)
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
		
	if (!TestVoteDelay(client))
	{
		return Plugin_Handled;
	}
	
	char text[256];
	GetCmdArgString(text, sizeof(text));

	char answers[GENERIC_COUNT][ANSWER_SIZE];
	int answerCount;	
	int len = BreakString(text, g_voteArg, sizeof(g_voteArg));
	int pos = len;
	
	char answers_list[GENERIC_COUNT * (ANSWER_SIZE + 3)];
	
	while (args > 1 && pos != -1 && answerCount < GENERIC_COUNT)
	{	
		pos = BreakString(text[len], answers[answerCount], sizeof(answers[]));
		answerCount++;
		
		if (pos != -1)
		{
			len += pos;
		}	
	}
	g_voteType = VoteType_Question;
	
	g_hVoteMenu = new Menu(Handler_VoteCallback, MENU_ACTIONS_ALL);
	g_hVoteMenu.SetTitle("%s", g_voteArg);
	
	if (answerCount < 2)
	{
		g_hVoteMenu.AddItem(VOTE_YES, "Yes");
		g_hVoteMenu.AddItem(VOTE_NO, "No");
		Format(answers_list, sizeof(answers_list), " \"Yes\" \"No\"");
	}
	else
	{
		for (int i = 0; i < answerCount; i++)
		{
			g_hVoteMenu.AddItem(answers[i], answers[i]);
			Format(answers_list, sizeof(answers_list), "%s \"%s\"", answers_list, answers[i]);
		}	
	}
	
	LogAction(client, -1, "\"%L\" initiated a generic vote (question \"%s\" / answers%s).", client, g_voteArg, answers_list);
	ShowActivity2(client, "[SM] ", "%t", "Initiate Vote", g_voteArg);
	
	g_hVoteMenu.ExitButton = false;
	g_hVoteMenu.DisplayVoteToAll(20);		
	
	return Plugin_Handled;	
}

public int Handler_VoteCallback(Menu menu, MenuAction action, int param1, int param2)
{
	if (action == MenuAction_End)
	{
		delete g_hVoteMenu;
	}
	else if (action == MenuAction_Display)
	{
	 	if (g_voteType != VoteType_Question)
	 	{
			char title[64];
			menu.GetTitle(title, sizeof(title));
			
	 		char buffer[255];
			Format(buffer, sizeof(buffer), "%T", title, param1, g_voteInfo[VOTE_NAME]);

			Panel panel = view_as<Panel>(param2);
			panel.SetTitle(buffer);
		}
	}
	else if (action == MenuAction_DisplayItem)
	{
		char display[64];
		menu.GetItem(param2, "", 0, _, display, sizeof(display));
	 
	 	if (strcmp(display, "No") == 0 || strcmp(display, "Yes") == 0)
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
		char item[PLATFORM_MAX_PATH], display[64];
		float percent, limit;
		int votes, totalVotes;

		GetMenuVoteInfo(param2, votes, totalVotes);
		menu.GetItem(param1, item, sizeof(item), _, display, sizeof(display));
		
		if (strcmp(item, VOTE_NO) == 0 && param1 == 1)
		{
			votes = totalVotes - votes; // Reverse the votes to be in relation to the Yes option.
		}
		
		percent = float(votes) / float(totalVotes);
		
		if (g_voteType != VoteType_Question)
		{
			limit = g_Cvar_Limits[g_voteType].FloatValue;
		}
		
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
				case VoteType_Question:
				{
					if (strcmp(item, VOTE_NO) == 0 || strcmp(item, VOTE_YES) == 0)
					{
						strcopy(item, sizeof(item), display);
					}
					
					PrintToChatAll("[SM] %t", "Vote End", g_voteArg, item);
				}
				
				case VoteType_Map:
				{
					// single-vote items don't use the display item
					char displayName[PLATFORM_MAX_PATH];
					GetMapDisplayName(item, displayName, sizeof(displayName));
					LogAction(-1, -1, "Changing map to %s due to vote.", item);
					PrintToChatAll("[SM] %t", "Changing map", displayName);
					DataPack dp;
					CreateDataTimer(5.0, Timer_ChangeMap, dp);
					dp.WriteString(item);		
				}
					
				case VoteType_Kick:
				{
					int voteTarget;
					if((voteTarget = GetClientOfUserId(g_voteTarget)) == 0)
					{
						LogAction(-1, -1, "Vote kick failed, unable to kick \"%s\" (reason \"%s\")", g_voteInfo[VOTE_NAME], "Player no longer available");
					}
					else
					{
						if (g_voteArg[0] == '\0')
						{
							strcopy(g_voteArg, sizeof(g_voteArg), "Votekicked");
						}
						
						PrintToChatAll("[SM] %t", "Kicked target", "_s", g_voteInfo[VOTE_NAME]);					
						LogAction(-1, voteTarget, "Vote kick successful, kicked \"%L\" (reason \"%s\")", voteTarget, g_voteArg);
						
						ServerCommand("kickid %d \"%s\"", g_voteTarget, g_voteArg);					
					}
				}
					
				case VoteType_Ban:
				{
					if (g_voteArg[0] == '\0')
					{
						strcopy(g_voteArg, sizeof(g_voteArg), "Votebanned");
					}
					
					int minutes = g_Cvar_Voteban.IntValue;
					
					PrintToChatAll("[SM] %t", "Banned player", g_voteInfo[VOTE_NAME], minutes);
					
					int voteTarget;
					if((voteTarget = GetClientOfUserId(g_voteTarget)) == 0)
					{
						LogAction(-1, -1, "Vote ban successful, banned \"%s\" (%s) (minutes \"%d\") (reason \"%s\")", g_voteInfo[VOTE_NAME], g_voteInfo[VOTE_AUTHID], minutes, g_voteArg);
						
						BanIdentity(g_voteInfo[VOTE_AUTHID],
								  minutes,
								  BANFLAG_AUTHID,
								  g_voteArg,
								  "sm_voteban");
					}
					else
					{
						LogAction(-1, voteTarget, "Vote ban successful, banned \"%L\" (minutes \"%d\") (reason \"%s\")", voteTarget, minutes, g_voteArg);
						
						BanClient(voteTarget,
								  minutes,
								  BANFLAG_AUTO,
								  g_voteArg,
								  "Banned by vote",
								  "sm_voteban");
					}
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
		char voter[64], junk[64], choice[64];
		GetClientName(param1, voter, sizeof(voter));
		menu.GetItem(param2, junk, sizeof(junk), _, choice, sizeof(choice));
		PrintToChatAll("[SM] %T", "Vote Select", LANG_SERVER, voter, choice);
	}
}
*/

bool TestVoteDelay(int client)
{
	if (CheckCommandAccess(client, "sm_vote_delay_bypass", ADMFLAG_CONVARS, true))
	{
		return true;
	}
	
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

public Action Timer_ChangeMap(Handle timer, DataPack dp)
{
	char mapname[PLATFORM_MAX_PATH];
	
	dp.Reset();
	dp.ReadString(mapname, sizeof(mapname));
	
	ForceChangeLevel(mapname, "sm_votemap Result");
	
	return Plugin_Stop;
}
