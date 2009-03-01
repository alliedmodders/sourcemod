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

#if defined _timers_included
 #endinput
#endif
#define _timers_included

#include <datapack>

#define TIMER_REPEAT			(1<<0)		/**< Timer will repeat until it returns Plugin_Stop */
#define TIMER_FLAG_NO_MAPCHANGE	(1<<1)		/**< Timer will not carry over mapchanges */
#define TIMER_HNDL_CLOSE		(1<<9)		/**< Deprecated define, replaced by below */
#define TIMER_DATA_HNDL_CLOSE		(1<<9)		/**< Timer will automatically call CloseHandle() on its data when finished */

/**
 * Any of the following prototypes will work for a timed function.
 */
funcenum Timer
{
	/**
 	 * Called when the timer interval has elapsed.
 	 * 
 	 * @param timer			Handle to the timer object.
 	 * @param hndl			Handle passed to CreateTimer() when timer was created.
 	 * @return				Plugin_Stop to stop a repeating timer, any other value for
 	 *						default behavior.
 	 */
	Action:public(Handle:timer, Handle:hndl),
	
	/**
 	 * Called when the timer interval has elapsed.
 	 * 
 	 * @param timer			Handle to the timer object.
 	 * @param data			Data passed to CreateTimer() when timer was created.
 	 * @return				Plugin_Stop to stop a repeating timer, any other value for
 	 *						default behavior.
 	 */
	Action:public(Handle:timer, any:data),
	
	/**
 	 * Called when the timer interval has elapsed.
 	 * 
 	 * @param timer			Handle to the timer object.
 	 * @return				Plugin_Stop to stop a repeating timer, any other value for
 	 *						default behavior.
 	 */
	Action:public(Handle:timer),
};

/**
 * Creates a basic timer.  Calling CloseHandle() on a timer will end the timer.
 *
 * @param interval			Interval from the current game time to execute the given function.
 * @param func				Function to execute once the given interval has elapsed.
 * @param data				Handle or value to pass through to the timer callback function.
 * @param flags				Flags to set (such as repeatability or auto-Handle closing).
 * @return					Handle to the timer object.  You do not need to call CloseHandle().
 *							If the timer could not be created, INVALID_HANDLE will be returned.
 */
native Handle:CreateTimer(Float:interval, Timer:func, any:data=INVALID_HANDLE, flags=0);

/**
 * Kills a timer.  Use this instead of CloseHandle() if you need more options.
 *
 * @param autoClose			If autoClose is true, the data that was passed to CreateTimer() will
 *							be closed as a handle if TIMER_DATA_HNDL_CLOSE was not specified.
 * @noreturn
 */
native KillTimer(Handle:timer, bool:autoClose=false);

/**
 * Manually triggers a timer so its function will be called.
 *
 * @param timer				Timer Handle to trigger.
 * @param reset				If reset is true, the elapsed time counter is reset
 *							so the full interval must pass again.
 * @noreturn
 */
native TriggerTimer(Handle:timer, bool:reset=false);

/**
 * Returns the simulated game time.  
 *
 * This time is internally maintained by SourceMod and is based on the game 
 * tick count and tick rate.  Unlike GetGameTime(), it will increment past 
 * map changes and while no players are connected.  Unlike GetEngineTime(), 
 * it will not increment based on the system clock (i.e. it is still bound 
 * to the ticking process).
 *
 * @return					Time based on the game tick count.
 */
native Float:GetTickedTime();

/**
 * Returns an estimate of the time left before the map ends.  If the server 
 * has not processed any frames yet (i.e. no players have joined the map yet), 
 * then the time left returned will always be infinite.
 *
 * @param timeleft			Variable to store the time, in seconds.  If the 
 *							value is less than 0, the time limit is infinite.
 * @return					True if the operation is supported, false otherwise.
 */
native bool:GetMapTimeLeft(&timeleft);

/**
 * Retrieves the current map time limit.  If the server has not processed any 
 * frames yet (i.e. no players have joined the map yet), then the time limit 
 * returned will always be 0.
 *
 * @param time				Set to the number of total seconds in the map time 
 *							limit, or 0 if there is no time limit set.
 * @return					True on success, false if operation is not supported.
 */
native bool:GetMapTimeLimit(&time);

/**
 * Extends the map time limit in a way that will notify all plugins.
 *
 * @param time				Number of seconds to extend map time limit by.
 *							The number can be negative to decrease the time limit.
 *							If 0, the map will be set to have no time limit.
 * @return					True on success, false if operation is not supported.
 */
native bool:ExtendMapTimeLimit(time);

/**
 * Returns the number of seconds in between game server ticks.
 *
 * Note: A tick, in this context, is a frame.
 *
 * @return					Number of seconds in between ticks.
 */
native Float:GetTickInterval();

/**
 * Notification that the map's time left has changed via a change in the time 
 * limit or a change in the game rules (such as mp_restartgame).  This is useful 
 * for plugins trying to create timers based on the time left in the map.
 *
 * Calling ExtendMapTimeLimit() from here, without proper precaution, will 
 * cause infinite recursion.
 *
 * If the operation is not supported, this will never be called.

 * If the server has not yet processed any frames (i.e. no players have joined 
 * the map yet), then this will be called once the server begins ticking, even 
 * if there is no time limit set.
 */
forward OnMapTimeLeftChanged();

/**
 * Returns whether or not the server is processing frames or not.  
 *
 * The server does not process frames until at least one client joins the game.  
 * Once the first player has in, even if that player, leaves, the server's 
 * timers and entities will work.
 *
 * @return					True if the server is ticking, false otherwise.
 */
native bool:IsServerProcessing();

/**
 * Creates a timer associated with a new datapack, and returns the datapack.
 * @note The datapack is automatically freed when the timer ends.
 * @note The position of the datapack is not reset or changed for the timer function.
 *
 * @param interval			Interval from the current game time to execute the given function.
 * @param func				Function to execute once the given interval has elapsed.
 * @param datapack			The newly created datapack is passed though this by-reference 
 *							parameter to the timer callback function.
 * @param flags				Timer flags.
 * @return					Handle to the timer object.  You do not need to call CloseHandle().
 */
stock Handle:CreateDataTimer(Float:interval, Timer:func, &Handle:datapack, flags=0)
{
	datapack = CreateDataPack();
	flags |= TIMER_DATA_HNDL_CLOSE;
	return CreateTimer(interval, func, datapack, flags);
}

