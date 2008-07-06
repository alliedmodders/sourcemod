#include <sourcemod>
#include <sdktools>

public Plugin:myinfo = 
{
	name = "Entity Output Hook Testing",
	author = "AlliedModders LLC",
	description = "Test suite for Entity Output Hooks",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

public OnPluginStart()
{
	HookEntityOutput("point_spotlight", "OnLightOn", OutputHook);
	
	HookEntityOutput("func_door", "OnOpen", OutputHook);
	HookEntityOutput("func_door_rotating", "OnOpen", OutputHook);
	HookEntityOutput("func_door", "OnClose", OutputHook);
	HookEntityOutput("func_door_rotating", "OnClose", OutputHook);
}

public OutputHook(const String:name[], caller, activator, Float:delay)
{
	LogMessage("[ENTOUTPUT] %s", name);
}

public OnMapStart()
{
	new ent = FindEntityByClassname(-1, "point_spotlight");
	
	if (ent == -1)
	{
		LogError("Could not find a point_spotlight");
		ent = CreateEntityByName("point_spotlight");
		DispatchSpawn(ent);
	}

	HookSingleEntityOutput(ent, "OnLightOn", OutputHook, true);
	HookSingleEntityOutput(ent, "OnLightOff", OutputHook, true);
	
	AcceptEntityInput(ent, "LightOff", ent, ent);
	AcceptEntityInput(ent, "LightOn", ent, ent);
	
	AcceptEntityInput(ent, "LightOff", ent, ent);
	AcceptEntityInput(ent, "LightOn", ent, ent);
	
	HookSingleEntityOutput(ent, "OnLightOn", OutputHook, false);
	HookSingleEntityOutput(ent, "OnLightOff", OutputHook, false);
	
	AcceptEntityInput(ent, "LightOff", ent, ent);
	AcceptEntityInput(ent, "LightOn", ent, ent);
	AcceptEntityInput(ent, "LightOff", ent, ent);
	AcceptEntityInput(ent, "LightOn", ent, ent);
	
	//Comment these out (and reload the plugin heaps) to test for leaks on plugin unload
	UnhookSingleEntityOutput(ent, "OnLightOn", OutputHook);
	UnhookSingleEntityOutput(ent, "OnLightOff", OutputHook);
}