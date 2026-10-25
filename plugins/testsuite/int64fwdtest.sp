#include <sourcemod>

public Plugin myinfo =
{
	name = "Int64 Forward Testing Lab",
	author = "AlliedModders LLC",
	description = "Tests passing 64-bit integers through forwards and function calls",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

public void OnPluginStart()
{
	RegServerCmd("test_int64fwd", Command_Int64Forward);
}

public void OnInt64Received(int64 value, int marker)
{
	PrintToServer("[%d] value = %x:%x (expected: %x:%x)", marker,
		view_as<int>(value >> 32), view_as<int>(value), 0x12345678, 0x0badf00d);
}

public Action Command_Int64Forward(int args)
{
	int64 value = (view_as<int64>(0x12345678) << 32) | view_as<int64>(0x0badf00d);

	GlobalForward global = new GlobalForward("OnInt64Received", ET_Ignore, Param_Int64, Param_Cell);
	Call_StartForward(global);
	Call_PushInt64(value);
	Call_PushCell(1);
	PrintToServer("[1] Call_Finish = %d (expected: %d)", Call_Finish(), SP_ERROR_NONE);
	delete global;

	PrivateForward any = new PrivateForward(ET_Ignore, Param_Any, Param_Cell);
	any.AddFunction(null, OnInt64Received);
	Call_StartForward(any);
	Call_PushInt64(value);
	Call_PushCell(2);
	PrintToServer("[2] Call_Finish = %d (expected: %d)", Call_Finish(), SP_ERROR_NONE);
	delete any;

	Call_StartFunction(null, OnInt64Received);
	Call_PushInt64(value);
	Call_PushCell(3);
	PrintToServer("[3] Call_Finish = %d (expected: %d)", Call_Finish(), SP_ERROR_NONE);

	PrivateForward cell = new PrivateForward(ET_Ignore, Param_Cell, Param_Cell);
	cell.AddFunction(null, OnInt64Received);
	Call_StartForward(cell);
	Call_PushInt64(value);
	Call_PushCell(4);
	PrintToServer("[4] Call_Finish = %d (expected: %d, and no value printed)", Call_Finish(), SP_ERROR_PARAM);
	delete cell;

	return Plugin_Handled;
}
