#include <sourcemod>

public Plugin:myinfo = 
{
	name = "Function Call Testing Lab",
	author = "AlliedModders LLC",
	description = "Tests basic function calls",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

public OnPluginStart()
{
	RegServerCmd("test_callfunc", Command_CallFunc);
	RegServerCmd("test_callfunc_reentrant", Command_ReentrantCallFunc);
}

public OnCallFuncReceived(num, Float:fnum, String:str[], String:str2[], &val, &Float:fval, array[], array2[], size, hello2[1])
{
	PrintToServer("Inside OnCallFuncReceived...");

	PrintToServer("num = %d (expected: %d)", num, 5);
	PrintToServer("fnum = %f (expected: %f)", fnum, 7.17);
	PrintToServer("str[] = \"%s\" (expected: \"%s\")", str, "Gaben");
	PrintToServer("str2[] = \"%s\" (expected: \"%s\")", str2, ".taf si nebaG");
	
	PrintToServer("val = %d (expected %d, setting to %d)", val, 62, 15);
	val = 15;
	
	PrintToServer("fval = %f (expected: %f, setting to %f)", fval, 6.25, 1.5);
	fval = 1.5; 
	
	PrintToServer("Printing %d elements of array[] (expected: %d)", size, 6);
	for (new i = 0; i < size; i++)
	{
		PrintToServer("array[%d] = %d (expected: %d)", i, array[i], i);
	}
	for (new i = 0; i < size; i++)
	{
		PrintToServer("array2[%d] = %d (expected: %d)", i, array[i], i);
	}
	
	/* This shouldn't get copied back */
	strcopy(str, strlen(str) + 1, "Yeti");
	/* This should get copied back */
	strcopy(str2, strlen(str2) + 1, "Gaben is fat.");
	
	/* This should get copied back */
	array[0] = 5;
	array[1] = 6;
	/* This shouldn't get copied back */
	hello2[0] = 25;
	
	return 42;
}

public OnReentrantCallReceived(num, String:str[])
{
	new err, ret;
	
	PrintToServer("Inside OnReentrantCallReceived...");
	
	PrintToServer("num = %d (expected: %d)", num, 7);
	PrintToServer("str[] = \"%s\" (expected: \"%s\")", str, "nana");
	
	new Function:func = GetFunctionByName(null, "OnReentrantCallReceivedTwo");
	
	if (func == INVALID_FUNCTION)
	{
		PrintToServer("Failed to get the function id of OnReentrantCallReceivedTwo");
		return 0;
	}
	
	PrintToServer("Calling OnReentrantCallReceivedTwo...");
	
	Call_StartFunction(null, func);
	Call_PushFloat(8.0);
	err = Call_Finish(ret);
	
	PrintToServer("Call to OnReentrantCallReceivedTwo has finished!");
	PrintToServer("Error code = %d (expected: %d)", err, 0);
	PrintToServer("Return value = %d (expected: %d)", ret, 707);
	
	return 11;
}

public OnReentrantCallReceivedTwo(Float:fnum)
{
	PrintToServer("Inside OnReentrantCallReceivedTwo...");
	
	PrintToServer("fnum = %f (expected: %f)", fnum, 8.0);

	return 707;
}

public Action:Command_CallFunc(args)
{
	new a = 62;
	new Float:b = 6.25;
	new const String:what[] = "Gaben";
	new String:truth[] = ".taf si nebaG";
	new hello[] = {0, 1, 2, 3, 4, 5};
	new hello2[] = {9};
	new pm = 6;
	new err;
	new ret;
	
	new Function:func = GetFunctionByName(null, "OnCallFuncReceived");
	
	if (func == INVALID_FUNCTION)
	{
		PrintToServer("Failed to get the function id of OnCallFuncReceived");
		return Plugin_Handled;
	}
	
	PrintToServer("Calling OnCallFuncReceived...");
	
	Call_StartFunction(null, func);
	Call_PushCell(5);
	Call_PushFloat(7.17);
	Call_PushString(what);
	Call_PushStringEx(truth, sizeof(truth), SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
	Call_PushCellRef(a);
	Call_PushFloatRef(b);
	Call_PushArrayEx(hello, pm, SM_PARAM_COPYBACK);
	Call_PushArray(hello, pm);
	Call_PushCell(pm);
	Call_PushArray(hello2, 1);
	err = Call_Finish(ret);
	
	PrintToServer("Call to OnCallFuncReceived has finished!");
	PrintToServer("Error code = %d (expected: %d)", err, 0);
	PrintToServer("Return value = %d (expected: %d)", ret, 42);
	
	PrintToServer("a = %d (expected: %d)", a, 15);
	PrintToServer("b = %f (expected: %f)", b, 1.5);
	PrintToServer("what = \"%s\" (expected: \"%s\")", what, "Gaben");
	PrintToServer("truth = \"%s\" (expected: \"%s\")", truth, "Gaben is fat.");
	PrintToServer("hello[0] = %d (expected: %d)", hello[0], 5);
	PrintToServer("hello[1] = %d (expected: %d)", hello[1], 6);
	PrintToServer("hello2[0] = %d (expected: %d)", hello2[0], 9);
	
	return Plugin_Handled;
}

public Action:Command_ReentrantCallFunc(args)
{	
	new err, ret;
	new Function:func = GetFunctionByName(null, "OnReentrantCallReceived");
	
	if (func == INVALID_FUNCTION)
	{
		PrintToServer("Failed to get the function id of OnReentrantCallReceived");
		return Plugin_Handled;
	}
	
	PrintToServer("Calling OnReentrantCallReceived...");
	
	Call_StartFunction(null, func);
	Call_PushCell(7);
	Call_PushString("nana");
	err = Call_Finish(ret);
	
	PrintToServer("Call to OnReentrantCallReceived has finished!");
	PrintToServer("Error code = %d (expected: %d)", err, 0);
	PrintToServer("Return value = %d (expected: %d)", ret, 11);
	
	return Plugin_Handled;
}
