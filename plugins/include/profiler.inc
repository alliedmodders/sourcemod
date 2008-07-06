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

#if defined _profiler_included
 #endinput
#endif
#define _profiler_included

/**
 * ONLY AVAILABLE ON WINDOWS RIGHT NOW K.
 */
 
/**
 * Creates a new profile object.  The Handle must be freed
 * using CloseHandle().
 *
 * @return			Handle to the profiler object.
 */
native Handle:CreateProfiler();

/**
 * Starts profiling.
 *
 * @param prof		Profiling object.
 * @noreturn
 * @error			Invalid Handle.
 */
native StartProfiling(Handle:prof);

/**
 * Stops profiling.
 *
 * @param prof		Profiling object.
 * @noreturn
 * @error			Invalid Handle or profiling was never started.
 */
native StopProfiling(Handle:prof);

/**
 * Returns the amount of high-precision time in seconds
 * that passed during the profiler's last start/stop 
 * cycle.
 *
 * @param prof		Profiling object.
 * @return			Time elapsed in seconds.
 * @error			Invalid Handle.
 */
native Float:GetProfilerTime(Handle:prof);
