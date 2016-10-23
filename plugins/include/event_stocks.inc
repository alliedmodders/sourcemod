#if defined _event_stocks_included
 #endinput
#endif
#define _event_stocks_included


/**
 * Given a game folder name, find the round end event string.
 *
 * This is a wrapper for finding the round end event string.
 * It's intended to be used when hooking the round end event.
 *
 * @param game_folder		The name of the game folder.
 * @return			Round end event from gamedata
 */

stock char RoundEndString()
{
	static Handle gameConfig = LoadGameConfigFile("events.games");	
	if (gameConfig == null)
	{
		SetFailState("Unable to load game config events.games");
		return null;
	}
	
	static char event_name[16];
	if (event_name == null)
	{
		GameConfGetKeyValue(gameConfig, "RoundEnd", event_name, sizeof(event_name)) && event_name[0])	
	}
	
	return event_name;
}
