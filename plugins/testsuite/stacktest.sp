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
	int test[20];
	char buffer[42];
	
	test[0] = 5
	test[1] = 7
	
	ArrayStack stack = ArrayStack(30);
	stack.Push(50);
	stack.PushArray(test, 2);
	stack.PushArray(test, 2);
	stack.PushString("space craaab");
	stack.Push(12);
	
	PrintToServer("empty? %d", stack.Empty);
	
	stack.Pop();
	stack.PopString(buffer, sizeof(buffer));
	PrintToServer("popped: \"%s\"", buffer);
	test[0] = 0
	test[1] = 0
	PrintToServer("values: %d, %d", test[0], test[1]);
	stack.PopArray(test, 2);
	PrintToServer("popped: %d, %d", test[0], test[1]);
	test[0] = stack.Pop(1);
	PrintToServer("popped: x, %d", test[0]);
	test[0] = stack.Pop();
	PrintToServer("popped: %d", test[0]);
	
	PrintToServer("empty? %d", stack.Empty);
	
	delete stack;
	return Plugin_Handled;
}
