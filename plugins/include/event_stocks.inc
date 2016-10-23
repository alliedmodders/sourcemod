#if defined _event_stocks_included
 #endinput
#endif
#define _event_stocks_included

#define EVENT_RE_DEFAULT "round_end"
#define EVENT_GS_DEFAULT "game_start"

/**
 * A wrapper function for loading the game data for events.
 * The static tag makes it only load the file once (hopefully)
 *
 * @return			Game data for events or null if failed
 */
 
stock static Handle GameEventsData()
{
	return LoadGameConfigFile("events.games");
}

/**
 * Searches through gamedata to find the proper round end event.
 *
 * This is a wrapper for finding the round end event string.
 * It's intended to be used when hooking the round end event.
 *
 * @return			Round end event from gamedata
 */

stock char RoundEndString()
{
	static char event_name[16];
	
	if (event_name == null)
	{	
		// If retrieval fails, assume the default option
		if (!GameConfGetKeyValue(GameEventsData(), "RoundEnd", event_name, sizeof(event_name)) && event_name[0]))
		{
			Format(event_name, sizeof(event_name), EVENT_RE_DEFAULT);
		}
	}
	
	return event_name;
}

/**
 * Searches through gamedata to find the proper game start event.
 *
 * This is a wrapper for finding the game start event string.
 * It's intended to be used when hooking the game start event.
 *
 * @return			Game start event from gamedata
 */

stock char GameStartString()
{
	static char event_name[16];
	
	if (event_name == null)
	{
		// If retrieval fails, assume the default option
		if (!GameConfGetKeyValue(GameEventsData(), "GameStart", event_name, sizeof(event_name)) && event_name[0]))
		{
			Format(event_name, sizeof(event_name), EVENT_GS_DEFAULT);
		}
	}
	
	return event_name;
}
