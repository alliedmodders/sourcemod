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

#define EH_CHAR_SIZE 32 // What to change the char buffer size? Update it here.

/* Update these three strings when adding new event game data */

//The Enum: For the physical index of the event name
enum EventGameData 
{
	EventHook_RoundEnd = 0,
	EventHook_GameStart
}

//The Event Name: As you've specified in the game data
stock char gd_event_name[EventGameData][] = 
{
	"RoundEnd",
	"GameStart"
};

//The Default Name: What do we return, if game data reading fails?
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
  
stock char GetEventHookName(EventGameData event)
{
	static bool got_name[EventGameData] = false;
	static char event_name[EventGameData][EH_CHAR_SIZE];
	
	int index = view_as<int>(event);	
	if (!got_name[index])
	{	
		Format(event_name[index], EH_CHAR_SIZE, GetEventNameEx(event));		
		got_name = true;
	}
	
	return event_name[index];
}

/**
 * Same as GetEventHookName() except for value is not cached.
 *
 * @parm			GameEventsData enum
 * @return			event string for current game
 */
  
stock char GetEventHookNameEx(EventGameData event)
{
	char event_name[EH_CHAR_SIZE];
	int index = view_as<int>(event);
	
	Handle gc = LoadGameConfigFile("sm-events.games");
	bool exists = GameConfGetKeyValue(gc, gd_event_name[index], event_name, EH_CHAR_SIZE);
	CloseHandle(gc);
	
	if (!exists)
	{
		Format(event_name, EH_CHAR_SIZE, gd_event_default[index]);	
	}
	
	return event_name;
}
