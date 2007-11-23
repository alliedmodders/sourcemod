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

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, false, bool, const char *, const char *, const char *, const char *, bool, bool);

TF2Tools g_TF2Tools;
IGameConfig *g_pGameConf = NULL;

SMEXT_LINK(&g_TF2Tools);

extern sp_nativeinfo_t g_TFNatives[];

bool TF2Tools::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "bintools.ext", true, true);

	char conf_error[255];
	if (!gameconfs->LoadGameConfigFile("sm-tf2.games", &g_pGameConf, conf_error, sizeof(conf_error)))
	{
		if (error)
		{
			snprintf(error, maxlength, "Could not read sm-cstrike.games.txt: %s", conf_error);
		}
		return false;
	}

	sharesys->AddNatives(myself, g_TFNatives);
	sharesys->RegisterLibrary(myself, "tf2");

	return true;
}

bool TF2Tools::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);

	return true;
}

void TF2Tools::SDK_OnUnload()
{
	gameconfs->CloseGameConfigFile(g_pGameConf);
}

void TF2Tools::SDK_OnAllLoaded()
{
}

bool TF2Tools::QueryRunning(char *error, size_t maxlength)
{
	return true;
}

