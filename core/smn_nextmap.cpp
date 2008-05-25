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

#include "sm_globals.h"
#include "NextMap.h"

static cell_t sm_GetNextMap(IPluginContext *pCtx, const cell_t *params)
{
	const char *map = g_NextMap.GetNextMap();

	if (map[0] == 0)
	{
		return 0;
	}

	pCtx->StringToLocal(params[1], params[2], map);

	return 1;
}

static cell_t sm_SetNextMap(IPluginContext *pCtx, const cell_t *params)
{
	char *map;
	pCtx->LocalToString(params[1], &map);

	return g_NextMap.SetNextMap(map);
}


REGISTER_NATIVES(nextmapnatives)
{
	{"GetNextMap",		sm_GetNextMap},
	{"SetNextMap",		sm_SetNextMap},
	{NULL,						NULL},
};
