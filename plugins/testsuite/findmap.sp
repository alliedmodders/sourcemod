#include <sourcemod>

public void OnPluginStart()
{
	RegServerCmd("test_findmap", test_findmap);
}

public Action test_findmap( int argc )
{
	char mapName[PLATFORM_MAX_PATH];
	GetCmdArg(1, mapName, sizeof(mapName));
	
	char resultName[18];
	switch (FindMap(mapName, mapName, sizeof(mapName)))
	{
	case FindMap_Found:
		strcopy(resultName, sizeof(resultName), "Found");
	case FindMap_NotFound:
		strcopy(resultName, sizeof(resultName), "NotFound");
	case FindMap_FuzzyMatch:
		strcopy(resultName, sizeof(resultName), "FuzzyMatch");
	case FindMap_NonCanonical:
		strcopy(resultName, sizeof(resultName), "NonCanonical");
	case FindMap_PossiblyAvailable:
		strcopy(resultName, sizeof(resultName), "PossiblyAvailable");
	}
	
	PrintToServer("FindMap says %s - \"%s\"", resultName, mapName);
	
	return Plugin_Handled;
}