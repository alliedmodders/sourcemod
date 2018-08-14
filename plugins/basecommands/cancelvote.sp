/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basecommands Plugin
 * Provides cancelvote functionality.
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
 
void PerformCancelVote(int client)
{
	if (!IsVoteInProgress())
	{
		ReplyToCommand(client, "[SM] %t", "Vote Not In Progress");
		return;
	}

	ShowActivity2(client, "[SM] ", "%t", "Cancelled Vote");
	
	CancelVote();
}
	
public void AdminMenu_CancelVote(TopMenu topmenu, 
							  TopMenuAction action,
							  TopMenuObject object_id,
							  int param,
							  char[] buffer,
							  int maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Cancel vote", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		PerformCancelVote(param);
		RedisplayAdminMenu(topmenu, param);	
	}
	else if (action == TopMenuAction_DrawOption)
	{
		buffer[0] = IsVoteInProgress() ? ITEMDRAW_DEFAULT : ITEMDRAW_IGNORE;
	}
}

public Action Command_CancelVote(int client, int args)
{
	PerformCancelVote(client);

	return Plugin_Handled;
}
