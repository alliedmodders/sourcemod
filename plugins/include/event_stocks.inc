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

enum EventGameData 
{
	eRoundEnd = 0,
	eGameStart
}

methodmap EventName
{
	#define EH_CHAR_SIZE 32 // What to change the char buffer size? Update it here.
	
	/**
	 * Gets the default value for a event
	 * 
	 * @parm enum		EventGameData
	 * @return char		Name
	 */
	
	public static char DefaultValue(EventGameData event) 
	{
		char Default[EventGameData][] =
		{
			"round_end",
			"game_start"
		};
		
		return Default[view_as<int>(event)];
	}
	
	/**
	 * Gets the game data name
	 * 
	 * @parm enum		EventGameData
	 * @return char		Name
	 */
	
	public static char DataName(EventGameData event)
	{
		char Name[EventGameData][] = {
			"RoundEnd",
			"GameStart"
		};
		
		return Name[view_as<int>(event)];
	}
	
	/**
	 * Gets and caches the game data for event names from keyvalues
	 *
	 * @return Handle	Game Data
	 */
	
	public static Handle GetData()
	{
		static Handle gc = null;
	
		if (gc == null)
		{
			gc = LoadGameConfigFile("sm-events.games");
		}
		
		return gc;
	}
	
	/**
	 * Same as EventName.GetString() except for value is not cached.
	 *
	 * @parm			GameEventsData enum
	 * @return			event string for current game
	 */
	 
	public static char GetStringEx(EventGameData event)
	{
		char event_name[EH_CHAR_SIZE];

		//Get gamedata for event from keyvalues. If it fails, return the default name, else return the found event_name
		return 	! GameConfGetKeyValue(EventName.GetData(), EventName.DataName(event), event_name, EH_CHAR_SIZE) 
				? EventName.DefaultValue(event) 
				: event_name;
	}
	
	/**
	 * Searches through gamedata to find an event name.
	 *
	 * Caches the event name after reading from keyvalues first time.
	 * This wrapper is intended to be used when hooking an event.
	 *
	 * @parm			GameEventsData enum
	 * @return			event string for current game
	 */
	 
	public static char GetString(EventGameData event)
	{
		static bool got_name[EventGameData] = false;
		static char event_name[EventGameData][EH_CHAR_SIZE];
				
		int index = view_as<int>(event);	
		if (!got_name[index])
		{	
			Format(event_name[index], EH_CHAR_SIZE, EventName.GetStringEx(event));		
			got_name[index] = true;
		}
				
		return event_name[index];
	}
};
