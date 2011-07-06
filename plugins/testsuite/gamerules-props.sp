#include <sourcemod>
#include <sdktools>

enum Game { Game_TF, Game_SWARM }
new Game:g_Game;

public APLRes:AskPluginLoad2(Handle:myself, bool:late, String:error[], err_max)
{
	decl String:gamedir[64];
	GetGameFolderName(gamedir, sizeof(gamedir));
	if (!strcmp(gamedir, "tf"))
	{
		g_Game = Game_TF;
		return APLRes_Success;
	}
	
	if (!strcmp(gamedir, "swarm"))
	{
		g_Game = Game_SWARM;
		return APLRes_Success;
	}
	
	strcopy(error, err_max, "These tests are only supported on TF2 and Alien Swarm");
	return APLRes_Failure;
}

public OnPluginStart()
{	
	if (g_Game == Game_TF)
	{
		RegConsoleCmd("gr_getstate", gr_getstate);
		RegConsoleCmd("gr_ttr", gr_ttr);
		RegConsoleCmd("gr_tbs", gr_tbs);
		RegConsoleCmd("gr_nextspawn", gr_nextspawn);
		RegConsoleCmd("gr_settime", gr_settime);
		RegConsoleCmd("gr_getgoal", gr_getgoal);
		RegConsoleCmd("gr_setgoal", gr_setgoal);
	}
	else if (g_Game == Game_SWARM)
	{
		HookEvent("entity_killed", entity_killed);
		RegConsoleCmd("gr_getstimmer", gr_getstimmer);
	}
}

public Action:gr_getstate(client, argc)
{
	ReplyToCommand(client, "Round state is %d", GameRules_GetRoundState());
	return Plugin_Handled;
}

stock Float:GameRules_GetTimeUntilRndReset()
{
	new Float:flRestartTime = GameRules_GetPropFloat("m_flRestartRoundTime");
	if (flRestartTime == -1.0)
		return flRestartTime;
	
	return flRestartTime - GetGameTime();
}

public Action:gr_ttr(client, argc)
{
	ReplyToCommand(client, "Time til restart is %.2f", GameRules_GetTimeUntilRndReset());
	return Plugin_Handled;
}

public Action:gr_tbs(client, argc)
{
	ReplyToCommand(client, "Time between spawns for team2 is %.2f", GameRules_GetPropFloat("m_TeamRespawnWaveTimes", 2));
	return Plugin_Handled;
}

public Action:gr_nextspawn(client, argc)
{
	ReplyToCommand(client, "Next spawn for team2 is %.2f", GameRules_GetPropFloat("m_flNextRespawnWave", 2));
	return Plugin_Handled;
}

public Action:gr_settime(client, argc)
{
	GameRules_SetPropFloat("m_TeamRespawnWaveTimes", 2.0, 2, true);
	return Plugin_Handled;
}

public Action:gr_getgoal(client, argc)
{
	decl String:goal[64];
	GameRules_GetPropString("m_pszTeamGoalStringRed", goal, sizeof(goal));
	ReplyToCommand(client, "Red goal string is \"%s\"", goal);
	return Plugin_Handled;
}

public Action:gr_setgoal(client, argc)
{
	GameRules_SetPropString("m_pszTeamGoalStringRed", "urururur", true);
	
	decl String:goal[64];
	GameRules_GetPropString("m_pszTeamGoalStringRed", goal, sizeof(goal));
	ReplyToCommand(client, "Red goal string is \"%s\"", goal);
	return Plugin_Handled;
}

//.. DIE
public entity_killed(Handle:event, const String:name[], bool:dontBroadcast)
{
	new ent = GetEventInt(event, "entindex_killed");
	if (!IsValidEdict(ent))
		return;
	
	decl String:classname[64];
	GetEdictClassname(ent, classname, sizeof(classname));
	
	if (!!strcmp(classname, "asw_marine"))
		return;
	
	decl Float:deathvec[3];
	GameRules_GetPropVector("m_vMarineDeathPos", deathvec);
	PrintToChatAll("Death vec is %.3f %.3f %.3f", deathvec[0], deathvec[1], deathvec[2]);
}

// you can manually induce this with asw_PermaStim (needs sv_cheats)
public Action:gr_getstimmer(client, argc)
{
	ReplyToCommand(client, "%N (%.2f/%.2f)",
		GameRules_GetPropEnt("m_hStartStimPlayer"),
		GameRules_GetPropFloat("m_flStimStartTime"),
		GameRules_GetPropFloat("m_flStimEndTime"));
	return Plugin_Handled;
}
