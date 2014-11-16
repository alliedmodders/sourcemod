#include <sourcemod>

public Plugin:myinfo = 
{
	name = "Forward Testing Lab #1",
	author = "AlliedModders LLC",
	description = "Tests suite #1 for forwards created by plugins",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

new Handle:g_GlobalFwd = null;
new Handle:g_PrivateFwd = null;

public OnPluginStart()
{	
	RegServerCmd("test_create_gforward", Command_CreateGlobalForward);
	RegServerCmd("test_create_pforward", Command_CreatePrivateForward);
	RegServerCmd("test_exec_gforward", Command_ExecGlobalForward);
	RegServerCmd("test_exec_pforward", Command_ExecPrivateForward);
}

public OnPluginEnd()
{
	delete g_GlobalFwd;
	delete g_PrivateFwd;
}

public Action:Command_CreateGlobalForward(args)
{
	if (g_GlobalFwd != null)
	{
		delete g_GlobalFwd;
	}
	
	g_GlobalFwd = CreateGlobalForward("OnGlobalForward", ET_Ignore, Param_Any, Param_Cell, Param_Float, Param_String, Param_Array, Param_CellByRef, Param_FloatByRef);
	
	if (g_GlobalFwd == null)
	{
		PrintToServer("Failed to create global forward!");
	}
	
	return Plugin_Handled;
}

public Action:Command_CreatePrivateForward(args)
{
	new Handle:pl;
	new Function:func;
	
	if (g_PrivateFwd != null)
	{
		delete g_PrivateFwd;
	}
	
	g_PrivateFwd = CreateForward(ET_Hook, Param_Cell, Param_String, Param_VarArgs);
	
	if (g_PrivateFwd == null)
	{
		PrintToServer("Failed to create private forward!")
	}
	
	pl = FindPluginByFile("fwdtest2.smx");
	
	if (!pl)
	{
		PrintToServer("Could not find fwdtest2.smx!");
		return Plugin_Handled;
	}
	
	func = GetFunctionByName(pl, "OnPrivateForward");
	
	/* This shouldn't happen, but oh well */
	if (func == INVALID_FUNCTION)
	{
		PrintToServer("Could not find \"OnPrivateForward\" in fwdtest2.smx!");
		return Plugin_Handled;
	}
	
	if (!AddToForward(g_PrivateFwd, pl, func) || !AddToForward(g_PrivateFwd, GetMyHandle(), ZohMyGod))
	{
		PrintToServer("Failed to add functions to private forward!");
		return Plugin_Handled;
	}
	
	return Plugin_Handled;
}

public Action:Command_ExecGlobalForward(args)
{
	new a = 99;
	new Float:b = 4.215;
	new err, ret;
	
	if (g_GlobalFwd == null)
	{
		PrintToServer("Failed to execute global forward. Create it first.");
		return Plugin_Handled;
	}
	
	PrintToServer("Beginning call to %d functions in global forward \"OnGlobalForward\"", GetForwardFunctionCount(g_GlobalFwd));
	
	Call_StartForward(g_GlobalFwd);
	Call_PushCell(OnPluginStart);
	Call_PushCell(7);
	Call_PushFloat(-8.5);
	Call_PushString("Anata wa doko desu ka?");
	Call_PushArray({0.0, 1.1, 2.2}, 3);
	Call_PushCellRef(a);
	Call_PushFloatRef(b);
	err = Call_Finish(ret);
	
	PrintToServer("Call to global forward \"OnGlobalForward\" completed");
	PrintToServer("Error code = %d (expected: %d)", err, 0);
	PrintToServer("Return value = %d (expected: %d)", ret, Plugin_Continue);
	
	PrintToServer("a = %d (expected: %d)", a, 777);
	PrintToServer("b = %f (expected: %f)", b, -0.782);
	
	return Plugin_Handled;
}

public Action:Command_ExecPrivateForward(args)
{
	new err, ret;
	
	if (g_PrivateFwd == null)
	{
		PrintToServer("Failed to execute private forward. Create it first.");
		return Plugin_Handled;
	}
	
	PrintToServer("Beginning call to %d functions in private forward", GetForwardFunctionCount(g_PrivateFwd));
	
	Call_StartForward(g_PrivateFwd);
	Call_PushCell(24);
	Call_PushString("I am a format string: %d %d %d %d %d %d");
	Call_PushCell(0);
	Call_PushCell(1);
	Call_PushCell(2);
	Call_PushCell(3);
	Call_PushCell(4);
	Call_PushCell(5);
	err = Call_Finish(ret);
	
	PrintToServer("Call to private forward completed");
	PrintToServer("Error code = %d (expected: %d)", err, 0);
	PrintToServer("Return value = %d (expected: %d)", ret, Plugin_Handled);

	return Plugin_Handled;
}

public Action:ZohMyGod(num, const String:format[], ...)
{
	decl String:buffer[128];
	
	PrintToServer("Inside private forward #1");
	
	PrintToServer("num = %d (expected: %d)", num, 24);
	
	VFormat(buffer, sizeof(buffer), format, 3);
	PrintToServer("buffer = \"%s\" (expected: \"%s\")", buffer, "I am a format string: 0 1 2 3 4 5");
	
	PrintToServer("End private forward #1");
	
	return Plugin_Continue;
}
