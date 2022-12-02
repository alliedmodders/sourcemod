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
#include "HalfLife2.h"
#include "sourcemm_api.h"
#include "sm_stringutil.h"
#include "sourcehook.h"
#include "logic_bridge.h"
#include "compat_wrappers.h"
#include <time.h>
#include <bridge/include/ILogger.h>

NextMapManager g_NextMap;

#if SOURCE_ENGINE != SE_DARKMESSIAH
SH_DECL_HOOK2_void(IVEngineServer, ChangeLevel, SH_NOATTRIB, 0, const char *, const char *);
#else
SH_DECL_HOOK4_void(IVEngineServer, ChangeLevel, SH_NOATTRIB, 0, const char *, const char *, const char *, bool);
#endif

#if SOURCE_ENGINE >= SE_ORANGEBOX
SH_DECL_EXTERN1_void(ConCommand, Dispatch, SH_NOATTRIB, false, const CCommand &);
#else
SH_DECL_EXTERN0_void(ConCommand, Dispatch, SH_NOATTRIB, false);
#endif

ConCommand *changeLevelCmd = NULL;

ConVar sm_nextmap("sm_nextmap", "", FCVAR_NOTIFY);
ConVar sm_maphistory_size("sm_maphistory_size", "20");

bool g_forcedChange = false;

void NextMapManager::OnSourceModAllInitialized_Post()
{
	SH_ADD_HOOK(IVEngineServer, ChangeLevel, engine, SH_MEMBER(this, &NextMapManager::HookChangeLevel), false);

	ConCommand *pCmd = FindCommand("changelevel");
	if (pCmd != NULL)
	{
		SH_ADD_HOOK(ConCommand, Dispatch, pCmd, SH_STATIC(CmdChangeLevelCallback), false);
		changeLevelCmd = pCmd;
	}
}

void NextMapManager::OnSourceModShutdown()
{
	SH_REMOVE_HOOK(IVEngineServer, ChangeLevel, engine, SH_MEMBER(this, &NextMapManager::HookChangeLevel), false);

	if (changeLevelCmd != NULL)
	{
		SH_REMOVE_HOOK(ConCommand, Dispatch, changeLevelCmd, SH_STATIC(CmdChangeLevelCallback), false);
	}

	SourceHook::List<MapChangeData *>::iterator iter;
	iter = m_mapHistory.begin();

	while (iter != m_mapHistory.end())
	{
		delete (MapChangeData *)*iter;
		iter = m_mapHistory.erase(iter);
	}
}

const char *NextMapManager::GetNextMap()
{
	return sm_nextmap.GetString();
}

bool NextMapManager::SetNextMap(const char *map)
{
	if (!g_HL2.IsMapValid(map))
	{
		return false;
	}

	sm_nextmap.SetValue(map);

	return true;
}

#if SOURCE_ENGINE != SE_DARKMESSIAH
void NextMapManager::HookChangeLevel(const char *map, const char *unknown)
#else
void NextMapManager::HookChangeLevel(const char *map, const char *unknown, const char *video, bool bLongLoading)
#endif
{
	if (g_forcedChange)
	{
		logger->LogMessage("[SM] Changed map to \"%s\"", map);
		RETURN_META(MRES_IGNORED);
	}

	const char *newmap = sm_nextmap.GetString();

	if (newmap[0] == 0 || !g_HL2.IsMapValid(newmap))
	{
		RETURN_META(MRES_IGNORED);
	}

	logger->LogMessage("[SM] Changed map to \"%s\"", newmap);

	ke::SafeStrcpy(m_tempChangeInfo.m_mapName, sizeof(m_tempChangeInfo.m_mapName), newmap);
	ke::SafeStrcpy(m_tempChangeInfo.m_changeReason, sizeof(m_tempChangeInfo.m_changeReason), "Normal level change");

#if SOURCE_ENGINE != SE_DARKMESSIAH
	RETURN_META_NEWPARAMS(MRES_IGNORED, &IVEngineServer::ChangeLevel, (newmap, unknown));
#else
	RETURN_META_NEWPARAMS(MRES_IGNORED, &IVEngineServer::ChangeLevel, (newmap, unknown, video, bLongLoading));
#endif
}

void NextMapManager::OnSourceModLevelChange( const char *mapName )
{
	/* Skip the first 'mapchange' when the server starts up */
	if (m_tempChangeInfo.startTime != 0)
	{
		if (strcmp(mapName, m_tempChangeInfo.m_mapName) == 0)
		{
			/* The map change was as we expected */
			m_mapHistory.push_back(new MapChangeData(lastMap, m_tempChangeInfo.m_changeReason, m_tempChangeInfo.startTime));
		}
		else
		{
			/* Something intercepted the mapchange */
			char newReason[255];
			ke::SafeSprintf(newReason, sizeof(newReason), "%s (Map overridden)", m_tempChangeInfo.m_changeReason);
			m_mapHistory.push_back(new MapChangeData(lastMap, newReason, m_tempChangeInfo.startTime));
		}

		int historydiff = sm_maphistory_size.GetInt();
		if (historydiff > 0)
		{
			historydiff -= m_mapHistory.size();
		} else if (historydiff < 0)
		{
			historydiff = (m_mapHistory.size() * -1);
		}

		for (SourceHook::List<MapChangeData *>::iterator iter = m_mapHistory.begin(); historydiff++ < 0; iter = m_mapHistory.erase(iter))
		{
			delete (MapChangeData *)*iter;
		}
	}

	m_tempChangeInfo.m_mapName[0] ='\0';
	m_tempChangeInfo.m_changeReason[0] = '\0';
	m_tempChangeInfo.startTime = time(NULL);
	ke::SafeStrcpy(lastMap, sizeof(lastMap), mapName);
}

void NextMapManager::ForceChangeLevel( const char *mapName, const char* changeReason )
{
	/* Store the mapname and reason */
	ke::SafeStrcpy(m_tempChangeInfo.m_mapName, sizeof(m_tempChangeInfo.m_mapName), mapName);
	ke::SafeStrcpy(m_tempChangeInfo.m_changeReason, sizeof(m_tempChangeInfo.m_changeReason), changeReason);

	/* Change level and skip our hook */
	g_forcedChange = true;
	engine->ChangeLevel(mapName, NULL);
	g_forcedChange = false;
}

NextMapManager::NextMapManager()
{
	m_tempChangeInfo = MapChangeData();
	m_mapHistory = SourceHook::List<MapChangeData *>();
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
void CmdChangeLevelCallback(const CCommand &command)
{
#else
void CmdChangeLevelCallback()
{
	CCommand command;
#endif

	if (command.ArgC() < 2)
	{
		return;
	}

	if (g_NextMap.m_tempChangeInfo.m_mapName[0] == '\0')
	{
		ke::SafeStrcpy(g_NextMap.m_tempChangeInfo.m_mapName, sizeof(g_NextMap.m_tempChangeInfo.m_mapName), command.Arg(1));
		ke::SafeStrcpy(g_NextMap.m_tempChangeInfo.m_changeReason, sizeof(g_NextMap.m_tempChangeInfo.m_changeReason), "changelevel Command");
	}
}
