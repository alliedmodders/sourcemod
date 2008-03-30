#include <sourcemod>

public Plugin:myinfo = 
{
	name = "Forward Testing Lab #2",
	author = "AlliedModders LLC",
	description = "Tests suite #2 for forwards created by plugins",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

public Action:OnPrivateForward(num, const String:format[], ...)
{
	decl String:buffer[128];
	
	PrintToServer("Inside private forward #2");
	
	PrintToServer("num = %d (expected: %d)", num, 24);
	
	VFormat(buffer, sizeof(buffer), format, 3);
	PrintToServer("buffer = \"%s\" (expected: \"%s\")", buffer, "I am a format string: 0 1 2 3 4 5");
	
	PrintToServer("End private forward #2");
	
	return Plugin_Handled;
}

public OnGlobalForward(Function:a, b, Float:c, const String:d[], Float:e[3], &f, &Float:g)
{
	PrintToServer("Inside global forward \"OnGlobalForward\"");
	
	PrintToServer("a = %d (expected: %d)", a, 11);
	PrintToServer("b = %d (expected: %d)", b, 7);
	PrintToServer("c = %f (expected: %f)", c, -8.5);
	PrintToServer("d = \"%s\" (expected: \"%s\")", d, "Anata wa doko desu ka?");
	PrintToServer("e = %f %f %f (expected: %f %f %f)", e[0], e[1], e[2], 0.0, 1.1, 2.2);
	
	PrintToServer("f = %d (expected %d, setting to %d)", f, 99, 777);
	f = 777;
	
	PrintToServer("g = %f (expected %f, setting to %f)", g, 4.215, -0.782);
	g = -0.782;
}
