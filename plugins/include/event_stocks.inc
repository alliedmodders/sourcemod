#if defined _event_stocks_included
 #endinput
#endif
#define _event_stocks_included

#define EVENT_RE_DEFAULT "round_end"
#define EVENT_GS_DEFAULT "game_start"

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
	static bool got_name = false;
	static char event_name[16];
	
	if (!got_name)
	{	
		Handle gc = LoadGameConfigFile("events.games");
		bool exists = GameConfGetKeyValue(gc, "RoundEnd", event_name, sizeof(event_name));
		CloseHandle(gc);
		
		if (!exists)
		{
			Format(event_name, sizeof(event_name), EVENT_RE_DEFAULT);
		}
		
		got_name = true;
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
	static bool got_name = false;
	static char event_name[16];
	
	if (!got_name)
	{
		Handle gc = LoadGameConfigFile("events.games");
		bool exists = GameConfGetKeyValue(gc, "GameStart", event_name, sizeof(event_name));
		CloseHandle(gc);
		
		if (!exists)
		{
			Format(event_name, sizeof(event_name), EVENT_GS_DEFAULT);
		}

		got_name = true;
	}
	
	return event_name;
}
