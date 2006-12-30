#include <sourcemod>

public Plugin:myinfo = 
{
	name = "Test Plugin",
	author = "BAILOPAN",
	description = "Tests Stuff",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
}

copy(String:dest[], maxlength, const String:source[])
{
	new len
	
	while (source[len] != '\0' && len < maxlength)
	{
		dest[len] = source[len]
		len++
	}
	
	dest[len] = '\0'
}

public bool:AskPluginLoad(Handle:myself, bool:late, String:error[], err_max)
{
	return true
}
