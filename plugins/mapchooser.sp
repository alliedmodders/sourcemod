/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Mapchooser Plugin
 * Creates a map vote at appropriate times, setting sm_nextmap to the winning
 * vote
 *
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
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
#include <mapchooser>
#include <nextmap>

public Plugin:myinfo =
{
	name = "MapChooser",
	author = "AlliedModders LLC",
	description = "Automated Map Voting",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

/* Valve ConVars */
ConVar g_Cvar_Winlimit;
ConVar g_Cvar_Maxrounds;
ConVar g_Cvar_Fraglimit;
ConVar g_Cvar_Bonusroundtime;

/* Plugin ConVars */
ConVar g_Cvar_StartTime;
ConVar g_Cvar_StartRounds;
ConVar g_Cvar_StartFrags;
ConVar g_Cvar_ExtendTimeStep;
ConVar g_Cvar_ExtendRoundStep;
ConVar g_Cvar_ExtendFragStep;
ConVar g_Cvar_ExcludeMaps;
ConVar g_Cvar_IncludeMaps;
ConVar g_Cvar_NoVoteMode;
ConVar g_Cvar_Extend;
ConVar g_Cvar_DontChange;
ConVar g_Cvar_EndOfMapVote;
ConVar g_Cvar_VoteDuration;
ConVar g_Cvar_RunOff;
ConVar g_Cvar_RunOffPercent;

new Handle:g_VoteTimer = INVALID_HANDLE;
new Handle:g_RetryTimer = INVALID_HANDLE;

/* Data Handles */
new Handle:g_MapList = null;
new Handle:g_NominateList = null;
new Handle:g_NominateOwners = null;
new Handle:g_OldMapList = null;
new Handle:g_NextMapList = null;
Menu g_VoteMenu;

new g_Extends;
new g_TotalRounds;
new bool:g_HasVoteStarted;
new bool:g_WaitingForVote;
new bool:g_MapVoteCompleted;
new bool:g_ChangeMapAtRoundEnd;
new bool:g_ChangeMapInProgress;
new g_mapFileSerial = -1;

new MapChange:g_ChangeTime;

new Handle:g_NominationsResetForward = null;
new Handle:g_MapVoteStartedForward = null;

/* Upper bound of how many team there could be */
#define MAXTEAMS 10
new g_winCount[MAXTEAMS];

#define VOTE_EXTEND "##extend##"
#define VOTE_DONTCHANGE "##dontchange##"

public OnPluginStart()
{
	LoadTranslations("mapchooser.phrases");
	LoadTranslations("common.phrases");
	
	new arraySize = ByteCountToCells(PLATFORM_MAX_PATH);
	g_MapList = CreateArray(arraySize);
	g_NominateList = CreateArray(arraySize);
	g_NominateOwners = CreateArray(1);
	g_OldMapList = CreateArray(arraySize);
	g_NextMapList = CreateArray(arraySize);
	
	g_Cvar_EndOfMapVote = CreateConVar("sm_mapvote_endvote", "1", "Specifies if MapChooser should run an end of map vote", _, true, 0.0, true, 1.0);

	g_Cvar_StartTime = CreateConVar("sm_mapvote_start", "3.0", "Specifies when to start the vote based on time remaining.", _, true, 1.0);
	g_Cvar_StartRounds = CreateConVar("sm_mapvote_startround", "2.0", "Specifies when to start the vote based on rounds remaining. Use 0 on TF2 to start vote during bonus round time", _, true, 0.0);
	g_Cvar_StartFrags = CreateConVar("sm_mapvote_startfrags", "5.0", "Specifies when to start the vote base on frags remaining.", _, true, 1.0);
	g_Cvar_ExtendTimeStep = CreateConVar("sm_extendmap_timestep", "15", "Specifies how much many more minutes each extension makes", _, true, 5.0);
	g_Cvar_ExtendRoundStep = CreateConVar("sm_extendmap_roundstep", "5", "Specifies how many more rounds each extension makes", _, true, 1.0);
	g_Cvar_ExtendFragStep = CreateConVar("sm_extendmap_fragstep", "10", "Specifies how many more frags are allowed when map is extended.", _, true, 5.0);	
	g_Cvar_ExcludeMaps = CreateConVar("sm_mapvote_exclude", "5", "Specifies how many past maps to exclude from the vote.", _, true, 0.0);
	g_Cvar_IncludeMaps = CreateConVar("sm_mapvote_include", "5", "Specifies how many maps to include in the vote.", _, true, 2.0, true, 6.0);
	g_Cvar_NoVoteMode = CreateConVar("sm_mapvote_novote", "1", "Specifies whether or not MapChooser should pick a map if no votes are received.", _, true, 0.0, true, 1.0);
	g_Cvar_Extend = CreateConVar("sm_mapvote_extend", "0", "Number of extensions allowed each map.", _, true, 0.0);
	g_Cvar_DontChange = CreateConVar("sm_mapvote_dontchange", "1", "Specifies if a 'Don't Change' option should be added to early votes", _, true, 0.0);
	g_Cvar_VoteDuration = CreateConVar("sm_mapvote_voteduration", "20", "Specifies how long the mapvote should be available for.", _, true, 5.0);
	g_Cvar_RunOff = CreateConVar("sm_mapvote_runoff", "0", "Hold run of votes if winning choice is less than a certain margin", _, true, 0.0, true, 1.0);
	g_Cvar_RunOffPercent = CreateConVar("sm_mapvote_runoffpercent", "50", "If winning choice has less than this percent of votes, hold a runoff", _, true, 0.0, true, 100.0);
	
	RegAdminCmd("sm_mapvote", Command_Mapvote, ADMFLAG_CHANGEMAP, "sm_mapvote - Forces MapChooser to attempt to run a map vote now.");
	RegAdminCmd("sm_setnextmap", Command_SetNextmap, ADMFLAG_CHANGEMAP, "sm_setnextmap <map>");

	g_Cvar_Winlimit = FindConVar("mp_winlimit");
	g_Cvar_Maxrounds = FindConVar("mp_maxrounds");
	g_Cvar_Fraglimit = FindConVar("mp_fraglimit");
	g_Cvar_Bonusroundtime = FindConVar("mp_bonusroundtime");
	
	if (g_Cvar_Winlimit || g_Cvar_Maxrounds)
	{
		char folder[64];
		GetGameFolderName(folder, sizeof(folder));

		if (strcmp(folder, "tf") == 0)
		{
			HookEvent("teamplay_win_panel", Event_TeamPlayWinPanel);
			HookEvent("teamplay_restart_round", Event_TFRestartRound);
			HookEvent("arena_win_panel", Event_TeamPlayWinPanel);
		}
		else if (strcmp(folder, "nucleardawn") == 0)
		{
			HookEvent("round_win", Event_RoundEnd);
		}
		else
		{
			HookEvent("round_end", Event_RoundEnd);
		}
	}
	
	if (g_Cvar_Fraglimit)
	{
		HookEvent("player_death", Event_PlayerDeath);		
	}
	
	AutoExecConfig(true, "mapchooser");
	
	//Change the mp_bonusroundtime max so that we have time to display the vote
	//If you display a vote during bonus time good defaults are 17 vote duration and 19 mp_bonustime
	if (g_Cvar_Bonusroundtime)
	{
		g_Cvar_Bonusroundtime.SetBounds(ConVarBound_Upper, true, 30.0);		
	}
	
	g_NominationsResetForward = CreateGlobalForward("OnNominationRemoved", ET_Ignore, Param_String, Param_Cell);
	g_MapVoteStartedForward = CreateGlobalForward("OnMapVoteStarted", ET_Ignore);
}

public APLRes:AskPluginLoad2(Handle:myself, bool:late, String:error[], err_max)
{
	RegPluginLibrary("mapchooser");	
	
	CreateNative("NominateMap", Native_NominateMap);
	CreateNative("RemoveNominationByMap", Native_RemoveNominationByMap);
	CreateNative("RemoveNominationByOwner", Native_RemoveNominationByOwner);
	CreateNative("InitiateMapChooserVote", Native_InitiateVote);
	CreateNative("CanMapChooserStartVote", Native_CanVoteStart);
	CreateNative("HasEndOfMapVoteFinished", Native_CheckVoteDone);
	CreateNative("GetExcludeMapList", Native_GetExcludeMapList);
	CreateNative("GetNominatedMapList", Native_GetNominatedMapList);
	CreateNative("EndOfMapVoteEnabled", Native_EndOfMapVoteEnabled);

	return APLRes_Success;
}

public OnConfigsExecuted()
{
	if (ReadMapList(g_MapList,
					 g_mapFileSerial, 
					 "mapchooser",
					 MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_MAPSFOLDER)
		!= null)
		
	{
		if (g_mapFileSerial == -1)
		{
			LogError("Unable to create a valid map list.");
		}
	}
	
	CreateNextVote();
	SetupTimeleftTimer();
	
	g_TotalRounds = 0;
	
	g_Extends = 0;
	
	g_MapVoteCompleted = false;
	
	ClearArray(g_NominateList);
	ClearArray(g_NominateOwners);
	
	for (new i=0; i<MAXTEAMS; i++)
	{
		g_winCount[i] = 0;	
	}
	

	/* Check if mapchooser will attempt to start mapvote during bonus round time - TF2 Only */
	if (g_Cvar_Bonusroundtime && !g_Cvar_StartRounds.IntValue)
	{
		if (g_Cvar_Bonusroundtime.FloatValue <= g_Cvar_VoteDuration.FloatValue)
		{
			LogError("Warning - Bonus Round Time shorter than Vote Time. Votes during bonus round may not have time to complete");
		}
	}
}

public OnMapEnd()
{
	g_HasVoteStarted = false;
	g_WaitingForVote = false;
	g_ChangeMapAtRoundEnd = false;
	g_ChangeMapInProgress = false;
	
	g_VoteTimer = null;
	g_RetryTimer = null;
	
	decl String:map[PLATFORM_MAX_PATH];
	GetCurrentMap(map, sizeof(map));
	PushArrayString(g_OldMapList, map);
				
	if (GetArraySize(g_OldMapList) > g_Cvar_ExcludeMaps.IntValue)
	{
		RemoveFromArray(g_OldMapList, 0);
	}	
}

public OnClientDisconnect(client)
{
	new index = FindValueInArray(g_NominateOwners, client);
	
	if (index == -1)
	{
		return;
	}
	
	new String:oldmap[PLATFORM_MAX_PATH];
	GetArrayString(g_NominateList, index, oldmap, sizeof(oldmap));
	Call_StartForward(g_NominationsResetForward);
	Call_PushString(oldmap);
	Call_PushCell(GetArrayCell(g_NominateOwners, index));
	Call_Finish();
	
	RemoveFromArray(g_NominateOwners, index);
	RemoveFromArray(g_NominateList, index);
}

public Action:Command_SetNextmap(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_setnextmap <map>");
		return Plugin_Handled;
	}

	decl String:map[PLATFORM_MAX_PATH];
	GetCmdArg(1, map, sizeof(map));

	if (!IsMapValid(map))
	{
		ReplyToCommand(client, "[SM] %t", "Map was not found", map);
		return Plugin_Handled;
	}

	ShowActivity(client, "%t", "Changed Next Map", map);
	LogAction(client, -1, "\"%L\" changed nextmap to \"%s\"", client, map);

	SetNextMap(map);
	g_MapVoteCompleted = true;

	return Plugin_Handled;
}

public OnMapTimeLeftChanged()
{
	if (GetArraySize(g_MapList))
	{
		SetupTimeleftTimer();
	}
}

SetupTimeleftTimer()
{
	new time;
	if (GetMapTimeLeft(time) && time > 0)
	{
		new startTime = g_Cvar_StartTime.IntValue * 60;
		if (time - startTime < 0 && g_Cvar_EndOfMapVote.BoolValue && !g_MapVoteCompleted && !g_HasVoteStarted)
		{
			InitiateVote(MapChange_MapEnd, null);		
		}
		else
		{
			if (g_VoteTimer != null)
			{
				KillTimer(g_VoteTimer);
				g_VoteTimer = null;
			}	
			
			//g_VoteTimer = CreateTimer(float(time - startTime), Timer_StartMapVote, _, TIMER_FLAG_NO_MAPCHANGE);
			new Handle:data;
			g_VoteTimer = CreateDataTimer(float(time - startTime), Timer_StartMapVote, data, TIMER_FLAG_NO_MAPCHANGE);
			WritePackCell(data, _:MapChange_MapEnd);
			WritePackCell(data, _:INVALID_HANDLE);
			ResetPack(data);
		}		
	}
}

public Action:Timer_StartMapVote(Handle:timer, Handle:data)
{
	if (timer == g_RetryTimer)
	{
		g_WaitingForVote = false;
		g_RetryTimer = null;
	}
	else
	{
		g_VoteTimer = null;
	}
	
	if (!GetArraySize(g_MapList) || !g_Cvar_EndOfMapVote.BoolValue || g_MapVoteCompleted || g_HasVoteStarted)
	{
		return Plugin_Stop;
	}
	
	new MapChange:mapChange = MapChange:ReadPackCell(data);
	new Handle:hndl = Handle:ReadPackCell(data);

	InitiateVote(mapChange, hndl);

	return Plugin_Stop;
}

public Event_TFRestartRound(Handle:event, const String:name[], bool:dontBroadcast)
{
	/* Game got restarted - reset our round count tracking */
	g_TotalRounds = 0;	
}

public Event_TeamPlayWinPanel(Event event, const String:name[], bool:dontBroadcast)
{
	if (g_ChangeMapAtRoundEnd)
	{
		g_ChangeMapAtRoundEnd = false;
		CreateTimer(2.0, Timer_ChangeMap, INVALID_HANDLE, TIMER_FLAG_NO_MAPCHANGE);
		g_ChangeMapInProgress = true;
	}
	
	new bluescore = event.GetInt("blue_score");
	new redscore = event.GetInt("red_score");
		
	if (event.GetInt("round_complete") == 1 || StrEqual(name, "arena_win_panel"))
	{
		g_TotalRounds++;
		
		if (!GetArraySize(g_MapList) || g_HasVoteStarted || g_MapVoteCompleted || !g_Cvar_EndOfMapVote.BoolValue)
		{
			return;
		}
		
		CheckMaxRounds(g_TotalRounds);
		
		switch(event.GetInt("winning_team"))
		{
			case 3:
			{
				CheckWinLimit(bluescore);
			}
			case 2:
			{
				CheckWinLimit(redscore);				
			}			
			//We need to do nothing on winning_team == 0 this indicates stalemate.
			default:
			{
				return;
			}			
		}
	}
}
/* You ask, why don't you just use team_score event? And I answer... Because CSS doesn't. */
public Event_RoundEnd(Event event, const String:name[], bool:dontBroadcast)
{
	if (g_ChangeMapAtRoundEnd)
	{
		g_ChangeMapAtRoundEnd = false;
		CreateTimer(2.0, Timer_ChangeMap, INVALID_HANDLE, TIMER_FLAG_NO_MAPCHANGE);
		g_ChangeMapInProgress = true;
	}
	
	new winner;
	if (strcmp(name, "round_win") == 0)
	{
		// Nuclear Dawn
		winner = event.GetInt("team");
	}
	else
	{
		winner = event.GetInt("winner");
	}
	
	if (winner == 0 || winner == 1 || !g_Cvar_EndOfMapVote.BoolValue)
	{
		return;
	}
	
	if (winner >= MAXTEAMS)
	{
		SetFailState("Mod exceed maximum team count - Please file a bug report.");	
	}

	g_TotalRounds++;
	
	g_winCount[winner]++;
	
	if (!GetArraySize(g_MapList) || g_HasVoteStarted || g_MapVoteCompleted)
	{
		return;
	}
	
	CheckWinLimit(g_winCount[winner]);
	CheckMaxRounds(g_TotalRounds);
}

public CheckWinLimit(winner_score)
{	
	if (g_Cvar_Winlimit)
	{
		int winlimit = g_Cvar_Winlimit.IntValue;
		if (winlimit)
		{			
			if (winner_score >= (winlimit - g_Cvar_StartRounds.IntValue))
			{
				InitiateVote(MapChange_MapEnd, null);
			}
		}
	}
}

public CheckMaxRounds(roundcount)
{		
	if (g_Cvar_Maxrounds)
	{
		int maxrounds = g_Cvar_Maxrounds.IntValue;
		if (maxrounds)
		{
			if (roundcount >= (maxrounds - g_Cvar_StartRounds.IntValue))
			{
				InitiateVote(MapChange_MapEnd, null);
			}			
		}
	}
}

public Event_PlayerDeath(Event event, const String:name[], bool:dontBroadcast)
{
	if (!GetArraySize(g_MapList) || !g_Cvar_Fraglimit || g_HasVoteStarted)
	{
		return;
	}
	
	if (!g_Cvar_Fraglimit.IntValue || !g_Cvar_EndOfMapVote.BoolValue)
	{
		return;
	}

	if (g_MapVoteCompleted)
	{
		return;
	}

	new fragger = GetClientOfUserId(event.GetInt("attacker"));

	if (!fragger)
	{
		return;
	}

	if (GetClientFrags(fragger) >= (g_Cvar_Fraglimit.IntValue - g_Cvar_StartFrags.IntValue))
	{
		InitiateVote(MapChange_MapEnd, null);
	}
}

public Action:Command_Mapvote(client, args)
{
	InitiateVote(MapChange_MapEnd, null);

	return Plugin_Handled;	
}

/**
 * Starts a new map vote
 *
 * @param when			When the resulting map change should occur.
 * @param inputlist		Optional list of maps to use for the vote, otherwise an internal list of nominations + random maps will be used.
 * @param noSpecials	Block special vote options like extend/nochange (upgrade this to bitflags instead?)
 */
InitiateVote(MapChange:when, Handle:inputlist=null)
{
	g_WaitingForVote = true;
	
	if (IsVoteInProgress())
	{
		// Can't start a vote, try again in 5 seconds.
		//g_RetryTimer = CreateTimer(5.0, Timer_StartMapVote, _, TIMER_FLAG_NO_MAPCHANGE);
		
		new Handle:data;
		g_RetryTimer = CreateDataTimer(5.0, Timer_StartMapVote, data, TIMER_FLAG_NO_MAPCHANGE);
		WritePackCell(data, _:when);
		WritePackCell(data, _:inputlist);
		ResetPack(data);
		return;
	}
	
	/* If the main map vote has completed (and chosen result) and its currently changing (not a delayed change) we block further attempts */
	if (g_MapVoteCompleted && g_ChangeMapInProgress)
	{
		return;
	}
	
	g_ChangeTime = when;
	
	g_WaitingForVote = false;
		
	g_HasVoteStarted = true;
	g_VoteMenu = new Menu(Handler_MapVoteMenu, MenuAction:MENU_ACTIONS_ALL);
	g_VoteMenu.SetTitle("Vote Nextmap");
	g_VoteMenu.VoteResultCallback = Handler_MapVoteFinished;

	/* Call OnMapVoteStarted() Forward */
	Call_StartForward(g_MapVoteStartedForward);
	Call_Finish();
	
	/**
	 * TODO: Make a proper decision on when to clear the nominations list.
	 * Currently it clears when used, and stays if an external list is provided.
	 * Is this the right thing to do? External lists will probably come from places
	 * like sm_mapvote from the adminmenu in the future.
	 */
	 
	char map[PLATFORM_MAX_PATH];
	
	/* No input given - User our internal nominations and maplist */
	if (inputlist == null)
	{
		int nominateCount = GetArraySize(g_NominateList);
		int voteSize = g_Cvar_IncludeMaps.IntValue;
		
		/* Smaller of the two - It should be impossible for nominations to exceed the size though (cvar changed mid-map?) */
		int nominationsToAdd = nominateCount >= voteSize ? voteSize : nominateCount;
		
		
		for (new i=0; i<nominationsToAdd; i++)
		{
			GetArrayString(g_NominateList, i, map, sizeof(map));
			g_VoteMenu.AddItem(map, map);
			RemoveStringFromArray(g_NextMapList, map);
			
			/* Notify Nominations that this map is now free */
			Call_StartForward(g_NominationsResetForward);
			Call_PushString(map);
			Call_PushCell(GetArrayCell(g_NominateOwners, i));
			Call_Finish();
		}
		
		/* Clear out the rest of the nominations array */
		for (new i=nominationsToAdd; i<nominateCount; i++)
		{
			GetArrayString(g_NominateList, i, map, sizeof(map));
			/* These maps shouldn't be excluded from the vote as they weren't really nominated at all */
			
			/* Notify Nominations that this map is now free */
			Call_StartForward(g_NominationsResetForward);
			Call_PushString(map);
			Call_PushCell(GetArrayCell(g_NominateOwners, i));
			Call_Finish();			
		}
		
		/* There should currently be 'nominationsToAdd' unique maps in the vote */
		
		new i = nominationsToAdd;
		new count = 0;
		new availableMaps = GetArraySize(g_NextMapList);
		
		while (i < voteSize)
		{
			if (count >= availableMaps)
			{
				//Run out of maps, this will have to do.
				break;
			}
			
			GetArrayString(g_NextMapList, count, map, sizeof(map));
			count++;
			
			/* Insert the map and increment our count */
			g_VoteMenu.AddItem(map, map);
			i++;
		}
		
		/* Wipe out our nominations list - Nominations have already been informed of this */
		ClearArray(g_NominateOwners);
		ClearArray(g_NominateList);
	}
	else //We were given a list of maps to start the vote with
	{
		new size = GetArraySize(inputlist);
		
		for (new i=0; i<size; i++)
		{
			GetArrayString(inputlist, i, map, sizeof(map));
			
			if (IsMapValid(map))
			{
				g_VoteMenu.AddItem(map, map);
			}	
		}
	}
	
	/* Do we add any special items? */
	if ((when == MapChange_Instant || when == MapChange_RoundEnd) && g_Cvar_DontChange.BoolValue)
	{
		g_VoteMenu.AddItem(VOTE_DONTCHANGE, "Don't Change");
	}
	else if (g_Cvar_Extend.BoolValue && g_Extends < g_Cvar_Extend.IntValue)
	{
		g_VoteMenu.AddItem(VOTE_EXTEND, "Extend Map");
	}
	
	/* There are no maps we could vote for. Don't show anything. */
	if (g_VoteMenu.ItemCount == 0)
	{
		g_HasVoteStarted = false;
		delete g_VoteMenu;
		g_VoteMenu = null;
		return;
	}
	
	int voteDuration = g_Cvar_VoteDuration.IntValue;

	g_VoteMenu.ExitButton = false;
	g_VoteMenu.DisplayVoteToAll(voteDuration);

	LogAction(-1, -1, "Voting for next map has started.");
	PrintToChatAll("[SM] %t", "Nextmap Voting Started");
}

public Handler_VoteFinishedGeneric(Menu menu,
						   num_votes, 
						   num_clients,
						   const client_info[][2], 
						   num_items,
						   const item_info[][2])
{
	char map[PLATFORM_MAX_PATH];
	menu.GetItem(item_info[0][VOTEINFO_ITEM_INDEX], map, sizeof(map));

	if (strcmp(map, VOTE_EXTEND, false) == 0)
	{
		g_Extends++;
		
		int time;
		if (GetMapTimeLimit(time))
		{
			if (time > 0)
			{
				ExtendMapTimeLimit(g_Cvar_ExtendTimeStep.IntValue * 60);						
			}
		}
		
		if (g_Cvar_Winlimit)
		{
			int winlimit = g_Cvar_Winlimit.IntValue;
			if (winlimit)
			{
				g_Cvar_Winlimit.IntValue = winlimit + g_Cvar_ExtendRoundStep.IntValue;
			}					
		}
		
		if (g_Cvar_Maxrounds)
		{
			new maxrounds = g_Cvar_Maxrounds.IntValue;
			if (maxrounds)
			{
				g_Cvar_Maxrounds.IntValue = maxrounds + g_Cvar_ExtendRoundStep.IntValue;
			}
		}
		
		if (g_Cvar_Fraglimit)
		{
			int fraglimit = g_Cvar_Fraglimit.IntValue;
			if (fraglimit)
			{
				g_Cvar_Fraglimit.IntValue = fraglimit + g_Cvar_ExtendFragStep.IntValue;
			}
		}

		PrintToChatAll("[SM] %t", "Current Map Extended", RoundToFloor(float(item_info[0][VOTEINFO_ITEM_VOTES])/float(num_votes)*100), num_votes);
		LogAction(-1, -1, "Voting for next map has finished. The current map has been extended.");
		
		// We extended, so we'll have to vote again.
		g_HasVoteStarted = false;
		CreateNextVote();
		SetupTimeleftTimer();
		
	}
	else if (strcmp(map, VOTE_DONTCHANGE, false) == 0)
	{
		PrintToChatAll("[SM] %t", "Current Map Stays", RoundToFloor(float(item_info[0][VOTEINFO_ITEM_VOTES])/float(num_votes)*100), num_votes);
		LogAction(-1, -1, "Voting for next map has finished. 'No Change' was the winner");
		
		g_HasVoteStarted = false;
		CreateNextVote();
		SetupTimeleftTimer();
	}
	else
	{
		if (g_ChangeTime == MapChange_MapEnd)
		{
			SetNextMap(map);
		}
		else if (g_ChangeTime == MapChange_Instant)
		{
			new Handle:data;
			CreateDataTimer(2.0, Timer_ChangeMap, data);
			WritePackString(data, map);
			g_ChangeMapInProgress = false;
		}
		else // MapChange_RoundEnd
		{
			SetNextMap(map);
			g_ChangeMapAtRoundEnd = true;
		}
		
		g_HasVoteStarted = false;
		g_MapVoteCompleted = true;
		
		PrintToChatAll("[SM] %t", "Nextmap Voting Finished", map, RoundToFloor(float(item_info[0][VOTEINFO_ITEM_VOTES])/float(num_votes)*100), num_votes);
		LogAction(-1, -1, "Voting for next map has finished. Nextmap: %s.", map);
	}	
}

public Handler_MapVoteFinished(Menu menu,
						   int num_votes, 
						   int num_clients,
						   const client_info[][2], 
						   int num_items,
						   const item_info[][2])
{
	if (g_Cvar_RunOff.BoolValue && num_items > 1)
	{
		float winningvotes = float(item_info[0][VOTEINFO_ITEM_VOTES]);
		float required = num_votes * (g_Cvar_RunOffPercent.FloatValue / 100.0);
		
		if (winningvotes < required)
		{
			/* Insufficient Winning margin - Lets do a runoff */
			g_VoteMenu = CreateMenu(Handler_MapVoteMenu, MenuAction:MENU_ACTIONS_ALL);
			g_VoteMenu.SetTitle("Runoff Vote Nextmap");
			SetVoteResultCallback(g_VoteMenu, Handler_VoteFinishedGeneric);

			char map[PLATFORM_MAX_PATH];
			char info1[PLATFORM_MAX_PATH];
			char info2[PLATFORM_MAX_PATH];
			
			menu.GetItem(item_info[0][VOTEINFO_ITEM_INDEX], map, sizeof(map), _, info1, sizeof(info1));
			g_VoteMenu.AddItem(map, info1);
			menu.GetItem(item_info[1][VOTEINFO_ITEM_INDEX], map, sizeof(map), _, info2, sizeof(info2));
			g_VoteMenu.AddItem(map, info2);
			
			int voteDuration = g_Cvar_VoteDuration.IntValue;
			g_VoteMenu.ExitButton = false;
			g_VoteMenu.DisplayVoteToAll(voteDuration);
			
			/* Notify */
			float map1percent = float(item_info[0][VOTEINFO_ITEM_VOTES])/ float(num_votes) * 100;
			float map2percent = float(item_info[1][VOTEINFO_ITEM_VOTES])/ float(num_votes) * 100;
			
			
			PrintToChatAll("[SM] %t", "Starting Runoff", g_Cvar_RunOffPercent.FloatValue, info1, map1percent, info2, map2percent);
			LogMessage("Voting for next map was indecisive, beginning runoff vote");
					
			return;
		}
	}
	
	Handler_VoteFinishedGeneric(menu, num_votes, num_clients, client_info, num_items, item_info);
}

public Handler_MapVoteMenu(Menu menu, MenuAction action, int param1, int param2)
{
	switch (action)
	{
		case MenuAction_End:
		{
			g_VoteMenu = null;
			delete menu;
		}
		
		case MenuAction_Display:
		{
	 		decl String:buffer[255];
			Format(buffer, sizeof(buffer), "%T", "Vote Nextmap", param1);

			Panel panel = Panel:param2;
			panel.SetTitle(buffer);
		}		
		
		case MenuAction_DisplayItem:
		{
			if (menu.ItemCount - 1 == param2)
			{
				char map[PLATFORM_MAX_PATH], buffer[255];
				menu.GetItem(param2, map, sizeof(map));
				if (strcmp(map, VOTE_EXTEND, false) == 0)
				{
					Format(buffer, sizeof(buffer), "%T", "Extend Map", param1);
					return RedrawMenuItem(buffer);
				}
				else if (strcmp(map, VOTE_DONTCHANGE, false) == 0)
				{
					Format(buffer, sizeof(buffer), "%T", "Dont Change", param1);
					return RedrawMenuItem(buffer);					
				}
			}
		}		
	
		case MenuAction_VoteCancel:
		{
			// If we receive 0 votes, pick at random.
			if (param1 == VoteCancel_NoVotes && g_Cvar_NoVoteMode.BoolValue)
			{
				new count = menu.ItemCount;
				decl String:map[PLATFORM_MAX_PATH];
				menu.GetItem(0, map, sizeof(map));
				
				// Make sure the first map in the menu isn't one of the special items.
				// This would mean there are no real maps in the menu, because the special items are added after all maps. Don't do anything if that's the case.
				if (strcmp(map, VOTE_EXTEND, false) != 0 && strcmp(map, VOTE_DONTCHANGE, false) != 0)
				{
					// Get a random map from the list.
					new item = GetRandomInt(0, count - 1);
					menu.GetItem(item, map, sizeof(map));
					
					// Make sure it's not one of the special items.
					while (strcmp(map, VOTE_EXTEND, false) == 0 || strcmp(map, VOTE_DONTCHANGE, false) == 0)
					{
						item = GetRandomInt(0, count - 1);
						menu.GetItem(item, map, sizeof(map));
					}
					
					SetNextMap(map);
					g_MapVoteCompleted = true;
				}
			}
			else
			{
				// We were actually cancelled. I guess we do nothing.
			}
			
			g_HasVoteStarted = false;
		}
	}
	
	return 0;
}

public Action:Timer_ChangeMap(Handle:hTimer, Handle:dp)
{
	g_ChangeMapInProgress = false;
	
	new String:map[PLATFORM_MAX_PATH];
	
	if (dp == null)
	{
		if (!GetNextMap(map, sizeof(map)))
		{
			//No passed map and no set nextmap. fail!
			return Plugin_Stop;	
		}
	}
	else
	{
		ResetPack(dp);
		ReadPackString(dp, map, sizeof(map));		
	}
	
	ForceChangeLevel(map, "Map Vote");
	
	return Plugin_Stop;
}

bool:RemoveStringFromArray(Handle:array, String:str[])
{
	new index = FindStringInArray(array, str);
	if (index != -1)
	{
		RemoveFromArray(array, index);
		return true;
	}
	
	return false;
}

CreateNextVote()
{
	ClearArray(g_NextMapList);
	
	char map[PLATFORM_MAX_PATH];
	new Handle:tempMaps  = CloneArray(g_MapList);
	
	GetCurrentMap(map, sizeof(map));
	RemoveStringFromArray(tempMaps, map);
	
	if (g_Cvar_ExcludeMaps.IntValue && GetArraySize(tempMaps) > g_Cvar_ExcludeMaps.IntValue)
	{
		for (new i = 0; i < GetArraySize(g_OldMapList); i++)
		{
			GetArrayString(g_OldMapList, i, map, sizeof(map));
			RemoveStringFromArray(tempMaps, map);
		}	
	}

	int limit = (g_Cvar_IncludeMaps.IntValue < GetArraySize(tempMaps) ? g_Cvar_IncludeMaps.IntValue : GetArraySize(tempMaps));
	for (int i = 0; i < limit; i++)
	{
		int b = GetRandomInt(0, GetArraySize(tempMaps) - 1);
		GetArrayString(tempMaps, b, map, sizeof(map));		
		PushArrayString(g_NextMapList, map);
		RemoveFromArray(tempMaps, b);
	}
	
	delete tempMaps;
}

bool:CanVoteStart()
{
	if (g_WaitingForVote || g_HasVoteStarted)
	{
		return false;	
	}
	
	return true;
}

NominateResult:InternalNominateMap(String:map[], bool:force, owner)
{
	if (!IsMapValid(map))
	{
		return Nominate_InvalidMap;
	}
	
	/* Map already in the vote */
	if (FindStringInArray(g_NominateList, map) != -1)
	{
		return Nominate_AlreadyInVote;	
	}
	
	new index;

	/* Look to replace an existing nomination by this client - Nominations made with owner = 0 aren't replaced */
	if (owner && ((index = FindValueInArray(g_NominateOwners, owner)) != -1))
	{
		new String:oldmap[PLATFORM_MAX_PATH];
		GetArrayString(g_NominateList, index, oldmap, sizeof(oldmap));
		Call_StartForward(g_NominationsResetForward);
		Call_PushString(oldmap);
		Call_PushCell(owner);
		Call_Finish();
		
		SetArrayString(g_NominateList, index, map);
		return Nominate_Replaced;
	}
	
	/* Too many nominated maps. */
	if (GetArraySize(g_NominateList) >= g_Cvar_IncludeMaps.IntValue && !force)
	{
		return Nominate_VoteFull;
	}
	
	PushArrayString(g_NominateList, map);
	PushArrayCell(g_NominateOwners, owner);
	
	while (GetArraySize(g_NominateList) > g_Cvar_IncludeMaps.IntValue)
	{
		char oldmap[PLATFORM_MAX_PATH];
		GetArrayString(g_NominateList, 0, oldmap, sizeof(oldmap));
		Call_StartForward(g_NominationsResetForward);
		Call_PushString(oldmap);
		Call_PushCell(GetArrayCell(g_NominateOwners, 0));
		Call_Finish();
		
		RemoveFromArray(g_NominateList, 0);
		RemoveFromArray(g_NominateOwners, 0);
	}
	
	return Nominate_Added;
}

/* Add natives to allow nominate and initiate vote to be call */

/* native  bool:NominateMap(const String:map[], bool:force, &NominateError:error); */
public Native_NominateMap(Handle:plugin, numParams)
{
	new len;
	GetNativeStringLength(1, len);
	
	if (len <= 0)
	{
	  return false;
	}
	
	new String:map[len+1];
	GetNativeString(1, map, len+1);
	
	return _:InternalNominateMap(map, GetNativeCell(2), GetNativeCell(3));
}

bool:InternalRemoveNominationByMap(String:map[])
{	
	for (new i = 0; i < GetArraySize(g_NominateList); i++)
	{
		new String:oldmap[PLATFORM_MAX_PATH];
		GetArrayString(g_NominateList, i, oldmap, sizeof(oldmap));

		if(strcmp(map, oldmap, false) == 0)
		{
			Call_StartForward(g_NominationsResetForward);
			Call_PushString(oldmap);
			Call_PushCell(GetArrayCell(g_NominateOwners, i));
			Call_Finish();

			RemoveFromArray(g_NominateList, i);
			RemoveFromArray(g_NominateOwners, i);

			return true;
		}
	}
	
	return false;
}

/* native  bool:RemoveNominationByMap(const String:map[]); */
public Native_RemoveNominationByMap(Handle:plugin, numParams)
{
	new len;
	GetNativeStringLength(1, len);
	
	if (len <= 0)
	{
	  return false;
	}
	
	new String:map[len+1];
	GetNativeString(1, map, len+1);
	
	return _:InternalRemoveNominationByMap(map);
}

bool:InternalRemoveNominationByOwner(owner)
{	
	new index;

	if (owner && ((index = FindValueInArray(g_NominateOwners, owner)) != -1))
	{
		new String:oldmap[PLATFORM_MAX_PATH];
		GetArrayString(g_NominateList, index, oldmap, sizeof(oldmap));

		Call_StartForward(g_NominationsResetForward);
		Call_PushString(oldmap);
		Call_PushCell(owner);
		Call_Finish();

		RemoveFromArray(g_NominateList, index);
		RemoveFromArray(g_NominateOwners, index);

		return true;
	}
	
	return false;
}

/* native  bool:RemoveNominationByOwner(owner); */
public Native_RemoveNominationByOwner(Handle:plugin, numParams)
{	
	return _:InternalRemoveNominationByOwner(GetNativeCell(1));
}

/* native InitiateMapChooserVote(); */
public Native_InitiateVote(Handle:plugin, numParams)
{
	new MapChange:when = MapChange:GetNativeCell(1);
	new Handle:inputarray = Handle:GetNativeCell(2);
	
	LogAction(-1, -1, "Starting map vote because outside request");
	InitiateVote(when, inputarray);
}

public Native_CanVoteStart(Handle:plugin, numParams)
{
	return CanVoteStart();	
}

public Native_CheckVoteDone(Handle:plugin, numParams)
{
	return g_MapVoteCompleted;
}

public Native_EndOfMapVoteEnabled(Handle:plugin, numParams)
{
	return g_Cvar_EndOfMapVote.BoolValue;
}

public Native_GetExcludeMapList(Handle:plugin, numParams)
{
	new Handle:array = Handle:GetNativeCell(1);
	
	if (array == null)
	{
		return;	
	}
	new size = GetArraySize(g_OldMapList);
	decl String:map[PLATFORM_MAX_PATH];
	
	for (new i=0; i<size; i++)
	{
		GetArrayString(g_OldMapList, i, map, sizeof(map));
		PushArrayString(array, map);	
	}
	
	return;
}

public Native_GetNominatedMapList(Handle:plugin, numParams)
{
	new Handle:maparray = Handle:GetNativeCell(1);
	new Handle:ownerarray = Handle:GetNativeCell(2);
	
	if (maparray == null)
		return;

	decl String:map[PLATFORM_MAX_PATH];

	for (new i = 0; i < GetArraySize(g_NominateList); i++)
	{
		GetArrayString(g_NominateList, i, map, sizeof(map));
		PushArrayString(maparray, map);

		// If the optional parameter for an owner list was passed, then we need to fill that out as well
		if(ownerarray != null)
		{
			new index = GetArrayCell(g_NominateOwners, i);
			PushArrayCell(ownerarray, index);
		}
	}

	return;
}
