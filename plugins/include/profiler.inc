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
