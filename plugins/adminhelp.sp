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
	
	char arg[64], CmdName[20];
	int PageNum = 1;
	bool DoSearch;

	GetCmdArg(0, CmdName, sizeof(CmdName));

	if (GetCmdArgs() >= 1)
	{
		GetCmdArg(1, arg, sizeof(arg));
		StringToIntEx(arg, PageNum);
		PageNum = (PageNum <= 0) ? 1 : PageNum;
	}

	DoSearch = (strcmp("sm_help", CmdName) == 0) ? false : true;

	if (GetCmdReplySource() == SM_REPLY_TO_CHAT)
	{
		ReplyToCommand(client, "[SM] %t", "See console for output");
	}

	char Name[64];
	char Desc[255];
	char NoDesc[128];
	int Flags;
	Handle CmdIter = GetCommandIterator();

	FormatEx(NoDesc, sizeof(NoDesc), "%T", "No description available", client);

	if (DoSearch)
	{
		int i = 1;
		while (ReadCommandIterator(CmdIter, Name, sizeof(Name), Flags, Desc, sizeof(Desc)))
		{
			if ((StrContains(Name, arg, false) != -1) && CheckCommandAccess(client, Name, Flags))
			{
				PrintToConsole(client, "[%03d] %s - %s", i++, Name, (Desc[0] == '\0') ? NoDesc : Desc);
			}
		}

		if (i == 1)
		{
			PrintToConsole(client, "%t", "No matching results found");
		}
	} else {
		PrintToConsole(client, "%t", "SM help commands");		

		/* Skip the first N commands if we need to */
		if (PageNum > 1)
		{
			int i;
			int EndCmd = (PageNum-1) * COMMANDS_PER_PAGE - 1;
			for (i=0; ReadCommandIterator(CmdIter, Name, sizeof(Name), Flags, Desc, sizeof(Desc)) && i<EndCmd; )
			{
				if (CheckCommandAccess(client, Name, Flags))
				{
					i++;
				}
			}

			if (i == 0)
			{
				PrintToConsole(client, "%t", "No commands available");
				delete CmdIter;
				return Plugin_Handled;
			}
		}

		/* Start printing the commands to the client */
		int i;
		int StartCmd = (PageNum-1) * COMMANDS_PER_PAGE;
		for (i=0; ReadCommandIterator(CmdIter, Name, sizeof(Name), Flags, Desc, sizeof(Desc)) && i<COMMANDS_PER_PAGE; )
		{
			if (CheckCommandAccess(client, Name, Flags))
			{
				i++;
				PrintToConsole(client, "[%03d] %s - %s", i+StartCmd, Name, (Desc[0] == '\0') ? NoDesc : Desc);
			}
		}

		if (i == 0)
		{
			PrintToConsole(client, "%t", "No commands available");
		} else {
			PrintToConsole(client, "%t", "Entries n - m in page k", StartCmd+1, i+StartCmd, PageNum);
		}

		/* Test if there are more commands available */
		if (ReadCommandIterator(CmdIter, Name, sizeof(Name), Flags, Desc, sizeof(Desc)) && CheckCommandAccess(client, Name, Flags))
		{
			PrintToConsole(client, "%t", "Type sm_help to see more", PageNum+1);
		}
	}

	delete CmdIter;

	return Plugin_Handled;
}
