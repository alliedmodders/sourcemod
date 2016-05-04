/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basefunvotes Plugin
 * Provides vote ff functionality
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

void DisplayVoteFFMenu(int client)
{
	if (IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote in Progress");
		return;
	}	
	
	if (!TestVoteDelay(client))
	{
		return;
	}
	
	LogAction(client, -1, "\"%L\" initiated a friendly fire vote.", client);
	ShowActivity2(client, "[SM] ", "%t", "Initiated Vote FF");
	
	g_voteType = ff;
	g_voteInfo[VOTE_NAME][0] = '\0';
	
	g_hVoteMenu = new Menu(Handler_VoteCallback, MENU_ACTIONS_ALL);
	
	if (g_Cvar_FF.BoolValue)
	{
		g_hVoteMenu.SetTitle("Voteff Off");
	}
	else
	{
		g_hVoteMenu.SetTitle("Voteff On");
	}
	
	g_hVoteMenu.AddItem(VOTE_YES, "Yes");
	g_hVoteMenu.AddItem(VOTE_NO, "No");
	g_hVoteMenu.ExitButton = false;
	g_hVoteMenu.DisplayVoteToAll(20);
}

public void AdminMenu_VoteFF(TopMenu topmenu, 
							  TopMenuAction action,
							  TopMenuObject object_id,
							  int param,
							  char[] buffer,
							  int maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Vote FF", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayVoteFFMenu(param);
	}
	else if (action == TopMenuAction_DrawOption)
	{	
		/* disable this option if a vote is already running */
		buffer[0] = !IsNewVoteAllowed() ? ITEMDRAW_IGNORE : ITEMDRAW_DEFAULT;
	}
}

public Action Command_VoteFF(int client, int args)
{
	if (args > 0)
	{
		ReplyToCommand(client, "[SM] Usage: sm_voteff");
		return Plugin_Handled;	
	}
	
	DisplayVoteFFMenu(client);
	
	return Plugin_Handled;
}
