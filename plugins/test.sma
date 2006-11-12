#include <sourcemod>

public Plugin:myinfo = 
{
	name = "Test Plugin",
	author = "BAILOPAN",
	description = "Tests Stuff",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
}

public Test(num, &num2)
{
	num2 += num
	
	return num
}

public Test2(num, &num2)
{
	num2 += num
	
	return num
}
