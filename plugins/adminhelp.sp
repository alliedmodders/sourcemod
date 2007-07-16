/**
 * adminhelp.sp
 * Displays and searches SourceMod commands and descriptions.
 * This file is part of SourceMod, Copyright (C) 2004-2007 AlliedModders LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#pragma semicolon 1

#include <sourcemod>

#define COMMANDS_PER_PAGE	10

public Plugin:myinfo = 
{
	name = "Admin Help",
	author = "AlliedModders LLC",
	description = "Display command information",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

public OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("plugin.adminhelp");
	RegConsoleCmd("sm_help", HelpCmd, "Displays SourceMod commands and descriptions");
	RegConsoleCmd("sm_searchcmd", HelpCmd, "Searches SourceMod commands");
}

public Action:HelpCmd(client, args)
{
	decl String:arg[64], String:CmdName[20];
	new PageNum = 1;
	new bool:DoSearch;

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

	decl String:Name[64];
	decl String:Desc[255];
	decl String:NoDesc[128];
	new Flags;
	new Handle:CmdIter = GetCommandIterator();

	FormatEx(NoDesc, sizeof(NoDesc), "%t", "No description available");

	if (DoSearch)
	{
		new i = 1;
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
			new i;
			new EndCmd = (PageNum-1) * COMMANDS_PER_PAGE - 1;
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
				CloseHandle(CmdIter);
				return Plugin_Handled;
			}
		}

		/* Start printing the commands to the client */
		new i;
		new StartCmd = (PageNum-1) * COMMANDS_PER_PAGE;
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

	CloseHandle(CmdIter);

	return Plugin_Handled;
}
