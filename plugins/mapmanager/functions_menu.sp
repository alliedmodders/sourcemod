/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Map Management Plugin
 * Contains misc functions.
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

DisplayWhenMenu(client, bool:vote = false)
{
	new Handle:
	
	if (!vote)
	{
		menu = CreateMenu(MenuHandler_ChangeMap);
	}
	else
	{
		menu = CreateMenu(MenuHandler_VoteWhen);
	}
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Map Change When", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddMenuItem(menu, "i", "Immediately");
	AddMenuItem(menu, "r", "Round End");
	AddMenuItem(menu, "e", "Map End");
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);	
}

DisplayConfirmVoteMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_Confirm);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Confirm Vote", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddMenuItem(menu, "Confirm", "Start the Vote");
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);	
}

DisplayAcceptVoteMenu(String:map[])
{
	new Handle:menu = CreateMenu(MenuHandler_Accept);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Accept Vote", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddMenuItem(menu, map, "Accept Vote");
	
	DisplayMenu(menu, g_VoteMapInUse, MENU_TIME_FOREVER);	
}

DisplayVoteMapMenu(client, mapCount, String:maps[5][])
{
	LogAction(client, -1, "\"%L\" initiated a map vote.", client);
	ShowActivity2(client, "[SM] ", "%t", "Initiated Vote Map");
	
	new Handle:menu = CreateMenu(Handler_VoteCallback, MenuAction:MENU_ACTIONS_ALL);
	
	if (mapCount == 1)
	{
		//strcopy(g_voteInfo[VOTE_NAME], sizeof(g_voteInfo[]), maps[0]);
			
		SetMenuTitle(menu, "Change Map To");
		AddMenuItem(menu, maps[0], "Yes");
		AddMenuItem(menu, VOTE_NO, "No");
	}
	else
	{
		//g_voteInfo[VOTE_NAME][0] = '\0';
		
		SetMenuTitle(menu, "Map Vote");
		for (new i = 0; i < mapCount; i++)
		{
			AddMenuItem(menu, maps[i], maps[i]);
		}	
	}
	
	SetMenuExitButton(menu, false);
	VoteMenuToAll(menu, 20);		
}
