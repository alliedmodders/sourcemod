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

/** Various parsing globals */
new bool:g_FlagsSet[26];					/* Maps whether flags are set */
new AdminFlag:g_FlagLetters[26];			/* Maps the flag letters */		
new bool:g_LoggedFileName = false;			/* Whether or not the file name has been logged */
new g_ErrorCount = 0;						/* Current error count */
new g_IgnoreLevel = 0;						/* Nested ignored section count, so users can screw up files safely */
new String:g_Filename[PLATFORM_MAX_PATH];	/* Used for error messages */

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

ParseError(const String:format[], {Handle,String,Float,_}:...)
{
	decl String:buffer[512];
	
	if (!g_LoggedFileName)
	{
		LogError("Error(s) detected parsing admin_levels.cfg:");
		g_LoggedFileName = true;
	}
	
	VFormat(buffer, sizeof(buffer), format, 2);
	
	LogError(" (%d) %s", ++g_ErrorCount, buffer);
}

InitGlobalStates()
{
	g_ErrorCount = 0;
	g_IgnoreLevel = 0;
	g_LoggedFileName = false;
}
