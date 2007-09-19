/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Counter-Strike:Source Extension
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#include "extension.h"
#include "RegNatives.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

CStrike g_CStrike;		/**< Global singleton for extension's main interface */
IBinTools *g_pBinTools = NULL;
IGameConfig *g_pGameConf = NULL;

SMEXT_LINK(&g_CStrike);

extern sp_nativeinfo_t g_CSNatives[];

bool CStrike::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "bintools.ext", true, true);

	if (!gameconfs->LoadGameConfigFile("sm-cstrike.games", &g_pGameConf, error, maxlength))
	{
		return false;
	}

	sharesys->AddNatives(myself, g_CSNatives);

	return true;
}

void CStrike::SDK_OnUnload()
{
	g_RegNatives.UnregisterAll();
	gameconfs->CloseGameConfigFile(g_pGameConf);
}

void CStrike::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);
}

bool CStrike::QueryRunning(char *error, size_t maxlength)
{
	SM_CHECK_IFACE(BINTOOLS, g_pBinTools);

	return true;
}

bool CStrike::QueryInterfaceDrop(SMInterface *pInterface)
{
	if (pInterface == g_pBinTools)
	{
		return false;
	}

	return IExtensionInterface::QueryInterfaceDrop(pInterface);
}

void CStrike::NotifyInterfaceDrop(SMInterface *pInterface)
{
	g_RegNatives.UnregisterAll();
}
