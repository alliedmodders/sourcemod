/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
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

#ifndef _INCLUDE_SOURCEMOD_TIMER_SYSTEM_H_
#define _INCLUDE_SOURCEMOD_TIMER_SYSTEM_H_

/**
 * @file ITimerSystem.h
 * @brief Contains functions for creating and managing timers.
 */


#include <IShareSys.h>
#include <IForwardSys.h>

#define SMINTERFACE_TIMERSYS_NAME		"ITimerSys"
#define SMINTERFACE_TIMERSYS_VERSION	4

namespace SourceMod
{
	class ITimer;

	/**
	 * @brief Interface for map timers.
	 */
	class IMapTimer
	{
	public:
		/**
		 * Returns the current map time limit in seconds.
		 *
		 * @return				Time limit, in seconds, or 
		 *						0 if there is no limit.
		 */
		virtual int GetMapTimeLimit() =0;
		
		/**
		 * Extends the map limit (either positively or negatively) in seconds.
		 *
		 * @param extra_time	Time to extend map by.  If 0, the map will 
		 *						be set to have no time limit.
		 */
		virtual void ExtendMapTimeLimit(int extra_time) =0;

		/**
		 * Tells the map timer whether it is being used or not.
		 *
		 * Map timers are automatically enabled when they are set, and 
		 * automatically disabled if being un-set.
		 *
		 * @param enabled		True if enabling, false if disabling.
		 */
		virtual void SetMapTimerStatus(bool enabled) =0;
	};

	/**
	 * @brief Event callbacks for when a timer is executed.
	 */
	class ITimedEvent
	{
	public:
		/**
		 * @brief Called when a timer is executed.
		 *
		 * @param pTimer		Pointer to the timer instance.
		 * @param pData			Private pointer passed from host.
		 * @return				Pl_Stop to stop timer, Pl_Continue to continue.
		 */
		virtual ResultType OnTimer(ITimer *pTimer, void *pData) =0;

		/**
		 * @brief Called when the timer has been killed.
		 *
		 * @param pTimer		Pointer to the timer instance.
		 * @param pData			Private data pointer passed from host.
		 */
		virtual void OnTimerEnd(ITimer *pTimer, void *pData) =0;
	};

	#define TIMER_FLAG_REPEAT			(1<<0)		/**< Timer will repeat until stopped */
	#define TIMER_FLAG_NO_MAPCHANGE		(1<<1)		/**< Timer will not carry over mapchanges */

	class ITimerSystem : public SMInterface
	{
	public:
		const char *GetInterfaceName()
		{
			return SMINTERFACE_TIMERSYS_NAME;
		}
		unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_TIMERSYS_VERSION;
		}
	public:
		/**
		 * @brief Creates a timed event.
		 *
		 * @param pCallbacks		Pointer to ITimedEvent callbacks.
		 * @param fInterval			Interval, in seconds, of the timed event to occur.
		 *							The smallest allowed interval is 0.1 seconds.
		 * @param pData				Private data to pass on to the timer.
		 * @param flags				Extra flags to pass on to the timer.
		 * @return					An ITimer pointer on success, NULL on 
		 *							failure.
		 */
		virtual ITimer *CreateTimer(ITimedEvent *pCallbacks, 
										 float fInterval, 
										 void *pData,
										 int flags) =0;

		/**
		 * @brief Kills a timer.
		 *
		 * @param pTimer			Pointer to the ITimer structure.
		 * @return
		 */
		virtual void KillTimer(ITimer *pTimer) =0;

		/**
		 * @brief Arbitrarily fires a timer.  If the timer is not a repeating 
		 * timer, this will also kill the timer.
		 *
		 * @param pTimer			Pointer to the ITimer structure.
		 * @param delayExec			If true, and the timer is repeating, the
		 *							next execution will be delayed by its 
		 *							interval.
		 * @return
		 */
		virtual void FireTimerOnce(ITimer *pTimer, bool delayExec=false) =0;

		/**
		 * @brief Sets the interface for dealing with map time limits.
		 *
		 * @param pMapTimer			Map timer interface pointer.
		 * @return					Old pointer.
		 */
		virtual IMapTimer *SetMapTimer(IMapTimer *pTimer) =0;

		/**
		 * @brief Notification that the map's time left has changed 
		 * via a change in the time limit or a change in the game rules (
		 * such as mp_restartgame).
		 */
		virtual void MapTimeLeftChanged() =0;

		/**
		 * @brief Returns the current universal tick time.  This 
		 * replacement for gpGlobals->curtime and engine->Time() correctly 
		 * keeps track of ticks.
		 *
		 * During simulation, it is incremented by the difference between 
		 * gpGlobals->curtime and the last simulated tick.  Otherwise, 
		 * it is incremented by the interval per tick.
		 *
		 * It is not reset past map changes.
		 *
		 * @return					Universal ticked time.
		 */
		virtual float GetTickedTime() =0;

		/**
		 * @brief Notification that the "starting point" in the game has has 
		 * changed.  This does not invoke MapTimeLeftChanged() automatically.
		 *
		 * @param offset			Optional offset to add to the new time.
		 */
		virtual void NotifyOfGameStart(float offset = 0.0f) =0;

		/**
		 * @brief Returns the time left in the map.
		 *
		 * @param pTime				Pointer to store time left, in seconds.
		 * 							If there is no time limit, the number will 
		 * 							be below 0.
		 * @return					True on success, false if no support.
		 */
		virtual bool GetMapTimeLeft(float *pTime) =0;

		/**
		 * @brief Returns the interface for dealing with map time limits.
		 *
		 * @return					Map timer interface.
		 */
		virtual IMapTimer *GetMapTimer() = 0;
	};
}

#endif //_INCLUDE_SOURCEMOD_TIMER_SYSTEM_H_
