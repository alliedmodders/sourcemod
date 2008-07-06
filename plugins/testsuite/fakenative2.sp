#include <sourcemod>

native TestNative1(gaben, ...);
native TestNative2(String:buffer[]);
native TestNative3(value1, &value2);
native TestNative4(const input[], output[], size);
native TestNative5(bool:local, String:buffer[], maxlength, const String:fmt[], {Handle,Float,String,_}:...);

public Plugin:myinfo = 
{
	name = "FakeNative Testing Lab #2",
	author = "AlliedModders LLC",
	description = "Test suite #2 for dynamic natives",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

public OnPluginStart()
{
	RegServerCmd("test_native1", Test_Native1);
	RegServerCmd("test_native2", Test_Native2);
	RegServerCmd("test_native3", Test_Native3);
	RegServerCmd("test_native4", Test_Native4);
	RegServerCmd("test_native5", Test_Native5);
}

public Action:Test_Native1(args)
{
	PrintToServer("Returned: %d", TestNative1(1));
	PrintToServer("Returned: %d", TestNative1(1,2));
	PrintToServer("Returned: %d", TestNative1(1,2,3,4));
	
	return Plugin_Handled;
}

public Action:Test_Native2(args)
{
	new String:buffer[] = "IBM";
	
	PrintToServer("Before: %s", buffer);
	
	TestNative2(buffer);
	
	PrintToServer("AfteR: %s", buffer);
	
	return Plugin_Handled;
}

public Action:Test_Native3(args)
{
	new value1 = 5, value2 = 6
	
	PrintToServer("Before: %d, %d", value1, value2);
	
	TestNative3(value1, value2);
	
	PrintToServer("After: %d, %d", value1, value2);
	
	return Plugin_Handled;
}

public Action:Test_Native4(args)
{
	new input[5] = {0, 1, 2, 3, 4};
	new output[5];
	
	TestNative4(input, output, 5);
	
	PrintToServer("[0=%d] [1=%d] [2=%d] [3=%d] [4=%d]", input[0], input[1], input[2], input[3], input[4]);
	PrintToServer("[0=%d] [1=%d] [2=%d] [3=%d] [4=%d]", output[0], output[1], output[2], output[3], output[4]);
	
	return Plugin_Handled;
}

public Action:Test_Native5(args)
{
	new String:buffer1[512];
	new String:buffer2[512];
	
	TestNative5(true, buffer1, sizeof(buffer1), "%d gabens in the %s", 50, "gabpark");
	TestNative5(true, buffer2, sizeof(buffer2), "%d gabens in the %s", 50, "gabpark");
	
	PrintToServer("Test 1: %s", buffer1);
	PrintToServer("Test 2: %s", buffer2);
	
	return Plugin_Handled;
}
