/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#include "NextMap.h"
#include "Logger.h"
#include "sourcemm_api.h"
#include "sm_stringutil.h"

NextMapManager g_NextMap;

SH_DECL_HOOK2_void(IVEngineServer, ChangeLevel, SH_NOATTRIB, 0, const char *, const char *);

ConVar sm_nextmap("sm_nextmap", "", FCVAR_NOTIFY);

void NextMapManager::OnSourceModAllInitialized_Post()
{
#if defined ORANGEBOX_BUILD
	SH_ADD_HOOK(IVEngineServer, ChangeLevel, engine, SH_MEMBER(this, &NextMapManager::HookChangeLevel), false);
#else
	SH_ADD_HOOK_MEMFUNC(IVEngineServer, ChangeLevel, engine, this, &NextMapManager::HookChangeLevel, false);
#endif
}

void NextMapManager::OnSourceModShutdown()
{
#if defined ORANGEBOX_BUILD
	SH_REMOVE_HOOK(IVEngineServer, ChangeLevel, engine, SH_MEMBER(this, &NextMapManager::HookChangeLevel), false);
#else
	SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, ChangeLevel, engine, this, &NextMapManager::HookChangeLevel, false);
#endif
}

const char *NextMapManager::GetNextMap()
{
	return sm_nextmap.GetString();
}

bool NextMapManager::SetNextMap(const char *map)
{
	if (!engine->IsMapValid(map))
	{
		return false;
	}

	sm_nextmap.SetValue(map);

	return true;
}

void NextMapManager::HookChangeLevel(const char *map, const char *unknown)
{
	const char *newmap = sm_nextmap.GetString();

	if (newmap[0] == 0 || !engine->IsMapValid(newmap))
	{
		RETURN_META(MRES_IGNORED);
	}

	g_Logger.LogMessage("[SM] Changed map to \"%s\"", newmap);

	RETURN_META_NEWPARAMS(MRES_IGNORED, &IVEngineServer::ChangeLevel, (newmap, unknown));
}
