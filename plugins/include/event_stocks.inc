/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#if defined _event_stocks_included
 #endinput
#endif
#define _event_stocks_included

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
	static char event_name[32];
	
	if (!got_name)
	{	
		Format(event_name, sizeof(event_name), GetEventString(gd_RoundEnd));		
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
	static char event_name[32];
	
	if (!got_name)
	{
		Format(event_name, sizeof(event_name), GetEventString(gd_GameStart));
		got_name = true;
	}
	
	return event_name;
}

/* Wrapper for adding new events to game data */
enum EventGameData 
{
	gd_RoundEnd = 0,
	gd_GameStart
}
stock char gd_event_name[EventGameData][] = 
{
	"RoundEnd",
	"GameStart"
};
stock char gd_event_default[EventGameData][] =
{
	"round_end",
	"game_start"
};

/**
 * Searches through gamedata to find an event name.
 *
 * This is a wrapper for finding a game event string.
 * It's intended to be used when hooking an event.
 *
 * @parm			GameEventsData enum
 * @return			event string for current game
 */

stock char GetEventString(EventGameData data)
{
	char event_name[32];
	int gd_index = view_as<int>(data);
	
	Handle gc = LoadGameConfigFile("events.games");
	bool exists = GameConfGetKeyValue(gc, gd_event_name[gd_index], event_name, sizeof(event_name));
	CloseHandle(gc);
	
	if (!exists)
	{
		Format(event_name, sizeof(event_name), gd_event_default[gd_index]);	
	}
	
	return event_name;
}
