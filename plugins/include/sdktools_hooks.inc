/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2009 AlliedModders LLC.  All rights reserved.
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

#if defined _sdktools_hooks_included
 #endinput
#endif
#define _sdktools_hooks_included

#define FEATURECAP_PLAYERRUNCMD_11PARAMS    "SDKTools PlayerRunCmd 11Params"

/**
 * Called when a clients movement buttons are being processed (Read Only)
 *
 * @param client        Index of the client.
 * @param buttons       Current commands (as bitflags - see entity_prop_stocks.inc).
 * @param impulse       Current impulse command.
 * @param vel           Players desired velocity.
 * @param angles        Players desired view angles.
 * @param weapon        Entity index of the new weapon if player switches weapon, 0 otherwise.
 * @param subtype       Weapon subtype when selected from a menu.
 * @param cmdnum        Command number. Increments from the first command sent.
 * @param tickcount     Tick count. A client's prediction based on the server's GetGameTickCount value.
 * @param seed          Random seed. Used to determine weapon recoil, spread, and other predicted elements.
 * @param mouse         Mouse direction (x, y).
 */
forward void OnPlayerRunCmdPre(int client, int buttons, int impulse, const float vel[3], const float angles[3], int weapon, int subtype, int cmdnum, int tickcount, int seed, const int mouse[2]);

/**
 * Called when a clients movement buttons are being processed
 *
 * @param client        Index of the client.
 * @param buttons       Copyback buffer containing the current commands (as bitflags - see entity_prop_stocks.inc).
 * @param impulse       Copyback buffer containing the current impulse command.
 * @param vel           Players desired velocity.
 * @param angles        Players desired view angles.
 * @param weapon        Entity index of the new weapon if player switches weapon, 0 otherwise.
 * @param subtype       Weapon subtype when selected from a menu.
 * @param cmdnum        Command number. Increments from the first command sent.
 * @param tickcount     Tick count. A client's prediction based on the server's GetGameTickCount value.
 * @param seed          Random seed. Used to determine weapon recoil, spread, and other predicted elements.
 * @param mouse         Mouse direction (x, y).
 * @return              Plugin_Handled to block the commands from being processed, Plugin_Continue otherwise.
 *
 * @note To see if all 11 params are available, use FeatureType_Capability and FEATURECAP_PLAYERRUNCMD_11PARAMS.
 */
forward Action OnPlayerRunCmd(int client, int &buttons, int &impulse, float vel[3], float angles[3], int &weapon, int &subtype, int &cmdnum, int &tickcount, int &seed, int mouse[2]);

/**
 * Called after a clients movement buttons were processed.
 *
 * @param client        Index of the client.
 * @param buttons       The current commands (as bitflags - see entity_prop_stocks.inc).
 * @param impulse       The current impulse command.
 * @param vel           Players desired velocity.
 * @param angles        Players desired view angles.
 * @param weapon        Entity index of the new weapon if player switches weapon, 0 otherwise.
 * @param subtype       Weapon subtype when selected from a menu.
 * @param cmdnum        Command number. Increments from the first command sent.
 * @param tickcount     Tick count. A client's prediction based on the server's GetGameTickCount value.
 * @param seed          Random seed. Used to determine weapon recoil, spread, and other predicted elements.
 * @param mouse         Mouse direction (x, y).
 */
forward void OnPlayerRunCmdPost(int client, int buttons, int impulse, const float vel[3], const float angles[3], int weapon, int subtype, int cmdnum, int tickcount, int seed, const int mouse[2]);

/**
 * Called when a client requests a file from the server.
 *
 * @param client        Client index.
 * @param sFile         Requested file path.
 *
 * @return              Plugin_Handled to block the transfer, Plugin_Continue to let it proceed.
 */
forward Action OnFileSend(int client, const char[] sFile);

/**
 * Called when a client sends a file to the server.
 *
 * @param client        Client index.
 * @param sFile         Requested file path.
 *
 * @return              Plugin_Handled to block the transfer, Plugin_Continue to let it proceed.
 */
forward Action OnFileReceive(int client, const char[] sFile);
