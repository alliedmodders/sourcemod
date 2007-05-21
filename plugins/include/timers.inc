/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be used 
 * or modified under the Terms and Conditions of its License Agreement, which is found 
 * in LICENSE.txt.  The Terms and Conditions for making SourceMod extensions/plugins 
 * may change at any time.  To view the latest information, see:
 *   http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#if defined _timers_included
 #endinput
#endif
#define _timers_included

#include <datapack>

#define TIMER_REPEAT		(1<<0)		/**< Timer will repeat until it returns Plugin_Stop */
#define TIMER_HNDL_CLOSE	(1<<9)		/**< Timer will automatically call CloseHandle() on its value when finished */

/**
 * Any of the following prototypes will work for a timed function.
 */
funcenum Timer
{
	/**
 	 * Called when the timer interval has elapsed.
 	 * 
 	 * @param timer			Handle to the timer object.
 	 * @param hndl			Handle passed when the timer was created.
 	 * @return				Plugin_Stop to stop a repeating timer, any other value for
 	 *						default behavior.
 	 */
	Action:public(Handle:timer, Handle:hndl),
	
	/**
 	 * Called when the timer interval has elapsed.
 	 * 
 	 * @param timer			Handle to the timer object.
 	 * @param value			Value passed when the timer was created.
 	 * @return				Plugin_Stop to stop a repeating timer, any other value for
 	 *						default behavior.
 	 */
	Action:public(Handle:timer, any:value),
	
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
 * @param value				Handle or value to give to the timer function.
 * @param flags				Flags to set (such as repeatability or auto-Handle closing).
 * @return					Handle to the timer object.  You do not need to call CloseHandle().
 */
native Handle:CreateTimer(Float:interval, Timer:func, any:value=INVALID_HANDLE, flags=0);

/**
 * Kills a timer.  Use this instead of CloseHandle() if you need more options.
 *
 * @param autoClose			If autoClose is true, the timer's value will be 
 *							closed as a handle if TIMER_HNDL_CLOSE was not specified.
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
 * @param simulated			Retrieves whether or not the tick count
 *							is being manually simulated by SourceMod.
 *							This is the case if no players have joined
 *							the map yet.
 * @return					Time based on the game tick count.
 */
native Float:GetTickedTime(&bool:simulated);

/**
 * Creates a timer associated with a new data pack, and returns the datapack.
 * @note The datapack is automatically freed when the timer ends.
 * @note The position of the datapack is not reset or changed for the timer function.
 *
 * @param interval			Interval from the current game time to execute the given function.
 * @param func				Function to execute once the given interval has elapsed.
 * @param data				The newly created datapack is passed though this by-reference parameter.
 * @param flags				Timer flags.
 * @return					Handle to the timer object.  You do not need to call CloseHandle().
 */
stock Handle:CreateDataTimer(Float:interval, Timer:func, &Handle:data, flags=0)
{
	data = CreateDataPack();
	flags |= TIMER_HNDL_CLOSE;
	return CreateTimer(interval, func, data, flags);
}
