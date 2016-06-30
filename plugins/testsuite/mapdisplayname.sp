#include <sourcemod>

public void OnPluginStart()
{
	RegServerCmd("test_mapdisplayname", test_mapdisplayname);
}

public Action test_mapdisplayname( int argc )
{
	char mapName[PLATFORM_MAX_PATH];
	GetCmdArg(1, mapName, sizeof(mapName));
	
	char displayName[PLATFORM_MAX_PATH];
	
	if (GetMapDisplayName(mapName, displayName, sizeof(displayName)))
	{
		PrintToServer("GetMapDisplayName says \"%s\" for \"%s\"", displayName, mapName);
	}
	else
	{
		PrintToServer("GetMapDisplayName says \"%s\" was not found or not resolved", mapName);
	}
	
	return Plugin_Handled;
}