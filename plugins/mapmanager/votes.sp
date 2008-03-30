/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Map Manager Plugin
 * Contains vote callbacks
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

// sm_votemap
public Handler_VoteCallback(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_End)
	{
		CloseHandle(menu);
	}
	else if (action == MenuAction_Display)
	{
	 	decl String:title[64];
		GetMenuTitle(menu, title, sizeof(title));
			
		decl String:map[64];
		GetMenuItem(menu, 1, map, sizeof(map));
		
 		decl String:buffer[255];
		Format(buffer, sizeof(buffer), "%T", title, param1, map);

		new Handle:panel = Handle:param2;
		SetPanelTitle(panel, buffer);
	}
	else if (action == MenuAction_DisplayItem)
	{
		decl String:display[64];
		GetMenuItem(menu, param2, "", 0, _, display, sizeof(display));
	 
	 	if (strcmp(display, "No") == 0 || strcmp(display, "Yes") == 0)
	 	{
			decl String:buffer[255];
			Format(buffer, sizeof(buffer), "%T", display, param1);

			return RedrawMenuItem(buffer);
		}
	}
	else if (action == MenuAction_VoteCancel && param1 == VoteCancel_NoVotes)
	{
		PrintToChatAll("[SM] %t", "No Votes Cast");
	}	
	else if (action == MenuAction_VoteEnd)
	{
		decl String:item[64], String:display[64];
		new Float:percent, Float:limit, votes, totalVotes;

		GetMenuVoteInfo(param2, votes, totalVotes);
		GetMenuItem(menu, param1, item, sizeof(item), _, display, sizeof(display));
		
		if (strcmp(item, VOTE_NO) == 0 && param1 == 1)
		{
			votes = totalVotes - votes; // Reverse the votes to be in relation to the Yes option.
		}
		
		percent = GetVotePercent(votes, totalVotes);
		
		limit = GetConVarFloat(g_Cvar_VoteMap);
		
		// A multi-argument vote is "always successful", but have to check if its a Yes/No vote.
		if ((strcmp(item, VOTE_YES) == 0 && FloatCompare(percent,limit) < 0 && param1 == 0) || (strcmp(item, VOTE_NO) == 0 && param1 == 1))
		{
			LogAction(-1, -1, "Vote failed.");
			PrintToChatAll("[SM] %t", "Votemap Failed", RoundToNearest(100.0*limit), RoundToNearest(100.0*percent), totalVotes);
		}
		else
		{
			PrintToChatAll("[SM] %t", "Votemap Successful", RoundToNearest(100.0*percent), totalVotes);

			if (g_VoteMapInUse && IsClientInGame(g_VoteMapInUse))
			{
				DisplayAcceptVoteMenu(item);
			}
			else
			{
				LogAction(-1, -1, "Changing map to %s due to vote.", item);
				PrintToChatAll("[SM] %t", "Changing map", item);
				SetMapChange(0, item, g_Client_Data[g_VoteMapInUse][0])
			}	
		}
	}
	
	return 0;
}

public Handler_MapMapVoteMenu(Handle:menu, MenuAction:action, param1, param2)
{
	switch (action)
	{
		case MenuAction_End:
		{
			CloseHandle(menu);
		}
		
		case MenuAction_Display:
		{
	 		decl String:oldTitle[255], String:buffer[255];
			GetMenuTitle(menu, oldTitle, sizeof(oldTitle));
			Format(buffer, sizeof(buffer), "%T", oldTitle, param1);

			new Handle:panel = Handle:param2;
			SetPanelTitle(panel, buffer);
		}
		
		case MenuAction_DisplayItem:
		{
			if (GetMenuItemCount(menu) - 1 == param2)
			{
				decl String:buffer[255];
				Format(buffer, sizeof(buffer), "%T", "Don't Change", param1);
				return RedrawMenuItem(buffer);
			}
		}		

		// Why am I commented out? Because BAIL hasn't decided yet if
		// vote notification will be built into the Vote API.
		/*case MenuAction_Select:
		{
			decl String:Name[32], String:Map[32];
			GetClientName(param1, Name, sizeof(Name));
			GetMenuItem(menu, param2, Map, sizeof(Map));

			PrintToChatAll("[SM] %s has voted for map '%s'", Name, Map);
		}*/
		
		case MenuAction_VoteCancel:
		{
			if (param1 == VoteCancel_NoVotes)
			{
				PrintToChatAll("[SM] %t", "No Votes");
				g_RTVEnded = true;
			}
		}

		case MenuAction_VoteEnd:
		{
			new String:map[64];
			
			GetMenuItem(menu, param1, map, sizeof(map));
			
			if (GetMenuItemCount(menu) - 1 == param1) // This should always match the "Keep Current" option
			{
				PrintToChatAll("[SM] %t", "Current Map Stays");
				LogMessage("[SM] Rockthevote has ended, current map kept.");
			}
			else
			{
				PrintToChatAll("[SM] %t", "Changing Maps", map);
				LogMessage("[SM] Rockthevote has ended, changing to map %s.", map);
				new Handle:dp;
				CreateDataTimer(5.0, Timer_ChangeMap, dp);
				WritePackString(dp, map);
			}
			
			g_RTVEnded = true;
		}
	}
	
	return 0;
}

public Handler_MapVoteMenu(Handle:menu, MenuAction:action, param1, param2)
{
	switch (action)
	{
		case MenuAction_End:
		{
			g_VoteMenu = INVALID_HANDLE;
			CloseHandle(menu);
		}
		
		case MenuAction_Display:
		{
	 		decl String:buffer[255];
			Format(buffer, sizeof(buffer), "%T", "Vote Nextmap", param1);

			new Handle:panel = Handle:param2;
			SetPanelTitle(panel, buffer);
		}		
		
		case MenuAction_DisplayItem:
		{
			if (GetMenuItemCount(menu) - 1 == param2)
			{
				decl String:map[64], String:buffer[255];
				GetMenuItem(menu, param2, map, sizeof(map));
				if (strcmp(map, VOTE_EXTEND, false) == 0)
				{
					Format(buffer, sizeof(buffer), "%T", "Extend Map", param1);
					return RedrawMenuItem(buffer);
				}
			}
		}		

		// Why am I commented out? Because BAIL hasn't decided yet if
		// vote notification will be built into the Vote API.
		/*case MenuAction_Select:
		{
			decl String:Name[32], String:Map[32];
			GetClientName(param1, Name, sizeof(Name));
			GetMenuItem(menu, param2, Map, sizeof(Map));

			PrintToChatAll("[SM] %s has voted for map '%s'", Name, Map);
		}*/
		
		case MenuAction_VoteCancel:
		{
			// If we receive 0 votes, pick at random.
			if (param1 == VoteCancel_NoVotes && GetConVarBool(g_Cvar_NoVoteMode))
			{
				new count = GetMenuItemCount(menu);
				new item = GetRandomInt(0, count - 1);
				decl String:map[32];
				GetMenuItem(menu, item, map, sizeof(map));
				
				while (strcmp(map, VOTE_EXTEND, false) == 0)
				{
					item = GetRandomInt(0, count - 1);
					GetMenuItem(menu, item, map, sizeof(map));
				}
				
				SetNextMap(map);			
			}
			else
			{
				// We were actually cancelled. What should we do?
			}
		}

		case MenuAction_VoteEnd:
		{
			decl String:map[32];
			GetMenuItem(menu, param1, map, sizeof(map));

			if (strcmp(map, VOTE_EXTEND, false) == 0)
			{
				new time;
				if (GetMapTimeLimit(time))
				{
					if (time > 0 && time < GetConVarInt(g_Cvar_ExtendTimeMax))
					{
						ExtendMapTimeLimit(GetConVarInt(g_Cvar_ExtendTimeStep)*60);						
					}
				}
				
				if (g_Cvar_Winlimit != INVALID_HANDLE)
				{
					new winlimit = GetConVarInt(g_Cvar_Winlimit);
					if (winlimit && winlimit < GetConVarInt(g_Cvar_ExtendRoundMax))
					{
						SetConVarInt(g_Cvar_Winlimit, winlimit + GetConVarInt(g_Cvar_ExtendRoundStep));
					}					
				}
				
				if (g_Cvar_Maxrounds != INVALID_HANDLE)
				{
					new maxrounds = GetConVarInt(g_Cvar_Maxrounds);
					if (maxrounds && maxrounds < GetConVarInt(g_Cvar_ExtendRoundMax))
					{
						SetConVarInt(g_Cvar_Maxrounds, maxrounds + GetConVarInt(g_Cvar_ExtendRoundStep));
					}
				}
				
				if (g_Cvar_Fraglimit != INVALID_HANDLE)
				{
					new fraglimit = GetConVarInt(g_Cvar_Fraglimit);
					if (fraglimit && fraglimit < GetConVarInt(g_Cvar_ExtendFragMax))
					{
						SetConVarInt(g_Cvar_Fraglimit, fraglimit + GetConVarInt(g_Cvar_ExtendFragStep));						
					}
				}

				PrintToChatAll("[SM] %t", "Current Map Extended");
				LogMessage("Voting for next map has finished. The current map has been extended.");
				
				// We extended, so we'll have to vote again.
				g_HasVoteStarted = false;
				CreateNextVote();
				SetupTimeleftTimer();
			}
			else
			{
				SetNextMap(map);
			}
		}
	}
	
	return 0;
}