/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Map Manager Plugin
 * Contains globals and defines
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

#define VOTE_EXTEND "##extend##"
#define VOTE_YES "###yes###"
#define VOTE_NO "###no###"

new Handle:hTopMenu = INVALID_HANDLE;			// TopMenu

new Handle:g_MapCycle = INVALID_HANDLE;			// mapcyclefile maps
new Handle:g_MapList = INVALID_HANDLE;			// maplist.txt maps
new Handle:g_MapHistory = INVALID_HANDLE;		// History of maps played
new Handle:g_NextVoteMaps = INVALID_HANDLE;		// Array of maps for next RTV or MC vote
new Handle:g_NominatedMaps = INVALID_HANDLE;	// Array of maps that have been nominated

new Handle:g_Menu_Map = INVALID_HANDLE;			// Menu of maps used by sm_map in admin menu
new Handle:g_Menu_Votemap = INVALID_HANDLE;		// Menu of maps used by sm_votemap in admin menu
new Handle:g_Menu_Nominate = INVALID_HANDLE;		// Menu of maps used by Nomination system

new Handle:g_SelectedMaps;						// List of maps chosen so far by a user in sm_votemap admin menu
new g_VoteMapInUse;								// Client index of admin using sm_votemap
new String:g_Client_Data[MAXCLIENTS+1][64]; 	// Used to hold bits of client data during sm_votemap

new bool:g_MapChangeSet;						// True if a command or vote has set the map
new bool:g_MapChanged;							// True if a map change has been issued
new g_MapChangeWhen;							// Either 'i' for immediate, 'r' for round end, or 'e' for end of map.

new UserMsg:g_VGUIMenu;							// VGUI usermsg id for nextmap
new bool:g_NextMapEnabled = true				// If set to false, all features requiring nextmap are disabled.
new bool:g_IntermissionCalled;					// Has the end of map intermission begun?
new String:g_NextMap[64];						// All important! This is the next map!
new g_MapPos = -1;								// Position in mapcycle

new bool:g_RTV_Voted[MAXPLAYERS+1] = {false, ...};	// Whether or not a player has voted for RTV
new bool:g_RTV_Allowed = false;					// True if RTV is available to players. Used to delay rtv votes.
new bool:g_RTV_Started = false;					// Indicates that the actual map vote has started
new bool:g_RTV_Ended = false;					// Indicates that the actual map vote has concluded
new g_RTV_Voters = 0;							// Total voters connected. Doesn't include fake clients.
new g_RTV_Votes = 0;							// Total number of "say rtv" votes
new g_RTV_VotesNeeded = 0;						// Necessary votes before map vote begins. (voters * percent_needed)

new bool:g_Nominated[MAXPLAYERS+1] = {false, ...};	// Whether or not a player has nominated a map

new Handle:g_TeamScores = INVALID_HANDLE;		// Array of team scores
new g_TotalRounds;								// Total rounds played this map
new bool:g_HasVoteStarted;						// Whether or not MapChooser has begun

// ConVar handles
new Handle:g_Cvar_NextMap = INVALID_HANDLE;
new Handle:g_Cvar_VoteMap = INVALID_HANDLE;
new Handle:g_Cvar_Excludes = INVALID_HANDLE;
new Handle:g_Cvar_StartTime = INVALID_HANDLE;
new Handle:g_Cvar_StartRounds = INVALID_HANDLE;
new Handle:g_Cvar_StartFrags = INVALID_HANDLE;
new Handle:g_Cvar_ExtendTimeMax = INVALID_HANDLE;
new Handle:g_Cvar_ExtendTimeStep = INVALID_HANDLE;
new Handle:g_Cvar_ExtendRoundMax = INVALID_HANDLE;
new Handle:g_Cvar_ExtendRoundStep = INVALID_HANDLE;
new Handle:g_Cvar_ExtendFragMax = INVALID_HANDLE;
new Handle:g_Cvar_ExtendFragStep = INVALID_HANDLE;
new Handle:g_Cvar_Mapfile = INVALID_HANDLE;
new Handle:g_Cvar_ExcludeMaps = INVALID_HANDLE;
new Handle:g_Cvar_IncludeMaps = INVALID_HANDLE;
new Handle:g_Cvar_NoVoteMode = INVALID_HANDLE;
new Handle:g_Cvar_Extend = INVALID_HANDLE;

new Handle:g_VoteTimer = INVALID_HANDLE;
new Handle:g_RetryTimer = INVALID_HANDLE;

// VALVe ConVars
new Handle:g_Cvar_Chattime = INVALID_HANDLE;
new Handle:g_Cvar_Winlimit = INVALID_HANDLE;
new Handle:g_Cvar_Maxrounds = INVALID_HANDLE;
new Handle:g_Cvar_Fraglimit = INVALID_HANDLE;
