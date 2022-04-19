/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Admin Help Plugin
 * Displays and searches SourceMod commands and descriptions.
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

#pragma newdecls required

#define COMMANDS_PER_PAGE	10

public Plugin myinfo = 
{
	name = "Admin Help",
	author = "AlliedModders LLC",
	description = "Display command information",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

public void OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("adminhelp.phrases");
	RegConsoleCmd("sm_help", HelpCmd, "Displays SourceMod commands and descriptions");
	RegConsoleCmd("sm_searchcmd", HelpCmd, "Searches SourceMod commands");
}

public Action HelpCmd(int client, int args)
{
	if (client && !IsClientInGame(client))
	{
		return Plugin_Handled;
	}
	
	char arg[64], cmdName[20];
	int pageNum = 1;
	bool doSearch;

	GetCmdArg(0, cmdName, sizeof(cmdName));

	if (args >= 1)
	{
		GetCmdArg(1, arg, sizeof(arg));
		StringToIntEx(arg, pageNum);
		pageNum = (pageNum <= 0) ? 1 : pageNum;
	}

	doSearch = (strcmp("sm_help", cmdName) == 0) ? false : true;

	if (GetCmdReplySource() == SM_REPLY_TO_CHAT)
	{
		ReplyToCommand(client, "[SM] %t", "See console for output");
	}

	char name[64];
	char desc[255];
	char noDesc[128];
	CommandIterator cmdIter = new CommandIterator();

	FormatEx(noDesc, sizeof(noDesc), "%T", "No description available", client);

	if (doSearch)
	{
		int i = 1;
		while (cmdIter.Next())
		{
			cmdIter.GetName(name, sizeof(name));
			cmdIter.GetDescription(desc, sizeof(desc));

			if ((StrContains(name, arg, false) != -1) && CheckCommandAccess(client, name, cmdIter.Flags))
			{
				PrintToConsole(client, "[%03d] %s - %s", i++, name, (desc[0] == '\0') ? noDesc : desc);
			}
		}

		if (i == 1)
		{
			PrintToConsole(client, "%t", "No matching results found");
		}
	} else {
		PrintToConsole(client, "%t", "SM help commands");		

		/* Skip the first N commands if we need to */
		if (pageNum > 1)
		{
			int i;
			int endCmd = (pageNum-1) * COMMANDS_PER_PAGE - 1;
			for (i=0; cmdIter.Next() && i<endCmd; )
			{
				cmdIter.GetName(name, sizeof(name));

				if (CheckCommandAccess(client, name, cmdIter.Flags))
				{
					i++;
				}
			}

			if (i == 0)
			{
				PrintToConsole(client, "%t", "No commands available");
				delete cmdIter;
				return Plugin_Handled;
			}
		}

		/* Start printing the commands to the client */
		int i;
		int StartCmd = (pageNum-1) * COMMANDS_PER_PAGE;
		for (i=0; cmdIter.Next() && i<COMMANDS_PER_PAGE; )
		{
			cmdIter.GetName(name, sizeof(name));
			cmdIter.GetDescription(desc, sizeof(desc));
			
			if (CheckCommandAccess(client, name, cmdIter.Flags))
			{
				i++;
				PrintToConsole(client, "[%03d] %s - %s", i+StartCmd, name, (desc[0] == '\0') ? noDesc : desc);
			}
		}

		if (i == 0)
		{
			PrintToConsole(client, "%t", "No commands available");
		} else {
			PrintToConsole(client, "%t", "Entries n - m in page k", StartCmd+1, i+StartCmd, pageNum);
		}

		/* Test if there are more commands available */
		if (cmdIter.Next())
		{
			PrintToConsole(client, "%t", "Type sm_help to see more", pageNum+1);
		}
	}

	delete cmdIter;

	return Plugin_Handled;
}
