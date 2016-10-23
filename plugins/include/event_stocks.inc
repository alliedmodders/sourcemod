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
 * Caches the event name after reading from keyvalues first time.
 * This wrapper is intended to be used when hooking an event.
 *
 * @parm			GameEventsData enum
 * @return			event string for current game
 */
  
stock char GetEventName(EventGameData event)
{
	static bool got_name[EventGameData] = false;
	static char event_name[EventGameData][32];
	
	int index = view_as<int>(event);	
	if (!got_name[index])
	{	
		Format(event_name[index], sizeof(event_name[index]), GetEventNameEx(event));		
		got_name = true;
	}
	
	return event_name[index];
}

/**
 * Same as GetEventName() except for value is read from keyvalues each time.
 *
 * @parm			GameEventsData enum
 * @return			event string for current game
 */
  
stock char GetEventNameEx(EventGameData event)
{
	char event_name[32];
	int index = view_as<int>(event);
	
	Handle gc = LoadGameConfigFile("events.games");
	bool exists = GameConfGetKeyValue(gc, gd_event_name[index], event_name, sizeof(event_name));
	CloseHandle(gc);
	
	if (!exists)
	{
		Format(event_name, sizeof(event_name), gd_event_default[index]);	
	}
	
	return event_name;
}
