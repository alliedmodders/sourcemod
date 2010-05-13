#include <sourcemod>

public Plugin:myinfo = 
{
	name = "Test target filters",
	author = "AlliedModders LLC",
	description = "Tests target filters",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

public OnPluginStart()
{
	AddMultiTargetFilter("@crab", filter, "all players", true)
}

public bool:filter(const String:pattern[], Handle:clients)
{
	for (new i = 1; i <= MaxClients; i++) {
		if (IsPlayerInGame(i))
			PushArrayCell(clients, i)
	}

	return true
}

