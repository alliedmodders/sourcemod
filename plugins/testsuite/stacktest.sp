#include <sourcemod>

public Plugin:myinfo = 
{
	name = "Stack Tests",
	author = "AlliedModders LLC",
	description = "Tests stack functions",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

public OnPluginStart()
{
	RegServerCmd("test_stack", Test_Stack);
}

public Action:Test_Stack(args)
{
	new Handle:stack;
	new test[20]
	decl String:buffer[42];
	
	test[0] = 5
	test[1] = 7
	
	stack = CreateStack(30);
	PushStackCell(stack, 50);
	PushStackArray(stack, test, 2);
	PushStackArray(stack, test, 2);
	PushStackString(stack, "space craaab");
	PushStackCell(stack, 12);
	
	PrintToServer("empty? %d", IsStackEmpty(stack));
	
	PopStack(stack);
	PopStackString(stack, buffer, sizeof(buffer));
	PrintToServer("popped: \"%s\"", buffer);
	test[0] = 0
	test[1] = 0
	PrintToServer("values: %d, %d", test[0], test[1]);
	PopStackArray(stack, test, 2);
	PrintToServer("popped: %d, %d", test[0], test[1]);
	PopStackCell(stack, test[0], 1);
	PrintToServer("popped: x, %d", test[0]);
	PopStackCell(stack, test[0]);
	PrintToServer("popped: %d", test[0]);
	
	PrintToServer("empty? %d", IsStackEmpty(stack));
	
	CloseHandle(stack);
	
	return Plugin_Handled;
}
