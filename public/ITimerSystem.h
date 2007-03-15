/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod, Copyright (C) 2004-2007 AlliedModders LLC. 
 * All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be 
 * used or modified under the Terms and Conditions of its License Agreement, 
 * which is found in public/licenses/LICENSE.txt.  As of this notice, derivative 
 * works must be licensed under the GNU General Public License (version 2 or 
 * greater).  A copy of the GPL is included under public/licenses/GPL.txt.
 * 
 * To view the latest information, see: http://www.sourcemod.net/license.php
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
#define SMINTERFACE_TIMERSYS_VERSION	1

namespace SourceMod
{
	class ITimer;

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

	#define TIMER_FLAG_REPEAT		(1<<0)

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
		 * @return					An ITimer pointer on success, NULL on failure.
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
		 *							next execution will be delayed by its interval.
		 * @return
		 */
		virtual void FireTimerOnce(ITimer *pTimer, bool delayExec=false) =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_TIMER_SYSTEM_H_
