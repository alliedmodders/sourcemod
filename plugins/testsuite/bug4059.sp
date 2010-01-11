#include <sourcemod>

public OnPluginStart()
{
	new Handle:hostname = FindConVar("hostname")
	HookConVarChange(hostname, OnChange)
	HookEvent("player_team", cb)
	RegServerCmd("test_bug4059", Test_Bug)
}

public Action:cb(Handle:event, const String:name[], bool:dontBroadcast)
{
	UnhookEvent(name, cb)
	PrintToServer("whee")
	HookEvent(name, cb)
	return Plugin_Handled
}

public OnChange(Handle:convar, const String:oldValue[], const String:newValue[])
{
	PrintToServer("called: %x", convar)
	UnhookConVarChange(convar, OnChange)
	ResetConVar(convar)
	HookConVarChange(convar, OnChange)
}

public Action:Test_Bug(args)
{
	ServerCommand("hostname \"bug4059\"")
}

