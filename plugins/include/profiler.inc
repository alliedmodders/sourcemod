/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2018 AlliedModders LLC.  All rights reserved.
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

#if defined _profiler_included
 #endinput
#endif
#define _profiler_included

/**
 * ONLY AVAILABLE ON WINDOWS RIGHT NOW K.
 */

methodmap Profiler < Handle
{
	// Creates a new profile object.  The Handle must be freed
	// using delete or CloseHandle().
	//
	// @return              A new Profiler Handle.
	public native Profiler();
	
	// Starts a cycle for profiling.
	public native void Start();

	// Stops a cycle for profiling.
	//
	// @error               Profiler was never started.
	public native void Stop();
	
	// Returns the amount of high-precision time in seconds
	// that passed during the profiler's last start/stop 
	// cycle.
	//
	// @return              Time elapsed in seconds.
	property float Time {
		public native get();
	}
};

/**
 * Creates a new profile object.  The Handle must be freed
 * using delete or CloseHandle().
 *
 * @return              Handle to the profiler object.
 */
native Profiler CreateProfiler();

/**
 * Starts profiling.
 *
 * @param prof          Profiling object.
 * @error               Invalid Handle.
 */
native void StartProfiling(Handle prof);

/**
 * Stops profiling.
 *
 * @param prof          Profiling object.
 * @error               Invalid Handle or profiling was never started.
 */
native void StopProfiling(Handle prof);

/**
 * Returns the amount of high-precision time in seconds
 * that passed during the profiler's last start/stop 
 * cycle.
 *
 * @param prof          Profiling object.
 * @return              Time elapsed in seconds.
 * @error               Invalid Handle.
 */
native float GetProfilerTime(Handle prof);

/**
 * Mark the start of a profiling event.
 *
 * @param group     Budget group. This can be "all" for a default, or a short
 *                  description like "Timers" or "Events".
 * @param name      A name to attribute to this profiling event.
 */
native void EnterProfilingEvent(const char[] group, const char[] name);

/**
 * Mark the end of the last profiling event. This must be called in the same
 * stack frame as StartProfilingEvent(). Not doing so, or throwing errors,
 * will make the resulting profile very wrong.
 */
native void LeaveProfilingEvent();

/**
 * Returns true if the global profiler is enabled; false otherwise. It is
 * not necessary to call this before Enter/LeaveProfilingEvent.
 */
native bool IsProfilingActive();
