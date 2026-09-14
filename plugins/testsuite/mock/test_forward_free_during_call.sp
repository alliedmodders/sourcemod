#pragma semicolon 1
#include <testing>

#pragma newdecls required

public Plugin myinfo = 
{
	name = "Forward Free During Call Test",
	author = "AlliedModders LLC",
	description = "Tests that freeing a private forward from within one of its own callbacks does not crash",
	version = "1.0.0.0",
	url = "https://www.sourcemod.net/"
};

public void OnPluginStart() {
	SetTestContext("Forward Free During Call Test");
	Test_FreeForwardDuringOwnCall();
	PrintToServer("PASS");
}

void Test_FreeForwardDuringOwnCall() {
	Handle fwd = CreateForward(ET_Ignore, Param_Cell);
	AddToForward(fwd, INVALID_HANDLE, ForwardCall);

	Call_StartForward(fwd);
	Call_PushCell(fwd);
	Call_Finish();

	// If we got here without crashing, the forward was safely destroyed.
}

public void ForwardCall(Handle fwd) {
	// Mirrors the NativeVotes pattern that triggered issue #1041: freeing the
	// forward's own handle from within one of its own callbacks.
	delete fwd;
}
