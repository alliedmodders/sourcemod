/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basefunvotes Plugin
 * Provides votegravity functionality
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

void DisplayVoteGravityMenu(int client, int count, char[][] items)
{
	LogAction(client, -1, "\"%L\" initiated a gravity vote.", client);
	ShowActivity2(client, "[SM] ", "%t", "Initiated Vote Gravity");
	
	g_voteType = gravity;
	
	g_hVoteMenu = new Menu(Handler_VoteCallback, MENU_ACTIONS_ALL);
	
	if (count == 1)
	{
		strcopy(g_voteInfo[VOTE_NAME], sizeof(g_voteInfo[]), items[0]);
			
		g_hVoteMenu.SetTitle("Change Gravity To");
		g_hVoteMenu.AddItem(items[0], "Yes");
		g_hVoteMenu.AddItem(VOTE_NO, "No");
	}
	else
	{
		g_voteInfo[VOTE_NAME][0] = '\0';
		
		g_hVoteMenu.SetTitle("Gravity Vote");
		for (int i = 0; i < count; i++)
		{
			g_hVoteMenu.AddItem(items[i], items[i]);
		}	
	}
	
	g_hVoteMenu.ExitButton = false;
	g_hVoteMenu.DisplayVoteToAll(20);
}

public void AdminMenu_VoteGravity(TopMenu topmenu, 
							  TopMenuAction action,
							  TopMenuObject object_id,
							  int param,
							  char[] buffer,
							  int maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Gravity vote", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		/* Might need a better way of selecting the list of pre-defined gravity choices */
		char items[5][5] ={"200","400","800","1600","3200"};
		DisplayVoteGravityMenu(param,5, items);
	}
	else if (action == TopMenuAction_DrawOption)
	{	
		/* disable this option if a vote is already running */
		buffer[0] = !IsNewVoteAllowed() ? ITEMDRAW_IGNORE : ITEMDRAW_DEFAULT;
	}
}

public Action Command_VoteGravity(int client, int args)
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
	
	char text[256];
	GetCmdArgString(text, sizeof(text));

	char items[5][64];
	int count;	
	int len, pos;
	
	while (pos != -1 && count < 5)
	{	
		pos = BreakString(text[len], items[count], sizeof(items[]));
		
		float temp;
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
	
	DisplayVoteGravityMenu(client, count, items);
	
	return Plugin_Handled;	
}
