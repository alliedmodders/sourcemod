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

#if defined _commandline_included_
  #endinput
#endif
#define _commandline_included_

/**
 * Gets the full command line the server was launched with.
 *
 * @param commandLine		Buffer to store the command line in.
 * @param maxlen			Maximum length of the command line buffer.
 * @return					True if the command line is valid; otherwise, false.
 * @error					No command line available, or no mod support.
 */
native bool:GetCommandLine(String:commandLine[], maxlen);

/**
 * Gets the value of a command line parameter the server was launched with.
 *
 * @param param		The command line parameter to get the value of.
 * @param value		Buffer to store the parameter value in.
 * @param maxlen	Maximum length of the value buffer.
 * @param defValue	The default value to return if the parameter wasn't specified.
 * @return			True if the command line is valid; otherwise, false.
 * @error			No command line available, or no mod support.
 */
native GetCommandLineParam(const String:param[], String:value[], maxlen, const String:defValue[]="");

/**
 * Gets the value of a command line parameter the server was launched with.
 *
 * @param param		The command line parameter to get the value of.
 * @param defValue	The default value to return if the parameter wasn't specified.
 * @return			The integer value of the command line parameter value.
 * @error			No command line available, or no mod support.
 */
native GetCommandLineParamInt(const String:param[], defValue=0);

/**
 * Gets the value of a command line parameter the server was launched with.
 *
 * @param param		The command line parameter to get the value of.
 * @param defValue	The default value to return if the parameter wasn't specified.
 * @return			The floating point value of the command line parameter value.
 * @error			No command line available, or no mod support.
 */
native Float:GetCommandLineParamFloat(const String:param[], Float:defValue=0.0);

/**
 * Determines if a specific command line parameter is present.
 *
 * @param param		The command line parameter to test.
 * @return			True if the command line parameter is specified; otherwise, false.
 * @error			No command line available, or no mod support.
 */
native bool:FindCommandLineParam(const String:param[]);
