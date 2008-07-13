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
#include "sourcehook.h"
#include "sm_srvcmds.h"

NextMapManager g_NextMap;

SH_DECL_HOOK2_void(IVEngineServer, ChangeLevel, SH_NOATTRIB, 0, const char *, const char *);

#if defined ORANGEBOX_BUILD
SH_DECL_EXTERN1_void(ConCommand, Dispatch, SH_NOATTRIB, false, const CCommand &);
#else
extern bool __SourceHook_FHAddConCommandDispatch(void *,bool,class fastdelegate::FastDelegate0<void>);
extern bool __SourceHook_FHRemoveConCommandDispatch(void *,bool,class fastdelegate::FastDelegate0<void>);
#endif

ConCommand *changeLevelCmd = NULL;

ConVar sm_nextmap("sm_nextmap", "", FCVAR_NOTIFY);

bool g_forcedChange = false;

void NextMapManager::OnSourceModAllInitialized_Post()
{
#if defined ORANGEBOX_BUILD
	SH_ADD_HOOK(IVEngineServer, ChangeLevel, engine, SH_MEMBER(this, &NextMapManager::HookChangeLevel), false);
#else
	SH_ADD_HOOK_MEMFUNC(IVEngineServer, ChangeLevel, engine, this, &NextMapManager::HookChangeLevel, false);
#endif

	ConCommandBase *pBase = icvar->GetCommands();
	ConCommand *pCmd = NULL;
	while (pBase)
	{
		if (strcmp(pBase->GetName(), "changelevel") == 0)
		{
			/* Don't want to return convar with same name */
			if (!pBase->IsCommand())
			{
				break;
			}

			pCmd = (ConCommand *)pBase;
			break;
		}
		pBase = const_cast<ConCommandBase *>(pBase->GetNext());
	}

	if (pCmd != NULL)
	{
		SH_ADD_HOOK_STATICFUNC(ConCommand, Dispatch, pCmd, CmdChangeLevelCallback, false);
		changeLevelCmd = pCmd;
	}
}

void NextMapManager::OnSourceModShutdown()
{
#if defined ORANGEBOX_BUILD
	SH_REMOVE_HOOK(IVEngineServer, ChangeLevel, engine, SH_MEMBER(this, &NextMapManager::HookChangeLevel), false);
#else
	SH_REMOVE_HOOK_MEMFUNC(IVEngineServer, ChangeLevel, engine, this, &NextMapManager::HookChangeLevel, false);
#endif

	if (changeLevelCmd != NULL)
	{
		SH_REMOVE_HOOK_STATICFUNC(ConCommand, Dispatch, changeLevelCmd, CmdChangeLevelCallback, false);
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
	if (!engine->IsMapValid(map))
	{
		return false;
	}

	sm_nextmap.SetValue(map);

	return true;
}

void NextMapManager::HookChangeLevel(const char *map, const char *unknown)
{
	if (g_forcedChange)
	{
		g_Logger.LogMessage("[SM] Changed map to \"%s\"", map);
		RETURN_META(MRES_IGNORED);
	}

	const char *newmap = sm_nextmap.GetString();

	if (newmap[0] == 0 || !engine->IsMapValid(newmap))
	{
		RETURN_META(MRES_IGNORED);
	}

	g_Logger.LogMessage("[SM] Changed map to \"%s\"", newmap);

	UTIL_Format(m_tempChangeInfo.m_mapName, sizeof(m_tempChangeInfo.m_mapName), newmap);
	UTIL_Format(m_tempChangeInfo.m_changeReason, sizeof(m_tempChangeInfo.m_changeReason), "Normal level change");

	RETURN_META_NEWPARAMS(MRES_IGNORED, &IVEngineServer::ChangeLevel, (newmap, unknown));
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
			UTIL_Format(newReason, sizeof(newReason), "%s (Map overridden)", m_tempChangeInfo.m_changeReason);
			m_mapHistory.push_back(new MapChangeData(lastMap, newReason, m_tempChangeInfo.startTime));
		}

		/* TODO: Should this be customizable? */
		if (m_mapHistory.size() > 20)
		{
			SourceHook::List<MapChangeData *>::iterator iter;
			iter = m_mapHistory.begin();

			delete (MapChangeData *)*iter;

			m_mapHistory.erase(iter);
		}
	}

	m_tempChangeInfo.m_mapName[0] ='\0';
	m_tempChangeInfo.m_changeReason[0] = '\0';
	m_tempChangeInfo.startTime = time(NULL);
	UTIL_Format(lastMap, sizeof(lastMap), mapName);
}

void NextMapManager::ForceChangeLevel( const char *mapName, const char* changeReason )
{
	/* Store the mapname and reason */
	UTIL_Format(m_tempChangeInfo.m_mapName, sizeof(m_tempChangeInfo.m_mapName), mapName);
	UTIL_Format(m_tempChangeInfo.m_changeReason, sizeof(m_tempChangeInfo.m_changeReason), changeReason);

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

#if defined ORANGEBOX_BUILD
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
		UTIL_Format(g_NextMap.m_tempChangeInfo.m_mapName, sizeof(g_NextMap.m_tempChangeInfo.m_mapName), command.Arg(1));
		UTIL_Format(g_NextMap.m_tempChangeInfo.m_changeReason, sizeof(g_NextMap.m_tempChangeInfo.m_changeReason), "changelevel Command");
	}
}