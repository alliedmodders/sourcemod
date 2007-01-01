#include <sourcemod>
#include <handles>

public Plugin:myinfo = 
{
	name = "Test Plugin",
	author = "BAILOPAN",
	description = "Tests Stuff",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

native PrintStuff(const String:buffer[]);

public bool:AskPluginLoad(Handle:myself, bool:late, String:error[], err_max)
{
	new String:buffer[PLATFORM_MAX_PATH];
	new FileType:type;
	
	new Handle:dir = OpenDirectory("addons/stripper");
	while (ReadDirEntry(dir, buffer, sizeof(buffer), type))
	{
		decl String:stuff[1024];
		Format(stuff, sizeof(stuff), "Type: %d Dir: %s", _:type, buffer)
		PrintStuff(stuff);
	}
	CloseHandle(dir);
	
	return true;
}
