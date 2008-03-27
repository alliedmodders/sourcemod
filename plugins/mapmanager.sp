/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Map Management Plugin
 * Provides all map related functionality, including map changing, map voting,
 * and nextmap.
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

public Plugin:myinfo =
{
	name = "Map Manager",
	author = "AlliedModders LLC",
	description = "Map Management",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

#include "mapmanagement/globals.sp"
#include "mapmanagement/commands.sp"
#include "mapmanagement/events.sp"
#include "mapmanagement/functions.sp"
#include "mapmanagement/menus.sp"
#include "mapmanagement/timers.sp"
#include "mapmanagement/votes.sp"

public OnPluginStart()
{	
	LoadTranslations("mapmanager.phrases");

	// Prepare nextmap functionality.	
	g_VGUIMenu = GetUserMessageId("VGUIMenu");
	if (g_VGUIMenu == INVALID_MESSAGE_ID)
	{
		LogError("FATAL: Cannot find VGUIMenu user message id. MapManager crippled.");
		g_NextMapEnabled = false;
	}
	HookUserMessage(g_VGUIMenu, UserMsg_VGUIMenu);

	// Create all of the arrays, sized for a 64 character string.
	new arraySize = ByteCountToCells(64);
	g_MapCycle = CreateArray(arraySize);
	g_MapList = CreateArray(arraySize);
	g_MapHistory = CreateArray(arraySize);
	g_NextVoteMaps = CreateArray(arraySize);
	g_SelectedMaps = CreateArray(arraySize);
	g_NominatedMaps = CreateArray(arraySize);
	
	g_TeamScores = CreateArray(2);

	// Hook say
	RegConsoleCmd("say", Command_Say);
	RegConsoleCmd("say_team", Command_Say);	

	// Register all commands.
	RegAdminCmd("sm_map", Command_Map, ADMFLAG_CHANGEMAP, "sm_map <map> [r/e]");
	RegAdminCmd("sm_setnextmap", Command_SetNextmap, ADMFLAG_CHANGEMAP, "sm_setnextmap <map>");
	RegAdminCmd("sm_votemap", Command_Votemap, ADMFLAG_VOTE|ADMFLAG_CHANGEMAP, "sm_votemap <mapname> [mapname2] ... [mapname5] ");
	RegAdminCmd("sm_maplist", Command_List, ADMFLAG_GENERIC, "sm_maplist");
	RegAdminCmd("sm_nominate", Command_Addmap, ADMFLAG_CHANGEMAP, "sm_nominate <mapname> - Nominates a map for RockTheVote and MapChooser. Overrides existing nominations.");
	RegAdminCmd("sm_mapvote", Command_Mapvote, ADMFLAG_CHANGEMAP, "sm_mapvote - Forces the MapChooser vote to occur.");
	
	if (GetCommandFlags("nextmap") == INVALID_FCVAR_FLAGS)
	{
		RegServerCmd("nextmap", Command_Nextmap);
	}	
	
	// Register all convars
	g_Cvar_Nextmap = CreateConVar("sm_nextmap", "", "Sets the Next Map", FCVAR_NOTIFY);
	
	g_Cvar_MapCount = CreateConVar("sm_mm_maps", "4", "Number of maps to be voted on at end of map or RTV. 2 to 6. (Def 4)", 0, true, 2.0, true, 6.0);	
	g_Cvar_Excludes = CreateConVar("sm_mm_exclude", "5", "Specifies how many past maps to exclude from end of map vote and RTV.", _, true, 0.0);
	
	g_Cvar_MapChooser = CreateConVar("sm_mm_mapchooser", "0", "Enables MapChooser (End of Map voting)", 0, true, 0.0, true, 1.0);
	g_Cvar_RockTheVote = CreateConVar("sm_mm_rockthevote", "0", "Enables RockTheVote (Player initiated map votes)", 0, true, 0.0, true, 1.0);
	g_Cvar_Randomize = CreateConVar("sm_mm_randomize", "0", "Enabled Randomizer (Randomly picks the next map)", 0, true, 0.0, true, 1.0);
	g_Cvar_Nominate = CreateConVar("sm_mm_nominate", "1", "Enables nomination system.", 0, true, 0.0, true, 1.0);
	
	g_Cvar_VoteMap = CreateConVar("sm_mm_votemap", "0.60", "Percentage required for successful sm_votemap.", 0, true, 0.05, true, 1.0);
	g_Cvar_RTVLimit = CreateConVar("sm_mm_rtvlimit", "0.60", "Required percentage of players needed to rockthevote", 0, true, 0.05, true, 1.0);
	g_Cvar_MinPlayers = CreateConVar("sm_mm_minplayers", "0", "Number of players required before RTV will be enabled.", 0, true, 0.0, true, float(MAXPLAYERS));
	
	g_Cvar_StartTime = CreateConVar("sm_mapvote_start", "3.0", "Specifies when to start the vote based on time remaining.", _, true, 1.0);
	g_Cvar_StartRounds = CreateConVar("sm_mapvote_startround", "2.0", "Specifies when to start the vote based on rounds remaining.", _, true, 1.0);
	g_Cvar_StartFrags = CreateConVar("sm_mapvote_startfrags", "5.0", "Specifies when to start the vote base on frags remaining.", _, true, 1.0);
	g_Cvar_ExtendTimeMax = CreateConVar("sm_extendmap_maxtime", "90", "Specifies the maximum amount of time a map can be extended", _, true, 0.0);
	g_Cvar_ExtendTimeStep = CreateConVar("sm_extendmap_timestep", "15", "Specifies how much many more minutes each extension makes", _, true, 5.0);
	g_Cvar_ExtendRoundMax = CreateConVar("sm_extendmap_maxrounds", "30", "Specfies the maximum amount of rounds a map can be extended", _, true, 0.0);
	g_Cvar_ExtendRoundStep = CreateConVar("sm_extendmap_roundstep", "5", "Specifies how many more rounds each extension makes", _, true, 5.0);
	g_Cvar_ExtendFragMax = CreateConVar("sm_extendmap_maxfrags", "150", "Specfies the maximum frags allowed that a map can be extended.", _, true, 0.0);
	g_Cvar_ExtendFragStep = CreateConVar("sm_extendmap_fragstep", "10", "Specifies how many more frags are allowed when map is extended.", _, true, 5.0);	
	g_Cvar_Mapfile = CreateConVar("sm_mapvote_file", "configs/maps.ini", "Map file to use. (Def sourcemod/configs/maps.ini)");
	g_Cvar_ExcludeMaps = CreateConVar("sm_mapvote_exclude", "5", "Specifies how many past maps to exclude from the vote.", _, true, 0.0);
	g_Cvar_IncludeMaps = CreateConVar("sm_mapvote_include", "5", "Specifies how many maps to include in the vote.", _, true, 2.0, true, 6.0);
	g_Cvar_NoVoteMode = CreateConVar("sm_mapvote_novote", "1", "Specifies whether or not MapChooser should pick a map if no votes are received.", _, true, 0.0, true, 1.0);
	g_Cvar_Extend = CreateConVar("sm_mapvote_extend", "1", "Specifies whether or not MapChooser will allow the map to be extended.", _, true, 0.0, true, 1.0);
	
	// Find game convars
	g_Cvar_Chattime = FindConVar("mp_chattime");
	g_Cvar_Winlimit = FindConVar("mp_winlimit");
	g_Cvar_Maxrounds = FindConVar("mp_maxrounds");
	g_Cvar_Fraglimit = FindConVar("mp_fraglimit");	

	if (GetCommandFlags("nextmap") == INVALID_FCVAR_FLAGS)
	{
		RegServerCmd("nextmap", Command_Nextmap);
	}
	
	// Hook events
	HookEvent("round_end", Event_RoundEnd);	// We always require round_end
	if (g_Cvar_Fraglimit != INVALID_HANDLE)
	{
		HookEvent("player_death", Event_PlayerDeath);		
	}	
	
	// Set to the current map so OnMapStart() will know what to do
	decl String:currentMap[64];
	GetCurrentMap(currentMap, 64);
	SetNextmap(currentMap);	

	// Create necessary menus for TopMenu
	g_Menu_Map = CreateMenu(MenuHandler_Map);
	SetMenuTitle(g_Menu_Map, "Please select a map");
	SetMenuExitBackButton(g_Menu_Map, true);
	
	g_Menu_Votemap = CreateMenu(MenuHandler_VoteMap, MenuAction_DrawItem);
	SetMenuTitle(g_Menu_Votemap, "Please select a map");
	SetMenuExitBackButton(g_Menu_Votemap, true);	
	
	// Bind TopMenu commands to adminmenu_maplist.ini, in cases it doesn't exist in maplists.cfg
	decl String:mapListPath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, mapListPath, sizeof(mapListPath), "configs/adminmenu_maplist.ini");
	SetMapListCompatBind("sm_map menu", mapListPath);
	SetMapListCompatBind("sm_votemap menu", mapListPath);

	// Account for late loading
	new Handle:topmenu;
	if (LibraryExists("adminmenu") && ((topmenu = GetAdminTopMenu()) != INVALID_HANDLE))
	{
		OnAdminMenuReady(topmenu);
	}	
	
	AutoExecConfig(true, "mapmanager");
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
	
	new TopMenuObject:server_commands = FindTopMenuCategory(hTopMenu, ADMINMENU_SERVERCOMMANDS);

	if (server_commands != INVALID_TOPMENUOBJECT)
	{
		AddToTopMenu(hTopMenu,
			"sm_map",
			TopMenuObject_Item,
			AdminMenu_Map,
			server_commands,
			"sm_map",
			ADMFLAG_CHANGEMAP);
	}

	new TopMenuObject:voting_commands = FindTopMenuCategory(hTopMenu, ADMINMENU_VOTINGCOMMANDS);

	if (voting_commands != INVALID_TOPMENUOBJECT)
	{
		AddToTopMenu(hTopMenu,
			"sm_votemap",
			TopMenuObject_Item,
			AdminMenu_VoteMap,
			voting_commands,
			"sm_votemap",
			ADMFLAG_VOTE|ADMFLAG_CHANGEMAP);
	}
}

public OnLibraryRemoved(const String:name[])
{
	if (strcmp(name, "adminmenu") == 0)
	{
		hTopMenu = INVALID_HANDLE;
	}
}

public OnConfigsExecuted()
{
	// Add map logic here
	
	// Get the current and last maps.
	decl String:lastMap[64], String:currentMap[64];
	GetConVarString(g_Cvar_Nextmap, lastMap, 64);
	GetCurrentMap(currentMap, 64);
	
	// Why am I doing this? If we switched to a new map, but it wasn't what we expected (Due to sm_map, sm_votemap, or
	// some other plugin/command), we don't want to scramble the map cycle. Or for example, admin switches to a custom map
	// not in mapcyclefile. So we keep it set to the last expected nextmap. - ferret
	if (strcmp(lastMap, currentMap) == 0)
	{
		FindAndSetNextMap();
	}
	
	// Build map menus for sm_map, sm_votemap, and RTV.
	BuildMapMenu(g_Menu_Map, list);
	BuildMapMenu(g_Menu_VoteMap, list);
	
	// If the Randomize option is on, randomize!
	if (GetConVarBool(g_Cvar_Randomize))
	{
		CreateTimer(5.0, Timer_RandomizeNextmap);
	}
	
	// If MapChooser is active, start it up!
	if (GetConVarBool(g_Cvar_MapChooser))
	{
		SetupTimeleftTimer();
		SetConVarString(g_Cvar_Nextmap, "Pending Vote");
	}
	
	// If RockTheVote is active, start it up!
	if (GetConVarBool(g_Cvar_RockTheVote))
	{
		BuildMapMenu(g_Menu_RTV, list);
		CreateTimer(30.0, Timer_DelayRTV);
	}	
}

// Reinitialize all our various globals
public OnMapStart()
{
	if (g_Nominate != INVALID_HANDLE)
	{
		ClearArray(g_Nominate);
	}
	
	if (g_TeamScores != INVALID_HANDLE)
	{
		ClearArray(g_TeamScores);
	}

	g_TotalRounds = 0;	
	
	g_RTV_Voters = 0;
	g_RTV_Votes = 0;
	g_RTV_VotesNeeded = 0;
	g_RTV_Started = false;
	g_RTV_Ended = false;
}

// Reset globals as necessary and kill timers
public OnMapEnd()
{
	g_IntermissionCalled = false;
	g_HasVoteStarted = false;
		
	g_RTV_Allowed = false;	
	
	if (g_VoteTimer != INVALID_HANDLE)
	{
		KillTimer(g_VoteTimer);
		g_VoteTimer = INVALID_HANDLE;
	}
	
	if (g_RetryTimer != INVALID_HANDLE)
	{
		KillTimer(g_RetryTimer);
		g_RetryTimer = INVALID_HANDLE;
	}
}

public bool:OnClientConnect(client, String:rejectmsg[], maxlen)
{
	if (IsFakeClient(client))
	{
		return true;
	}
	
	// If RTV is active, deal with vote counting.
	if (GetConVarBool(g_Cvar_RockTheVote))
	{
		g_RTV_Voted[client] = false;
		g_RTV_Voters++;
		g_RTV_VotesNeeded = RoundToFloor(float(g_Voters) * GetConVarFloat(g_Cvar_Needed));
	}

	// If Nominate is active, let the new client nominate
	if (GetConVarBool(g_Cvar_Nominate))
	{
		g_Nominated[client] = false;
	}
	
	return true;
}

public OnClientDisconnect(client)
{
	if (IsFakeClient(client))
	{
		return;
	}
	
	// If RTV is active, deal with vote counting.
	if (GetConVarBool(g_Cvar_RockTheVote))
	{
		if(g_RTV_Voted[client])
		{
			g_RTV_Votes--;
		}

		g_RTV_Voters--;
		g_RTV_VotesNeeded = RoundToFloor(float(g_RTV_Voters) * GetConVarFloat(g_Cvar_RTVLimit));
		
		// If this client caused us to fall below the RTV threshold and its allowed be started, start it.	
		if (g_RTV_Votes && g_RTV_Voters && g_RTV_Votes >= g_RTV_VotesNeeded && g_RTV_Allowed && !g_RTV_Started) 
		{
			g_RTV_Started = true;
			CreateTimer(2.0, Timer_StartRTV, TIMER_FLAG_NO_MAPCHANGE);
		}	
	}
}

public OnMapTimeLeftChanged()
{
	if (GetConVarBool(g_Cvar_MapChooser))
	{
		SetupTimeleftTimer();
	}
}