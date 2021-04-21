/**
 * vim: set ts=4 sw=4 tw=99 noet:
 * =============================================================================
 * SourceMod BinTools Extension
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
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

#include <sourcemod_version.h>
#include "extension.h"
#include "CallMaker.h"

/**
 * @file extension.cpp
 * @brief Implements BinTools extension code.
 */

BinTools g_BinTools;		/**< Global singleton for extension's main interface */
CallMaker g_CallMaker;
CallMaker2 g_CallMaker2;
ISourcePawnEngine *g_SPEngine;

SMEXT_LINK(&g_BinTools);

bool BinTools::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	g_SPEngine = g_pSM->GetScriptingEngine();
	g_pShareSys->AddInterface(myself, &g_CallMaker);

	return true;
}

const char *BinTools::GetExtensionVerString()
{
	return SOURCEMOD_VERSION;
}

const char *BinTools::GetExtensionDateString()
{
	return SOURCEMOD_BUILD_TIME;
}

