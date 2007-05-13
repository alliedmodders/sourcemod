/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod SDK Tools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC. All rights reserved.
 * ===============================================================
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#include "extension.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

SDKTools g_SdkTools;		/**< Global singleton for extension's main interface */
IServerGameEnts *gameents = NULL;
IBinTools *g_pBinTools = NULL;
IPlayerManager *g_pPlayers = NULL;

SMEXT_LINK(&g_SdkTools);

bool SDKTools::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	SM_GET_IFACE(PLAYERMANAGER, g_pPlayers);

	g_pShareSys->AddDependency(myself, "bintools.ext", true, true);

	return true;
}

bool SDKTools::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(serverFactory, gameents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);

	return true;
}

void SDKTools::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);
}

bool SDKTools::QueryRunning(char *error, size_t maxlength)
{
	SM_CHECK_IFACE(BINTOOLS, g_pBinTools);

	return true;
}
