/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basic Chat Plugin
 * Implements basic communication commands.
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

public Plugin myinfo = 
{
	name = "Basic Chat",
	author = "AlliedModders LLC",
	description = "Basic Communication Commands",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

#define CHAT_SYMBOL '@'

char g_ColorNames[13][10] = {"White", "Red", "Green", "Blue", "Yellow", "Purple", "Cyan", "Orange", "Pink", "Olive", "Lime", "Violet", "Lightblue"};
int g_Colors[13][3] = {{255,255,255},{255,0,0},{0,255,0},{0,0,255},{255,255,0},{255,0,255},{0,255,255},{255,128,0},{255,0,128},{128,255,0},{0,255,128},{128,0,255},{0,128,255}};

ConVar g_Cvar_Chatmode;

EngineVersion g_GameEngine = Engine_Unknown;

public void OnPluginStart()
{
	LoadTranslations("common.phrases");
	
	g_GameEngine = GetEngineVersion();

	g_Cvar_Chatmode = CreateConVar("sm_chat_mode", "1", "Allows player's to send messages to admin chat.", 0, true, 0.0, true, 1.0);

	RegAdminCmd("sm_say", Command_SmSay, ADMFLAG_CHAT, "sm_say <message> - sends message to all players");
	RegAdminCmd("sm_csay", Command_SmCsay, ADMFLAG_CHAT, "sm_csay <message> - sends centered message to all players");
	
	/* HintText does not work on Dark Messiah */
	if (g_GameEngine != Engine_DarkMessiah)
	{
		RegAdminCmd("sm_hsay", Command_SmHsay, ADMFLAG_CHAT, "sm_hsay <message> - sends hint message to all players");	
	}
	
	RegAdminCmd("sm_tsay", Command_SmTsay, ADMFLAG_CHAT, "sm_tsay [color] <message> - sends top-left message to all players");
	RegAdminCmd("sm_chat", Command_SmChat, ADMFLAG_CHAT, "sm_chat <message> - sends message to admins");
	RegAdminCmd("sm_psay", Command_SmPsay, ADMFLAG_CHAT, "sm_psay <name or #userid> <message> - sends private message");
	RegAdminCmd("sm_msay", Command_SmMsay, ADMFLAG_CHAT, "sm_msay <message> - sends message as a menu panel");
}

public Action OnClientSayCommand(int client, const char[] command, const char[] sArgs)
{
	int startidx;
	if (sArgs[startidx] != CHAT_SYMBOL)
		return Plugin_Continue;
	
	startidx++;
	
	if (strcmp(command, "say", false) == 0)
	{
		if (sArgs[startidx] != CHAT_SYMBOL) // sm_say alias
		{
			if (!CheckCommandAccess(client, "sm_say", ADMFLAG_CHAT))
			{
				return Plugin_Continue;
			}
			
			SendChatToAll(client, sArgs[startidx]);
			LogAction(client, -1, "\"%L\" triggered sm_say (text %s)", client, sArgs[startidx]);
			
			return Plugin_Stop;
		}
		
		startidx++;

		if (sArgs[startidx] != CHAT_SYMBOL) // sm_psay alias
		{
			if (!CheckCommandAccess(client, "sm_psay", ADMFLAG_CHAT))
			{
				return Plugin_Continue;
			}
			
			char arg[64];
			
			int len = BreakString(sArgs[startidx], arg, sizeof(arg));
			int target = FindTarget(client, arg, true, false);
			
			if (target == -1 || len == -1)
				return Plugin_Stop;
			
			SendPrivateChat(client, target, sArgs[startidx+len]);
			
			return Plugin_Stop;
		}
		
		startidx++;
		
		// sm_csay alias
		if (!CheckCommandAccess(client, "sm_csay", ADMFLAG_CHAT))
		{
			return Plugin_Continue;
		}
		
		DisplayCenterTextToAll(client, sArgs[startidx]);
		LogAction(client, -1, "\"%L\" triggered sm_csay (text %s)", client, sArgs[startidx]);
		
		return Plugin_Stop;
	}
	else if (strcmp(command, "say_team", false) == 0 || strcmp(command, "say_squad", false) == 0)
	{
		if (!CheckCommandAccess(client, "sm_chat", ADMFLAG_CHAT) && !g_Cvar_Chatmode.BoolValue)
		{
			return Plugin_Continue;
		}
		
		SendChatToAdmins(client, sArgs[startidx]);
		LogAction(client, -1, "\"%L\" triggered sm_chat (text %s)", client, sArgs[startidx]);
		
		return Plugin_Stop;
	}
	
	return Plugin_Continue;
}

public Action Command_SmSay(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_say <message>");
		return Plugin_Handled;	
	}
	
	char text[192];
	GetCmdArgString(text, sizeof(text));

	SendChatToAll(client, text);
	LogAction(client, -1, "\"%L\" triggered sm_say (text %s)", client, text);
	
	return Plugin_Handled;		
}

public Action Command_SmCsay(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_csay <message>");
		return Plugin_Handled;	
	}
	
	char text[192];
	GetCmdArgString(text, sizeof(text));
	
	DisplayCenterTextToAll(client, text);
	
	LogAction(client, -1, "\"%L\" triggered sm_csay (text %s)", client, text);
	
	return Plugin_Handled;		
}

public Action Command_SmHsay(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_hsay <message>");
		return Plugin_Handled;  
	}
	
	char text[192];
	GetCmdArgString(text, sizeof(text));
 
	char nameBuf[MAX_NAME_LENGTH];
	
	for (int i = 1; i <= MaxClients; i++)
	{
		if (!IsClientInGame(i) || IsFakeClient(i))
		{
			continue;
		}
		FormatActivitySource(client, i, nameBuf, sizeof(nameBuf));
		PrintHintText(i, "%s: %s", nameBuf, text);
	}
	
	LogAction(client, -1, "\"%L\" triggered sm_hsay (text %s)", client, text);
	
	return Plugin_Handled;	
}

public Action Command_SmTsay(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_tsay <message>");
		return Plugin_Handled;  
	}
	
	char text[192], colorStr[16];
	GetCmdArgString(text, sizeof(text));
	
	int len = BreakString(text, colorStr, 16);
		
	int color = FindColor(colorStr);
	char nameBuf[MAX_NAME_LENGTH];
	
	if (color == -1)
	{
		color = 0;
		len = 0;
	}
	
	for (int i = 1; i <= MaxClients; i++)
	{
		if (!IsClientInGame(i) || IsFakeClient(i))
		{
			continue;
		}
		FormatActivitySource(client, i, nameBuf, sizeof(nameBuf));
		SendDialogToOne(i, color, "%s: %s", nameBuf, text[len]);
	}

	LogAction(client, -1, "\"%L\" triggered sm_tsay (text %s)", client, text);
	
	return Plugin_Handled;	
}

public Action Command_SmChat(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_chat <message>");
		return Plugin_Handled;	
	}	
	
	char text[192];
	GetCmdArgString(text, sizeof(text));

	SendChatToAdmins(client, text);
	LogAction(client, -1, "\"%L\" triggered sm_chat (text %s)", client, text);
	
	return Plugin_Handled;	
}

public Action Command_SmPsay(int client, int args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_psay <name or #userid> <message>");
		return Plugin_Handled;	
	}	
	
	char text[192], arg[64];
	GetCmdArgString(text, sizeof(text));

	int len = BreakString(text, arg, sizeof(arg));
	
	int target = FindTarget(client, arg, true, false);
		
	if (target == -1)
		return Plugin_Handled;	
	
	SendPrivateChat(client, target, text[len]);
	
	return Plugin_Handled;	
}

public Action Command_SmMsay(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_msay <message>");
		return Plugin_Handled;	
	}
	
	char text[192];
	GetCmdArgString(text, sizeof(text));

	SendPanelToAll(client, text);

	LogAction(client, -1, "\"%L\" triggered sm_msay (text %s)", client, text);
	
	return Plugin_Handled;		
}

int FindColor(const char[] color)
{
	for (int i = 0; i < sizeof(g_ColorNames); i++)
	{
		if (strcmp(color, g_ColorNames[i], false) == 0)
			return i;
	}
	
	return -1;
}

void SendChatToAll(int client, const char[] message)
{
	char nameBuf[MAX_NAME_LENGTH];
	
	for (int i = 1; i <= MaxClients; i++)
	{
		if (!IsClientInGame(i) || IsFakeClient(i))
		{
			continue;
		}
		FormatActivitySource(client, i, nameBuf, sizeof(nameBuf));
		
		if (g_GameEngine == Engine_CSGO)
			PrintToChat(i, " \x01\x0B\x04%t: \x01%s", "Say all", nameBuf, message);
		else
			PrintToChat(i, "\x04%t: \x01%s", "Say all", nameBuf, message);
	}
}

void DisplayCenterTextToAll(int client, const char[] message)
{
	char nameBuf[MAX_NAME_LENGTH];
	
	for (int i = 1; i <= MaxClients; i++)
	{
		if (!IsClientInGame(i) || IsFakeClient(i))
		{
			continue;
		}
		FormatActivitySource(client, i, nameBuf, sizeof(nameBuf));
		PrintCenterText(i, "%s: %s", nameBuf, message);
	}
}

void SendChatToAdmins(int from, const char[] message)
{
	int fromAdmin = CheckCommandAccess(from, "sm_chat", ADMFLAG_CHAT);
	for (int i = 1; i <= MaxClients; i++)
	{
		if (IsClientInGame(i) && (from == i || CheckCommandAccess(i, "sm_chat", ADMFLAG_CHAT)))
		{
			if (g_GameEngine == Engine_CSGO)
				PrintToChat(i, " \x01\x0B\x04%t: \x01%s", fromAdmin ? "Chat admins" : "Chat to admins", from, message);
			else
				PrintToChat(i, "\x04%t: \x01%s", fromAdmin ? "Chat admins" : "Chat to admins", from, message);
		}	
	}
}

void SendDialogToOne(int client, int color, const char[] text, any ...)
{
	char message[100];
	VFormat(message, sizeof(message), text, 4);	
	
	KeyValues kv = new KeyValues("Stuff", "title", message);
	kv.SetColor("color", g_Colors[color][0], g_Colors[color][1], g_Colors[color][2], 255);
	kv.SetNum("level", 1);
	kv.SetNum("time", 10);
	
	CreateDialog(client, kv, DialogType_Msg);

	delete kv;
}

void SendPrivateChat(int client, int target, const char[] message)
{
	if (!client)
	{
		PrintToServer("(Private to %N) %N: %s", target, client, message);
	}
	else if (target != client)
	{
		if (g_GameEngine == Engine_CSGO)
			PrintToChat(client, " \x01\x0B\x04%t: \x01%s", "Private say to", target, client, message);
		else
			PrintToChat(client, "\x04%t: \x01%s", "Private say to", target, client, message);
	}
  
	if (g_GameEngine == Engine_CSGO)
		PrintToChat(target, " \x01\x0B\x04%t: \x01%s", "Private say to", target, client, message);
	else
		PrintToChat(target, "\x04%t: \x01%s", "Private say to", target, client, message);
	LogAction(client, target, "\"%L\" triggered sm_psay to \"%L\" (text %s)", client, target, message);
}

void SendPanelToAll(int from, char[] message)
{
	char title[100];
	Format(title, 64, "%N:", from);
	
	ReplaceString(message, 192, "\\n", "\n");
	
	Panel mSayPanel = new Panel();
	mSayPanel.SetTitle(title);
	mSayPanel.DrawItem("", ITEMDRAW_SPACER);
	mSayPanel.DrawText(message);
	mSayPanel.DrawItem("", ITEMDRAW_SPACER);
	mSayPanel.CurrentKey = GetMaxPageItems(mSayPanel.Style);
	mSayPanel.DrawItem("Exit", ITEMDRAW_CONTROL);

	for(int i = 1; i <= MaxClients; i++)
	{
		if(IsClientInGame(i) && !IsFakeClient(i))
		{
			mSayPanel.Send(i, Handler_DoNothing, 10);
		}
	}

	delete mSayPanel;
}

public int Handler_DoNothing(Menu menu, MenuAction action, int param1, int param2)
{
	/* Do nothing */
	return 0;
}
