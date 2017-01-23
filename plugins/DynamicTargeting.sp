#pragma semicolon 1
#define PLUGIN_VERSION "1.0"

#include <sourcemod>
#include <DynamicTargeting>

#pragma newdecls required

public Plugin myinfo =
{
	name = "Dynamic Targeting",
	author = "BotoX",
	description = "",
	version = PLUGIN_VERSION,
	url = ""
}

char g_PlayerNames[MAXPLAYERS + 1][MAX_NAME_LENGTH];
Handle g_PlayerData[MAXPLAYERS + 1];

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	CreateNative("AmbiguousMenu", Native_AmbiguousMenu);
	RegPluginLibrary("DynamicTargeting");

	return APLRes_Success;
}

public void OnClientDisconnect(int client)
{
	if(g_PlayerData[client] != INVALID_HANDLE)
	{
		CloseHandle(g_PlayerData[client]);
		g_PlayerData[client] = INVALID_HANDLE;
	}
}

int CreateAmbiguousMenu(int client, const char[] sCommand, const char[] sArgString, const char[] sPattern, int FilterFlags)
{
	Menu menu = new Menu(MenuHandler_AmbiguousMenu, MenuAction_Select|MenuAction_Cancel|MenuAction_End|MenuAction_DrawItem|MenuAction_DisplayItem);
	menu.ExitButton = true;

	char sTitle[32 + MAX_TARGET_LENGTH];
	FormatEx(sTitle, sizeof(sTitle), "Target \"%s\" is ambiguous.", sPattern);
	menu.SetTitle(sTitle);

	int Players = 0;
	int[] aClients = new int[MaxClients + 1];

	for(int i = 1; i <= MaxClients; i++)
	{
		if(!IsClientConnected(i) || i == client)
			continue;

		if(FilterFlags & COMMAND_FILTER_NO_BOTS && IsFakeClient(i))
			continue;

		if(!(FilterFlags & COMMAND_FILTER_CONNECTED) && !IsClientInGame(i))
			continue;

		if(FilterFlags & COMMAND_FILTER_ALIVE && !IsPlayerAlive(i))
			continue;

		if(FilterFlags & COMMAND_FILTER_DEAD && IsPlayerAlive(i))
			continue;

		// insert player names into g_PlayerNames array
		GetClientName(i, g_PlayerNames[i], sizeof(g_PlayerNames[]));

		if(StrContains(g_PlayerNames[i], sPattern, false) != -1)
			aClients[Players++] = i;
	}

	// sort aClients array by player name
	SortCustom1D(aClients, Players, SortByPlayerName);

	// insert players sorted
	char sUserId[12];
	char sDisp[MAX_NAME_LENGTH + 16];
	for(int i = 0; i < Players; i++)
	{
		IntToString(GetClientUserId(aClients[i]), sUserId, sizeof(sUserId));

		FormatEx(sDisp, sizeof(sDisp), "%s (%s)", g_PlayerNames[aClients[i]], sUserId);
		menu.AddItem(sUserId, sDisp);
	}

	DataPack pack = new DataPack();
	pack.WriteString(sCommand);
	pack.WriteString(sArgString);
	pack.WriteString(sPattern);
	pack.WriteCell(FilterFlags);

	if(g_PlayerData[client] != INVALID_HANDLE)
	{
		CloseHandle(g_PlayerData[client]);
		g_PlayerData[client] = INVALID_HANDLE;
	}
	CancelClientMenu(client);

	g_PlayerData[client] = pack;
	menu.Display(client, MENU_TIME_FOREVER);

	return 0;
}

public int MenuHandler_AmbiguousMenu(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_End:
		{
			CloseHandle(menu);
		}
		case MenuAction_Cancel:
		{
			if(g_PlayerData[param1] != INVALID_HANDLE)
			{
				CloseHandle(g_PlayerData[param1]);
				g_PlayerData[param1] = INVALID_HANDLE;
			}
		}
		case MenuAction_Select:
		{
			int Style;
			char sItem[32];
			char sDisp[MAX_NAME_LENGTH + 16];
			menu.GetItem(param2, sItem, sizeof(sItem), Style, sDisp, sizeof(sDisp));

			int UserId = StringToInt(sItem);
			int client = GetClientOfUserId(UserId);
			if(!client)
			{
				PrintToChat(param1, "\x04[DynamicTargeting]\x01 Player no longer available.");
				menu.DisplayAt(param1, GetMenuSelectionPosition(), MENU_TIME_FOREVER);
				return 0;
			}

			DataPack pack = view_as<DataPack>(g_PlayerData[param1]);
			pack.Reset();

			char sCommand[128];
			pack.ReadString(sCommand, sizeof(sCommand));

			char sArgString[256];
			pack.ReadString(sArgString, sizeof(sArgString));

			char sPattern[MAX_TARGET_LENGTH];
			pack.ReadString(sPattern, sizeof(sPattern));

			int Result = ReCallAmbiguous(param1, client, sCommand, sArgString, sPattern);

			return Result;
		}
		case MenuAction_DrawItem:
		{
			int Style;
			char sItem[32];
			menu.GetItem(param2, sItem, sizeof(sItem), Style);

			int UserId = StringToInt(sItem);
			int client = GetClientOfUserId(UserId);
			if(!client) // Player disconnected
				return ITEMDRAW_DISABLED;

			return Style;
		}
		case MenuAction_DisplayItem:
		{
			int Style;
			char sItem[32];
			char sDisp[MAX_NAME_LENGTH + 16];
			menu.GetItem(param2, sItem, sizeof(sItem), Style, sDisp, sizeof(sDisp));

			if(!sItem[0])
				return 0;

			char sBuffer[MAX_NAME_LENGTH + 16];
			int UserId = StringToInt(sItem);
			int client = GetClientOfUserId(UserId);
			if(!client) // Player disconnected
				return 0;

			GetClientName(client, g_PlayerNames[client], sizeof(g_PlayerNames[]));
			FormatEx(sBuffer, sizeof(sBuffer), "%s (%d)", g_PlayerNames[client], UserId);

			if(!StrEqual(sDisp, sBuffer))
				return RedrawMenuItem(sBuffer);

			return 0;
		}
	}

	return 0;
}

int ReCallAmbiguous(int client, int newClient, const char[] sCommand, const char[] sArgString, const char[] sPattern)
{
	char sTarget[16];
	FormatEx(sTarget, sizeof(sTarget), "#%d", GetClientUserId(newClient));

	char sNewArgString[256];
	strcopy(sNewArgString, sizeof(sNewArgString), sArgString);

	char sPart[256];
	int CurrentIndex = 0;
	int NextIndex = 0;

	while(NextIndex != -1 && CurrentIndex < sizeof(sNewArgString))
	{
		NextIndex = BreakString(sNewArgString[CurrentIndex], sPart, sizeof(sPart));

		if(StrEqual(sPart, sPattern))
		{
			ReplaceStringEx(sNewArgString[CurrentIndex], sizeof(sNewArgString) - CurrentIndex, sPart, sTarget);
			break;
		}

		CurrentIndex += NextIndex;
	}

	FakeClientCommandEx(client, "%s %s", sCommand, sNewArgString);

	return 0;
}

public int Native_AmbiguousMenu(Handle plugin, int numParams)
{
	int client = GetNativeCell(1);

	if(client > MaxClients || client <= 0)
	{
		ThrowNativeError(SP_ERROR_NATIVE, "Client is not valid.");
		return -1;
	}

	if(!IsClientInGame(client))
	{
		ThrowNativeError(SP_ERROR_NATIVE, "Client is not in-game.");
		return -1;
	}

	if(IsFakeClient(client))
	{
		ThrowNativeError(SP_ERROR_NATIVE, "Client is fake-client.");
		return -1;
	}

	char sCommand[128];
	GetNativeString(2, sCommand, sizeof(sCommand));

	char sArgString[256];
	GetNativeString(3, sArgString, sizeof(sArgString));

	char sPattern[MAX_TARGET_LENGTH];
	GetNativeString(4, sPattern, sizeof(sPattern));

	int FilterFlags = GetNativeCell(5);

	return CreateAmbiguousMenu(client, sCommand, sArgString, sPattern, FilterFlags);
}

public int SortByPlayerName(int elem1, int elem2, const int[] array, Handle hndl)
{
	return strcmp(g_PlayerNames[elem1], g_PlayerNames[elem2], false);
}
