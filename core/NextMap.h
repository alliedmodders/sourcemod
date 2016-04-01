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

#ifndef _INCLUDE_SOURCEMOD_NEXTMAP_H_
#define _INCLUDE_SOURCEMOD_NEXTMAP_H_

#include "sm_globals.h"
#include <eiface.h>
#include "sh_list.h"
#include "sm_stringutil.h"
#include <amtl/am-string.h>

struct MapChangeData
{
	MapChangeData(const char *mapName, const char *changeReason, time_t time)
	{
		ke::SafeStrcpy(m_mapName, sizeof(m_mapName), mapName);
		ke::SafeStrcpy(m_changeReason, sizeof(m_changeReason), changeReason);
		startTime = time;
	}

	MapChangeData()
	{
		m_mapName[0] = '\0';
		m_changeReason[0] = '\0';
		startTime = 0;
	}

	char m_mapName[PLATFORM_MAX_PATH];
	char m_changeReason[100];
	time_t startTime;
};

#if SOURCE_ENGINE >= SE_ORANGEBOX
void CmdChangeLevelCallback(const CCommand &command);
#else
void CmdChangeLevelCallback();
#endif

class NextMapManager : public SMGlobalClass
{
public:
	NextMapManager();

#if SOURCE_ENGINE >= SE_ORANGEBOX
	friend void CmdChangeLevelCallback(const CCommand &command);
#else
	friend void CmdChangeLevelCallback();
#endif

	void OnSourceModAllInitialized_Post();
	void OnSourceModShutdown();
	void OnSourceModLevelChange(const char *mapName);

	const char *GetNextMap();
	bool SetNextMap(const char *map);

	void ForceChangeLevel(const char *mapName, const char* changeReason);

#if SOURCE_ENGINE != SE_DARKMESSIAH
	void HookChangeLevel(const char *map, const char *unknown);
#else
	void HookChangeLevel(const char *map, const char *unknown, const char *video, bool bLongLoading);
#endif

public:
	SourceHook::List<MapChangeData *> m_mapHistory;

private:
	MapChangeData m_tempChangeInfo;
	char lastMap[32];
};

extern NextMapManager g_NextMap;

#endif //_INCLUDE_SOURCEMOD_NEXTMAP_H_
