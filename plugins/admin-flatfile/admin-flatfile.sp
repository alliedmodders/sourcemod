#include <sourcemod>
#include <textparse>

public Plugin:myinfo = 
{
	name = "Admin Base",
	author = "AlliedModders LLC",
	description = "Reads admin files",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

/** Various globals */
new AdminFlag:g_FlagLetters[26];			/* Maps the flag letters */		
new bool:g_LoggedFileName = false;			/* Whether or not the file name has been logged */
new g_ErrorCount = 0;

#include "admin-levels.sp"
#include "admin-overrides.sp"
#include "admin-groups.sp"

public OnRebuildAdminCache(AdminCachePart:part)
{
	RefreshLevels();
	if (part == AdminCache_Overrides)
	{
		ReadOverrides();
	} else if (part == AdminCache_Groups)
	{
		ReadGroups();
	}
}
